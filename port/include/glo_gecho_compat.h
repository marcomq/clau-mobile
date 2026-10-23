/*
 * glo_gecho_compat.h - Gecho-only names that v1.0.118 uses unguarded in the Whale build.
 * Part of flono, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0.
 * LEDs, IR sensors, SD card and the Gecho menu do not exist on the Whale: macros are no-ops, functions are stubs.
 */
#pragma once
#include <stdint.h>
#include <dirent.h>

#define GLO_NOP ((void)0)

#define IR_SENSORS 4
#define IR_SENSOR_VALUE_S1 ir_res[0]
#define IR_SENSOR_VALUE_S2 ir_res[1]
#define IR_SENSOR_VALUE_S3 ir_res[2]
#define IR_SENSOR_VALUE_S4 ir_res[3]
#define IR_sensors_THRESHOLD_1 0.100f
#define IR_sensors_THRESHOLD_2 0.194f
#define IR_sensors_THRESHOLD_3 0.288f
#define IR_sensors_THRESHOLD_4 0.381f
#define IR_sensors_THRESHOLD_5 0.475f
#define IR_sensors_THRESHOLD_6 0.569f
#define IR_sensors_THRESHOLD_7 0.663f
#define IR_sensors_THRESHOLD_8 0.756f
#define IR_sensors_THRESHOLD_9 0.850f

#define SENSOR_ID_RED 0
#define SENSOR_ID_ORANGE 1
#define SENSOR_ID_BLUE 2
#define SENSOR_ID_WHITE 3
#define SENSOR_THRESHOLD_ORANGE_4 (ir_res[1] > IR_sensors_THRESHOLD_7)
#define SENSOR_THRESHOLD_ORANGE_3 (ir_res[1] > IR_sensors_THRESHOLD_5)
#define SENSOR_THRESHOLD_ORANGE_2 (ir_res[1] > IR_sensors_THRESHOLD_3)
#define SENSOR_THRESHOLD_ORANGE_1 (ir_res[1] > IR_sensors_THRESHOLD_1)
#define SENSOR_THRESHOLD_RED_9 (ir_res[0] > IR_sensors_THRESHOLD_9)
#define SENSOR_THRESHOLD_RED_8 (ir_res[0] > IR_sensors_THRESHOLD_8)
#define SENSOR_THRESHOLD_RED_7 (ir_res[0] > IR_sensors_THRESHOLD_7)
#define SENSOR_THRESHOLD_RED_6 (ir_res[0] > IR_sensors_THRESHOLD_6)
#define SENSOR_THRESHOLD_RED_5 (ir_res[0] > IR_sensors_THRESHOLD_5)
#define SENSOR_THRESHOLD_RED_4 (ir_res[0] > IR_sensors_THRESHOLD_4)
#define SENSOR_THRESHOLD_RED_3 (ir_res[0] > IR_sensors_THRESHOLD_3)
#define SENSOR_THRESHOLD_RED_2 (ir_res[0] > IR_sensors_THRESHOLD_2)
#define SENSOR_THRESHOLD_RED_1 (ir_res[0] > IR_sensors_THRESHOLD_1)
#define SENSOR_THRESHOLD_BLUE_1 (ir_res[2] > IR_sensors_THRESHOLD_8)
#define SENSOR_THRESHOLD_BLUE_2 (ir_res[2] > IR_sensors_THRESHOLD_6)
#define SENSOR_THRESHOLD_BLUE_3 (ir_res[2] > IR_sensors_THRESHOLD_4)
#define SENSOR_THRESHOLD_BLUE_4 (ir_res[2] > IR_sensors_THRESHOLD_2)
#define SENSOR_THRESHOLD_BLUE_5 (ir_res[2] > IR_sensors_THRESHOLD_1)
#define SENSOR_THRESHOLD_WHITE_1 (ir_res[3] > IR_sensors_THRESHOLD_8)
#define SENSOR_THRESHOLD_WHITE_2 (ir_res[3] > IR_sensors_THRESHOLD_7)
#define SENSOR_THRESHOLD_WHITE_3 (ir_res[3] > IR_sensors_THRESHOLD_6)
#define SENSOR_THRESHOLD_WHITE_4 (ir_res[3] > IR_sensors_THRESHOLD_5)
#define SENSOR_THRESHOLD_WHITE_5 (ir_res[3] > IR_sensors_THRESHOLD_4)
#define SENSOR_THRESHOLD_WHITE_6 (ir_res[3] > IR_sensors_THRESHOLD_3)
#define SENSOR_THRESHOLD_WHITE_7 (ir_res[3] > IR_sensors_THRESHOLD_2)
#define SENSOR_THRESHOLD_WHITE_8 (ir_res[3] > IR_sensors_THRESHOLD_1)

#define SEQUENCER_INDICATOR_DIV 4
#define SETTINGS_INDICATOR_BASE 0x101
#define SETTINGS_INDICATOR_ANIMATE_LEFT_4 SETTINGS_INDICATOR_BASE+1
#define SETTINGS_INDICATOR_ANIMATE_RIGHT_4 SETTINGS_INDICATOR_BASE+2
#define SETTINGS_INDICATOR_ANIMATE_LEFT_8 SETTINGS_INDICATOR_BASE+3
#define SETTINGS_INDICATOR_ANIMATE_RIGHT_8 SETTINGS_INDICATOR_BASE+4
#define SETTINGS_INDICATOR_ANIMATE_IRS SETTINGS_INDICATOR_BASE+5
#define SETTINGS_INDICATOR_ANIMATE_3DS SETTINGS_INDICATOR_BASE+6
#define SETTINGS_INDICATOR_ANIMATE_ALL_LEDS_OFF SETTINGS_INDICATOR_BASE+7
#define SETTINGS_INDICATOR_ANIMATE_ALL_LEDS_ON SETTINGS_INDICATOR_BASE+8
#define SETTINGS_INDICATOR_ANIMATE_FILL_8_LEFT SETTINGS_INDICATOR_BASE+9
#define SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT SETTINGS_INDICATOR_BASE+10
#define SETTINGS_INDICATOR_ANIMATE_CLEAR_8_LEFT SETTINGS_INDICATOR_BASE+11
#define SETTINGS_INDICATOR_ANIMATE_CLEAR_8_RIGHT SETTINGS_INDICATOR_BASE+12
#define SETTINGS_INDICATOR_EQ_BASS SETTINGS_INDICATOR_BASE+21
#define SETTINGS_INDICATOR_EQ_TREBLE SETTINGS_INDICATOR_BASE+22
#define SETTINGS_INDICATOR_MIDI_POLYPHONY SETTINGS_INDICATOR_BASE+30

#define LED_SIG_ON GLO_NOP
#define LED_SIG_OFF GLO_NOP
#define LED_RDY_ON GLO_NOP
#define LED_RDY_OFF GLO_NOP
#define LED_R8_0_ON GLO_NOP
#define LED_R8_0_OFF GLO_NOP
#define LED_R8_1_ON GLO_NOP
#define LED_R8_1_OFF GLO_NOP
#define LED_R8_2_ON GLO_NOP
#define LED_R8_2_OFF GLO_NOP
#define LED_R8_3_ON GLO_NOP
#define LED_R8_3_OFF GLO_NOP
#define LED_R8_4_ON GLO_NOP
#define LED_R8_4_OFF GLO_NOP
#define LED_R8_5_ON GLO_NOP
#define LED_R8_5_OFF GLO_NOP
#define LED_R8_6_ON GLO_NOP
#define LED_R8_6_OFF GLO_NOP
#define LED_R8_7_ON GLO_NOP
#define LED_R8_7_OFF GLO_NOP
#define LED_O4_0_ON GLO_NOP
#define LED_O4_0_OFF GLO_NOP
#define LED_O4_1_ON GLO_NOP
#define LED_O4_1_OFF GLO_NOP
#define LED_O4_2_ON GLO_NOP
#define LED_O4_2_OFF GLO_NOP
#define LED_O4_3_ON GLO_NOP
#define LED_O4_3_OFF GLO_NOP

/* The Whale has no SET, U4 or RST button. */
#define BUTTON_SET_ON 0
#define BUTTON_U4_ON 0
#define BUTTON_RST_ON 0
#define ANY_BUTTON_ON (BUTTON_U1_ON || BUTTON_U2_ON || BUTTON_U3_ON)
#define USER_BUTTON_ON ANY_BUTTON_ON
#define NO_BUTTON_ON (!ANY_BUTTON_ON)
#define BUTTON_ON(x) ((x)==1?BUTTON_U1_ON:((x)==2?BUTTON_U2_ON:((x)==3?BUTTON_U3_ON:0)))
#define WHICH_USER_BUTTON_ON (BUTTON_U1_ON?1:BUTTON_U2_ON?2:BUTTON_U3_ON?3:0)

#define BUTTON_SET 0
#define BUTTON_1 1
#define BUTTON_2 2
#define BUTTON_3 3
#define BUTTON_4 4
#define BUTTON_RST 5
#define BUTTONS_N 6
#define BUTTON_EVENT_NONE 0
#define BUTTON_EVENT_SHORT_PRESS 10
#define BUTTON_EVENT_LONG_PRESS 20
#define BUTTON_EVENT_SET_PLUS 30
#define BUTTON_EVENT_RST_PLUS 40
#define BUTTON_EVENT_LONG_RELEASE 50
#define BUTTON_EVENT_PRESS 60
#define BUTTON_EVENT_RELEASE 70

#define SD_CARD_SAMPLE_NOT_USED 0
#define SD_CARD_SAMPLE_OPENING 1
#define SD_CARD_SAMPLE_OPENED 2

#define PARAMETER_CONTROL_SENSORS_ACCELEROMETER 0
#define PARAMETER_CONTROL_SENSORS_IRS 1
#define PARAMETER_CONTROL_SENSORS_DEFAULT PARAMETER_CONTROL_SENSORS_IRS
#define PARAMETER_CONTROL_SELECTED_ACC (!use_acc_or_ir_sensors)
#define PARAMETER_CONTROL_SELECTED_IRS (use_acc_or_ir_sensors)

#define CONTEXT_MENU_AGC_MAX_GAIN 1
#define CONTEXT_MENU_ANALOG_VOLUME 2
#define CONTEXT_MENU_TRANSPOSE 5

#define MAIN_MENU_LEVEL_1 1
#define MAIN_MENU_LEVEL_2 2
#define MAIN_MENU_LEVEL_3 3
#define MAIN_MENU_LEDS_OFF_LEVEL MAIN_MENU_LEVEL_1
#define MAIN_MENU_LEDS_OFF_BTN BUTTON_1
#define MAIN_MENU_SENSORS_CONFIG_LEVEL MAIN_MENU_LEVEL_1
#define MAIN_MENU_SENSORS_CONFIG_BTN BUTTON_2
#define MAIN_MENU_ACC_INVERT_LEVEL MAIN_MENU_LEVEL_1
#define MAIN_MENU_ACC_INVERT_BTN BUTTON_3
#define MAIN_MENU_ACC_ORIENTATION_LEVEL MAIN_MENU_LEVEL_1
#define MAIN_MENU_ACC_ORIENTATION_BTN BUTTON_4
#define MAIN_MENU_AGC_ENABLE_LEVEL MAIN_MENU_LEVEL_2
#define MAIN_MENU_AGC_ENABLE_BTN BUTTON_1
#define MAIN_MENU_AUTO_POWER_OFF_LEVEL MAIN_MENU_LEVEL_2
#define MAIN_MENU_AUTO_POWER_OFF_BTN BUTTON_2
#define MAIN_MENU_SD_CARD_SPEED_LEVEL MAIN_MENU_LEVEL_2
#define MAIN_MENU_SD_CARD_SPEED_BTN BUTTON_3
#define MAIN_MENU_SAMPLING_RATE_LEVEL MAIN_MENU_LEVEL_2
#define MAIN_MENU_SAMPLING_RATE_BTN BUTTON_4
#define MAIN_MENU_MIDI_CTRL_SETUP_LEVEL MAIN_MENU_LEVEL_3
#define MAIN_MENU_MIDI_CTRL_SETUP_BTN BUTTON_1
#define MAIN_MENU_MIDI_CTRL_RESET_LEVEL MAIN_MENU_LEVEL_3
#define MAIN_MENU_MIDI_CTRL_RESET_BTN BUTTON_2
#define MAIN_MENU_MIDI_SYNC_MODE_LEVEL MAIN_MENU_LEVEL_3
#define MAIN_MENU_MIDI_SYNC_MODE_BTN BUTTON_3
#define MAIN_MENU_MIDI_POLYPHONY_LEVEL MAIN_MENU_LEVEL_3
#define MAIN_MENU_MIDI_POLYPHONY_BTN BUTTON_4

#define GET_USER_NUMBER_CANCELED 9999

#define DYNAMIC_LIMITER_THRESHOLD_ECHO_DCO DYNAMIC_LIMITER_THRESHOLD_ECHO
#define DYNAMIC_LIMITER_THRESHOLD_ECHO_V1 8000.0f

/* Defined in v1.0.118 codec.h only for the Gecho; same period as TIMING_EVERY_200_MS. */
#define TIMING_BY_SAMPLE_EVERY_100_MS sampleCounter%(2*I2S_AUDIOFREQ/10)

#ifdef __cplusplus
extern "C" {
#endif

extern float ir_res[];
extern int sequencer_LEDs_timing_seq;
extern uint8_t binaural_LEDs_timing_seq;
extern int SENSORS_LEDS_indication_enabled;
extern int user_buttons_cnt;
extern int btn_event_ext;
extern int next_event_lock;
extern uint8_t load_SD_card_sample;
extern uint8_t SD_card_sample_buffer_half;
extern int use_acc_or_ir_sensors;
extern int long_press_set_button;
extern const char *FW_VERSION;
extern const char *BINARY_ID;

void LED_W8_all_OFF(void);
void LED_B5_all_ON(void);
void LED_B5_all_OFF(void);
void LED_O4_all_OFF(void);
void LED_R8_all_OFF(void);
void LEDs_all_OFF(void);
void LED_R8_set(int led, int status);
void LED_O4_set(int led, int status);
void LED_W8_set(int led, int status);
void LED_R8_set_byte(int value);
void LED_O4_set_byte(int value);
void LED_B5_set_byte(int value);
void LED_W8_set_byte(int value);
void display_number_of_chords(int row1, int row2);
void display_BCD_numbers(char *digits, int length);
void indicate_context_setting(uint16_t leds, int blink, int delay);
void indicate_error(uint16_t leds, int blink, int delay);
void indicate_eq_bass_setting(int value, uint16_t setting);
void LED_sequencer_indicate_position(int chord);

uint64_t get_user_number(void);
void firmware_update_SD(void);
void factory_data_load_SD(void);
void factory_reset_firmware(void);
int reload_config(void);
void factory_reset(void);

#ifdef __cplusplus
}
#endif
