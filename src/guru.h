// guru.h -- Guru error-handling and reporting system.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"
#include <fstream>

#define GURU_INFO		0	// General logging information.
#define GURU_WARN		1	// Warnings, non-fatal stuff.
#define GURU_ERROR		2	// Serious errors. Shit is going down.
#define GURU_CRITICAL	3	// Critical system failure.
#define GURU_STACK		4	// Stack traces.

class Guru
{
public:
			Guru();
			~Guru();
	void	activate() { fully_active = true; }	// Allows the Guru system to display error message screens.
	void	deactivate() { fully_active = false; }	// Deactivates the display system; for if we get some horrid bug in the rendering code.
	void	log(std::string msg, int type = GURU_INFO);	// Logs a message in the system log file.
	void	halt(string error);	// Stops the game and displays an error messge.
	void	redraw();			// Redraws the error screen when needed.

private:
	std::ofstream	syslog;	// The system log file.
	bool	dead_already;	// Have we already died? Is this crash within the Guru subsystem?
	bool	fully_active;	// Is the Guru system fully activated yet?
	int		redraw_cycle;	// Used by the rendering system.
	bool	flash_state;	// Is the box flashing?
	string	message;		// The error message.
};

extern Guru*	guru;	// External access to the Guru object.
