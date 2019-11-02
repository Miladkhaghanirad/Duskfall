// duskfall.cpp -- Program main entry point. Not much to see here, but this is where it starts.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "dungeon.h"
#include "duskfall.h"
#include "guru.h"
#include "iocore.h"
#include "prefs.h"
#include "static-data.h"
#include "title.h"
#include "wiki.h"

// Program main entry point.
int main(int argc, char* argv[])
{
	// Check command-line parameters.
	vector<string> parameters(argv, argv + argc);

	guru = std::make_shared<Guru>();
	prefs::init();
	iocore = std::make_shared<IOCore>();
	static_data();
	wiki = std::make_shared<Wiki>();
	guru->log("Everything looks good! Starting the game!", GURU_INFO);
	title::title_screen();
	iocore = nullptr;
	guru = nullptr;
	return 0;
}
