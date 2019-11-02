// stack-trace.cpp -- The stack-trace macro, to make logfiles a little more useful.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "duskfall.h"


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
