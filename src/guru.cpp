// guru.cpp -- Guru error-handling and reporting system.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "filex.h"
#include "guru.h"
#include "iocore.h"
#include "strx.h"

#include <ctime>
#include <fstream>

#define FILENAME_LOG	"userdata/log.txt"

namespace guru
{

bool			dead_already;	// Have we already died? Is this crash within the Guru subsystem?
bool			flash_state;	// Is the box flashing?
bool			fully_active;	// Is the Guru system fully activated yet?
string			message;		// The error message.
int				redraw_cycle;	// Used by the rendering system.
std::ofstream	syslog;			// The system log file.


// Closes the Guru log file.
void close_syslog()
{
	STACK_TRACE();
	log("Guru system shutting down.");
	log("The rest is silence.");
	syslog.close();
}

// Tells Guru whether or not the console is initialized and can handle rendering error messages.
void console_ready(bool ready)
{
	fully_active = ready;
}

// Guru meditation error.
void halt(string error)
{
	STACK_TRACE();
	log("Software Failure, Halting Execution", GURU_CRITICAL);
	log(error, GURU_CRITICAL);

	if (StackTrace::funcs.size())
	{
		log("Stack trace follows:", GURU_STACK);
		while(true)
		{
			log(strx::itos(StackTrace::funcs.size() - 1) + ": " + StackTrace::funcs.top(), GURU_STACK);
			if (StackTrace::funcs.size() > 1) StackTrace::funcs.pop();
			else break;
		}
	}

	if (!fully_active) exit(1);

	if (dead_already)
	{
		log("Detected cleanup in process, attempting to die peacefully.", GURU_WARN);
		exit(2);
	}
	dead_already = true;	// You only die once.

	message = error;
	iocore::cls();
	iocore::flip();
	while(true)
	{
		redraw_cycle++;
		if (redraw_cycle >= 50)
		{
			redraw_cycle = 0;
			redraw();
			iocore::flip();
			flash_state = !flash_state;
		}
		iocore::wait_for_key(10);
	}
}

// Catches a segfault or other fatal signal.
void intercept_signal(int sig)
{
	STACK_TRACE();
	string sig_type;
	switch(sig)
	{
		case SIGABRT: sig_type = "Software requested abort."; break;
		case SIGFPE: sig_type = "Floating-point exception."; break;
		case SIGILL: sig_type = "Illegal instruction."; break;
		case SIGSEGV: sig_type = "Segmentation fault."; break;
		default: sig_type = "Intercepted unknown signal."; break;
	}

	// Disable the signals for now, to stop a cascade.
	signal(SIGABRT, SIG_IGN);
	signal(SIGSEGV, SIG_IGN);
	signal(SIGILL, SIG_IGN);
	signal(SIGFPE, SIG_IGN);
	halt(sig_type);
}

// Logs a message in the system log file.
void log(string msg, int type)
{
	STACK_TRACE();
	if (!syslog.is_open()) return;
	string txt_tag = "???", tag_colour = "{5F}";
	switch(type)
	{
		case GURU_INFO: txt_tag = ""; tag_colour = ""; break;
		case GURU_WARN: txt_tag = "[WARN] "; tag_colour = "{5E}"; break;
		case GURU_ERROR: txt_tag = "[ERROR] "; tag_colour = "{5C}"; break;
		case GURU_CRITICAL: txt_tag = "[CRITICAL] "; tag_colour = "{5D}"; break;
		case GURU_STACK: txt_tag = ""; tag_colour = "{5F}"; break;
	}

	const time_t now = time(nullptr);
	const tm *ptm = localtime(&now);
	char buffer[32];
	strftime(&buffer[0], 32, "%H:%M:%S", ptm);
	string time_str = &buffer[0];
	msg = "[" + time_str + "] " + txt_tag + msg;
	syslog << msg << std::endl;
}

// Opens the output log for messages.
void open_syslog()
{
	STACK_TRACE();
	filex::make_dir("userdata");
	remove(FILENAME_LOG);
	syslog.open(FILENAME_LOG);
	log("Guru error-handling system is online. Hooking signals...");
	if (signal(SIGABRT, intercept_signal) == SIG_ERR) halt("Failed to hook abort signal.");
	if (signal(SIGSEGV, intercept_signal) == SIG_ERR) halt("Failed to hook segfault signal.");
	if (signal(SIGILL, intercept_signal) == SIG_ERR) halt("Failed to hook illegal instruction signal.");
	if (signal(SIGFPE, intercept_signal) == SIG_ERR) halt("Failed to hook floating-point exception signal.");
}

// Redraws the error screen when needed.
void redraw()
{
	STACK_TRACE();
	iocore::cls();
	if (flash_state) iocore::box(iocore::midcol() - 20, iocore::midrow() - 3, 41, 7, Colour::TERM_LRED);
	iocore::print("Software Failure, Halting Execution", iocore::midcol() - 17, iocore::midrow() - 1, Colour::TERM_LRED);
	iocore::print(message, iocore::midcol() - (message.size() / 2), iocore::midrow() + 1, Colour::TERM_LRED);
	iocore::flip();
}

}	// namespace guru
