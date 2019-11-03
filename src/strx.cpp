// strx.cpp -- Extended string utility functions.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

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

// Splits an ANSI string into a vector of strings, to a given line length.
vector<string> ansi_vector_split(string source, unsigned int line_len)
{
	STACK_TRACE();
	vector<string> output;

	// Check to see if the line of text has the no-split tag at the start.
	if (source.size() >= 5)
	{
		if (!source.substr(0, 5).compare("^000^"))
		{
			output.push_back(source);
			return output;
		}
	}

	// Check to see if the line is too short to be worth splitting.
	if (source.size() <= line_len)
	{
		output.push_back(source);
		return output;
	}

	// Split the string into individual words.
	vector<string> words = string_explode(source, " ");

	// Keep track of the current line and our position on it.
	unsigned int current_line = 0, line_pos = 0;
	string last_ansi = "{57}";	// The last ANSI tag we encountered; white by default.

	// Start with an empty string.
	output.push_back("");

	for (auto word : words)
	{
		if (word == "{nl}")	// Check for new-line marker.
		{
			if (line_pos > 0)
			{
				line_pos = 0; current_line += 2; output.push_back(" "); output.push_back(last_ansi);
			}
		}
		else if (word == "{lb}")
		{
			if (line_pos > 0)
			{
				line_pos = 0; current_line += 1; output.push_back(last_ansi);
			}
		}
		else
		{
			unsigned int length = word.length();		// Find the length of the word.

			// If the word includes high-ASCII tags, adjust the length.
			size_t htag_pos = word.find("^");
			bool high_ascii = false;
			if (htag_pos != string::npos)
			{
				if (word.size() > htag_pos + 4)
				{
					if (word.at(htag_pos + 4) == '^')
					{
						length -= word_count(word, "^") * 2;
						high_ascii = true;
					}
				}
			}

			const int ansi_count = word_count(word, "{");	// Count the ANSI tags.
			if (ansi_count) length -= (ansi_count * 4);	// Reduce the length if one or more ANSI tags are found.
			if (length + line_pos >= line_len)	// Is the word too long for the current line?
			{
				line_pos = 0; current_line++;	// CR;LF
				output.push_back(last_ansi);	// Start the line with the last ANSI tag we saw.
			}
			if (ansi_count)
			{
				// Duplicate the last-used ANSI tag.
				const string::size_type flo = word.find_last_of("{");
				if (flo != string::npos)
				{
					if (word.size() >= flo + 4)
					{
						const string last_test = word.substr(flo, 4);
						if (last_test.compare("{nl}") || last_test.compare("{lb}")) last_ansi = last_test;
					}
				}
			}
			if (line_pos != 0)	// NOT the start of a new line?
			{
				length++; output.at(current_line) += " ";
			}
			// Is the word STILL too long to fit over a single line?
			// Don't attempt this on high-ASCII words.
			while (length > line_len && !high_ascii)
			{
				const string trunc = word.substr(0, line_len);
				word = word.substr(line_len);
				output.at(current_line) += trunc;
				line_pos = 0; current_line++;
				output.push_back(last_ansi);	// Start the line with the last ANSI tag we saw.
				length = word.size();	// Adjusts the length for what we have left over.
			}
			output.at(current_line) += word; line_pos += length;
		}
	}

	return output;
}

// Converts a vector to a comma-separated list.
string comma_list(vector<string> vec, bool use_and)
{
	STACK_TRACE();
	if (!vec.size()) return "";
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

// Converts a float or double to a string.
string ftos(double num)
{
	STACK_TRACE();
	std::stringstream ss;
	ss << num;
	return ss.str();
}

// FNV string hash.
unsigned int hash(string s)
{
	STACK_TRACE();
	size_t result = 2166136261U;
	std::string::const_iterator end = s.end();
	for (std::string::const_iterator iter = s.begin(); iter != end; ++iter)
		result = 127 * result + static_cast<unsigned char>(*iter);
	return result;
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

// Converts a string to lower-case.
string str_tolower(string str)
{
	STACK_TRACE();
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

// Converts a string to upper-case.
string str_toupper(string str)
{
	STACK_TRACE();
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

// String split/explode function.
vector<string> string_explode(string str, string separator)
{
	STACK_TRACE();
	vector<string> results;

	string::size_type pos = str.find(separator, 0);
	const int pit = separator.length();

	while(pos != string::npos)
	{
		if (pos == 0) results.push_back("");
		else results.push_back(str.substr(0, pos));
        str.erase(0, pos + pit);
        pos = str.find(separator, 0);
    }
    results.push_back(str);

    return results;
}

// Trims out leading, trailing, and excess (more than one at a time) spaces from a string.
string trim_excess_spaces(string source)
{
	STACK_TRACE();
	return std::regex_replace(source, std::regex("^ +| +$|( ) +"), "$1");
}

int word_count(string str, string word)
{
	STACK_TRACE();
	int count = 0;
	string::size_type word_pos = 0;
	while(word_pos != string::npos)
	{
		word_pos = str.find(word, word_pos);
		if (word_pos != string::npos)
		{
			count++;
			word_pos += word.length();
		}
	}
	return count;
}

}	// namespace strx
