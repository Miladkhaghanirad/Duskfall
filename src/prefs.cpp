// prefs.h -- Game preferences and keybind config windows, as well as easy code access to the prefs.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "filex.h"
#include "guru.h"
#include "iocore.h"
#include "prefs.h"
#include "strx.h"

#include "sdl2/SDL.h"
#include "SQLiteCpp/SQLiteCpp.h"


Prefs::Prefs() : changed(false), done(false), selected(0), has_cpu_heavy(false), has_must_restart(false), y_pos(-11) { }

PrefsEntry::PrefsEntry() : cpu_heavy(false), id(0), is_boolean(false), is_slider(false), must_restart(false), selected(0), slider_size(0), y_pos(0) { }


namespace prefs
{

#define FILENAME_PREFS	"userdata/prefs.dat"
#define PREFS_KEYBINDS	25

// Shift			letter
// Ctrl				letter - 64
// Alt				131104 + letter
// Shift-Ctrl		65504 + letter
// Shift-Alt		131072 + letter
// Ctrl-Alt			32768 + letter
// Shift-Ctrl-Alt	32768 + letter
#define KEYMOD_CTRL			-64
#define KEYMOD_ALT			131104
#define KEYMOD_SHIFT_CTRL	65536
#define KEYMOD_SHIFT_ALT	131072
#define KEYMOD_CTRL_ALT		32768

#define KEY_CLOSE_DEFAULT			'C'
#define KEY_EAST_DEFAULT			'd'
#define KEY_MENU_CANCEL_DEFAULT		SDLK_ESCAPE
#define KEY_MENU_OK_DEFAULT			SDLK_RETURN
#define KEY_MENU_OK_2_DEFAULT		' '
#define KEY_NORTH_DEFAULT			'w'
#define KEY_NORTHEAST_DEFAULT		'e'
#define KEY_NORTHWEST_DEFAULT		'q'
#define KEY_OPEN_DEFAULT			'o'
#define KEY_OPTIONS_WINDOW_DEFAULT	(KEYMOD_SHIFT_CTRL + 'O')
#define KEY_QUIT_GAME_DEFAULT		(KEYMOD_SHIFT_CTRL + 'X')
#define KEY_SAVE_DEFAULT			(KEYMOD_CTRL + 'S')
#define KEY_SCREENSHOT_DEFAULT		SDLK_PRINTSCREEN
#define KEY_SCROLL_BOTTOM_DEFAULT	SDLK_END
#define KEY_SCROLL_DOWN_DEFAULT		SDLK_DOWN
#define KEY_SCROLL_LEFT_DEFAULT		SDLK_LEFT
#define KEY_SCROLL_PAGEDOWN_DEFAULT	SDLK_PAGEDOWN
#define KEY_SCROLL_PAGEUP_DEFAULT	SDLK_PAGEUP
#define KEY_SCROLL_RIGHT_DEFAULT	SDLK_RIGHT
#define KEY_SCROLL_TOP_DEFAULT		SDLK_HOME
#define KEY_SCROLL_UP_DEFAULT		SDLK_UP
#define KEY_SOUTH_DEFAULT			's'
#define KEY_SOUTHEAST_DEFAULT		'c'
#define KEY_SOUTHWEST_DEFAULT		'z'
#define KEY_WEST_DEFAULT			'a'

#define ANIMATION_DEFAULT			true	// Two-frame animation enabled/disabled.
#define DEATH_REPORTS_DEFAULT		true	// Generate death report text files?
#define FULLSCREEN_DEFAULT			false	// Run the game in full-screen mode?
#define MESSAGE_LOG_DIM_DEFAULT		true	// Dim the colours in the message log?
#define NTSC_FILTER_DEFAULT			true	// Whether or not the NTSC filter is enabled.
#define NTSC_MODE_DEFAULT			2		// Different post-processing modes (0 is least, 2 is most)
#define PALETTE_DEFAULT				0		// Which colour palette to use?
#define SCALE_MOD_DEFAULT			0		// Experimental surface scaling.
#define SCREEN_X_DEFAULT			1024	// Horizontal screen resolution (minimum: 1024)
#define SCREEN_Y_DEFAULT			600		// Vertical screen resolution (minimum: 600)
#define SCREENSHOT_TYPE_DEFAULT		2		// The format of screenshots (0 = BMP, 1 = PNG, 2 = JPEG)
#define VISUAL_GLITCHES_DEFAULT		0		// Do we want visual glitches?

enum { ID_SCREEN_RES = 100, ID_FULL_SCREEN, ID_SHADER, ID_PALETTE, ID_GLITCHES, ID_SS_FORMAT, ID_TEX_SCALING, ID_DEATH_REPORT, ID_MESSAGE_LOG_DIM, ID_NTSC_FILTER, ID_ANIMATION };

}	// namespace prefs


// Adds a new entry to the list.
void Prefs::add_item(PrefsEntry entry)
{
	STACK_TRACE();
	entry.y_pos = y_pos;
	y_pos += 2;
	entries.push_back(entry);
	if (entry.cpu_heavy) has_cpu_heavy = true;
	if (entry.must_restart) has_must_restart = true;
}

// Adjusts an option up or down.
void Prefs::adjust_option(signed char amount)
{
	STACK_TRACE();
	PrefsEntry& entry = entries.at(selected);
	if (entry.is_boolean) entry.selected = !entry.selected;
	else if (entry.is_slider)
	{
		if (entry.selected == 0 && amount == -1) entry.selected = entry.slider_size;
		else if (entry.selected == entry.slider_size && amount == 1) entry.selected = 0;
		else entry.selected += amount;
	}
	else
	{
		if (entry.selected == 0 && amount == -1) entry.selected = entry.options_str.size() - 1;
		else if (entry.selected == entry.options_str.size() - 1 && amount == 1) entry.selected = 0;
		else entry.selected += amount;
	}
	changed = true;
}

// Renders the prefs list, processes input.
void Prefs::render()
{
	STACK_TRACE();
	const int midcol = iocore::midcol(), midrow = iocore::midrow();
	iocore::cls();
	iocore::box(midcol - 27, midrow - 18, 55, 38, UI_COLOUR_BOX);
	const int name_pos = (iocore::get_cols() * 4) - (name.size() * 12);
	iocore::alagard_print(name, name_pos, (midrow - 16) * 8, Colour::CGA_WHITE);

	for (unsigned int i = 0; i < entries.size(); i++)
	{
		PrefsEntry& entry = entries.at(i);
		string line_str = entry.name;
		if (entry.must_restart)
		{
			if (selected == i) line_str = "{5C}^015^{5F}" + line_str;
			else line_str = "{54}^015^{57}" + line_str;
		}
		if (entry.cpu_heavy)
		{
			if (selected == i) line_str = "{5D}^019^{5F}" + line_str;
			else line_str = "{55}^019^{57}" + line_str;
		}
		if (i == selected) line_str = "{5F}[" + line_str + ": {5B}"; else line_str = "{57} " + line_str + ": {53}";
		if (entry.is_boolean)
		{
			if (entry.selected) line_str += "^326^";
			else line_str += "X";
		}
		else if (entry.is_slider)
		{
			for (unsigned int j = 0; j <= entry.slider_size; j++)
			{
				if (entry.selected == j) line_str += "^004^";
				else line_str += "^327^";
			}
		}
		else line_str += entry.options_str.at(entry.selected);
		if (i == selected) line_str += "{5F}]"; else line_str += " ";
		const unsigned int x_pos = midcol - (strx::ansi_strlen(line_str) / 2);
		iocore::ansi_print(line_str, x_pos, midrow + entry.y_pos);
	}
	string back_str = " ^325^ Back" ;
	if (selected == entries.size()) back_str = "[^325^ Back]";
	iocore::print(back_str, midcol - 4, midrow + entries.at(entries.size() -1).y_pos + 2, (selected == entries.size() ? Colour::CGA_WHITE : Colour::CGA_LGRAY));
	if (has_must_restart) iocore::ansi_print("{54}^015^{58} Must restart for change to take effect.", midcol - 20, midrow + 18);
	if (has_cpu_heavy) iocore::ansi_print("{55}^019^{58} Disable this option to reduce CPU load.", midcol - 20, midrow + 17);
	if (selected < entries.size())
	{
		if (entries.at(selected).must_restart) iocore::ansi_print("{5C}^015^{5F} Must restart for change to take effect.", midcol - 20, midrow + 18);
		if (entries.at(selected).cpu_heavy) iocore::ansi_print("{5D}^019^{5F} Disable this option to reduce CPU load.", midcol - 20, midrow + 17);
	}
	iocore::flip();

	changed = false;
	unsigned int key = iocore::wait_for_key();
	if (iocore::is_up(key) && selected > 0) selected--;
	else if (iocore::is_down(key) && selected < entries.size()) selected++;
	else if (key == LMB_KEY || key == RMB_KEY)
	{
		for (unsigned int i = 0; i < entries.size(); i++)
		{
			if (iocore::did_mouse_click(0, midrow + entries.at(i).y_pos, iocore::get_cols(), 1))
			{
				if (selected != i) selected = i;
				else adjust_option(key == LMB_KEY ? 1 : -1);
				break;
			}
		}
		if (iocore::did_mouse_click(iocore::midcol() - 4, iocore::midrow() + entries.at(entries.size() - 1).y_pos + 2, 8, 1))
		{
			if (selected != entries.size()) selected = entries.size();
			else done = true;
		}
	}
	else if (iocore::is_right(key) || iocore::is_select(key))
	{
		if (selected == entries.size()) done = true;
		else adjust_option(1);
	}
	else if (iocore::is_left(key))
	{
		if (selected == entries.size()) done = true;
		else adjust_option(-1);
	}
	else if (iocore::is_cancel(key)) done = true;
}

// Returns the ID of the currently selected preference.
unsigned int Prefs::selected_id()
{
	STACK_TRACE();
	return entries.at(selected).id;
}

// Returns the value on the selected preference.
unsigned int Prefs::selected_val()
{
	STACK_TRACE();
	return entries.at(selected).selected;
}


namespace prefs
{

unsigned int	keybinds[PREFS_KEYBINDS];	// Keybind definitions.

uint32_t key_defaults[PREFS_KEYBINDS] = { KEY_NORTH_DEFAULT, KEY_SOUTH_DEFAULT, KEY_EAST_DEFAULT, KEY_WEST_DEFAULT, KEY_NORTHEAST_DEFAULT, KEY_NORTHWEST_DEFAULT, KEY_SOUTHEAST_DEFAULT, KEY_SOUTHWEST_DEFAULT, KEY_QUIT_GAME_DEFAULT,
		KEY_OPTIONS_WINDOW_DEFAULT, KEY_MENU_OK_DEFAULT, KEY_MENU_OK_2_DEFAULT, KEY_MENU_CANCEL_DEFAULT, KEY_SCREENSHOT_DEFAULT, KEY_SAVE_DEFAULT, KEY_SCROLL_TOP_DEFAULT, KEY_SCROLL_BOTTOM_DEFAULT, KEY_SCROLL_PAGEUP_DEFAULT,
		KEY_SCROLL_PAGEDOWN_DEFAULT, KEY_SCROLL_UP_DEFAULT, KEY_SCROLL_DOWN_DEFAULT, KEY_SCROLL_LEFT_DEFAULT, KEY_SCROLL_RIGHT_DEFAULT, KEY_OPEN_DEFAULT, KEY_CLOSE_DEFAULT };
const string key_names[PREFS_KEYBINDS] = { "key_north", "key_south", "key_east", "key_west", "key_northeast", "key_northwest", "key_southeast", "key_southwest", "key_quit_game", "key_options_window", "key_menu_ok", "key_menu_ok_2",
		"key_menu_cancel", "key_screenshot", "key_save", "key_scroll_top", "key_scroll_bottom", "key_scroll_pageup", "key_scroll_pagedown", "key_scroll_up", "key_scroll_down", "key_scroll_left", "key_scroll_right", "key_open",
		"key_close" };

// Used by the keybinds window.
vector<string>			key_longname;
vector<unsigned int>	key_id;
vector<unsigned int>	key_current;

bool			animation = ANIMATION_DEFAULT;	// Two-frame animation enabled/disabled.
bool			death_reports = DEATH_REPORTS_DEFAULT;	// Generate death report text files?
bool			fullscreen = FULLSCREEN_DEFAULT;	// Fullscreen mode.
bool			glitch_warn = false;	// Have we shown the user the glitch warning screen?
bool			message_log_dim = MESSAGE_LOG_DIM_DEFAULT;	// Dim the colours in the message log?
bool			ntsc_filter = NTSC_FILTER_DEFAULT;	// Whether or not the NTSC filter is enabled.
unsigned char	ntsc_mode = NTSC_MODE_DEFAULT;	// NTSC post-processing level.
unsigned char	palette = PALETTE_DEFAULT;	// Which colour palette to use?
unsigned char	scale_mod = SCALE_MOD_DEFAULT;		// Experimental surface scaling.
short			screen_x = SCREEN_X_DEFAULT, screen_y = SCREEN_Y_DEFAULT;	// The starting screen X,Y size.
unsigned char	screenshot_type = SCREENSHOT_TYPE_DEFAULT;	// The type of screenshots to take (BMP/UPNG/CPNG)
unsigned char	visual_glitches = VISUAL_GLITCHES_DEFAULT;	// Visual glitches enabled/disabled.

// Resets a keybind to default.
void default_keybind(Keys key)
{
	STACK_TRACE();
	if (static_cast<int>(key) >= PREFS_KEYBINDS) guru::halt("Invalid keybind specified!");
	keybinds[key] = key_defaults[key];
}

// Loads and configures the user's preferences.
void init()
{
	STACK_TRACE();
	guru::log("Attempting to load prefs.dat file, if it exists.", GURU_INFO);

	// Set some defaults for the keybinds, since they're not set above like with the other prefs.
	for (int i = 0; i < PREFS_KEYBINDS; i++)
		default_keybind(static_cast<Keys>(i));

	// Check to ensure prefs.fox actually exists.
	bool file_exists = filex::file_exists(FILENAME_PREFS);

	if (!file_exists)
	{
		guru::log("Can't find prefs.dat, rebuilding default prefs file.", GURU_WARN);
		SQLite::Database(FILENAME_PREFS, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
	}

	try
	{
		SQLite::Database prefs_db(FILENAME_PREFS, SQLite::OPEN_READWRITE);

		if (file_exists)
		{
			SQLite::Statement prefs_query(prefs_db, "SELECT * FROM prefs");
			while (prefs_query.executeStep())
			{
				const string id = prefs_query.getColumn("pref").getString();
				const unsigned int value = prefs_query.getColumn("value").getUInt();
				if (id == "screen_x") screen_x = value;
				else if (id == "screen_y") screen_y = value;
				else if (id == "fullscreen") fullscreen = value;
				else if (id == "ntsc_mode") ntsc_mode = value;
				else if (id == "screenshot_type") screenshot_type = value;
				else if (id == "visual_glitches") visual_glitches = value;
				else if (id == "palette") palette = value;
				else if (id == "scale_mod") scale_mod = value;
				else if (id == "glitch_warn") glitch_warn = value;
				else if (id == "death_reports") death_reports = value;
				else if (id == "message_log_dim") message_log_dim = value;
				else if (id == "ntsc_filter") ntsc_filter = value;
				else if (id == "animation") animation = value;
				else guru::log("Unknown preference found in prefs.dat: " + id, GURU_WARN);
			}

			SQLite::Statement key_query(prefs_db, "SELECT * FROM keybinds");
			while (key_query.executeStep())
			{
				const string id = key_query.getColumn("key").getString();
				const unsigned int bind = key_query.getColumn("value").getUInt();
				bool found = false;
				for (int i = 0; i < PREFS_KEYBINDS; i++)
				{
					if (key_names[i] == id)
					{
						found = true;
						keybinds[i] = bind;
						break;
					}
				}
				if (!found) guru::log("Could not find keybind definitions in prefs.dat.", GURU_WARN);
			}
		}
		else save(&prefs_db);
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
	ui_init_keybinds();
}

// Returns the specified bound key.
unsigned int keybind(Keys key)
{
	STACK_TRACE();
	if (static_cast<int>(key) >= PREFS_KEYBINDS) guru::halt("Invalid keybind specified!");
	return keybinds[static_cast<unsigned int>(key)];
}

// Change the key bindings.
void keybinds_window()
{
	STACK_TRACE();
	iocore::cls();
	iocore::render_nebula(64530, 0, 0);
	int midrow = iocore::midrow(), midcol = iocore::midcol();
	unsigned int selected = 2;
	int offset = 0;


	bool waiting_for_new_key = false, update_key = false;

	while(true)
	{
		iocore::box(midcol - 27, midrow - 18, 55, 38, UI_COLOUR_BOX);
		iocore::box(midcol - 26, midrow - 17, 53, 4, Colour::CGA_LCYAN);
		iocore::print("ArrowKeys: Select Key - Enter: Remap Key", midcol - 20, midrow - 16, Colour::CGA_WHITE);
		iocore::print("Escape: Clear Key - D: Revert to Default", midcol - 20, midrow - 15, Colour::CGA_WHITE);

		string line_str;
		for (unsigned int i = 0; i < key_id.size(); i++)
		{
			const int pos_y = midrow - 12 + i + offset;
			if (pos_y < midrow - 12) continue;
			if (pos_y > midrow + 16) break;

			if (key_id.at(i) == UINT_MAX)
			{
				if (key_longname.at(i).size())
					iocore::ansi_print(key_longname.at(i), midcol - (strx::ansi_strlen(key_longname.at(i)) / 2), pos_y);
			}
			else
			{
				if (selected == i) line_str = "{5F}["; else line_str = "{5F} ";
				string key_str = ((waiting_for_new_key && selected == i) ? "{5B}choose key" : iocore::key_to_name(key_current.at(i)));
				if (!key_str.size()) key_str = "{5C}[unknown]";
				else
				{
					bool duplicate = false;
					for (unsigned int j = 0; j < key_id.size(); j++)
					{
						if (i == j) continue;
						if (key_current.at(i) == key_current.at(j)) duplicate = true;
					}
					if (duplicate) key_str = "{5C}" + key_str;
					else key_str = "{5A}" + key_str;
				}
				line_str += key_longname.at(i) + ": ";
				const int line_pos = midcol - strx::ansi_strlen(line_str) + 3;
				line_str += key_str;
				if (selected == i) line_str += "{5F}]"; else line_str += " ";
				iocore::ansi_print(line_str, line_pos, pos_y);
			}
		}

		if (selected == key_id.size()) line_str = "{5F}["; else line_str = "{5F} ";
		line_str += "^325^ Back";
		if (selected == key_id.size()) line_str += "{5F}]"; else line_str += " ";
		iocore::ansi_print(line_str, midcol - (strx::ansi_strlen(line_str) / 2), midrow + 18);

		iocore::flip();
		unsigned int key = iocore::wait_for_key();
		if (key == RESIZE_KEY)
		{
			iocore::cls();
			iocore::render_nebula(64530, 0, 0);
			midrow = iocore::midrow();
			midcol = iocore::midcol();
		}
		else if (waiting_for_new_key)
		{
			prefs::set_keybind(static_cast<Keys>(key_id.at(selected)), key);
			update_key = true;
			waiting_for_new_key = false;
		}
		else
		{
			if (key == 'd' && selected < key_longname.size())
			{
				prefs::default_keybind(static_cast<Keys>(key_id.at(selected)));
				update_key = true;
			}
			else if (key == SDLK_ESCAPE && selected < key_longname.size())
			{
				prefs::set_keybind(static_cast<Keys>(key_id.at(selected)), 0);
				update_key = true;
			}
			else if (iocore::is_up(key) && selected > 2)
			{
				selected--;
				while (key_id.at(selected) == UINT_MAX) selected--;
			}
			else if (iocore::is_down(key) && selected < key_id.size())
			{
				selected++;
				if (selected < key_id.size())
					while (key_id.at(selected) == UINT_MAX) selected++;
			}
			else if (iocore::is_select(key))
			{
				if (selected == key_id.size()) break;
				else waiting_for_new_key = true;
			}
			else if ((key == LMB_KEY && iocore::did_mouse_click(0, iocore::midrow() + 18, iocore::get_cols())) || key == RMB_KEY) return;
		}

		if (update_key)
		{
			key_current.at(selected) = prefs::keybind(static_cast<Keys>(key_id.at(selected)));
			update_key = false;
			prefs::save();
		}

		if (offset + static_cast<signed int>(selected) > 28) offset -= 14;
		else if (offset + static_cast<signed int>(selected) < 0) offset += 14;
		if (offset > 0) offset = 0;
		else if (offset < -static_cast<signed int>(key_id.size())) offset = -static_cast<signed int>(key_id.size());
	}
}

// Parses a string, replacing key tags such as %%KEY_MENU_CANCEL%% with the name of the key.
void parse_string_with_key_tags(string &str)
{
	STACK_TRACE();
	unsigned int cycles = 0;
	while (str.find("%%") != string::npos && ++cycles < 100)
	{
		const size_t start = str.find_first_of("%%");
		const size_t end = str.find("%%", start + 2);
		const string before = str.substr(0, start);
		const string after = str.substr(end + 2);
		const string tag = strx::str_tolower(str.substr(start + 2, end - start - 2));
		string new_tag = "???";
		for (unsigned int i = 0; i < PREFS_KEYBINDS; i++)
		{
			if (key_names[i] == tag)
			{
				new_tag = iocore::key_to_name(keybind(static_cast<Keys>(i)));
				break;
			}
		}
		str = before + new_tag + after;
	}
	if (cycles >= 100) guru::log("Timed out while attempting to parse key string: " + str);
}

// Where the user can change prefs and shit.
void prefs_window()
{
	STACK_TRACE();
	unsigned char selected = 0;
	bool done = false;
	while(!done)
	{
		const int midcol = iocore::midcol(), midrow = iocore::midrow(), cols = iocore::get_cols();
		iocore::cls();
		iocore::box(midcol - 27, midrow - 18, 55, 38, UI_COLOUR_BOX);

		iocore::alagard_print("GRAPHICS", (cols * 4) - (8 * 12), (midrow - 12) * 8, (selected == 0 ? Colour::CGA_LCYAN : Colour::CGA_GRAY));
		iocore::alagard_print("GAMEPLAY", (cols * 4) - (8 * 12), (midrow - 6) * 8, (selected == 1 ? Colour::CGA_LCYAN : Colour::CGA_GRAY));
		iocore::alagard_print("KEYBINDS", (cols * 4) - (8 * 12), midrow * 8, (selected == 2 ? Colour::CGA_LCYAN : Colour::CGA_GRAY));
		iocore::alagard_print("BACK", (cols * 4) - (4 * 12), (midrow + 12) * 8, (selected == 3 ? Colour::CGA_LCYAN : Colour::CGA_GRAY));

		iocore::flip();
		unsigned int key = iocore::wait_for_key();

		if (key == RMB_KEY) done = true;
		else if (key == LMB_KEY)
		{
			if (iocore::did_mouse_click(midcol - 12, midrow - 12, 24, 3)) { selected = 0; prefs_window_graphics(); }
			else if (iocore::did_mouse_click(midcol - 12, midrow - 6, 24, 3)) { selected = 1; prefs_window_gameplay(); }
			else if (iocore::did_mouse_click(midcol - 12, midrow, 24, 3)) { selected = 2; keybinds_window(); }
			else if (iocore::did_mouse_click(midcol - 6, midrow + 12, 12, 3)) done = true;
		}
		if (iocore::is_up(key) && selected > 0) selected--;
		else if (iocore::is_down(key) && selected < 3) selected++;
		else if (iocore::is_select(key))
		{
			switch(selected)
			{
				case 0: prefs_window_graphics(); break;
				case 1: prefs_window_gameplay(); break;
				case 2: keybinds_window(); break;
				case 3: done = true; break;
			}
		}
		else if (iocore::is_cancel(key)) done = true;
	}
}

// General gameplay stuff.
void prefs_window_gameplay()
{
	STACK_TRACE();

	PrefsEntry pe_death_report;
	Prefs prefs_screen;
	prefs_screen.name = "GAMEPLAY";

	pe_death_report.id = ID_DEATH_REPORT;
	pe_death_report.name = "Generate Death Reports";
	pe_death_report.is_boolean = true;
	pe_death_report.selected = prefs::death_reports;
	prefs_screen.add_item(pe_death_report);

	while (!prefs_screen.done)
	{
		prefs_screen.render();
		if (prefs_screen.changed)
		{
			const unsigned int val = prefs_screen.selected_val();
			switch(prefs_screen.selected_id())
			{
				case ID_DEATH_REPORT: prefs::death_reports = val; break;
			}
			prefs::save();
		}
	}
}

// Visual stuff goes here.
void prefs_window_graphics()
{
	STACK_TRACE();
	const int actual_resolution = prefs::screen_x * prefs::screen_y;
	int resolution_choice = 0;

	if (actual_resolution >= 2560 * 1440) resolution_choice = 15;
	else if (actual_resolution >= 1920 * 1080) resolution_choice = 14;
	else if (actual_resolution >= 1776 * 1000) resolution_choice = 13;
	else if (actual_resolution >= 1680 * 1050) resolution_choice = 12;
	else if (actual_resolution >= 1600 * 900) resolution_choice = 11;
	else if (actual_resolution >= 1360 * 1024) resolution_choice = 10;
	else if (actual_resolution >= 1280 * 1024) resolution_choice = 9;
	else if (actual_resolution >= 1440 * 900) resolution_choice = 8;
	else if (actual_resolution >= 1280 * 960) resolution_choice = 7;
	else if (actual_resolution >= 1366 * 768) resolution_choice = 6;
	else if (actual_resolution >= 1360 * 768) resolution_choice = 5;
	else if (actual_resolution >= 1280 * 800) resolution_choice = 4;
	else if (actual_resolution >= 1280 * 768) resolution_choice = 3;
	else if (actual_resolution >= 1280 * 720) resolution_choice = 2;
	else if (actual_resolution >= 1024 * 768) resolution_choice = 1;
	else resolution_choice = 0;

	PrefsEntry pe_screen_res, pe_full_screen, pe_shader, pe_palette, pe_glitches, pe_ss_format, pe_tex_scaling, pe_ml_dim, pe_ntsc_filter, pe_animation;
	Prefs prefs_screen;
	prefs_screen.name = "GRAPHICS";

	pe_full_screen.id = ID_FULL_SCREEN;
	pe_full_screen.name = "Full-Screen";
	pe_full_screen.is_boolean = true;
	pe_full_screen.selected = (prefs::fullscreen ? 1 : 0);
	pe_full_screen.must_restart = true;
	prefs_screen.add_item(pe_full_screen);

	pe_screen_res.id = ID_SCREEN_RES;
	pe_screen_res.name = "Screen Resolution";
	pe_screen_res.must_restart = true;
	pe_screen_res.selected = resolution_choice;
	pe_screen_res.options_str.push_back("1024x600");
	pe_screen_res.options_str.push_back("1024x768");
	pe_screen_res.options_str.push_back("1280x720");
	pe_screen_res.options_str.push_back("1280x768");
	pe_screen_res.options_str.push_back("1280x800");
	pe_screen_res.options_str.push_back("1360x768");
	pe_screen_res.options_str.push_back("1366x768");
	pe_screen_res.options_str.push_back("1280x960");
	pe_screen_res.options_str.push_back("1440x900");
	pe_screen_res.options_str.push_back("1280x1024");
	pe_screen_res.options_str.push_back("1360x1024");
	pe_screen_res.options_str.push_back("1600x900");
	pe_screen_res.options_str.push_back("1680x1050");
	pe_screen_res.options_str.push_back("1776x1000");
	pe_screen_res.options_str.push_back("1920x1080");
	pe_screen_res.options_str.push_back("2560x1440");
	prefs_screen.add_item(pe_screen_res);

	pe_tex_scaling.id = ID_TEX_SCALING;
	pe_tex_scaling.name = "Texture Scaling";
	pe_tex_scaling.selected = prefs::scale_mod;
	pe_tex_scaling.options_str.push_back("DISABLED");
	pe_tex_scaling.options_str.push_back("4:3");
	pe_tex_scaling.options_str.push_back("DOUBLE");
	pe_tex_scaling.options_str.push_back("TO WINDOW");
	pe_tex_scaling.must_restart = true;
	pe_tex_scaling.cpu_heavy = true;
	prefs_screen.add_item(pe_tex_scaling);

	pe_ntsc_filter.id = ID_NTSC_FILTER;
	pe_ntsc_filter.name = "NTSC Filter";
	pe_ntsc_filter.selected = prefs::ntsc_filter;
	pe_ntsc_filter.is_boolean = true;
	pe_ntsc_filter.selected = prefs::ntsc_filter;
	pe_ntsc_filter.must_restart = true;
	pe_ntsc_filter.cpu_heavy = true;
	prefs_screen.add_item(pe_ntsc_filter);

	pe_shader.id = ID_SHADER;
	pe_shader.name = "NTSC Shader";
	pe_shader.selected = prefs::ntsc_mode;
	pe_shader.options_str.push_back("RGB");
	pe_shader.options_str.push_back("S-VIDEO");
	pe_shader.options_str.push_back("COMPOSITE");
	pe_shader.options_str.push_back("MONOCHROME");
	prefs_screen.add_item(pe_shader);

	pe_palette.id = ID_PALETTE;
	pe_palette.name = "Colour Palette";
	pe_palette.selected = prefs::palette;
	pe_palette.options_str.push_back("FULL");
	pe_palette.options_str.push_back("CGA");
	pe_palette.options_str.push_back("NES");
	pe_palette.options_str.push_back("COLOURBLIND R/G");
	pe_palette.options_str.push_back("COLOURBLIND B");
	prefs_screen.add_item(pe_palette);

	pe_animation.id = ID_ANIMATION;
	pe_animation.name = "2-Frame Animation";
	pe_animation.selected = prefs::animation;
	pe_animation.is_boolean = true;
	pe_animation.selected = prefs::animation;
	prefs_screen.add_item(pe_animation);

	pe_glitches.id = ID_GLITCHES;
	pe_glitches.name = "Visual Glitches";
	pe_glitches.is_slider = true;
	pe_glitches.slider_size = 3;
	pe_glitches.selected = prefs::visual_glitches;
	prefs_screen.add_item(pe_glitches);

	pe_ss_format.id = ID_SS_FORMAT;
	pe_ss_format.name = "Screenshot Format";
	pe_ss_format.selected = prefs::screenshot_type;
	pe_ss_format.options_str.push_back("BMP");
	pe_ss_format.options_str.push_back("PNG");
	pe_ss_format.options_str.push_back("JPG");
	prefs_screen.add_item(pe_ss_format);

	pe_ml_dim.id = ID_MESSAGE_LOG_DIM;
	pe_ml_dim.name = "Dim Message Log";
	pe_ml_dim.selected = (prefs::message_log_dim ? 1 : 0);
	pe_ml_dim.is_boolean = true;
	prefs_screen.add_item(pe_ml_dim);

	while(!prefs_screen.done)
	{
		prefs_screen.render();
		if (prefs_screen.changed)
		{
			const unsigned int val = prefs_screen.selected_val();
			switch(prefs_screen.selected_id())
			{
				case ID_SCREEN_RES: resolution_choice = val; break;
				case ID_FULL_SCREEN: prefs::fullscreen = val; break;
				case ID_SHADER: prefs::ntsc_mode = val; iocore::update_ntsc_mode(); break;
				case ID_PALETTE: prefs::palette = val; break;
				case ID_GLITCHES: prefs::visual_glitches = val; break;
				case ID_SS_FORMAT: prefs::screenshot_type = val; break;
				case ID_TEX_SCALING: prefs::scale_mod = val; break;
				case ID_MESSAGE_LOG_DIM: prefs::message_log_dim = val; break;
				case ID_NTSC_FILTER: prefs::ntsc_filter = val; break;
				case ID_ANIMATION: prefs::animation = val; break;
			}
			if (prefs_screen.selected_id() == ID_SCREEN_RES)
			{
				switch(resolution_choice)
				{
					case 15: prefs::screen_x = 2560; prefs::screen_y = 1440; break;
					case 14: prefs::screen_x = 1920; prefs::screen_y = 1080; break;
					case 13: prefs::screen_x = 1776; prefs::screen_y = 1000; break;
					case 12: prefs::screen_x = 1680; prefs::screen_y = 1050; break;
					case 11: prefs::screen_x = 1600; prefs::screen_y = 900; break;
					case 10: prefs::screen_x = 1360; prefs::screen_y = 1024; break;
					case 9: prefs::screen_x = 1280; prefs::screen_y = 1024; break;
					case 8: prefs::screen_x = 1440; prefs::screen_y = 900; break;
					case 7: prefs::screen_x = 1280; prefs::screen_y = 960; break;
					case 6: prefs::screen_x = 1366; prefs::screen_y = 768; break;
					case 5: prefs::screen_x = 1360; prefs::screen_y = 768; break;
					case 4: prefs::screen_x = 1280; prefs::screen_y = 800; break;
					case 3: prefs::screen_x = 1280; prefs::screen_y = 768; break;
					case 2: prefs::screen_x = 1280; prefs::screen_y = 720; break;
					case 1: prefs::screen_x = 1024; prefs::screen_y = 768; break;
					case 0: prefs::screen_x = 1024; prefs::screen_y = 600; break;
				}
			}
		}
		prefs::save();
	}
}

// Saves the updated preferences file.
void save(SQLite::Database *prefs_db)
{
	STACK_TRACE();

	try
	{
		bool close_db = false;
		if (!prefs_db)
		{
			prefs_db = new SQLite::Database(FILENAME_PREFS, SQLite::OPEN_READWRITE);
			close_db = true;
		}

		SQLite::Transaction transaction(*prefs_db);
		prefs_db->exec("DROP TABLE IF EXISTS prefs; CREATE TABLE prefs ( key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, pref TEXT NOT NULL, value INTEGER NOT NULL ); "
				"DROP TABLE IF EXISTS keybinds; CREATE TABLE keybinds ( id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, key TEXT NOT NULL, value INTEGER NOT NULL );");

		auto sql_insert_pref = [prefs_db] (string pref, long long value)
				{
			SQLite::Statement sqls(*prefs_db, "INSERT INTO prefs (pref,value) VALUES (?,?)");
			sqls.bind(1, pref);
			sqls.bind(2, value);
			sqls.exec();
		};

		sql_insert_pref("screen_x", screen_x);
		sql_insert_pref("screen_y", screen_y);
		sql_insert_pref("fullscreen", fullscreen);
		sql_insert_pref("ntsc_mode", ntsc_mode);
		sql_insert_pref("screenshot_type", screenshot_type);
		sql_insert_pref("visual_glitches", visual_glitches);
		sql_insert_pref("palette", palette);
		sql_insert_pref("scale_mod", scale_mod);
		sql_insert_pref("glitch_warn", glitch_warn);
		sql_insert_pref("death_reports", death_reports);
		sql_insert_pref("message_log_dim", message_log_dim);
		sql_insert_pref("ntsc_filter", ntsc_filter);
		sql_insert_pref("animation", animation);

		auto sql_insert_keybind = [prefs_db] (string key, long long value)
		{
			SQLite::Statement sqls(*prefs_db, "INSERT INTO keybinds (key,value) VALUES (?,?)");
			sqls.bind(1, key);
			sqls.bind(2, value);
			sqls.exec();
		};

		for (int i = 0; i < PREFS_KEYBINDS; i++)
			sql_insert_keybind(key_names[i], keybinds[i]);

		transaction.commit();

		if (close_db) delete prefs_db;
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Sets a new keybind.
void set_keybind(Keys key, unsigned int new_key, bool reset_default)
{
	STACK_TRACE();
	if (static_cast<int>(key) >= PREFS_KEYBINDS) guru::halt("Invalid keybind specified!");
	keybinds[static_cast<unsigned int>(key)] = new_key;
	if (reset_default) key_defaults[static_cast<unsigned int>(key)] = new_key;
}

// Used by init_keybinds() to set up the keys.
void ui_add_keybind(string name, unsigned int id)
{
	STACK_TRACE();
	key_longname.push_back(name);
	key_id.push_back(id);
	if (id == UINT_MAX) key_current.push_back(0);
	else key_current.push_back(prefs::keybind(static_cast<Keys>(id)));
}

// Initializes the keybinds config data.
void ui_init_keybinds()
{
	STACK_TRACE();
	ui_add_keybind("{5F}^219^^178^^177^^176^ MOVEMENT ^176^^177^^178^^219^", UINT_MAX);
	ui_add_keybind("", UINT_MAX);
	ui_add_keybind("Travel North", Keys::NORTH);
	ui_add_keybind("Travel South", Keys::SOUTH);
	ui_add_keybind("Travel East", Keys::EAST);
	ui_add_keybind("Travel West", Keys::WEST);
	ui_add_keybind("Travel Northeast", Keys::NORTHEAST);
	ui_add_keybind("Travel Northwest", Keys::NORTHWEST);
	ui_add_keybind("Travel Southeast", Keys::SOUTHEAST);
	ui_add_keybind("Travel Southwest", Keys::SOUTHWEST);
	ui_add_keybind("", UINT_MAX);

	ui_add_keybind("{5F}^219^^178^^177^^176^ WORLD INTERACTION ^176^^177^^178^^219^", UINT_MAX);
	ui_add_keybind("", UINT_MAX);
	ui_add_keybind("Open Door", Keys::OPEN);
	ui_add_keybind("Close Door", Keys::CLOSE);
	ui_add_keybind("", UINT_MAX);

	ui_add_keybind("{5F}^219^^178^^177^^176^ MESSAGE LOG & WIKI ^176^^177^^178^^219^", UINT_MAX);
	ui_add_keybind("", UINT_MAX);
	ui_add_keybind("Scroll Up", Keys::SCROLL_UP);
	ui_add_keybind("Scroll Down", Keys::SCROLL_DOWN);
	ui_add_keybind("Select Left (Wiki)", Keys::SCROLL_LEFT);
	ui_add_keybind("Select Right (Wiki)", Keys::SCROLL_RIGHT);
	ui_add_keybind("Scroll PageUp", Keys::SCROLL_PAGEUP);
	ui_add_keybind("Scroll PageDown", Keys::SCROLL_PAGEDOWN);
	ui_add_keybind("Scroll Top", Keys::SCROLL_TOP);
	ui_add_keybind("Scroll Bottom", Keys::SCROLL_BOTTOM);

	ui_add_keybind("", UINT_MAX);
	ui_add_keybind("{5F}^219^^178^^177^^176^ MISCELLANEOUS ^176^^177^^178^^219^", UINT_MAX);
	ui_add_keybind("", UINT_MAX);
	ui_add_keybind("Menu: OK", Keys::MENU_OK);
	ui_add_keybind("Menu: OK", Keys::MENU_OK_2);
	ui_add_keybind("Menu: Cancel", Keys::MENU_CANCEL);
	ui_add_keybind("Save Game", Keys::SAVE);
	ui_add_keybind("Quit Game", Keys::QUIT_GAME);
	ui_add_keybind("Screenshot", Keys::SCREENSHOT);
	ui_add_keybind("Game Settings", Keys::OPTIONS_WINDOW);
}

}	// namespace prefs
