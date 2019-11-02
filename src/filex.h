// filex.h -- Extended file utility functions.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

namespace Json { class Value; }		// defined in json.cpp/json/json.h


namespace filex
{

bool			directory_exists(string dir);	// Checks if a directory exists.
bool			file_exists(string file);		// Checks if a file exists.
vector<string>	files_in_dir(string directory);	// Returns a list of files in a given directory.
Json::Value		load_json(string filename);		// Loads an individual JSON file, with error-checking.
void			make_dir(string dir);			// Makes a new directory, if it doesn't already exist.
bool			remove_directory(string path);	// Removes a given directory and anything within.
string			random_line(string filename, unsigned int lines);	// Returns a random line from a text file.

}	// namespace filex
