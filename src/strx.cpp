// strx.cpp -- Extended string utility functions.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "guru.h"
#include "strx.h"
#include <bits/stdc++.h>
#include <sstream>

namespace strx
{

// Gives the length of a string, adjusted by ANSI tags.
unsigned int ansi_strlen(string input)
{
	STACK_TRACE();
	unsigned int len = input.size();

	// Count any ANSI tags.
	const int openers = std::count(input.begin(), input.end(), '{');
	const int symbols = std::count(input.begin(), input.end(), '^');
	if (openers) len -= openers * 4;
	if (symbols) len -= (symbols / 2) * 4;

	// Return the result.
	return len;
}

// Converts a vector to a comma-separated list.
string comma_list(vector<string> vec, bool use_and)
{
	STACK_TRACE();
	if (!vec.size()) guru->halt("comma_list: empty string vector");
	if (vec.size() == 1) return vec.at(0);
	string plus = " and ";
	if (!use_and) plus = ", ";
	else if (vec.size() == 2) return vec.at(0) + plus + vec.at(1);

	string str;
	for (unsigned int i = 0; i < vec.size(); i++)
	{
		str += vec.at(i);
		if (i < vec.size() - 1)
		{
			if (i == vec.size() - 2) str += plus;
			else str += ", ";
		}
	}

	return str;
}

// Find and replace one string with another.
bool find_and_replace(string &input, string to_find, string to_replace)
{
	STACK_TRACE();
	string::size_type pos = 0;
	const string::size_type find_len = to_find.length(), replace_len = to_replace.length();
	if (find_len == 0) return false;
	bool found = false;
	while ((pos = input.find(to_find, pos)) != string::npos)
	{
		found = true;
		input.replace(pos, find_len, to_replace);
		pos += replace_len;
	}
	return found;
}

// Converts a hex string back to an integer.
unsigned int htoi(string hex_str)
{
	STACK_TRACE();
	std::stringstream ss;
	ss << std::hex << hex_str;
	unsigned int result;
	ss >> result;
	return result;
}

// Converts an integer to a string.
string itos(long long num)
{
	STACK_TRACE();
	std::stringstream ss;
	ss << num;
	return ss.str();
}

}	// namespace strx
