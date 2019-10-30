// duskfall.cpp -- Program entry point and stack trace macro.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "duskfall.h"
#include "guru.h"
#include "iocore.h"

// Program main entry point.
int main(int argc, char* argv[])
{
	// Check command-line parameters.
	vector<string> parameters(argv, argv + argc);

	guru = new Guru();
	iocore = new IOCore();

	// Attempt to cause a segfault.
	char *a;
	*a = 'b';

	iocore->print("Hello, world!", 1, 1, Colour::CGA_LGREEN);
	iocore->print("Alternate font test!", 1, 3, Colour::CGA_LYELLOW, PRINT_FLAG_ALT_FONT);
	iocore->flip();
	iocore->wait_for_key();

	delete iocore;
	delete guru;
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
