// compiler-options.h -- Various compile-time options, including settings for third-party libraries.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once

#define __STDC_LIMIT_MACROS
#define _USE_MATH_DEFINES

// JsonCpp is not happy unless we define this.
#define JSONCPP_USING_SECURE_MEMORY false

// Target platform.
#if defined(_WIN32) || defined(_WIN64)
#define TARGET_WINDOWS
#undef TARGET_LINUX
#undef TARGET_MAC
#elif defined(__linux)
#define TARGET_LINUX
#undef TARGET_WINDOWS
#undef TARGET_MAC
#elif defined(TARGET_OS_MAC)
#define TARGET_MAC
#undef TARGET_LINUX
#undef TARGET_WINDOWS
#endif

// Sanity checks.
#include <climits>
#if CHAR_BIT != 8 || UCHAR_MAX != 255 || USHRT_MAX != 65535 || UINT_MAX != 4294967295U || ULLONG_MAX != 18446744073709551615ULL
#error Non-standard integer lengths detected!
#endif
