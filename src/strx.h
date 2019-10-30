// strx.h -- Extended string utility functions.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

namespace strx
{

unsigned int	ansi_strlen(string input);	// Gives the length of a string, adjusted by ANSI tags.
vector<string>	ansi_vector_split(string source, unsigned int line_len);	// Splits an ANSI string into a vector of strings, to a given line length.
string			comma_list(vector<string> vec, bool use_and = true);	// Converts a vector to a comma-separated list.
bool			find_and_replace(string &input, string to_find, string to_replace);	// Find and replace one string with another.
unsigned int	htoi(string hex_str);	// Converts a hex string back to an integer.
string			itos(long long num);	// Converts an integer to a string.
vector<string>	string_explode(string str, string separator);	// String split/explode function.
int				word_count(string str, string word);	// Returns a count of the amount of times a string is found in a parent string.

}	// namespace strx
