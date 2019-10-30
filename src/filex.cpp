// filex.cpp -- Extended file utility functions.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "filex.h"

#include <dirent.h>
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

}	// namespace filex
