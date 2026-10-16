/*
 * glo_port_esp.h - minimal fake ESP-IDF 4.4 / FreeRTOS API surface used by the Glo firmware.
 * Every fake system header under port/include just includes this file.
 * Part of glo-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0.
 */
#ifndef GLO_PORT_ESP_H
#define GLO_PORT_ESP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------- attributes */
#define IRAM_ATTR
#define DRAM_ATTR
#define EXT_RAM_ATTR
#define RTC_DATA_ATTR
#define RTC_NOINIT_ATTR
#define WORD_ALIGNED_ATTR __attribute__((aligned(4)))
#define IRAM_DATA_ATTR

/* ---------------------------------------------------------------- errors */
typedef int esp_err_t;
#define ESP_OK                      0
#define ESP_FAIL                    -1
#define ESP_ERR_NO_MEM              0x101
#define ESP_ERR_INVALID_ARG         0x102
#define ESP_ERR_INVALID_STATE       0x103
#define ESP_ERR_INVALID_SIZE        0x104
#define ESP_ERR_NOT_FOUND           0x105
#define ESP_ERR_NOT_SUPPORTED       0x106
#define ESP_ERR_TIMEOUT             0x107
#define ESP_ERR_INVALID_RESPONSE    0x108
#define ESP_ERR_INVALID_CRC         0x109
#define ESP_ERR_INVALID_VERSION     0x10A
#define ESP_ERR_INVALID_MAC         0x10B
#define ESP_ERR_NVS_BASE            0x1100
#define ESP_ERR_NVS_NOT_INITIALIZED (ESP_ERR_NVS_BASE + 0x01)
#define ESP_ERR_NVS_NOT_FOUND       (ESP_ERR_NVS_BASE + 0x02)
#define ESP_ERR_NVS_TYPE_MISMATCH   (ESP_ERR_NVS_BASE + 0x03)
#define ESP_ERR_NVS_READ_ONLY       (ESP_ERR_NVS_BASE + 0x04)
#define ESP_ERR_NVS_NOT_ENOUGH_SPACE (ESP_ERR_NVS_BASE + 0x05)
#define ESP_ERR_NVS_INVALID_NAME    (ESP_ERR_NVS_BASE + 0x06)
#define ESP_ERR_NVS_INVALID_HANDLE  (ESP_ERR_NVS_BASE + 0x07)
#define ESP_ERR_NVS_INVALID_LENGTH  (ESP_ERR_NVS_BASE + 0x0c)
#define ESP_ERR_NVS_NO_FREE_PAGES   (ESP_ERR_NVS_BASE + 0x0d)
#define ESP_ERR_NVS_NEW_VERSION_FOUND (ESP_ERR_NVS_BASE + 0x10)
#define ESP_ERR_OTA_BASE            0x1500
#define ESP_ERROR_CHECK(x) do { esp_err_t __e = (x); (void)__e; } while (0)
const char *esp_err_to_name(esp_err_t code);

/* ---------------------------------------------------------------- FreeRTOS */
typedef uint32_t TickType_t;
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);
typedef void *QueueHandle_t;
typedef void *SemaphoreHandle_t;
typedef uint32_t StackType_t;
#define pdTRUE  1
#define pdFALSE 0
#define pdPASS  1
#define pdFAIL  0
#define portMAX_DELAY ((TickType_t)0xffffffffUL)
#define portTICK_PERIOD_MS ((TickType_t)1)
#define portTICK_RATE_MS portTICK_PERIOD_MS
#define configTICK_RATE_HZ 1000
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define tskNO_AFFINITY 0x7fffffff
#define tskIDLE_PRIORITY 0
#define configMAX_PRIORITIES 25
#define portNUM_PROCESSORS 2
#define portYIELD() glo_task_yield()
#define taskYIELD() glo_task_yield()
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
#define taskENTER_CRITICAL(m) ((void)(m))
#define taskEXIT_CRITICAL(m) ((void)(m))
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0

void glo_task_yield(void);
void vTaskDelay(const TickType_t ticks);
TickType_t xTaskGetTickCount(void);
BaseType_t xTaskCreatePinnedToCore(TaskFunction_t fn, const char *name, uint32_t stack, void *param,
                                   UBaseType_t prio, TaskHandle_t *handle, BaseType_t core);
BaseType_t xTaskCreate(TaskFunction_t fn, const char *name, uint32_t stack, void *param,
                       UBaseType_t prio, TaskHandle_t *handle);
void vTaskDelete(TaskHandle_t handle);
void vTaskSuspend(TaskHandle_t handle);
void vTaskResume(TaskHandle_t handle);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
TaskHandle_t xTaskGetIdleTaskHandleForCPU(UBaseType_t cpu);
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t handle);
void vTaskPrioritySet(TaskHandle_t handle, UBaseType_t prio);
BaseType_t xPortGetCoreID(void);
size_t xPortGetFreeHeapSize(void);
size_t xPortGetMinimumEverFreeHeapSize(void);
size_t esp_get_free_heap_size(void);
size_t esp_get_minimum_free_heap_size(void);
void *pvPortMalloc(size_t size);
void vPortFree(void *p);

/* ---------------------------------------------------------------- system */
typedef enum {
    ESP_RST_UNKNOWN, ESP_RST_POWERON, ESP_RST_EXT, ESP_RST_SW, ESP_RST_PANIC,
    ESP_RST_INT_WDT, ESP_RST_TASK_WDT, ESP_RST_WDT, ESP_RST_DEEPSLEEP, ESP_RST_BROWNOUT, ESP_RST_SDIO
} esp_reset_reason_t;
void esp_restart(void);
esp_reset_reason_t esp_reset_reason(void);
uint32_t esp_random(void);
void esp_fill_random(void *buf, size_t len);
esp_err_t esp_efuse_mac_get_default(uint8_t *mac);
esp_err_t esp_read_mac(uint8_t *mac, int type);
uint32_t esp_log_timestamp(void);
int64_t esp_timer_get_time(void);
void esp_deep_sleep_start(void);
esp_err_t esp_sleep_enable_ext0_wakeup(int gpio_num, int level);
esp_err_t esp_sleep_enable_ext1_wakeup(uint64_t mask, int mode);
esp_err_t esp_sleep_enable_timer_wakeup(uint64_t us);
#define ESP_EXT1_WAKEUP_ALL_LOW 0
#define ESP_EXT1_WAKEUP_ANY_HIGH 1
void bootloader_random_enable(void);
void bootloader_random_disable(void);
uint32_t esp_cpu_get_ccount(void);
#define xthal_get_ccount() esp_cpu_get_ccount()

/* task watchdog */
esp_err_t esp_task_wdt_init(uint32_t timeout, bool panic);
esp_err_t esp_task_wdt_deinit(void);
esp_err_t esp_task_wdt_add(TaskHandle_t handle);
esp_err_t esp_task_wdt_delete(TaskHandle_t handle);
esp_err_t esp_task_wdt_status(TaskHandle_t handle);
esp_err_t esp_task_wdt_reset(void);

/* logging */
#define ESP_LOGE(tag, fmt, ...) printf("E %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("W %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) printf("I %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) do { } while (0)
#define ESP_LOGV(tag, fmt, ...) do { } while (0)

/* events (legacy) */
typedef struct { int event_id; } system_event_t;
typedef esp_err_t (*system_event_cb_t)(void *ctx, system_event_t *event);

/* heap caps */
#define MALLOC_CAP_EXEC     (1 << 0)
#define MALLOC_CAP_32BIT    (1 << 1)
#define MALLOC_CAP_8BIT     (1 << 2)
#define MALLOC_CAP_DMA      (1 << 3)
#define MALLOC_CAP_SPIRAM   (1 << 10)
#define MALLOC_CAP_INTERNAL (1 << 11)
#define MALLOC_CAP_DEFAULT  (1 << 12)
void *heap_caps_malloc(size_t size, uint32_t caps);
void *heap_caps_calloc(size_t n, size_t size, uint32_t caps);
void *heap_caps_realloc(void *p, size_t size, uint32_t caps);
void heap_caps_free(void *p);
size_t heap_caps_get_free_size(uint32_t caps);
size_t heap_caps_get_largest_free_block(uint32_t caps);
size_t heap_caps_get_minimum_free_size(uint32_t caps);
bool heap_caps_check_integrity_all(bool print_errors);
void heap_caps_print_heap_info(uint32_t caps);

/* ---------------------------------------------------------------- registers / IO mux */
#define REG_WRITE(reg, val) ((void)(reg), (void)(val))
#define REG_READ(reg) ((void)(reg), 0u)
#define REG_SET_BIT(reg, bit) ((void)(reg), (void)(bit))
#define REG_CLR_BIT(reg, bit) ((void)(reg), (void)(bit))
#define REG_SET_FIELD(reg, f, v) ((void)(reg), (void)(v))
#define WRITE_PERI_REG(reg, val) ((void)(reg), (void)(val))
#define READ_PERI_REG(reg) ((void)(reg), 0u)
#define SET_PERI_REG_MASK(reg, mask) ((void)(reg), (void)(mask))
#define CLEAR_PERI_REG_MASK(reg, mask) ((void)(reg), (void)(mask))
#define SET_PERI_REG_BITS(reg, bit_map, value, shift) ((void)(reg), (void)(value))
#define PIN_FUNC_SELECT(reg, func) ((void)(reg), (void)(func))
#define PIN_INPUT_ENABLE(reg) ((void)(reg))
#define PIN_PULLUP_EN(reg) ((void)(reg))
#define PIN_PULLUP_DIS(reg) ((void)(reg))
#define PIN_PULLDWN_EN(reg) ((void)(reg))
#define PIN_PULLDWN_DIS(reg) ((void)(reg))
extern const uint32_t GPIO_PIN_MUX_REG[40];
#define PIN_FUNC_GPIO 2
#define PIN_CTRL 0
#define PERIPHS_IO_MUX_U0TXD_U 0
#define PERIPHS_IO_MUX_U0RXD_U 0
#define PERIPHS_IO_MUX_GPIO0_U 0
#define PERIPHS_IO_MUX_GPIO2_U 0
#define PERIPHS_IO_MUX_MTDI_U 0
#define PERIPHS_IO_MUX_MTCK_U 0
#define PERIPHS_IO_MUX_MTMS_U 0
#define PERIPHS_IO_MUX_MTDO_U 0
#define FUNC_U0TXD_CLK_OUT3 1
#define FUNC_U0RXD_CLK_OUT2 1
#define FUNC_GPIO0_CLK_OUT1 1
#define FUNC_U0TXD_GPIO1 2
#define FUNC_U0RXD_GPIO3 2

typedef enum {
    PERIPH_LEDC_MODULE = 0, PERIPH_UART0_MODULE, PERIPH_UART1_MODULE, PERIPH_UART2_MODULE,
    PERIPH_I2C0_MODULE, PERIPH_I2C1_MODULE, PERIPH_I2S0_MODULE, PERIPH_I2S1_MODULE,
    PERIPH_PWM0_MODULE, PERIPH_PWM1_MODULE
} periph_module_t;
void periph_module_enable(periph_module_t m);
void periph_module_disable(periph_module_t m);

/* rtc */
typedef enum {
    NO_MEAN = 0, POWERON_RESET = 1, SW_RESET = 3, OWDT_RESET = 4, DEEPSLEEP_RESET = 5,
    SDIO_RESET = 6, TG0WDT_SYS_RESET = 7, TG1WDT_SYS_RESET = 8, RTCWDT_SYS_RESET = 9,
    INTRUSION_RESET = 10, TGWDT_CPU_RESET = 11, SW_CPU_RESET = 12, RTCWDT_CPU_RESET = 13,
    EXT_CPU_RESET = 14, RTCWDT_BROWN_OUT_RESET = 15, RTCWDT_RTC_RESET = 16
} RESET_REASON;
RESET_REASON rtc_get_reset_reason(int cpu_no);

/* ---------------------------------------------------------------- GPIO */
typedef enum {
    GPIO_NUM_NC = -1,
    GPIO_NUM_0 = 0, GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7,
    GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15,
    GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_20, GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23,
    GPIO_NUM_25 = 25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_28, GPIO_NUM_29, GPIO_NUM_30, GPIO_NUM_31,
    GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_36, GPIO_NUM_37, GPIO_NUM_38, GPIO_NUM_39,
    GPIO_NUM_MAX
} gpio_num_t;
typedef enum {
    GPIO_MODE_DISABLE = 0, GPIO_MODE_INPUT = 1, GPIO_MODE_OUTPUT = 2, GPIO_MODE_OUTPUT_OD = 6,
    GPIO_MODE_INPUT_OUTPUT_OD = 7, GPIO_MODE_INPUT_OUTPUT = 3
} gpio_mode_t;
typedef enum { GPIO_PULLUP_ONLY, GPIO_PULLDOWN_ONLY, GPIO_PULLUP_PULLDOWN, GPIO_FLOATING } gpio_pull_mode_t;
typedef enum { GPIO_PULLUP_DISABLE = 0, GPIO_PULLUP_ENABLE = 1 } gpio_pullup_t;
typedef enum { GPIO_PULLDOWN_DISABLE = 0, GPIO_PULLDOWN_ENABLE = 1 } gpio_pulldown_t;
typedef enum {
    GPIO_INTR_DISABLE = 0, GPIO_INTR_POSEDGE, GPIO_INTR_NEGEDGE, GPIO_INTR_ANYEDGE,
    GPIO_INTR_LOW_LEVEL, GPIO_INTR_HIGH_LEVEL
} gpio_int_type_t;
typedef struct {
    uint64_t pin_bit_mask;
    gpio_mode_t mode;
    gpio_pullup_t pull_up_en;
    gpio_pulldown_t pull_down_en;
    gpio_int_type_t intr_type;
} gpio_config_t;
typedef void (*gpio_isr_t)(void *);
#define GPIO_SEL_0 (1ULL << 0)
esp_err_t gpio_config(const gpio_config_t *cfg);
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);
int gpio_get_level(gpio_num_t gpio_num);
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);
esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull);
esp_err_t gpio_pullup_en(gpio_num_t gpio_num);
esp_err_t gpio_pullup_dis(gpio_num_t gpio_num);
esp_err_t gpio_pulldown_en(gpio_num_t gpio_num);
esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num);
esp_err_t gpio_reset_pin(gpio_num_t gpio_num);
esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t t);
esp_err_t gpio_intr_enable(gpio_num_t gpio_num);
esp_err_t gpio_intr_disable(gpio_num_t gpio_num);
esp_err_t gpio_install_isr_service(int flags);
void gpio_uninstall_isr_service(void);
esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr, void *args);
esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num);
esp_err_t gpio_set_drive_capability(gpio_num_t gpio_num, int strength);

#define ESP_INTR_FLAG_LEVEL1 (1 << 1)
#define ESP_INTR_FLAG_IRAM   (1 << 10)

/* ---------------------------------------------------------------- I2S */
typedef enum { I2S_NUM_0 = 0, I2S_NUM_1 = 1, I2S_NUM_MAX } i2s_port_t;
typedef enum {
    I2S_BITS_PER_SAMPLE_8BIT = 8, I2S_BITS_PER_SAMPLE_16BIT = 16,
    I2S_BITS_PER_SAMPLE_24BIT = 24, I2S_BITS_PER_SAMPLE_32BIT = 32
} i2s_bits_per_sample_t;
typedef enum { I2S_BITS_PER_CHAN_DEFAULT = 0, I2S_BITS_PER_CHAN_16BIT = 16, I2S_BITS_PER_CHAN_32BIT = 32 } i2s_bits_per_chan_t;
typedef enum { I2S_MCLK_MULTIPLE_DEFAULT = 0, I2S_MCLK_MULTIPLE_128 = 128, I2S_MCLK_MULTIPLE_256 = 256, I2S_MCLK_MULTIPLE_384 = 384 } i2s_mclk_multiple_t;
typedef enum {
    I2S_CHANNEL_FMT_RIGHT_LEFT = 0, I2S_CHANNEL_FMT_ALL_RIGHT, I2S_CHANNEL_FMT_ALL_LEFT,
    I2S_CHANNEL_FMT_ONLY_RIGHT, I2S_CHANNEL_FMT_ONLY_LEFT
} i2s_channel_fmt_t;
typedef enum {
    I2S_COMM_FORMAT_STAND_I2S = 0x01, I2S_COMM_FORMAT_STAND_MSB = 0x03,
    I2S_COMM_FORMAT_STAND_PCM_SHORT = 0x04, I2S_COMM_FORMAT_STAND_PCM_LONG = 0x0C,
    I2S_COMM_FORMAT_I2S = 0x01, I2S_COMM_FORMAT_I2S_MSB = 0x01, I2S_COMM_FORMAT_I2S_LSB = 0x02,
    I2S_COMM_FORMAT_PCM = 0x04, I2S_COMM_FORMAT_PCM_SHORT = 0x04, I2S_COMM_FORMAT_PCM_LONG = 0x08
} i2s_comm_format_t;
typedef enum {
    I2S_MODE_MASTER = 1, I2S_MODE_SLAVE = 2, I2S_MODE_TX = 4, I2S_MODE_RX = 8,
    I2S_MODE_DAC_BUILT_IN = 16, I2S_MODE_ADC_BUILT_IN = 32, I2S_MODE_PDM = 64
} i2s_mode_t;
typedef struct {
    int mode;
    int sample_rate;
    int bits_per_sample;
    int channel_format;
    int communication_format;
    int intr_alloc_flags;
    int dma_buf_count;
    int dma_buf_len;
    bool use_apll;
    bool tx_desc_auto_clear;
    int fixed_mclk;
    int mclk_multiple;
    int bits_per_chan;
} i2s_driver_config_t;
typedef i2s_driver_config_t i2s_config_t;
typedef struct {
    int mck_io_num;
    int bck_io_num;
    int ws_io_num;
    int data_out_num;
    int data_in_num;
} i2s_pin_config_t;
#define I2S_PIN_NO_CHANGE (-1)
esp_err_t i2s_driver_install(i2s_port_t port, const i2s_driver_config_t *cfg, int queue_size, void *queue);
esp_err_t i2s_driver_uninstall(i2s_port_t port);
esp_err_t i2s_set_pin(i2s_port_t port, const i2s_pin_config_t *pins);
esp_err_t i2s_set_sample_rates(i2s_port_t port, uint32_t rate);
esp_err_t i2s_set_clk(i2s_port_t port, uint32_t rate, uint32_t bits, uint32_t ch);
esp_err_t i2s_zero_dma_buffer(i2s_port_t port);
esp_err_t i2s_start(i2s_port_t port);
esp_err_t i2s_stop(i2s_port_t port);
esp_err_t i2s_write(i2s_port_t port, const void *src, size_t size, size_t *bytes_written, TickType_t ticks);
esp_err_t i2s_read(i2s_port_t port, void *dest, size_t size, size_t *bytes_read, TickType_t ticks);
int i2s_write_bytes(i2s_port_t port, const void *src, size_t size, TickType_t ticks);
int i2s_read_bytes(i2s_port_t port, void *dest, size_t size, TickType_t ticks);
int i2s_push_sample(i2s_port_t port, const void *sample, TickType_t ticks);
int i2s_pop_sample(i2s_port_t port, void *sample, TickType_t ticks);

/* ---------------------------------------------------------------- I2C */
typedef enum { I2C_NUM_0 = 0, I2C_NUM_1 = 1, I2C_NUM_MAX } i2c_port_t;
typedef enum { I2C_MODE_SLAVE = 0, I2C_MODE_MASTER = 1 } i2c_mode_t;
typedef enum { I2C_MASTER_WRITE = 0, I2C_MASTER_READ = 1 } i2c_rw_t;
typedef enum { I2C_MASTER_ACK = 0, I2C_MASTER_NACK = 1, I2C_MASTER_LAST_NACK = 2 } i2c_ack_type_t;
typedef void *i2c_cmd_handle_t;
typedef struct {
    i2c_mode_t mode;
    int sda_io_num;
    int scl_io_num;
    bool sda_pullup_en;
    bool scl_pullup_en;
    union {
        struct { uint32_t clk_speed; } master;
        struct { uint8_t addr_10bit_en; uint16_t slave_addr; } slave;
    };
    uint32_t clk_flags;
} i2c_config_t;
esp_err_t i2c_param_config(i2c_port_t port, const i2c_config_t *cfg);
esp_err_t i2c_driver_install(i2c_port_t port, i2c_mode_t mode, size_t rx, size_t tx, int flags);
esp_err_t i2c_driver_delete(i2c_port_t port);
i2c_cmd_handle_t i2c_cmd_link_create(void);
void i2c_cmd_link_delete(i2c_cmd_handle_t cmd);
esp_err_t i2c_master_start(i2c_cmd_handle_t cmd);
esp_err_t i2c_master_stop(i2c_cmd_handle_t cmd);
esp_err_t i2c_master_write_byte(i2c_cmd_handle_t cmd, uint8_t data, bool ack_en);
esp_err_t i2c_master_write(i2c_cmd_handle_t cmd, const uint8_t *data, size_t len, bool ack_en);
esp_err_t i2c_master_read_byte(i2c_cmd_handle_t cmd, uint8_t *data, int ack);
esp_err_t i2c_master_read(i2c_cmd_handle_t cmd, uint8_t *data, size_t len, int ack);
esp_err_t i2c_master_cmd_begin(i2c_port_t port, i2c_cmd_handle_t cmd, TickType_t ticks);
esp_err_t i2c_set_timeout(i2c_port_t port, int timeout);

/* ---------------------------------------------------------------- UART */
typedef enum { UART_NUM_0 = 0, UART_NUM_1 = 1, UART_NUM_2 = 2, UART_NUM_MAX } uart_port_t;
typedef enum { UART_DATA_5_BITS, UART_DATA_6_BITS, UART_DATA_7_BITS, UART_DATA_8_BITS } uart_word_length_t;
typedef enum { UART_PARITY_DISABLE = 0, UART_PARITY_EVEN = 2, UART_PARITY_ODD = 3 } uart_parity_t;
typedef enum { UART_STOP_BITS_1 = 1, UART_STOP_BITS_1_5 = 2, UART_STOP_BITS_2 = 3 } uart_stop_bits_t;
typedef enum { UART_HW_FLOWCTRL_DISABLE = 0, UART_HW_FLOWCTRL_RTS, UART_HW_FLOWCTRL_CTS, UART_HW_FLOWCTRL_CTS_RTS } uart_hw_flowcontrol_t;
typedef struct {
    int baud_rate;
    uart_word_length_t data_bits;
    uart_parity_t parity;
    uart_stop_bits_t stop_bits;
    uart_hw_flowcontrol_t flow_ctrl;
    uint8_t rx_flow_ctrl_thresh;
    int source_clk;
} uart_config_t;
#define UART_PIN_NO_CHANGE (-1)
#define UART_SIGNAL_INV_DISABLE 0
#define UART_SIGNAL_TXD_INV (1 << 10)
#define UART_SIGNAL_RXD_INV (1 << 2)
#define UART_INVERSE_DISABLE 0
#define UART_INVERSE_TXD UART_SIGNAL_TXD_INV
#define UART_INVERSE_RXD UART_SIGNAL_RXD_INV
esp_err_t uart_param_config(uart_port_t port, const uart_config_t *cfg);
esp_err_t uart_set_pin(uart_port_t port, int tx, int rx, int rts, int cts);
esp_err_t uart_driver_install(uart_port_t port, int rx_size, int tx_size, int queue_size, QueueHandle_t *queue, int flags);
esp_err_t uart_driver_delete(uart_port_t port);
int uart_write_bytes(uart_port_t port, const void *src, size_t size);
int uart_read_bytes(uart_port_t port, void *buf, uint32_t length, TickType_t ticks);
esp_err_t uart_get_buffered_data_len(uart_port_t port, size_t *size);
esp_err_t uart_flush(uart_port_t port);
esp_err_t uart_flush_input(uart_port_t port);
esp_err_t uart_set_baudrate(uart_port_t port, uint32_t baud);
esp_err_t uart_set_line_inverse(uart_port_t port, uint32_t inverse_mask);
esp_err_t uart_wait_tx_done(uart_port_t port, TickType_t ticks);

/* ---------------------------------------------------------------- ADC / DAC */
typedef enum {
    ADC1_CHANNEL_0 = 0, ADC1_CHANNEL_1, ADC1_CHANNEL_2, ADC1_CHANNEL_3, ADC1_CHANNEL_4,
    ADC1_CHANNEL_5, ADC1_CHANNEL_6, ADC1_CHANNEL_7, ADC1_CHANNEL_MAX
} adc1_channel_t;
typedef enum {
    ADC2_CHANNEL_0 = 0, ADC2_CHANNEL_1, ADC2_CHANNEL_2, ADC2_CHANNEL_3, ADC2_CHANNEL_4,
    ADC2_CHANNEL_5, ADC2_CHANNEL_6, ADC2_CHANNEL_7, ADC2_CHANNEL_8, ADC2_CHANNEL_9, ADC2_CHANNEL_MAX
} adc2_channel_t;
typedef enum { ADC_ATTEN_DB_0 = 0, ADC_ATTEN_DB_2_5, ADC_ATTEN_DB_6, ADC_ATTEN_DB_11, ADC_ATTEN_0db = 0, ADC_ATTEN_2_5db, ADC_ATTEN_6db, ADC_ATTEN_11db } adc_atten_t;
typedef enum {
    ADC_WIDTH_BIT_9 = 0, ADC_WIDTH_BIT_10, ADC_WIDTH_BIT_11, ADC_WIDTH_BIT_12,
    ADC_WIDTH_9Bit = 0, ADC_WIDTH_10Bit, ADC_WIDTH_11Bit, ADC_WIDTH_12Bit
} adc_bits_width_t;
esp_err_t adc1_config_width(adc_bits_width_t w);
esp_err_t adc1_config_channel_atten(adc1_channel_t ch, adc_atten_t a);
int adc1_get_raw(adc1_channel_t ch);
esp_err_t adc2_config_width(adc_bits_width_t w);
esp_err_t adc2_config_channel_atten(adc2_channel_t ch, adc_atten_t a);
esp_err_t adc2_get_raw(adc2_channel_t ch, adc_bits_width_t w, int *raw);
typedef enum { DAC_CHANNEL_1 = 0, DAC_CHANNEL_2, DAC_CHANNEL_MAX } dac_channel_t;
esp_err_t dac_output_enable(dac_channel_t ch);
esp_err_t dac_output_disable(dac_channel_t ch);
esp_err_t dac_output_voltage(dac_channel_t ch, uint8_t v);

/* ---------------------------------------------------------------- LEDC / MCPWM */
typedef enum { LEDC_HIGH_SPEED_MODE = 0, LEDC_LOW_SPEED_MODE, LEDC_SPEED_MODE_MAX } ledc_mode_t;
typedef enum { LEDC_TIMER_0 = 0, LEDC_TIMER_1, LEDC_TIMER_2, LEDC_TIMER_3 } ledc_timer_t;
typedef enum { LEDC_CHANNEL_0 = 0, LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3, LEDC_CHANNEL_4, LEDC_CHANNEL_5, LEDC_CHANNEL_6, LEDC_CHANNEL_7 } ledc_channel_t;
typedef enum { LEDC_INTR_DISABLE = 0, LEDC_INTR_FADE_END } ledc_intr_type_t;
typedef int ledc_timer_bit_t;
typedef struct { ledc_mode_t speed_mode; union { ledc_timer_bit_t duty_resolution; ledc_timer_bit_t bit_num; }; ledc_timer_t timer_num; uint32_t freq_hz; int clk_cfg; } ledc_timer_config_t;
typedef struct { int gpio_num; ledc_mode_t speed_mode; ledc_channel_t channel; ledc_intr_type_t intr_type; ledc_timer_t timer_sel; uint32_t duty; int hpoint; } ledc_channel_config_t;
esp_err_t ledc_timer_config(const ledc_timer_config_t *cfg);
esp_err_t ledc_channel_config(const ledc_channel_config_t *cfg);
esp_err_t ledc_set_duty(ledc_mode_t mode, ledc_channel_t ch, uint32_t duty);
esp_err_t ledc_update_duty(ledc_mode_t mode, ledc_channel_t ch);
esp_err_t ledc_stop(ledc_mode_t mode, ledc_channel_t ch, uint32_t idle_level);

typedef enum { MCPWM_UNIT_0 = 0, MCPWM_UNIT_1 } mcpwm_unit_t;
typedef enum { MCPWM_TIMER_0 = 0, MCPWM_TIMER_1, MCPWM_TIMER_2 } mcpwm_timer_t;
typedef enum { MCPWM0A = 0, MCPWM0B, MCPWM1A, MCPWM1B, MCPWM2A, MCPWM2B } mcpwm_io_signals_t;
typedef enum { MCPWM_UP_COUNTER = 1, MCPWM_DOWN_COUNTER, MCPWM_UP_DOWN_COUNTER } mcpwm_counter_type_t;
typedef enum { MCPWM_DUTY_MODE_0 = 0, MCPWM_DUTY_MODE_1 } mcpwm_duty_type_t;
typedef struct { uint32_t frequency; float cmpr_a; float cmpr_b; mcpwm_duty_type_t duty_mode; mcpwm_counter_type_t counter_mode; } mcpwm_config_t;
typedef struct { int mcpwm0a_out_num; int mcpwm0b_out_num; int mcpwm1a_out_num; int mcpwm1b_out_num; int mcpwm2a_out_num; int mcpwm2b_out_num; } mcpwm_pin_config_t;
esp_err_t mcpwm_gpio_init(mcpwm_unit_t unit, mcpwm_io_signals_t sig, int gpio);
esp_err_t mcpwm_set_pin(mcpwm_unit_t unit, const mcpwm_pin_config_t *pins);
esp_err_t mcpwm_init(mcpwm_unit_t unit, mcpwm_timer_t timer, const mcpwm_config_t *cfg);
esp_err_t mcpwm_start(mcpwm_unit_t unit, mcpwm_timer_t timer);
esp_err_t mcpwm_stop(mcpwm_unit_t unit, mcpwm_timer_t timer);

/* ---------------------------------------------------------------- SPI flash (emulated) */
typedef uint32_t spi_flash_mmap_handle_t;
typedef enum { SPI_FLASH_MMAP_DATA, SPI_FLASH_MMAP_INST } spi_flash_mmap_memory_t;
#define SPI_FLASH_SEC_SIZE 4096
#define SPI_FLASH_MMU_PAGE_SIZE 0x10000
esp_err_t spi_flash_mmap(size_t src_addr, size_t size, spi_flash_mmap_memory_t memory,
                         const void **out_ptr, spi_flash_mmap_handle_t *out_handle);
void spi_flash_munmap(spi_flash_mmap_handle_t handle);
void spi_flash_mmap_dump(void);
esp_err_t spi_flash_read(size_t src_addr, void *dest, size_t size);
esp_err_t spi_flash_write(size_t dest_addr, const void *src, size_t size);
esp_err_t spi_flash_erase_range(size_t start_addr, size_t size);
esp_err_t spi_flash_erase_sector(size_t sector);
bool spi_flash_cache_enabled(void);
size_t spi_flash_get_chip_size(void);

/* ---------------------------------------------------------------- partitions / OTA */
typedef enum { ESP_PARTITION_TYPE_APP = 0x00, ESP_PARTITION_TYPE_DATA = 0x01, ESP_PARTITION_TYPE_ANY = 0xff } esp_partition_type_t;
typedef int esp_partition_subtype_t;
#define ESP_PARTITION_SUBTYPE_ANY 0xff
typedef struct {
    void *flash_chip;
    esp_partition_type_t type;
    esp_partition_subtype_t subtype;
    uint32_t address;
    uint32_t size;
    char label[17];
    bool encrypted;
} esp_partition_t;
typedef uint32_t esp_ota_handle_t;
#define OTA_SIZE_UNKNOWN 0xffffffff
const esp_partition_t *esp_partition_find_first(esp_partition_type_t type, esp_partition_subtype_t subtype, const char *label);
esp_err_t esp_partition_read(const esp_partition_t *p, size_t off, void *dst, size_t size);
esp_err_t esp_partition_write(const esp_partition_t *p, size_t off, const void *src, size_t size);
esp_err_t esp_partition_erase_range(const esp_partition_t *p, size_t off, size_t size);
const esp_partition_t *esp_ota_get_boot_partition(void);
const esp_partition_t *esp_ota_get_running_partition(void);
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *start);
esp_err_t esp_ota_begin(const esp_partition_t *p, size_t image_size, esp_ota_handle_t *out);
esp_err_t esp_ota_write(esp_ota_handle_t h, const void *data, size_t size);
esp_err_t esp_ota_end(esp_ota_handle_t h);
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *p);
typedef struct { uint32_t magic; } esp_image_header_t;

/* ---------------------------------------------------------------- NVS (file backed) */
typedef uint32_t nvs_handle_t;
typedef nvs_handle_t nvs_handle;
typedef enum { NVS_READONLY, NVS_READWRITE } nvs_open_mode_t;
typedef nvs_open_mode_t nvs_open_mode;
esp_err_t nvs_flash_init(void);
esp_err_t nvs_flash_erase(void);
esp_err_t nvs_flash_deinit(void);
esp_err_t nvs_open(const char *name, nvs_open_mode_t mode, nvs_handle_t *out);
void nvs_close(nvs_handle_t h);
esp_err_t nvs_commit(nvs_handle_t h);
esp_err_t nvs_erase_key(nvs_handle_t h, const char *key);
esp_err_t nvs_erase_all(nvs_handle_t h);
esp_err_t nvs_erase_namespace(nvs_handle_t h, const char *ns);
esp_err_t nvs_set_i8(nvs_handle_t h, const char *key, int8_t v);
esp_err_t nvs_set_u8(nvs_handle_t h, const char *key, uint8_t v);
esp_err_t nvs_set_i16(nvs_handle_t h, const char *key, int16_t v);
esp_err_t nvs_set_u16(nvs_handle_t h, const char *key, uint16_t v);
esp_err_t nvs_set_i32(nvs_handle_t h, const char *key, int32_t v);
esp_err_t nvs_set_u32(nvs_handle_t h, const char *key, uint32_t v);
esp_err_t nvs_set_i64(nvs_handle_t h, const char *key, int64_t v);
esp_err_t nvs_set_u64(nvs_handle_t h, const char *key, uint64_t v);
esp_err_t nvs_set_str(nvs_handle_t h, const char *key, const char *v);
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *v, size_t len);
esp_err_t nvs_get_i8(nvs_handle_t h, const char *key, int8_t *v);
esp_err_t nvs_get_u8(nvs_handle_t h, const char *key, uint8_t *v);
esp_err_t nvs_get_i16(nvs_handle_t h, const char *key, int16_t *v);
esp_err_t nvs_get_u16(nvs_handle_t h, const char *key, uint16_t *v);
esp_err_t nvs_get_i32(nvs_handle_t h, const char *key, int32_t *v);
esp_err_t nvs_get_u32(nvs_handle_t h, const char *key, uint32_t *v);
esp_err_t nvs_get_i64(nvs_handle_t h, const char *key, int64_t *v);
esp_err_t nvs_get_u64(nvs_handle_t h, const char *key, uint64_t *v);
esp_err_t nvs_get_str(nvs_handle_t h, const char *key, char *out, size_t *len);
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *len);

/* ---------------------------------------------------------------- SD card (always absent) */
typedef struct { int flags; int slot; int max_freq_khz; int io_voltage; } sdmmc_host_t;
typedef struct { int gpio_cd; int gpio_wp; int width; int flags; } sdmmc_slot_config_t;
typedef struct { int gpio_miso; int gpio_mosi; int gpio_sck; int gpio_cs; int gpio_cd; int gpio_wp; int gpio_int; int dma_channel; } sdspi_slot_config_t;
typedef struct { char name[8]; uint32_t capacity; uint32_t sector_size; } sdmmc_cid_t;
typedef struct { int capacity; int sector_size; int read_block_len; int card_command_class; int tr_speed; } sdmmc_csd_t;
typedef struct { sdmmc_host_t host; sdmmc_cid_t cid; sdmmc_csd_t csd; uint32_t ocr; uint32_t rca; int max_freq_khz; int is_mem; } sdmmc_card_t;
typedef struct { bool format_if_mount_failed; int max_files; size_t allocation_unit_size; } esp_vfs_fat_sdmmc_mount_config_t;
typedef esp_vfs_fat_sdmmc_mount_config_t esp_vfs_fat_mount_config_t;
#define SDMMC_HOST_FLAG_1BIT (1 << 0)
#define SDMMC_HOST_FLAG_4BIT (1 << 1)
#define SDMMC_HOST_FLAG_8BIT (1 << 2)
#define SDMMC_HOST_FLAG_SPI  (1 << 3)
#define SDMMC_FREQ_DEFAULT 20000
#define SDMMC_FREQ_HIGHSPEED 40000
#define SDMMC_FREQ_PROBING 400
#define SDMMC_HOST_SLOT_0 0
#define SDMMC_HOST_SLOT_1 1
#define SDMMC_SLOT_NO_CD (-1)
#define SDMMC_SLOT_NO_WP (-1)
#define SDMMC_SLOT_WIDTH_DEFAULT 0
#define SDMMC_HOST_DEFAULT() { SDMMC_HOST_FLAG_4BIT, SDMMC_HOST_SLOT_1, SDMMC_FREQ_DEFAULT, 3300 }
#define SDSPI_HOST_DEFAULT() { SDMMC_HOST_FLAG_SPI, SDMMC_HOST_SLOT_1, SDMMC_FREQ_DEFAULT, 3300 }
#define SDMMC_SLOT_CONFIG_DEFAULT() { SDMMC_SLOT_NO_CD, SDMMC_SLOT_NO_WP, SDMMC_SLOT_WIDTH_DEFAULT, 0 }
#define SDSPI_SLOT_CONFIG_DEFAULT() { 2, 15, 14, 13, SDMMC_SLOT_NO_CD, SDMMC_SLOT_NO_WP, -1, 1 }
#define SDMMC_SLOT_FLAG_INTERNAL_PULLUP (1 << 0)
esp_err_t esp_vfs_fat_sdmmc_mount(const char *base_path, const sdmmc_host_t *host, const void *slot_config,
                                  const esp_vfs_fat_sdmmc_mount_config_t *mount_config, sdmmc_card_t **out_card);
esp_err_t esp_vfs_fat_sdmmc_unmount(void);
void sdmmc_card_print_info(FILE *stream, const sdmmc_card_t *card);

typedef enum { SPI_HOST = 0, HSPI_HOST = 1, VSPI_HOST = 2 } spi_host_device_t;

/* FatFs (only what sd_card_info() touches) */
typedef uint32_t DWORD;
typedef struct { DWORD csize; DWORD n_fatent; } FATFS;
int f_getfree(const char *path, DWORD *nclst, FATFS **fatfs);

/* ---------------------------------------------------------------- brownout (no-op) */
#define FUNC_U0TXD_U0TXD 0
#define FUNC_GPIO0_GPIO0_0 0
#define RTC_CNTL_BROWN_OUT_REG 0
#define RTC_CNTL_BROWN_OUT_ENA 0
#define RTC_CNTL_BROWN_OUT_PD_RF_ENA 0
#define RTC_CNTL_BROWN_OUT_RST_WAIT_S 0
#define RTC_CNTL_DBROWN_OUT_THRES_S 0
#define RTC_CNTL_BROWN_OUT_INT_ENA_M 0
#define RTC_CNTL_INT_ENA_REG 0
typedef void (*intr_handler_t)(void *arg);
esp_err_t rtc_isr_register(intr_handler_t handler, void *arg, uint32_t mask);

/* ---------------------------------------------------------------- port-internal hooks */
void glo_port_check_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* GLO_PORT_ESP_H */
