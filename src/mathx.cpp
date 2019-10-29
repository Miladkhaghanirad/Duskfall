// mathx.cpp -- Extended math utility functions, pseudo-random number generator, etc.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "mathx.h"
#include "pcg/pcg_basic.h"
#include <chrono>


namespace mathx
{

// Checks to see if a flag is set.
bool check_flag(unsigned int flags, unsigned int flag_to_check)
{
	return ((flags & flag_to_check) == flag_to_check);
}

// Sets up PCG pseudorandom number generator.
void init()
{
	STACK_TRACE();

	// Get a much more accurate count, milliseconds since epoch.
	const std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());

	// Feed this shit into xorshift64*
	unsigned long long xorshift = ms.count();
	xorshift ^= xorshift >> 12; // a
	xorshift ^= xorshift << 25; // b
	xorshift ^= xorshift >> 27; // c
	xorshift *= UINT64_C(2685821657736338717);

	// Finally, feed the shifted seed into the RNG.
	pcg32_srandom(time(NULL) ^ reinterpret_cast<intptr_t>(&printf), xorshift);
}

// Returns a random number between 1 and max.
unsigned int rnd(unsigned int max)
{
	STACK_TRACE();
	if (max <= 1) return max;
	return pcg32_boundedrand(max) + 1;
}

}	// namespace mathx
