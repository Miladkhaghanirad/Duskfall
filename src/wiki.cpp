// wiki.cpp -- In-game wiki for game documentation and in-game help.
// Copyright (c) 2017, 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "filex.h"
#include "iocore.h"
#include "prefs.h"
#include "strx.h"
#include "wiki.h"

#include "jsoncpp/json/json.h"

#include <unordered_map>


#define WIKIBUF_MAX	256	// Maximum size of the wiki buffer.


namespace wiki
{

unsigned int	buffer_pos = 0;		// The position of the console buffer.
vector<std::pair<unsigned short, unsigned short>>	link_coords;	// Coordinates of the wiki links on the page.
vector<bool>	link_good;			// Is this a valid link?
unsigned int	link_selected = 0;	// The current active wiki link.
vector<string>	link_str;			// Strings for the links.
std::unordered_map<string, string>	wiki_data;	// The actual wiki entries, read from wiki.json
vector<string>	wiki_history;		// Previous wiki pages viewed.
vector<string>	wiki_prc;			// The nicely processed wiki buffer, ready for rendering.
vector<string>	wiki_raw;			// The raw, unprocessed wiki buffer.


// Attempts to retrieve wiki data; returns a blank string if the requested page does not exist.
string get_wiki_data(string key)
{
	STACK_TRACE();
	auto found = wiki_data.find(key);
	if (found == wiki_data.end()) return "";
	else return found->second;
}

// Loads the wiki data from JSON files.
void init()
{
	STACK_TRACE();
	Json::Value json = filex::load_json("wiki");
	const Json::Value::Members jmem = json.getMemberNames();
	for (unsigned int i = 0; i < jmem.size(); i++)
	{
		const string wiki_key = jmem.at(i);
		const string wiki_entry = json[jmem.at(i)].asString();
		wiki_data.insert(std::pair<string, string>(wiki_key, wiki_entry));
	}
}

// Processes input in the wiki window.
void process_key(unsigned int key)
{
	STACK_TRACE();
	const unsigned int height = iocore::get_rows() - 1;
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
	else if (key == prefs::keybind(Keys::SCROLL_UP) || key == prefs::keybind(Keys::SCROLL_PAGEUP) || key == MOUSEWHEEL_UP_KEY)
	{
		unsigned int magnitude = 1;
		if (key == prefs::keybind(Keys::SCROLL_PAGEUP)) magnitude = 10;
		else if (key == MOUSEWHEEL_UP_KEY) magnitude = 3;
		if (buffer_pos > magnitude) buffer_pos -= magnitude; else buffer_pos = 0;
		render();
	}
	else if (key == prefs::keybind(Keys::SCROLL_DOWN) || key == prefs::keybind(Keys::SCROLL_PAGEDOWN) || key == MOUSEWHEEL_DOWN_KEY)
	{
		unsigned int magnitude = 1;
		if (key == prefs::keybind(Keys::SCROLL_PAGEDOWN)) magnitude = 10;
		else if (key == MOUSEWHEEL_DOWN_KEY) magnitude = 3;
		buffer_pos += magnitude;
		if (buffer_pos >= wiki_prc.size()) buffer_pos = wiki_prc.size();
		if (wiki_prc.size() - buffer_pos < height || wiki_prc.size() < height) reset_buffer_pos();
		render();
	}
	else if ((key == prefs::keybind(Keys::SCROLL_LEFT) && link_selected > 0)
			|| (key == prefs::keybind(Keys::SCROLL_RIGHT) && link_selected < link_coords.size() - 1))
	{
		if (key == prefs::keybind(Keys::SCROLL_RIGHT)) link_selected++; else link_selected--;
		if (link_coords.size())
		{
			while (link_coords.at(link_selected).second < buffer_pos) buffer_pos--;
			while (link_coords.at(link_selected).second > buffer_pos + height - 1) buffer_pos++;
		}
		render();
	}
	else if (key == RESIZE_KEY)
	{
		process_wiki_buffer();
		buffer_pos = 0;
		render();
	}
	else if ((key == prefs::keybind(Keys::MENU_OK) || key == prefs::keybind(Keys::MENU_OK_2)) && link_str.size())
	{
		if (link_good.at(link_selected))
		{
			wiki(strx::str_toupper(link_str.at(link_selected)));
			render();
		}
	}
	else if (key == prefs::keybind(Keys::MENU_CANCEL) || key == RMB_KEY)
	{
		wiki_history.erase(wiki_history.end() - 1);
		if (wiki_history.size())
		{
			string page = wiki_history.at(wiki_history.size() - 1);
			wiki_history.erase(wiki_history.end() - 1);
			wiki(page);
		}
		else iocore::cls();
	}
	else if (key == LMB_KEY)
	{
		for (unsigned int i = 0; i < link_coords.size(); i++)
		{
			const unsigned short link_x = link_coords.at(i).first + 2;
			const unsigned short link_y = link_coords.at(i).second - buffer_pos + 1;
			if (iocore::did_mouse_click(link_x, link_y, link_str.at(i).size()))
			{
				link_selected = i;
				wiki(strx::str_toupper(link_str.at(link_selected)));
				render();
				break;
			}
		}
	}
}

// Processes the wiki buffer after an update or screen resize.
void process_wiki_buffer()
{
	STACK_TRACE();

	// Trim the wiki buffer if necessary.
	while (wiki_raw.size() > WIKIBUF_MAX)
		wiki_raw.erase(wiki_raw.begin());

	// Clear the processed buffer.
	wiki_prc.clear();
	if (!wiki_raw.size()) return;

	// Process the console buffer.
	for (auto raw : wiki_raw)
	{
		vector<string> line_vec = strx::ansi_vector_split(raw, iocore::get_cols() - 2);
		for (auto prc : line_vec)
			wiki_prc.push_back(prc);
	}

	// Process the wiki links.
	link_coords.clear();
	link_str.clear();
	link_good.clear();
	link_selected = 0;
	for (unsigned short i = 0; i < wiki_prc.size(); i++)
	{
		size_t start_pos = 0;
		while(true)
		{
			// Locate opening wiki tags.
			const size_t pos = wiki_prc.at(i).find("[", start_pos++);
			if (pos == string::npos) break;

			// See if there's any ANSI or glyphs affecting the line length.
			const unsigned int ansi_len = strx::ansi_strlen(wiki_prc.at(i).substr(0, pos));
			const unsigned int len = wiki_prc.at(i).substr(0, pos).size();
			const int diff = len - ansi_len;

			// Ensure the link is good, record it in the vectors if so.
			const std::pair<unsigned short, unsigned short> coord = { static_cast<uint16_t>(pos - diff), i };
			link_coords.push_back(coord);
			const size_t pos2 = wiki_prc.at(i).find("]", pos);
			if (pos2 == string::npos) { link_coords.erase(link_coords.end() - 1); break; }
			const string found = wiki_prc.at(i).substr(pos + 1, pos2 - pos - 1);
			link_str.push_back(found);
			start_pos = pos2;
			const string page_check = get_wiki_data("WIKI_" + strx::str_toupper(found) + "1");
			if (page_check.size())
			{
				if (page_check[0] == '#')
				{
					const string page_check_b = get_wiki_data("WIKI_" + page_check.substr(1) + "1");
					if (page_check_b.size()) link_good.push_back(true);
					else link_good.push_back(false);
				}
				else link_good.push_back(true);
			}
			else link_good.push_back(false);
		}
	}
}

// Redraws in the in-game wiki.
void render()
{
	STACK_TRACE();
	iocore::cls();
	const unsigned int height = iocore::get_rows() - 1;
	iocore::box(0, 0, iocore::get_cols(), height + 2, UI_COLOUR_BOX, BOX_FLAG_DOUBLE);
	if (wiki_prc.size())
	{
		unsigned int end = wiki_prc.size();
		if (end - buffer_pos > height) end = buffer_pos + height;
		for (unsigned int i = buffer_pos; i < end; i++)
		{
			iocore::ansi_print(wiki_prc.at(i), 1, i - buffer_pos + 1);
			for (unsigned int j = 0; j < link_coords.size(); j++)
			{
				if (link_coords.at(j).second == i)
				{
					Colour link_col = Colour::CGA_LCYAN;
					if (!link_good.at(j)) link_col = Colour::CGA_LRED;
					iocore::print_at(static_cast<Glyph>('['), link_coords.at(j).first + 1, link_coords.at(j).second - buffer_pos + 1, Colour::CGA_WHITE);
					iocore::print_at(static_cast<Glyph>(']'), link_coords.at(j).first + 2 + link_str.at(j).size(), link_coords.at(j).second - buffer_pos + 1, Colour::CGA_WHITE);
					if (link_selected == j) iocore::rect(link_coords.at(j).first + 2, link_coords.at(j).second - buffer_pos + 1, link_str.at(j).size(), 1, link_col);
					string link_string_processed = link_str.at(j);
					strx::find_and_replace(link_string_processed, "_", " ");
					iocore::print(link_string_processed, link_coords.at(j).first + 2, link_coords.at(j).second - buffer_pos + 1, link_selected == j ? Colour::BLACK_PALE : link_col, link_selected == j ? PRINT_FLAG_ALPHA : 0);
				}
			}
		}
	}
	iocore::flip();
}

// Resets the wiki buffer position.
void reset_buffer_pos()
{
	STACK_TRACE();
	buffer_pos = 0;
	const unsigned int height = iocore::get_rows() - 1;
	if (wiki_prc.size() > height) buffer_pos = wiki_prc.size() - height;
}

// Displays a specific wiki window.
void wiki(string page)
{
	STACK_TRACE();
	wiki_raw.clear();

	// Process link pages.
	const string link_page_check = get_wiki_data("WIKI_" + page + "1");
	if (link_page_check[0] == '#')
	{
		wiki(link_page_check.substr(1));
		return;
	}

	wiki_history.push_back(page);

	string line;

	for (int i = 1; i <= 10; i++)
	{
		line = get_wiki_data("WIKI_HEADER" + strx::itos(i));
		wiki_raw.push_back(line);
	}

	int line_num = 0;
	do
	{
		line = get_wiki_data("WIKI_" + page + strx::itos(++line_num));
		if (line.size())
		{
			prefs::parse_string_with_key_tags(line);
			wiki_raw.push_back(line);
			wiki_raw.push_back(" ");
		}
	} while (line.size());
	wiki_raw.erase(wiki_raw.end() - 1);

	process_wiki_buffer();
	buffer_pos = 0;
	render();
	while(true)
	{
		process_key(iocore::wait_for_key());
		if (!wiki_history.size()) break;
		if (wiki_history.at(wiki_history.size() - 1) != page) break;
	}
}

}	// namespace wiki
