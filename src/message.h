// message.h -- The message log window, where the game can tell the player important things.
// Copyright (c) 2017-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

enum class MC : unsigned char { NONE, INFO, GOOD, WARN, BAD, AWFUL };


namespace message
{

void	amend(string message);		// Amends the last message, adding additional text.
void	blank_line();				// Prints a blank line, natch.
void	load();						// Loads the output buffer from disk.
void	msg(string message, MC colours = MC::NONE);	// Adds a message to the message window.
void	process_input(unsigned int key);	// Processes scroll keys.
void	purge_buffer();				// Clears the entire output buffer.
void	render();					// Renders the message window.
void	reset_count();				// The player took their turn; reset the messages_since_last_reset count.
void	process_output_buffer();	// Processes the output buffer after an update or screen resize.
void	reset_buffer_pos();			// Resets the output buffer position.
void	save();						// Saves the output buffer to disk.

}	// namespace message
