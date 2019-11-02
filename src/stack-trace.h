// stack-trace.h -- The stack-trace macro, to make logfiles a little more useful.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "compiler-options.h"
#include <stack>

class	StackTrace
{
public:
	StackTrace(const char *func);
	~StackTrace();
	static std::stack<const char*>	funcs;
};
#define STACK_TRACE()	StackTrace local_stack(__PRETTY_FUNCTION__)
