// duskfall.h -- Primary header, includes commonly-used C/C++ libraries and other frequently-used code, as well as important #definitions.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

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

// Stack trace macro.
#include <stack>
class	StackTrace
{
public:
	StackTrace(const char *func);
	~StackTrace();
	static std::stack<const char*>	funcs;
};
#define STACK_TRACE()	StackTrace local_stack(__PRETTY_FUNCTION__)

// This stuff is used almost everywhere.
#include <memory>
#include <string>
#include <vector>
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;

struct s_rgb { unsigned char r, g, b; };	// RGB colour values.
