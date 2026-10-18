/* Host API: boot supervisor, realtime render (resample + codec gain), meters. Part of clau-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0. */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include "glo_core.h"
#include "shim_internal.h"

extern void app_main(void);
extern int codec_digital_volume, codec_analog_volume;
extern int channel_loop, channel_loop_remapped;
extern int short_press_seq_ptr, short_press_seq_cnt, short_press_sequence, short_press_volume_plus, short_press_volume_minus;
extern char short_press_seq[];
extern int button_play, button_volume_plus, button_volume_minus, button_volume_both, short_press_button_play;
extern uint8_t glo_run;
extern void glo_port_get_agc(int *enabled, int *max_gain_db, int *target_db);

#define CODEC_DIGITAL_MUTE 0x80
#define CODEC_ANALOG_MUTE 0x76
#define N_BANDS 8
/* The codec's mic AGC (attack 2 ms, decay 500 ms) with config_glo.txt's max gain and target (6x U2 / 6x U3 change
 * the max gain by 6 dB). Phone mics are hotter and noisier than the Glo's, and the master gain the mic passes through
 * is doubled, so both are lowered: the default 32 dB / -5 dB become x9 / 0.28. */
#define AGC_GAIN_OFFSET_DB 13
#define AGC_TARGET_OFFSET_DB 6
/* Each 6x U2 / 6x U3 moves the max gain away from config_glo.txt's default 32 dB twice as far as on the Glo. */
#define AGC_REF_DB 32
#define AGC_STEP_SCALE 2
/* Below ~-46 dBFS input is phone-mic self-noise: expand it down instead of amplifying it to the target. */
#define AGC_NOISE_GATE 0.005f
/* The firmware scales the mic by 0.0025 x 64 x 9 and casts to int16 unclamped: above ~0.69 it wraps (a loud crack),
 * and echo feedback adds on top. Peaks the AGC attack lets through are bent softly under this ceiling. */
#define MIC_KNEE 0.3f
#define MIC_CEILING 0.45f

char glo_data_dir[1024];
_Atomic float glo_accel[3] = { 0.0f, 0.0f, -1.0f };

static char *pending_config;
static size_t pending_config_len;
static const uint8_t *pending_samples;
static size_t pending_samples_len;
static atomic_int started;
static _Atomic float master_gain = 8.0f;

static _Atomic float level_rms[2];
static _Atomic float level_bands[N_BANDS];
#define SCOPE_LEN 4096
static float scope_buf[SCOPE_LEN];
static atomic_uint scope_pos;
static atomic_int scope_rate;
static atomic_uint underruns;
static atomic_int mic_agc_db = -1;
static atomic_int mic_offset_db;
static atomic_int mic_trim_db;

/* render-thread-only state */
static struct {
    float out_hist[4][2]; /* last four engine frames, oldest first */
    float mic_hist[4][2]; /* last four device mic frames, oldest first */
    double pos;
    float band_z[N_BANDS][2];
    float band_env[N_BANDS];
    float band_coef[N_BANDS][3];
    int band_rate;
    float rms_env[2];
    float agc_env, agc_gain, agc_release, agc_attack, agc_decay, agc_max, agc_target;
    float gate, gate_open, gate_close;
    int agc_on;
    int window_frames, bad_windows, settled, grace, in_skip, ring_resets;
    unsigned window_underruns;
} rs;

void glo_port_get_accel(float out[3])
{
    for (int i = 0; i < 3; i++)
        out[i] = atomic_load_explicit(&glo_accel[i], memory_order_relaxed);
}

void glo_set_config(const char *text, size_t len)
{
    free(pending_config);
    pending_config = malloc(len);
    pending_config_len = pending_config ? len : 0;
    if (pending_config)
        memcpy(pending_config, text, len);
}

void glo_set_samples(const uint8_t *data, size_t len)
{
    pending_samples = data;
    pending_samples_len = len;
}

static size_t load_file(const char *name, uint8_t *dst, size_t max)
{
    char path[1200];
    snprintf(path, sizeof path, "%s/%s", glo_data_dir, name);
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    size_t n = fread(dst, 1, max, f);
    fclose(f);
    return n;
}

/* Every [binaural_beats_program] line ends in "420:0]": parse_binaural_def() then scans past the line for a ','
 * (garbage on the ESP32, a fault on the host when the heap ends there). */
static void fix_binaural_programs(char *cfg)
{
    char *p = strstr(cfg, "[binaural_beats_program]");
    if (!p)
        return;
    char *end = strstr(p + 1, "\n[");
    while ((p = strstr(p, ":[")) && (!end || p < end))
        for (p += 2; *p && *p != ']' && *p != '\n'; p++)
            if (*p == ':')
                *p = ',';
}

static void load_flash(void)
{
    uint8_t *cfg = glo_flash + GLO_FLASH_CONFIG_ADDR;
    size_t cfg_max = GLO_FLASH_CONFIG_SIZE - 1; /* keep a NUL so strstr() terminates */
    if (!load_file("config_glo.txt", cfg, cfg_max) && pending_config)
        memcpy(cfg, pending_config, pending_config_len < cfg_max ? pending_config_len : cfg_max);
    fix_binaural_programs((char *)cfg);

    uint8_t *smp = glo_flash + GLO_FLASH_SAMPLES_ADDR;
    size_t smp_max = GLO_FLASH_SIZE - GLO_FLASH_SAMPLES_ADDR;
    size_t n = load_file("samples.bin", smp, smp_max);
    if (!n && pending_samples) {
        n = pending_samples_len < smp_max ? pending_samples_len : smp_max;
        memcpy(smp, pending_samples, n);
    }
    printf("glo: flash loaded, samples %zu bytes%s\n", n, n ? "" : " (none: background samples are silent)");
}

static atomic_int pending_unlock = -1;
static atomic_int all_unlocked;

/* Runs while no firmware thread is alive, so nothing else touches NVS. */
static void apply_pending_unlock(void)
{
    nvs_handle_t h;
    if (nvs_open("glo_settings", NVS_READWRITE, &h) != ESP_OK)
        return;
    int on = atomic_exchange(&pending_unlock, -1);
    if (on >= 0) {
        nvs_set_i8(h, "XTRA_CHNL", (int8_t)on);
        printf("glo: extra channels %s\n", on ? "unlocked" : "locked");
    }
    int8_t v = 0;
    atomic_store(&all_unlocked, nvs_get_i8(h, "XTRA_CHNL", &v) == ESP_OK && v);
    nvs_close(h);
}

/* A restart clears the Glo's RAM and the firmware relies on that: a stale channel_loop can exceed the channel count
 * after locking, a sequence typed before the restart would be decoded again, and a reset fired while a press was
 * being released leaves its counter set, which the next boot reads as another short press. */
static void reset_firmware_state(void)
{
    channel_loop = channel_loop_remapped = 0;
    short_press_seq_ptr = short_press_seq_cnt = 0;
    short_press_sequence = short_press_volume_plus = short_press_volume_minus = 0;
    button_play = button_volume_plus = button_volume_minus = button_volume_both = short_press_button_play = 0;
}

static void *supervisor(void *arg)
{
    (void)arg;
    for (;;) {
        atomic_store(&glo_kill, 0);
        glo_gpio_reset();
        glo_i2s_reset();
        glo_nvs_set_root(glo_data_dir);
        apply_pending_unlock();
        reset_firmware_state();
        glo_rtos_reset_clock();
        /* The published v1.0 source never sets this (later releases do in app_main), leaving only the fallback loop. */
        glo_run = 1;
        glo_rtos_spawn_app(app_main);
        glo_rtos_wait_all_exited();

        if (atomic_load(&glo_powered_off)) {
            printf("glo: powered off, hold U1 to power on\n");
            const struct timespec nap = { 0, 20000000 };
            while (!atomic_load(&glo_buttons[1]))
                nanosleep(&nap, NULL);
            atomic_store(&glo_powered_off, 0);
        }
        printf("glo: restarting firmware\n");
    }
    return NULL;
}

int glo_start(const char *data_dir)
{
    if (atomic_exchange(&started, 1))
        return 0;
    snprintf(glo_data_dir, sizeof glo_data_dir, "%s", data_dir ? data_dir : ".");
    glo_flash = calloc(1, GLO_FLASH_SIZE);
    if (!glo_flash)
        return -1;
    load_flash();

    pthread_t th;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int rc = pthread_create(&th, &attr, supervisor, NULL);
    pthread_attr_destroy(&attr);
    return rc == 0 ? 0 : -1;
}

/* ---------------------------------------------------------------- render */
static float codec_gain(void)
{
    int dv = codec_digital_volume, av = codec_analog_volume;
    if (dv >= CODEC_DIGITAL_MUTE || av >= CODEC_ANALOG_MUTE || atomic_load(&glo_powered_off))
        return 0.0f;
    return powf(10.0f, -0.5f * (float)(dv + av) / 20.0f) * atomic_load_explicit(&master_gain, memory_order_relaxed);
}

static void setup_bands(int rate)
{
    static const float centers[N_BANDS] = { 60, 150, 350, 700, 1500, 3000, 6000, 12000 };
    for (int b = 0; b < N_BANDS; b++) {
        float w = 2.0f * (float)M_PI * fminf(centers[b], 0.45f * rate) / (float)rate;
        float alpha = sinf(w) / (2.0f * 1.4f);
        float a0 = 1.0f + alpha;
        rs.band_coef[b][0] = alpha / a0;
        rs.band_coef[b][1] = -2.0f * cosf(w) / a0;
        rs.band_coef[b][2] = (1.0f - alpha) / a0;
        rs.band_z[b][0] = rs.band_z[b][1] = 0.0f;
    }
    rs.band_rate = rate;
}

static void setup_agc(int engine_rate)
{
    float r = (float)engine_rate;
    rs.agc_release = expf(-1.0f / (0.1f * r));
    rs.agc_attack = 1.0f - expf(-1.0f / (0.002f * r));
    rs.agc_decay = 1.0f - expf(-1.0f / (0.5f * r));
    rs.gate_open = 1.0f - expf(-1.0f / (0.005f * r));
    rs.gate_close = 1.0f - expf(-1.0f / (0.2f * r));
    int max_db, target_db;
    glo_port_get_agc(&rs.agc_on, &max_db, &target_db);
    max_db = AGC_REF_DB + (max_db - AGC_REF_DB) * AGC_STEP_SCALE;
    int offset_db = AGC_GAIN_OFFSET_DB + atomic_load_explicit(&mic_offset_db, memory_order_relaxed)
        - atomic_load_explicit(&mic_trim_db, memory_order_relaxed);
    rs.agc_max = powf(10.0f, (float)(max_db - offset_db) / 20.0f);
    rs.agc_target = powf(10.0f, (float)(target_db - AGC_TARGET_OFFSET_DB) / 20.0f);
    atomic_store_explicit(&mic_agc_db, rs.agc_on ? max_db : -1, memory_order_relaxed);
}

static void mic_agc(float lr[2])
{
    if (!rs.agc_on)
        return;
    float peak = fmaxf(fabsf(lr[0]), fabsf(lr[1]));
    rs.agc_env = peak > rs.agc_env ? peak : rs.agc_env * rs.agc_release;
    /* The gate fades its own way (5 ms open, 200 ms close), so word tails fade out and the gain is ready at the next onset. */
    float gate = fminf(1.0f, rs.agc_env / AGC_NOISE_GATE);
    gate *= gate;
    rs.gate += (gate < rs.gate ? rs.gate_close : rs.gate_open) * (gate - rs.gate);
    float want = fminf(rs.agc_max, rs.agc_target / fmaxf(rs.agc_env, 1e-6f));
    rs.agc_gain += (want < rs.agc_gain ? rs.agc_attack : rs.agc_decay) * (want - rs.agc_gain);
    lr[0] *= rs.agc_gain * rs.gate;
    lr[1] *= rs.agc_gain * rs.gate;
}

static float mic_limit(float v)
{
    float a = fabsf(v);
    if (a <= MIC_KNEE)
        return v;
    return copysignf(MIC_KNEE + (MIC_CEILING - MIC_KNEE) * tanhf((a - MIC_KNEE) / (MIC_CEILING - MIC_KNEE)), v);
}

/* Catmull-Rom between x0 and x1 at t: far fewer resampling artifacts than linear, at one frame of delay. */
static float cubic(const float h[4][2], int c, float t)
{
    float xm = h[0][c], x0 = h[1][c], x1 = h[2][c], x2 = h[3][c];
    return x0 + 0.5f * t * (x1 - xm + t * (2.0f * xm - 5.0f * x0 + 4.0f * x1 - x2 + t * (3.0f * (x0 - x1) + x2 - xm)));
}

static void push_hist(float h[4][2], float l, float r)
{
    memmove(h[0], h[1], 3 * sizeof h[0]);
    h[3][0] = l;
    h[3][1] = r;
}

static void next_engine_frame(const float *in_lr)
{
    uint32_t f;
    if (glo_i2s_out_pop(&f)) {
        push_hist(rs.out_hist, (float)(int16_t)(f >> 16) / 32768.0f, (float)(int16_t)(f & 0xffff) / 32768.0f);
    } else {
        push_hist(rs.out_hist, 0.0f, 0.0f);
        if (!atomic_load_explicit(&glo_powered_off, memory_order_relaxed))
            atomic_fetch_add_explicit(&underruns, 1, memory_order_relaxed);
        /* The firmware reads one mic frame per frame it writes: a mic frame without output would stay queued as latency. */
        return;
    }
    if (rs.in_skip > 0) {
        rs.in_skip--;
        return;
    }
    int16_t l = 0, r = 0;
    if (in_lr) {
        float mic[2] = { in_lr[0], in_lr[1] };
        mic_agc(mic);
        l = (int16_t)lrintf(mic_limit(mic[0]) * 32767.0f);
        r = (int16_t)lrintf(mic_limit(mic[1]) * 32767.0f);
    }
    glo_i2s_in_push(((uint32_t)(uint16_t)l << 16) | (uint16_t)r);
}

/* The ring starts large and shrinks by a quarter every clean half second, down to its floor and the callback's need.
 * An underrun while shrinking steps back up and ends the search; later, underruns in three consecutive windows (slow
 * CPU, large bursts) grow it by a quarter. The first 3 s after a reset do not count: the firmware may still be booting. */
static void adapt_ring(int frames, int device_rate, int need)
{
    rs.window_frames += frames;
    if (rs.window_frames < device_rate / 2)
        return;
    rs.window_frames = 0;
    unsigned n = atomic_load_explicit(&underruns, memory_order_relaxed);
    int bad = n != rs.window_underruns;
    rs.window_underruns = n;
    int resets = glo_i2s_ring_resets();
    if (resets != rs.ring_resets || !glo_i2s_ring_primed()) {
        rs.ring_resets = resets;
        rs.settled = 0;
        rs.bad_windows = 0;
        rs.grace = 6;
        return;
    }
    if (rs.grace > 0) {
        rs.grace--;
        return;
    }
    int ring = glo_i2s_ring_frames();
    if (bad && !rs.settled) {
        rs.settled = 1;
        glo_i2s_ensure_ring_frames(ring * 4 / 3);
    } else if (bad) {
        if (++rs.bad_windows >= 3) {
            glo_i2s_ensure_ring_frames(ring + (ring / 4 > 64 ? ring / 4 : 64));
            rs.bad_windows = 0;
        }
    } else {
        rs.bad_windows = 0;
        int floor = glo_i2s_ring_floor() > need ? glo_i2s_ring_floor() : need;
        if (!rs.settled && ring > floor) {
            int next = ring * 3 / 4 > floor ? ring * 3 / 4 : floor;
            glo_i2s_resize_ring(next);
            /* The firmware pauses until the ring drains to the new size; the mic frames of that pause would queue up. */
            rs.in_skip += ring - next;
        }
    }
    printf("glo: DEBUG ring %d settled %d\n", glo_i2s_ring_frames(), rs.settled); /* DEBUG */
}

void glo_render(float *out_lr, const float *in_lr, int frames, int device_rate)
{
    if (device_rate <= 0 || frames <= 0)
        return;
    if (rs.band_rate != device_rate)
        setup_bands(device_rate);

    const int engine_rate = atomic_load(&glo_engine_rate);
    const double step = (double)engine_rate / (double)device_rate;
    setup_agc(engine_rate);
    const int need = 2 * ((int)(frames * step) + 2);
    glo_i2s_ensure_ring_frames(need);
    const float gain = codec_gain();
    double sum[2] = { 0, 0 };
    double band_sum[N_BANDS] = { 0 };
    const unsigned scope_pos_w = atomic_load_explicit(&scope_pos, memory_order_relaxed);

    for (int i = 0; i < frames; i++) {
        rs.pos += step;
        if (in_lr)
            push_hist(rs.mic_hist, in_lr[2 * i], in_lr[2 * i + 1]);
        while (rs.pos >= 1.0) {
            /* The engine frame lies t device frames before frame i (here i-1, for the cubic's lookahead). */
            float mic[2];
            if (in_lr) {
                float t = (float)((rs.pos - 1.0) / step);
                for (int c = 0; c < 2; c++)
                    mic[c] = cubic(rs.mic_hist, c, 1.0f - t);
            }
            next_engine_frame(in_lr ? mic : NULL);
            rs.pos -= 1.0;
        }
        float t = (float)rs.pos;
        for (int c = 0; c < 2; c++) {
            float v = cubic(rs.out_hist, c, t) * gain;
            if (fabsf(v) > 0.8f)
                v = copysignf(0.8f + 0.2f * tanhf((fabsf(v) - 0.8f) / 0.2f), v);
            out_lr[2 * i + c] = v;
            sum[c] += (double)v * v;
        }
        float mono = 0.5f * (out_lr[2 * i] + out_lr[2 * i + 1]);
        for (int b = 0; b < N_BANDS; b++) {
            float *z = rs.band_z[b], *k = rs.band_coef[b];
            float y = k[0] * mono + z[0];
            z[0] = -k[1] * y + z[1];
            z[1] = -k[0] * mono - k[2] * y;
            band_sum[b] += (double)y * y;
        }
        scope_buf[(scope_pos_w + i) % SCOPE_LEN] = mono;
    }
    atomic_store_explicit(&scope_pos, scope_pos_w + (unsigned)frames, memory_order_release);
    atomic_store_explicit(&scope_rate, device_rate, memory_order_relaxed);

    adapt_ring(frames, device_rate, need);

    const float a = 0.3f;
    for (int c = 0; c < 2; c++) {
        rs.rms_env[c] += a * (sqrtf((float)(sum[c] / frames)) - rs.rms_env[c]);
        atomic_store_explicit(&level_rms[c], rs.rms_env[c], memory_order_relaxed);
    }
    for (int b = 0; b < N_BANDS; b++) {
        rs.band_env[b] += a * (sqrtf((float)(band_sum[b] / frames)) - rs.band_env[b]);
        atomic_store_explicit(&level_bands[b], rs.band_env[b], memory_order_relaxed);
    }
}

/* ---------------------------------------------------------------- controls / state */
void glo_set_accel(float x, float y, float z)
{
    atomic_store_explicit(&glo_accel[0], x, memory_order_relaxed);
    atomic_store_explicit(&glo_accel[1], y, memory_order_relaxed);
    atomic_store_explicit(&glo_accel[2], z, memory_order_relaxed);
}

void glo_set_button(int idx, int down)
{
    if (idx >= 1 && idx <= 3)
        atomic_store(&glo_buttons[idx], down ? 1 : 0);
}

void glo_set_ring_frames(int frames) { glo_i2s_set_ring_frames(frames); }
void glo_set_master_gain(float gain) { atomic_store(&master_gain, gain); }
void glo_set_mic_offset_db(int db) { atomic_store(&mic_offset_db, db); }
void glo_set_mic_trim_db(int db) { atomic_store(&mic_trim_db, db); }

void glo_get_led(int *r, int *g, int *b)
{
    int on = !atomic_load(&glo_powered_off);
    *r = on && !glo_gpio_level(GLO_PIN_LED_R);
    *g = on && !glo_gpio_level(GLO_PIN_LED_G);
    *b = on && !glo_gpio_level(GLO_PIN_LED_B);
}

int glo_get_channel(void) { return channel_loop; }
int glo_get_channel_id(void) { return channel_loop_remapped; }
int glo_get_engine_rate(void) { return atomic_load(&glo_engine_rate); }
int glo_is_powered(void) { return !atomic_load(&glo_powered_off); }
unsigned glo_get_underruns(void) { return atomic_load_explicit(&underruns, memory_order_relaxed); }
int glo_get_mic_agc_db(void) { return atomic_load_explicit(&mic_agc_db, memory_order_relaxed); }

/* Read without locking while the firmware's button thread writes: only for display. */
int glo_get_button_sequence(char *out, int max)
{
    int n = *(volatile int *)&short_press_seq_ptr;
    if (n > max)
        n = max;
    for (int i = 0; i < n; i++)
        out[i] = ((volatile char *)short_press_seq)[i];
    return n < 0 ? 0 : n;
}

int glo_get_pending_command(void)
{
    return (*(volatile int *)&short_press_sequence ? 1 : 0) | (*(volatile int *)&short_press_volume_plus ? 2 : 0)
        | (*(volatile int *)&short_press_volume_minus ? 4 : 0);
}

void glo_set_all_channels_unlocked(int on)
{
    atomic_store(&pending_unlock, on ? 1 : 0);
    glo_request_kill(0);
}

int glo_get_all_channels_unlocked(void) { return atomic_load(&all_unlocked); }

float glo_get_volume_db(void)
{
    int dv = codec_digital_volume, av = codec_analog_volume;
    if (dv >= CODEC_DIGITAL_MUTE || av >= CODEC_ANALOG_MUTE)
        return -1000.0f;
    return -0.5f * (float)(dv + av);
}

void glo_get_levels(float *rms_l, float *rms_r, float bands[8])
{
    *rms_l = atomic_load_explicit(&level_rms[0], memory_order_relaxed);
    *rms_r = atomic_load_explicit(&level_rms[1], memory_order_relaxed);
    for (int b = 0; b < N_BANDS; b++)
        bands[b] = atomic_load_explicit(&level_bands[b], memory_order_relaxed);
}

/* Read without locking while glo_render writes: only for display, the oldest samples may already be overwritten. */
int glo_get_scope(float *out, int n)
{
    if (n > SCOPE_LEN)
        n = SCOPE_LEN;
    unsigned end = atomic_load_explicit(&scope_pos, memory_order_acquire);
    for (int i = 0; i < n; i++)
        out[i] = ((volatile float *)scope_buf)[(end - (unsigned)n + (unsigned)i) % SCOPE_LEN];
    return atomic_load_explicit(&scope_rate, memory_order_relaxed);
}
