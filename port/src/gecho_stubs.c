/*
 * gecho_stubs.c - definitions v1.0.118 only provides in the Gecho build but references from the Whale build.
 * Part of clau-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0.
 */
#include <stdint.h>
#include "glo_config.h"

const char *BINARY_ID = "[0x000000001234]\n\0\0";
const char *FW_VERSION = "[Gv2/1.0.118]";
const char *GLO_HASH = "f495dc2975eec3287700ad297a9134a037a3a411";

float ir_res[IR_SENSORS];
int sensor_active[4];
int sequencer_LEDs_timing_seq;
uint8_t binaural_LEDs_timing_seq;
int SENSORS_LEDS_indication_enabled;
int user_buttons_cnt;
int btn_event_ext;
int next_event_lock;
uint8_t load_SD_card_sample;
uint8_t SD_card_sample_buffer_half;
int use_acc_or_ir_sensors = PARAMETER_CONTROL_SENSORS_ACCELEROMETER;
int long_press_set_button;

void LED_W8_all_OFF(void) {}
void LED_B5_all_OFF(void) {}
void LED_O4_all_OFF(void) {}
void LED_R8_all_OFF(void) {}
void LEDs_all_OFF(void) {}
void LED_R8_set(int led, int status) { (void)led; (void)status; }
void LED_O4_set(int led, int status) { (void)led; (void)status; }
void LED_R8_set_byte(int value) { (void)value; }
void LED_O4_set_byte(int value) { (void)value; }
void LED_B5_set_byte(int value) { (void)value; }
void LED_W8_set_byte(int value) { (void)value; }
void LED_sequencer_indicators(void) {}
void indicate_context_setting(uint16_t leds, int blink, int delay) { (void)leds; (void)blink; (void)delay; }
void indicate_error(uint16_t leds, int blink, int delay) { (void)leds; (void)blink; (void)delay; }

/* v1.0 played a voice sample here; v1.0.118 dropped the implementation. */
void voice_say(const char *segment) { (void)segment; }

/* ui.c still calls the v1.0 name; v1.0.118 renamed it to settings_reset(). */
void factory_reset(void) { settings_reset(); }
