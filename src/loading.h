// loading.h -- Displays a loading screen with progress bar.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"


namespace loading
{

void	loading_screen(unsigned int percentage, string str, bool floppy = true);	// Displays a loading screen with a specified percentage and message.

}	// namespace loading
