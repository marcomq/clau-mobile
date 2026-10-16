/*
 * sys/dirent.h - the firmware's sdcard.h includes it; bionic (Android) only has <dirent.h>.
 * Part of clau-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0.
 */
#pragma once

#ifdef __ANDROID__
#include <dirent.h>
#else
#include_next <sys/dirent.h>
#endif
