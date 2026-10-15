/*
 * glo_port_prelude.h - force-included (-include) into every firmware translation unit.
 * Part of glo-mobile, GPL-3.0.
 */
#pragma once

/* The firmware's micros()/millis() expect gettimeofday() to count from boot (float precision). */
#define gettimeofday glo_gettimeofday

#include "glo_port_esp.h"
