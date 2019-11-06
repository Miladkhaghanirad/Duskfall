// prefs.h -- Game preferences and keybind config windows, as well as easy code access to the prefs.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

namespace SQLite { class Database; }	// defined in SQLiteCpp/SQLiteCpp.h

enum Keys { NORTH, SOUTH, EAST, WEST, NORTHEAST, NORTHWEST, SOUTHEAST, SOUTHWEST, QUIT_GAME, OPTIONS_WINDOW, MENU_OK, MENU_OK_2, MENU_CANCEL, SCREENSHOT, SAVE, SCROLL_TOP, SCROLL_BOTTOM, SCROLL_PAGEUP, SCROLL_PAGEDOWN, SCROLL_UP,
	SCROLL_DOWN, SCROLL_LEFT, SCROLL_RIGHT, OPEN, CLOSE };

class PrefsEntry
{
public:
					PrefsEntry();
	bool			cpu_heavy;		// Is this an CPU-heavy option?
	unsigned char	id;				// The internal ID of this prefs option.
	bool			is_boolean;		// Is this prefs option a boolean?
	bool			is_slider;		// Is  this prefs option a menu slider?
	bool			must_restart;	// Do we have to restart to change this option?
	string			name;			// The name of this prefs option.
	vector<string>	options_str;	// Vector of strings for the selectable options.
	unsigned int	selected;		// The currently-selected option.
	unsigned char	slider_size;	// The size of a menu slider, if any.
	unsigned int	y_pos;			// The vertical position on the screen where this entry will be rendered.
};

class Prefs
{
public:
					Prefs();
	void			add_item(PrefsEntry entry);	// Adds a new entry to the list.
	void			render();		// Renders the prefs list, processes input.
	unsigned int	selected_id();	// Returns the ID of the currently selected preference.
	unsigned int	selected_val();	// Returns the value on the selected preference.

	bool			changed;	// Did the user change an option?
	bool			done;		// Are we done here?
	string			name;		// The name of this prefs screen.
	unsigned int	selected;	// The currently selected preference.

private:
	void		adjust_option(signed char amount);	// Adjusts an option up or down.

	vector<PrefsEntry>	entries;	// The prefs entries in this list.
	bool	has_cpu_heavy	;	// Do we have any CPU-heavy options?
	bool	has_must_restart;	// Do we have an option that requires a restart?
	int		y_pos;				// The vertical position of the prefs entries.
};

namespace prefs
{

extern bool				animation;			// Two-frame animation enabled/disabled.
extern bool				death_reports;		// Generate death report text files?
extern bool				fullscreen;			// Fullscreen mode.
extern bool				glitch_warn;		// Have we shown the user the glitch warning screen?
extern bool				message_log_dim;	// Dim the colours in the message log?
extern bool				ntsc_filter;		// Whether or not the NTSC filter is enabled.
extern unsigned char	ntsc_mode;			// NTSC post-processing level.
extern unsigned char	palette;			// Which colour palette to use?
extern unsigned char	scale_mod;			// Experimental surface scaling.
extern short			screen_x, screen_y;	// The starting screen X,Y size.
extern unsigned char	screenshot_type;	// The type of screenshots to take (BMP/UPNG/CPNG)
extern unsigned char	visual_glitches;	// Visual glitches enabled/disabled.

void	init();	// Loads and configures the user's preferences.
void	save(SQLite::Database *prefs_db = nullptr);	// Saves the updated preferences file.
unsigned int	keybind(Keys key);	// Returns the specified bound key.
void	set_keybind(Keys key, unsigned int new_key, bool reset_default = false);	// Sets a new keybind.
void	default_keybind(Keys key);	// Resets a keybind to default.
void	prefs_window();				// Where the user can change prefs and shit.
void	prefs_window_graphics();	// Visual stuff goes here.
void	prefs_window_gameplay();	// General gameplay stuff.
void	keybinds_window();			// Change the key bindings.
void	ui_init_keybinds();			// Initializes the keybinds config data.
void	ui_add_keybind(string name, unsigned int id);	// Used by init_keybinds() to set up the keys.
void	parse_string_with_key_tags(string &str);		// Parses a string, replacing key tags such as %%KEY_MENU_CANCEL%% with the name of the key.

}	// namespace prefs
