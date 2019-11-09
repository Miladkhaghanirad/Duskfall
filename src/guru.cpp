// guru.cpp -- Guru error-handling and reporting system.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "filex.h"
#include "guru.h"
#include "iocore.h"
#include "message.h"
#include "strx.h"

#include <chrono>
#include <ctime>
#include <fstream>

#define CASCADE_THRESHOLD		20	// The amount cascade_count can reach within CASCADE_TIMEOUT seconds before it triggers an abort screen.
#define CASCADE_TIMEOUT			30	// The number of seconds without an error to reset the cascade timer.
#define CASCADE_WEIGHT_CRITICAL	4	// The amount a critical type log entry will add to the cascade timer.
#define CASCADE_WEIGHT_ERROR	2	// The amount an error type log entry will add to the cascade timer.
#define CASCADE_WEIGHT_WARNING	1	// The amount a warning type log entry will add to the cascade timer.
#define FILENAME_LOG		"userdata/log.txt"

namespace guru

{
unsigned int	cascade_count = 0;		// Keeps track of rapidly-occurring, non-fatal error messages.
bool			cascade_failure = false;	// Is a cascade failure in progress?
std::chrono::time_point<std::chrono::system_clock> cascade_timer;	// Timer to check the speed of non-halting Guru warnings, to prevent cascade locks.
bool			dead_already = false;	// Have we already died? Is this crash within the Guru subsystem?
bool			flash_state = true;		// Is the box flashing?
bool			fully_active = false;	// Is the Guru system fully activated yet?
string			last_log_message;		// Records the last log message, to avoid spamming the log with repeats.
string			message;				// The error message.
bool			output_to_game = false;	// When this is set to true, Guru errors will output to the main game window.
int				redraw_cycle = 0;		// Used by the rendering system.
std::ofstream	syslog;					// The system log file.


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
	if (!ready) output_to_game = false;
}

// Enables or disables output of Guru logging into the in-game message window.
void game_output(bool enabled)
{
	output_to_game = enabled;
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
	output_to_game = false;

	message = error;
	if (message.size() > 39) message = message.substr(0, 38) + "^330^";
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
	if (msg == last_log_message) return;
	last_log_message = msg;
	string txt_tag = "???", tag_colour = "{5F}";
	MC message_colour = MC::NONE;
	switch(type)
	{
		case GURU_INFO: txt_tag = ""; tag_colour = ""; message_colour = MC::INFO; break;
		case GURU_WARN: txt_tag = "[WARN] "; tag_colour = "{5E}"; message_colour = MC::WARN; break;
		case GURU_ERROR: txt_tag = "[ERROR] "; tag_colour = "{5C}"; message_colour = MC::BAD; break;
		case GURU_CRITICAL: txt_tag = "[CRITICAL] "; tag_colour = "{5D}"; message_colour = MC::AWFUL; break;
		case GURU_STACK: txt_tag = ""; tag_colour = "{5F}"; break;
	}

	const time_t now = time(nullptr);
	const tm *ptm = localtime(&now);
	char buffer[32];
	strftime(&buffer[0], 32, "%H:%M:%S", ptm);
	string time_str = &buffer[0];
	msg = "[" + time_str + "] " + txt_tag + msg;
	syslog << msg << std::endl;

	if (output_to_game && type != GURU_STACK) message::msg("[*] " + msg, message_colour);
}

// Reports a non-fatal error, which will be logged and displayed in-game but will not halt execution unless it cascades.
void nonfatal(string error, int type)
{
	STACK_TRACE();
	if (cascade_failure) return;
	unsigned int cascade_weight = 0;
	switch(type)
	{
		case GURU_WARN: cascade_weight = CASCADE_WEIGHT_WARNING; break;
		case GURU_ERROR: cascade_weight = CASCADE_WEIGHT_ERROR; break;
		case GURU_CRITICAL: cascade_weight = CASCADE_WEIGHT_CRITICAL; break;
		default: nonfatal("Nonfatal error reported with incorrect severity specified.", GURU_WARN); break;
	}

	guru::log(error, type);

	if (cascade_weight)
	{
		std::chrono::duration<float> elapsed_seconds = std::chrono::system_clock::now() - cascade_timer;
		if (elapsed_seconds.count() <= CASCADE_TIMEOUT)
		{
			cascade_count += cascade_weight;
			if (cascade_count > CASCADE_THRESHOLD)
			{
				cascade_failure = true;
				guru::halt("Cascade failure detected!");
			}
		}
		else
		{
			cascade_timer = std::chrono::system_clock::now();
			cascade_count = 0;
		}
	}
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
	cascade_timer = std::chrono::system_clock::now();
}

// Redraws the error screen when needed.
void redraw()
{
	STACK_TRACE();
	iocore::cls();
	if (flash_state) iocore::box(iocore::midcol() - 20, iocore::midrow() - 3, 41, 7, Colour::TERM_LRED);
	iocore::print("Software Failure, Halting Execution", iocore::midcol() - 17, iocore::midrow() - 1, Colour::TERM_LRED);
	iocore::print(message, iocore::midcol() - (strx::ansi_strlen(message) / 2), iocore::midrow() + 1, Colour::TERM_LRED);
	iocore::flip();
}

}	// namespace guru
