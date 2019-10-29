// mathx.h -- Extended math utility functions, pseudo-random number generator, etc.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"


namespace mathx
{

bool			check_flag(unsigned int flags, unsigned int flag_to_check);	// Checks to see if a flag is set.
void			init();					// Sets up PCG pseudorandom number generator.
unsigned int	rnd(unsigned int max);	// Returns a random number between 1 and max.

}	// namespace mathx
