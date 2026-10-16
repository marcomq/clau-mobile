/* glo_core.h - host API of the Glo firmware running on the fake-ESP32 shim. Part of clau-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0. */
#ifndef GLO_CORE_H
#define GLO_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Optional, before glo_start: flash contents. Otherwise <data_dir>/config_glo.txt and <data_dir>/samples.bin are used. */
void glo_set_config(const char *text, size_t len);
void glo_set_samples(const uint8_t *data, size_t len);

/* Boots the firmware (app_main) on its own threads. data_dir must be writable (NVS lives there). 0 on success. */
int glo_start(const char *data_dir);

/* Realtime audio callback: interleaved stereo float, device rate. in_lr may be NULL (no mic). */
void glo_render(float *out_lr, const float *in_lr, int frames, int device_rate);

void glo_set_accel(float x, float y, float z); /* in g, (0,0,-1) = lying flat, face up */
void glo_set_button(int idx /* 1..3 = U1 PWR, U2, U3 */, int down);
void glo_set_ring_frames(int frames);           /* engine-side buffering, i.e. added latency */
void glo_set_master_gain(float gain);

void glo_get_led(int *r, int *g, int *b);
int glo_get_channel(void);
int glo_get_channel_id(void); /* index of the running channel in config_glo.txt [channels]; differs after reordering */
int glo_get_engine_rate(void);
int glo_is_powered(void);
unsigned glo_get_underruns(void); /* engine frames glo_render had to fill with silence while powered */
void glo_set_mic_offset_db(int db); /* lowers the mic AGC max gain further, for hotter mics */
void glo_set_mic_trim_db(int db);   /* user trim of the mic AGC max gain (app setting), added on top */
int glo_get_mic_agc_db(void);      /* the mic AGC max gain in the firmware's dB scale (6x U2 / 6x U3: +-12 dB), -1 while the AGC is off */
void glo_get_levels(float *rms_l, float *rms_r, float bands[8]);
int glo_get_button_sequence(char *out, int max); /* short presses typed so far, 'a' = U1, 'b' = U2, 'c' = U3; 0 when none */
int glo_get_pending_command(void); /* decoded commands the running channel has not picked up: 1 = sequence, 2 = U2 tap, 4 = U3 tap */
float glo_get_volume_db(void);     /* output volume (digital + analog), -1000 when muted */
void glo_set_all_channels_unlocked(int on); /* stores the extra-channel unlock (like -+-+-+-+ U1) and restarts the firmware */
int glo_get_all_channels_unlocked(void);

#ifdef __cplusplus
}
#endif

#endif /* GLO_CORE_H */
