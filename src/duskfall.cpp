// duskfall.cpp -- Program entry point and stack trace macro.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "dungeon.h"
#include "duskfall.h"
#include "guru.h"
#include "iocore.h"
#include "prefs.h"
#include "static-data.h"
#include "title.h"

// Program main entry point.
int main(int argc, char* argv[])
{
	// Check command-line parameters.
	vector<string> parameters(argv, argv + argc);

	guru = std::make_shared<Guru>();
	prefs::init();
	iocore = std::make_shared<IOCore>();
	static_data();
	guru->log("Everything looks good! Starting the game!", GURU_INFO);
	title::title_screen();
	iocore = nullptr;
	guru = nullptr;
	return 0;
}

// Stack trace system.
std::stack<const char*>	StackTrace::funcs;

StackTrace::StackTrace(const char *func)
{
	funcs.push(func);
}

StackTrace::~StackTrace()
{
	if (!funcs.empty()) funcs.pop();
}
