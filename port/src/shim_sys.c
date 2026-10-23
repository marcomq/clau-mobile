/* System, I2C (always ACKs, reads zeros), UART, emulated flash, absent SD card. Part of flono, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0. */
#include <stdlib.h>
#include "shim_internal.h"

uint8_t *glo_flash;

void esp_restart(void)
{
    glo_request_kill(0);
    glo_port_check_exit();
    for (;;)
        vTaskDelay(1000);
}

uint32_t esp_random(void) { return arc4random(); }
void bootloader_random_enable(void) {}
void bootloader_random_disable(void) {}

/* Task watchdog: nothing is ever subscribed, but init/deinit state is reported like ESP-IDF does. */
static atomic_int wdt_inited;
esp_err_t esp_task_wdt_init(uint32_t timeout, bool panic) { (void)timeout; (void)panic; atomic_store(&wdt_inited, 1); return ESP_OK; }
esp_err_t esp_task_wdt_deinit(void) { atomic_store(&wdt_inited, 0); return ESP_OK; }
esp_err_t esp_task_wdt_add(TaskHandle_t h) { (void)h; return ESP_OK; }
esp_err_t esp_task_wdt_delete(TaskHandle_t h) { (void)h; return ESP_OK; }
esp_err_t esp_task_wdt_status(TaskHandle_t h)
{
    if (!atomic_load(&wdt_inited))
        return ESP_ERR_INVALID_STATE;
    return h ? ESP_ERR_NOT_FOUND : ESP_OK;
}

const char *esp_err_to_name(esp_err_t code) { return code == ESP_OK ? "ESP_OK" : "ESP_FAIL"; }

esp_err_t rtc_isr_register(intr_handler_t handler, void *arg, uint32_t mask)
{
    (void)handler; (void)arg; (void)mask;
    return ESP_OK;
}

/* ---------------------------------------------------------------- I2C */
esp_err_t i2c_param_config(i2c_port_t port, const i2c_config_t *cfg) { (void)port; (void)cfg; return ESP_OK; }
esp_err_t i2c_driver_install(i2c_port_t port, i2c_mode_t mode, size_t rx, size_t tx, int flags)
{
    (void)port; (void)mode; (void)rx; (void)tx; (void)flags;
    return ESP_OK;
}
esp_err_t i2c_driver_delete(i2c_port_t port) { (void)port; return ESP_OK; }
/* Each command remembers its slave address (-1 until the first byte after a start). */
i2c_cmd_handle_t i2c_cmd_link_create(void)
{
    int *addr = malloc(sizeof *addr);
    if (!addr)
        abort(); /* the firmware never checks the handle */
    *addr = -1;
    return addr;
}
void i2c_cmd_link_delete(i2c_cmd_handle_t cmd) { free(cmd); }
esp_err_t i2c_master_start(i2c_cmd_handle_t cmd) { *(int *)cmd = -1; return ESP_OK; }
esp_err_t i2c_master_stop(i2c_cmd_handle_t cmd) { (void)cmd; return ESP_OK; }
esp_err_t i2c_master_write_byte(i2c_cmd_handle_t cmd, uint8_t data, bool ack_en)
{
    (void)ack_en;
    if (*(int *)cmd < 0)
        *(int *)cmd = data >> 1;
    return ESP_OK;
}
esp_err_t i2c_master_write(i2c_cmd_handle_t cmd, const uint8_t *data, size_t len, bool ack_en)
{
    (void)ack_en;
    if (len && *(int *)cmd < 0)
        *(int *)cmd = data[0] >> 1;
    return ESP_OK;
}
esp_err_t i2c_master_read_byte(i2c_cmd_handle_t cmd, uint8_t *data, int ack) { (void)cmd; (void)ack; *data = 0; return ESP_OK; }
esp_err_t i2c_master_read(i2c_cmd_handle_t cmd, uint8_t *data, size_t len, int ack)
{
    (void)cmd; (void)ack;
    memset(data, 0, len);
    return ESP_OK;
}
esp_err_t i2c_master_cmd_begin(i2c_port_t port, i2c_cmd_handle_t cmd, TickType_t ticks)
{
    (void)port; (void)ticks;
    glo_port_check_exit();
    /* Only the Whale's codec and accelerometer answer, so v1.0.118's hardware_check() bus scan stays in bounds. */
    int addr = *(int *)cmd;
    return addr < 0 || addr == 0x18 || addr == 0x1e || addr == 0x1f ? ESP_OK : ESP_FAIL;
}

/* ---------------------------------------------------------------- UART (MIDI etc.: nothing connected) */
esp_err_t uart_param_config(uart_port_t port, const uart_config_t *cfg) { (void)port; (void)cfg; return ESP_OK; }
esp_err_t uart_set_pin(uart_port_t port, int tx, int rx, int rts, int cts) { (void)port; (void)tx; (void)rx; (void)rts; (void)cts; return ESP_OK; }
esp_err_t uart_driver_install(uart_port_t port, int rx_size, int tx_size, int queue_size, QueueHandle_t *queue, int flags)
{
    (void)port; (void)rx_size; (void)tx_size; (void)queue_size; (void)queue; (void)flags;
    return ESP_OK;
}
esp_err_t uart_driver_delete(uart_port_t port) { (void)port; return ESP_OK; }
int uart_write_bytes(uart_port_t port, const void *src, size_t size) { (void)port; (void)src; return (int)size; }
int uart_read_bytes(uart_port_t port, void *buf, uint32_t length, TickType_t ticks)
{
    (void)port; (void)buf; (void)length;
    if (ticks)
        vTaskDelay(ticks > 10 ? 10 : ticks);
    return 0;
}
esp_err_t uart_flush(uart_port_t port) { (void)port; return ESP_OK; }
/* The firmware passes an int cast to size_t* (32 bits on the ESP32): write only 32 bits. */
esp_err_t uart_get_buffered_data_len(uart_port_t port, size_t *size) { (void)port; *(uint32_t *)size = 0; return ESP_OK; }

/* ---------------------------------------------------------------- SPI flash (in-memory image) */
esp_err_t spi_flash_mmap(size_t src_addr, size_t size, spi_flash_mmap_memory_t memory,
                         const void **out_ptr, spi_flash_mmap_handle_t *out_handle)
{
    (void)memory;
    if (!glo_flash || src_addr + size > GLO_FLASH_SIZE) {
        *out_ptr = NULL;
        return ESP_ERR_INVALID_ARG;
    }
    *out_ptr = glo_flash + src_addr;
    *out_handle = (spi_flash_mmap_handle_t)(src_addr >> 16) + 1;
    return ESP_OK;
}

void spi_flash_munmap(spi_flash_mmap_handle_t handle) { (void)handle; }
bool spi_flash_cache_enabled(void) { return true; }

/* ---------------------------------------------------------------- SD card: never present */
esp_err_t esp_vfs_fat_sdmmc_mount(const char *base_path, const sdmmc_host_t *host, const void *slot_config,
                                  const esp_vfs_fat_sdmmc_mount_config_t *mount_config, sdmmc_card_t **out_card)
{
    (void)base_path; (void)host; (void)slot_config; (void)mount_config;
    if (out_card)
        *out_card = NULL;
    return ESP_FAIL;
}
esp_err_t esp_vfs_fat_sdmmc_unmount(void) { return ESP_OK; }
void sdmmc_card_print_info(FILE *stream, const sdmmc_card_t *card) { (void)stream; (void)card; }
int f_getfree(const char *path, DWORD *nclst, FATFS **fatfs) { (void)path; (void)nclst; (void)fatfs; return -1; }

esp_err_t adc2_config_channel_atten(adc2_channel_t ch, adc_atten_t a) { (void)ch; (void)a; return ESP_OK; }
esp_err_t adc2_get_raw(adc2_channel_t ch, adc_bits_width_t w, int *raw) { (void)ch; (void)w; *raw = 0; return ESP_OK; }
