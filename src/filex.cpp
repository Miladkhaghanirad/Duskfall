// filex.cpp -- Extended file utility functions.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "filex.h"
#include "mathx.h"

#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>


namespace filex
{

// Checks if a directory exists.
bool directory_exists(string dir)
{
	STACK_TRACE();
	struct stat info;
	if (stat(dir.c_str(), &info) != 0) return false;
	if (info.st_mode & S_IFDIR) return true;
	else return false;
}

// Checks if a file exists.
bool file_exists(string file)
{
	STACK_TRACE();
	struct stat info;
	return (stat(file.c_str(), &info) == 0);
}

// Returns a list of files in a given directory.
vector<string> files_in_dir(string directory)
{
	STACK_TRACE();
	DIR *dir;
	struct dirent *ent;
	vector<string> files;
	if (!(dir = opendir(directory.c_str()))) return files;
	while ((ent = readdir(dir)))
	{
		string filename = string(ent->d_name);
		if (filename == "." || filename == "..") continue;
		files.push_back(filename);
	}
	closedir(dir);
	return files;
}

// Makes a new directory, if it doesn't already exist.
void make_dir(string dir)
{
	STACK_TRACE();
	if (directory_exists(dir)) return;

#ifdef TARGET_WINDOWS
	mkdir(dir.c_str());
#elif defined(TARGET_LINUX) || defined(TARGET_MAC)
	mkdir(dir.c_str(), 0777);
#endif
}

// Removes a given directory and anything within.
bool remove_directory(string path)
{
	STACK_TRACE();

	auto is_directory = [](char path[]) -> bool
	{
		if (path[strlen(path)] == '.') return true;
		for (int i = strlen(path) - 1; i >= 0; i--)
		{
			if (path[i] == '.') return false;
			else if (path[i] == '\\' || path[i] == '/') return true;
		}
		return false;
	};

	if (path[path.length()-1] != '\\') path += "\\";
	DIR *pdir = opendir(path.c_str());
	struct dirent *pent = nullptr;
	if (!pdir) return false;
	char file[256];

	int counter = 1;
	while ((pent = readdir(pdir)))
	{
		if (counter > 2)
     	{
			memset(file, 0, 256);
			strcat(file, path.c_str());
			if (!pent) return false;
			strcat(file, pent->d_name);
			if (is_directory(file)) remove_directory(file);
			else remove(file);
     	}
		counter++;
	}

	closedir(pdir);
	if (!rmdir(path.c_str())) return false;
	return true;
}

// Returns a random line from a text file.
string random_line(string filename, unsigned int lines)
{
	STACK_TRACE();
	std::ifstream text_file(filename);
	const unsigned int choice = mathx::rnd(lines) - 1;
	string line;
	unsigned int count = 0;
	while(getline(text_file, line))
	{
		if (choice == count++)
		{
			text_file.close();
			return line;
		}
	}
	text_file.close();
	return "[error]";
}

}	// namespace filex
