/* Host API: boot supervisor, realtime render (resample + codec gain), meters. Part of glo-mobile, GPL-3.0. */
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include "glo_core.h"
#include "shim_internal.h"

extern void app_main(void);
extern int codec_digital_volume, codec_analog_volume;
extern int channel_loop;
extern int glo_run;

#define CODEC_DIGITAL_MUTE 0x80
#define CODEC_ANALOG_MUTE 0x76
#define N_BANDS 8

char glo_data_dir[1024];
_Atomic float glo_accel[3] = { 0.0f, 0.0f, -1.0f };

static char *pending_config;
static size_t pending_config_len;
static const uint8_t *pending_samples;
static size_t pending_samples_len;
static atomic_int started;
static _Atomic float master_gain = 4.0f;

static _Atomic float level_rms[2];
static _Atomic float level_bands[N_BANDS];

/* render-thread-only state */
static struct {
    float prev[2], cur[2];
    double pos;
    float band_z[N_BANDS][2];
    float band_env[N_BANDS];
    float band_coef[N_BANDS][3];
    int band_rate;
    float rms_env[2];
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
    memcpy(pending_config, text, len);
    pending_config_len = len;
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

static void load_flash(void)
{
    uint8_t *cfg = glo_flash + GLO_FLASH_CONFIG_ADDR;
    size_t cfg_max = GLO_FLASH_CONFIG_SIZE - 1; /* keep a NUL so strstr() terminates */
    if (!load_file("config_glo.txt", cfg, cfg_max) && pending_config)
        memcpy(cfg, pending_config, pending_config_len < cfg_max ? pending_config_len : cfg_max);

    uint8_t *smp = glo_flash + GLO_FLASH_SAMPLES_ADDR;
    size_t smp_max = GLO_FLASH_SIZE - GLO_FLASH_SAMPLES_ADDR;
    size_t n = load_file("samples.bin", smp, smp_max);
    if (!n && pending_samples) {
        n = pending_samples_len < smp_max ? pending_samples_len : smp_max;
        memcpy(smp, pending_samples, n);
    }
    printf("glo: flash loaded, samples %zu bytes%s\n", n, n ? "" : " (none: background samples are silent)");
}

static void *supervisor(void *arg)
{
    (void)arg;
    for (;;) {
        atomic_store(&glo_kill, 0);
        glo_gpio_reset();
        glo_i2s_reset();
        glo_nvs_set_root(glo_data_dir);
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

static void next_engine_frame(const float *in_lr)
{
    rs.prev[0] = rs.cur[0];
    rs.prev[1] = rs.cur[1];
    uint32_t f;
    if (glo_i2s_out_pop(&f)) {
        rs.cur[0] = (float)(int16_t)(f >> 16) / 32768.0f;
        rs.cur[1] = (float)(int16_t)(f & 0xffff) / 32768.0f;
    } else {
        rs.cur[0] = rs.cur[1] = 0.0f;
    }
    int16_t l = 0, r = 0;
    if (in_lr) {
        l = (int16_t)lrintf(fmaxf(-1.0f, fminf(1.0f, in_lr[0])) * 32767.0f);
        r = (int16_t)lrintf(fmaxf(-1.0f, fminf(1.0f, in_lr[1])) * 32767.0f);
    }
    glo_i2s_in_push(((uint32_t)(uint16_t)l << 16) | (uint16_t)r);
}

void glo_render(float *out_lr, const float *in_lr, int frames, int device_rate)
{
    if (device_rate <= 0 || frames <= 0)
        return;
    if (rs.band_rate != device_rate)
        setup_bands(device_rate);

    const double step = (double)atomic_load(&glo_engine_rate) / (double)device_rate;
    const float gain = codec_gain();
    double sum[2] = { 0, 0 };
    double band_sum[N_BANDS] = { 0 };

    for (int i = 0; i < frames; i++) {
        rs.pos += step;
        while (rs.pos >= 1.0) {
            next_engine_frame(in_lr ? in_lr + 2 * i : NULL);
            rs.pos -= 1.0;
        }
        float t = (float)rs.pos;
        for (int c = 0; c < 2; c++) {
            float v = (rs.prev[c] + (rs.cur[c] - rs.prev[c]) * t) * gain;
            v = fmaxf(-1.0f, fminf(1.0f, v));
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
    }

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

void glo_get_led(int *r, int *g, int *b)
{
    int on = !atomic_load(&glo_powered_off);
    *r = on && !glo_gpio_level(GLO_PIN_LED_R);
    *g = on && !glo_gpio_level(GLO_PIN_LED_G);
    *b = on && !glo_gpio_level(GLO_PIN_LED_B);
}

int glo_get_channel(void) { return channel_loop; }
int glo_get_engine_rate(void) { return atomic_load(&glo_engine_rate); }
int glo_is_powered(void) { return !atomic_load(&glo_powered_off); }

void glo_get_levels(float *rms_l, float *rms_r, float bands[8])
{
    *rms_l = atomic_load_explicit(&level_rms[0], memory_order_relaxed);
    *rms_r = atomic_load_explicit(&level_rms[1], memory_order_relaxed);
    for (int b = 0; b < N_BANDS; b++)
        bands[b] = atomic_load_explicit(&level_bands[b], memory_order_relaxed);
}
