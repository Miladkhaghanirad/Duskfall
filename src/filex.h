// filex.h -- Extended file utility functions.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"


namespace filex
{

bool	directory_exists(string dir);	// Checks if a directory exists.
bool	file_exists(string file);		// Checks if a file exists.
vector<string>	files_in_dir(string directory);	// Returns a list of files in a given directory.
void	make_dir(string dir);			// Makes a new directory, if it doesn't already exist.

}	// namespace filex
