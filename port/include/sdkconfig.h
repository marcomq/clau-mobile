/* fake sdkconfig.h for clau-mobile: only what the firmware tests for */
#pragma once
#define CONFIG_FREERTOS_HZ 1000
#define CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ 240
#define CONFIG_TASK_WDT_CHECK_IDLE_TASK_CPU0 1
#define CONFIG_TASK_WDT_CHECK_IDLE_TASK_CPU1 1
