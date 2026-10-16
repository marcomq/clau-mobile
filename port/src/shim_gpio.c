/* GPIO: host touch zones as Whale buttons, RGB LED and power-hold pins captured. Part of clau-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0. */
#include "shim_internal.h"

atomic_int glo_buttons[4];
static atomic_int out_level[GPIO_NUM_MAX];

const uint32_t GPIO_PIN_MUX_REG[40];

void glo_gpio_reset(void)
{
    for (int i = 0; i < GPIO_NUM_MAX; i++)
        atomic_store(&out_level[i], 0);
    atomic_store(&out_level[GLO_PIN_LED_R], 1);
    atomic_store(&out_level[GLO_PIN_LED_G], 1);
    atomic_store(&out_level[GLO_PIN_LED_B], 1);
}

int glo_gpio_level(int pin)
{
    return (pin >= 0 && pin < GPIO_NUM_MAX) ? atomic_load(&out_level[pin]) : 0;
}

int gpio_get_level(gpio_num_t pin)
{
    glo_port_check_exit();
    switch ((int)pin) {
    case GLO_PIN_U1: return atomic_load(&glo_buttons[1]) ? 1 : 0;
    case GLO_PIN_U2: return atomic_load(&glo_buttons[2]) ? 0 : 1;
    case GLO_PIN_U3: return atomic_load(&glo_buttons[3]) ? 1 : 0;
    default: return glo_gpio_level(pin);
    }
}

esp_err_t gpio_set_level(gpio_num_t pin, uint32_t level)
{
    if (pin < 0 || pin >= GPIO_NUM_MAX)
        return ESP_ERR_INVALID_ARG;
    int prev = atomic_exchange(&out_level[pin], level ? 1 : 0);
    if (pin == GLO_PIN_POWER_HOLD && prev && !level)
        glo_request_kill(1);
    glo_port_check_exit();
    return ESP_OK;
}

esp_err_t gpio_set_direction(gpio_num_t pin, gpio_mode_t mode) { (void)pin; (void)mode; return ESP_OK; }
esp_err_t gpio_set_pull_mode(gpio_num_t pin, gpio_pull_mode_t pull) { (void)pin; (void)pull; return ESP_OK; }
esp_err_t gpio_config(const gpio_config_t *cfg) { (void)cfg; return ESP_OK; }
