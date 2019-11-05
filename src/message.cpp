// message.cpp -- The message log window, where the game can tell the player important things.
// Copyright (c) 2017-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "dungeon.h"
#include "guru.h"
#include "iocore.h"
#include "message.h"
#include "prefs.h"
#include "sidebar.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"

#define OUTBUF_MAX	128	// Maximum size of the output buffer.


namespace message
{

unsigned int	buffer_pos = 0;	// The position of the output buffer.
unsigned int	messages_since_last_reset = 0;	// How many messages arrived since the player last took their turn?
unsigned int	old_cols = 0;	// Old column count, for determining auto buffer shunting.
vector<string>	output_prc;		// The nicely processed output buffer, ready for rendering.
vector<unsigned int>	output_prc_count;	// Duplicate line count for the processed buffer.
vector<string>	output_raw;		// The raw, unprocessed output buffer.

// Amends the last message, adding additional text.
void amend(string message)
{
	STACK_TRACE();
	if (!output_raw.size()) { msg(message); return; }
	output_raw.at(output_raw.size() - 1) += message;
	process_output_buffer();
	render();
}

// Loads the output buffer from disk.
void load()
{
	STACK_TRACE();
	output_raw.clear();
	output_prc.clear();
	output_prc_count.clear();
	try
	{
		SQLite::Statement query(*world::save_db(), "SELECT line FROM msgbuffer");
		while (query.executeStep())
		{
			string line = query.getColumn("line").getString();
			output_raw.push_back(line);
		}
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}

	if (output_raw.size()) process_output_buffer();
}

// Adds a message to the output buffer.
void msg(string message, MC colours)
{
	STACK_TRACE();
	bool redraw = true;
	iocore::rect(iocore::get_cols() - SIDEBAR_WIDTH_8X8 - 1, iocore::get_rows() - 1, SIDEBAR_WIDTH_8X8 + 1, 1, Colour::BLACK);
	unsigned int key = 0;
	while (++messages_since_last_reset > MESSAGE_LOG_SIZE)
	{
		if (key == RESIZE_KEY || redraw)
		{
			iocore::cls();
			world::dungeon()->render();
			message::render();
			sidebar::render();
			iocore::print("-more-", iocore::get_cols_narrow() - SIDEBAR_WIDTH_5X8, iocore::get_rows() - 1, Colour::CGA_WHITE, PRINT_FLAG_NARROW);
			iocore::flip();
			redraw = false;
		}
		key = iocore::wait_for_key();
		if (iocore::is_select(key))
		{
			messages_since_last_reset = 1;
			break;
		}
	}

	string message_colour_str;
	switch(colours)
	{
		case MC::NONE: message_colour_str = "{5F}"; break;
		case MC::INFO: message_colour_str = "{5B}"; break;
		case MC::GOOD: message_colour_str = "{5A}"; break;
		case MC::WARN: message_colour_str = "{5E}"; break;
		case MC::BAD: message_colour_str = "{5C}"; break;
		case MC::AWFUL: message_colour_str = "{5D}"; break;
	}
	output_raw.push_back(message_colour_str + message);
	process_output_buffer();	// Reprocess the text, to make sure it's all where it should be.
	render();
}

// Processes input while in message-window mode.
void process_input(unsigned int key)
{
	STACK_TRACE();

	if (key == prefs::keybind(Keys::SCROLL_TOP))
	{
		buffer_pos = 0;
		render();
	}
	else if (key == prefs::keybind(Keys::SCROLL_BOTTOM))
	{
		reset_buffer_pos();
		render();
	}
	else if (key == prefs::keybind(Keys::SCROLL_PAGEUP) || key == MOUSEWHEEL_UP_KEY || key == prefs::keybind(Keys::SCROLL_UP))
	{
		unsigned int magnitude = 1;
		if (key == prefs::keybind(Keys::SCROLL_PAGEUP)) magnitude = MESSAGE_LOG_SIZE - 1;
		if (buffer_pos > magnitude) buffer_pos -= magnitude; else buffer_pos = 0;
		render();
	}
	else if (key == prefs::keybind(Keys::SCROLL_PAGEDOWN) || key == MOUSEWHEEL_DOWN_KEY || key == prefs::keybind(Keys::SCROLL_DOWN))
	{
		unsigned int magnitude = 1;
		if (key == prefs::keybind(Keys::SCROLL_PAGEDOWN)) magnitude = MESSAGE_LOG_SIZE - 1;
		buffer_pos += magnitude;
		if (buffer_pos >= output_prc.size()) buffer_pos = output_prc.size();
		unsigned int height = MESSAGE_LOG_SIZE;
		if (output_prc.size() - buffer_pos < height || output_prc.size() < height) reset_buffer_pos();
		render();
	}
}

// Processes the output buffer after an update or screen resize.
void process_output_buffer()
{
	STACK_TRACE();

	// Trim the output buffer if necessary.
	while (output_raw.size() > OUTBUF_MAX)
		output_raw.erase(output_raw.begin());

	// Clear the processed buffer.
	output_prc.clear();
	output_prc_count.clear();
	if (!output_raw.size()) return;

	// Process the output buffer.
	for (unsigned int i = 0; i < output_raw.size(); i++)
	{
		vector<string> line_vec = strx::ansi_vector_split(output_raw.at(i), iocore::get_cols_narrow() - 2);
		for (auto line : line_vec)
		{
			if (output_prc.size() && output_prc.at(output_prc.size() - 1) == line) output_prc_count.at(output_prc.size() - 1)++;
			else
			{
				output_prc.push_back(line);
				output_prc_count.push_back(1);
			}
		}
	}

	// Reset the buffer position if needed.
	reset_buffer_pos();
}

// Clears the entire output buffer.
void purge_buffer()
{
	STACK_TRACE();
	output_raw.clear();
	output_prc.clear();
	buffer_pos = 0;
	old_cols = 0;
}

// Renders the message window.
void render()
{
	STACK_TRACE();
	iocore::rect(0, iocore::get_rows() - MESSAGE_LOG_SIZE, iocore::get_cols() - SIDEBAR_WIDTH_8X8 - 1, MESSAGE_LOG_SIZE, Colour::BLACK);
	if (output_prc.size())
	{
		unsigned int end = output_prc.size();
		if (end - buffer_pos > MESSAGE_LOG_SIZE) end = buffer_pos + MESSAGE_LOG_SIZE;
		for (unsigned int i = buffer_pos; i < end; i++)
		{
			int dim_amount = 0;
			if (prefs::message_log_dim)
			{
				dim_amount = MESSAGE_LOG_SIZE - (i - buffer_pos) - 1;
				if (output_prc.size() < MESSAGE_LOG_SIZE) dim_amount -= MESSAGE_LOG_SIZE - output_prc.size();
			}
			iocore::ansi_print(output_prc.at(i) + (output_prc_count.at(i) > 1 ? " (" + strx::itos(output_prc_count.at(i)) + ")" : ""), 0, i - buffer_pos + iocore::get_rows() - MESSAGE_LOG_SIZE, PRINT_FLAG_NARROW, dim_amount);
		}
	}
}

// Resets the output buffer position.
void reset_buffer_pos()
{
	STACK_TRACE();
	buffer_pos = 0;
	const unsigned int height = MESSAGE_LOG_SIZE;
	if (output_prc.size() > height) buffer_pos = output_prc.size() - height;
}

// The player took their turn; reset the messages_since_last_reset count.
void reset_count()
{
	messages_since_last_reset = 0;
}

// Saves the output buffer to disk.
void save()
{
	STACK_TRACE();
	try
	{
		world::save_db()->exec("DROP TABLE IF EXISTS msgbuffer; CREATE TABLE msgbuffer ( key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, line TEXT )");
		for (unsigned int i = 0; i < output_raw.size(); i++)
		{
			SQLite::Statement query(*world::save_db(), "INSERT INTO msgbuffer (line) VALUES (?)");
			query.bind(1, output_raw.at(i));
			query.exec();
		}
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

}	// namespace message
