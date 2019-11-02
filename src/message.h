// message.h -- The message log window, where the game can tell the player important things.
// Copyright (c) 2017-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

#define MESSAGE_LOG_SIZE	6	// The height of the message log window.

enum class MC : unsigned char { NONE, INFO, GOOD, WARN, BAD, AWFUL };

class MessageLog
{
public:
			MessageLog();
	void	amend(string message);		// Amends the last message, adding additional text.
	void	blank_line();				// Prints a blank line, natch.
	void	load();						// Loads the output buffer from disk.
	void	msg(string message, MC colours = MC::NONE);	// Adds a message to the message window.
	void	process_input(unsigned int key);	// Processes scroll keys.
	void	purge_buffer();				// Clears the entire output buffer.
	void	render();					// Renders the message window.
	void	process_output_buffer();	// Processes the output buffer after an update or screen resize.
	void	save();						// Saves the output buffer to disk.

private:
	void	reset_buffer_pos();	// Resets the output buffer position.

	unsigned int	buffer_pos;		// The position of the output buffer.
	unsigned int	old_cols;		// Old column count, for determining auto buffer shunting.
	vector<string>	output_prc;		// The nicely processed output buffer, ready for rendering.
	vector<string>	output_raw;		// The raw, unprocessed output buffer.
};

extern shared_ptr<MessageLog>	msglog;	// The MessageLog object.
