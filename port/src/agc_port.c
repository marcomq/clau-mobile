/* Hands the firmware's AGC settings (config_glo.txt, changed by 6x U2 / 6x U3) to the mic AGC in glo_core_api.c.
 * Built with the firmware headers, unlike the shim. Part of glo-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0. */
#include "hw/ui.h"

void glo_port_get_agc(int *enabled, int *max_gain_db, int *target_db)
{
    *enabled = global_settings.AGC_ENABLED;
    *max_gain_db = global_settings.AGC_MAX_GAIN;
    *target_db = global_settings.AGC_TARGET_LEVEL;
}
