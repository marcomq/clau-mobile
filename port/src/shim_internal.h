/* Shared state between the fake ESP32 shim modules and the host API. Part of flono-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0. */
#pragma once

#include <stdatomic.h>
#include <stdint.h>
#include "glo_port_esp.h"

#define GLO_FLASH_SIZE (16 * 1024 * 1024)
#define GLO_FLASH_CONFIG_ADDR 0x90000
#define GLO_FLASH_CONFIG_SIZE 0x10000
#define GLO_FLASH_SAMPLES_ADDR 0x330000 /* SAMPLES_BASE: v1.0.118 moved it from 0xa0000 */

#define GLO_PIN_U1 34
#define GLO_PIN_U2 0
#define GLO_PIN_U3 35
#define GLO_PIN_LED_R 15
#define GLO_PIN_LED_G 14
#define GLO_PIN_LED_B 13
#define GLO_PIN_POWER_HOLD 2

extern uint8_t *glo_flash;
extern char glo_data_dir[1024];

extern atomic_int glo_kill;
extern atomic_int glo_powered_off;
extern atomic_int glo_buttons[4];
extern atomic_int glo_engine_rate;
extern _Atomic float glo_accel[3];

void glo_gpio_reset(void);
int glo_gpio_level(int pin);

void glo_i2s_reset(void);
int glo_i2s_out_pop(uint32_t *frame);
void glo_i2s_in_push(uint32_t frame);
void glo_i2s_set_ring_frames(int frames);
void glo_i2s_ensure_ring_frames(int frames);
int glo_i2s_ring_frames(void);
int glo_i2s_ring_floor(void);
int glo_i2s_ring_resets(void);
void glo_i2s_resize_ring(int frames);
int glo_i2s_ring_primed(void);

void glo_rtos_reset_clock(void);
void glo_rtos_spawn_app(void (*entry)(void));
void glo_rtos_wait_all_exited(void);
void glo_request_kill(int power_off);

void glo_nvs_set_root(const char *dir);
