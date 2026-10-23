/*
 * glo_port_prelude.h - force-included (-include) into every firmware translation unit.
 * Part of flono-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0.
 */
#pragma once

/* The firmware's micros()/millis() expect gettimeofday() to count from boot (float precision). */
#define gettimeofday glo_gettimeofday

#include "glo_port_esp.h"
#include "glo_gecho_compat.h"

#include <math.h>

/* Float-to-int16 for the patched casts in build.rs: a plain cast wraps around, a hard clip crackles on loud bass,
 * so above the knee it bends softly towards full scale. */
#define GLO_SAT16_KNEE 24000.0f
static inline int16_t glo_sat16(float v)
{
    float a = fabsf(v);
    if (a <= GLO_SAT16_KNEE)
        return (int16_t)v;
    a = GLO_SAT16_KNEE + (32767.0f - GLO_SAT16_KNEE) * tanhf((a - GLO_SAT16_KNEE) / (32767.0f - GLO_SAT16_KNEE));
    return (int16_t)copysignf(a, v);
}
