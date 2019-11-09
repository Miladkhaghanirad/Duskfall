// guru.h -- Guru error-handling and reporting system.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

#define GURU_INFO		0	// General logging information.
#define GURU_WARN		1	// Warnings, non-fatal stuff.
#define GURU_ERROR		2	// Serious errors. Shit is going down.
#define GURU_CRITICAL	3	// Critical system failure.
#define GURU_STACK		4	// Stack traces.

namespace guru
{

void	close_syslog();				// Closes the Guru log file.
void	console_ready(bool ready);	// Tells Guru whether or not the console is initialized and can handle rendering error messages.
void	game_output(bool enabled);	// Enables or disables output of Guru logging into the in-game message window.
void	halt(string error);			// Stops the game and displays an error messge.
void	intercept_signal(int sig);	// Catches a segfault or other fatal signal.
void	log(std::string msg, int type = GURU_INFO);	// Logs a message in the system log file.
void	nonfatal(string error, int type);	// Reports a non-fatal error, which will be logged and displayed in-game but will not halt execution unless it cascades.
void	open_syslog();				// Opens the output log for messages.
void	redraw();					// Redraws the error screen when needed.

}	// namespace guru
