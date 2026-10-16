/*
 * glo_port_prelude.h - force-included (-include) into every firmware translation unit.
 * Part of clau-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0.
 */
#pragma once

/* The firmware's micros()/millis() expect gettimeofday() to count from boot (float precision). */
#define gettimeofday glo_gettimeofday

#include "glo_port_esp.h"
#include "glo_gecho_compat.h"

/* Saturating float-to-int16 for the patched casts in build.rs (a plain cast wraps around). */
static inline int16_t glo_sat16(float v)
{
    return v >= 32767.0f ? 32767 : v <= -32768.0f ? -32768 : (int16_t)v;
}
