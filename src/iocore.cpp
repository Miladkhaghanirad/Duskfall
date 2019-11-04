// iocore.cpp -- The render core, handling display and user interaction, as well as program initialization, shutdown and cleanup functionality.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "guru.h"
#include "iocore.h"
#include "mathx.h"
#include "prefs.h"
#include "strx.h"
#include "version.h"

#include "BearLibTerminal/BearLibTerminal.h"

#include <chrono>
#include <cmath>
#include <unordered_map>


/****************************
 * BUILD VERSION GENERATION *
 ****************************/

#define BUILD_YEAR_CH0 (__DATE__[ 7])
#define BUILD_YEAR_CH1 (__DATE__[ 8])
#define BUILD_YEAR_CH2 (__DATE__[ 9])
#define BUILD_YEAR_CH3 (__DATE__[10])
#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')
#define BUILD_MONTH_CH0 ((BUILD_MONTH_IS_OCT || BUILD_MONTH_IS_NOV || BUILD_MONTH_IS_DEC) ? '1' : '0')
#define BUILD_MONTH_CH1 \
    ( \
        (BUILD_MONTH_IS_JAN) ? '1' : \
        (BUILD_MONTH_IS_FEB) ? '2' : \
        (BUILD_MONTH_IS_MAR) ? '3' : \
        (BUILD_MONTH_IS_APR) ? '4' : \
        (BUILD_MONTH_IS_MAY) ? '5' : \
        (BUILD_MONTH_IS_JUN) ? '6' : \
        (BUILD_MONTH_IS_JUL) ? '7' : \
        (BUILD_MONTH_IS_AUG) ? '8' : \
        (BUILD_MONTH_IS_SEP) ? '9' : \
        (BUILD_MONTH_IS_OCT) ? '0' : \
        (BUILD_MONTH_IS_NOV) ? '1' : \
        (BUILD_MONTH_IS_DEC) ? '2' : \
        /* error default */    '?' \
    )
#define BUILD_DAY_CH0 ((__DATE__[4] >= '0') ? (__DATE__[4]) : '0')
#define BUILD_DAY_CH1 (__DATE__[ 5])
#define BUILD_HOUR_CH0 (__TIME__[0])
#define BUILD_HOUR_CH1 (__TIME__[1])
#define BUILD_MIN_CH0 (__TIME__[3])
#define BUILD_MIN_CH1 (__TIME__[4])
#define BUILD_SEC_CH0 (__TIME__[6])
#define BUILD_SEC_CH1 (__TIME__[7])

static const char cc_date[] = { BUILD_YEAR_CH2, BUILD_YEAR_CH3, BUILD_MONTH_CH0, BUILD_MONTH_CH1, BUILD_DAY_CH0, BUILD_DAY_CH1, BUILD_HOUR_CH0, BUILD_HOUR_CH1, BUILD_MIN_CH0, BUILD_MIN_CH1, '\0' };

unsigned int build_version() { return atoi(cc_date); }

/***********************************
 * END OF BUILD VERSION GENERATION *
 ***********************************/


#define EXTRA_NES_COLOURS			53	// How many extra colours are added from the NES palette.
#define EXTRA_COLOURBLIND_COLOURS	32	// How many extra colours are added from the colourblind palettes.


namespace iocore
{

bool			cleaned_up;							// Have we run the exit functions already?
unsigned short	cols, rows, mid_col, mid_row;		// The number of columns and rows available, and the middle column/row.
unsigned char	exit_func_level;					// Keep track of what to clean up at exit.
unsigned short	mouse_clicked_x, mouse_clicked_y;	// Last clicked location for a mouse event.
std::unordered_map<std::string, s_rgb>	nebula_cache;	// Cache for the nebula() function.
unsigned short	nebula_cache_seed;					// The seed for the nebula cache.
int				screen_x, screen_y;					// Chosen screen resolution.


// Adjusts the colour palette, if needed.
Colour adjust_palette(Colour colour)
{
	static const unsigned char colour_table_cga[(MAX_COLOUR + 1)] = {
			0x2D, 0x43, 0x43, 0x43, 0x45, 0x45, 0x45, 0x45, 0x45, 0x43, 0x43, 0x43, 0x43, 0x0D, 0x45, 0x0D,
			0x57, 0x43, 0x43, 0x43, 0x45, 0x45, 0x45, 0x45, 0x45, 0x43, 0x43, 0x43, 0x43, 0x1D, 0x4D, 0x0D,
			0x4D, 0x4B, 0x4B, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4B, 0x4B, 0x4B, 0x4B, 0x2D, 0x4D, 0x0D,
			0x30, 0x4B, 0x4B, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4B, 0x4B, 0x4B, 0x4B, 0x57, 0x2D, 0x0D,
			0x0D, 0x43, 0x43, 0x43, 0x45, 0x45, 0x45, 0x57, 0x57, 0x43, 0x4B, 0x4B, 0x4D, 0x4D, 0x4D, 0x30,
			0x0D, 0x43, 0x43, 0x43, 0x45, 0x45, 0x45, 0x57, 0x2D, 0x4B, 0x4B, 0x4B, 0x4D, 0x4D, 0x4D, 0x30 };

	static const unsigned char colour_table_nes[(MAX_COLOUR + 1)] = {
			0x87, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x0D, 0x68, 0x0D,
			0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x1D, 0x75, 0x0D,
			0x82, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x96, 0x87, 0x8F, 0x0D,
			0x30, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x7A, 0x87, 0x0D,
			0x0D, 0x62, 0x6A, 0x79, 0x65, 0x64, 0x75, 0x6D, 0x60, 0x6F, 0x76, 0x93, 0x73, 0x71, 0x82, 0x30,
			0x0D, 0x62, 0x76, 0x79, 0x66, 0x64, 0x75, 0x6D, 0x87, 0x6E, 0x76, 0x86, 0x73, 0x71, 0x82, 0x30
 	};

	static const unsigned char colour_table_cblinda[(MAX_COLOUR + 1)] = {
			0x00, 0x9E, 0x9D, 0x9D, 0x96, 0x97, 0x97, 0x96, 0x95, 0x9D, 0x9D, 0x9D, 0x9D, 0x0D, 0x95, 0x0D,
			0x10, 0xA0, 0x9F, 0x9F, 0x98, 0x98, 0x97, 0x97, 0x9F, 0xA0, 0xA0, 0xA1, 0x1D, 0x1D, 0x98, 0x0D,
			0x9B, 0xA3, 0xA2, 0xA3, 0x99, 0x98, 0x98, 0x99, 0x99, 0xA4, 0xA4, 0xA4, 0xA4, 0x2D, 0x9B, 0x0D,
			0x30, 0xA4, 0xA3, 0xA3, 0x9B, 0x9B, 0x9B, 0x9B, 0x9C, 0xA4, 0xA4, 0xA4, 0xA4, 0x3D, 0x3E, 0x0F,
			0x0D, 0x9D, 0xA0, 0xA0, 0x95, 0x96, 0x99, 0x47, 0x48, 0x9E, 0xA3, 0xA4, 0x97, 0x98, 0x9B, 0x30,
			0x0D, 0x9D, 0xA2, 0xA1, 0x97, 0x97, 0x98, 0x57, 0x58, 0xA0, 0xA3, 0xA4, 0x9A, 0x9B, 0x9B, 0x30
	};

	static const unsigned char colour_table_cblindb[(MAX_COLOUR + 1)] = {
			0x00, 0xAD, 0xAD, 0xAF, 0xA6, 0xA7, 0xA7, 0xA6, 0xA5, 0xAD, 0xAD, 0xAD, 0xAD, 0x0D, 0xA5, 0x0D,
			0x10, 0xAF, 0xAE, 0xAE, 0xA8, 0xA8, 0xA8, 0xA7, 0xA7, 0xAF, 0xAF, 0xAF, 0xB0, 0x1D, 0xA7, 0x0D,
			0xAC, 0xB1, 0xB0, 0xB0, 0xA9, 0xAA, 0xAB, 0xAB, 0xAC, 0xB2, 0xB2, 0xB2, 0xB3, 0x2D, 0xAA, 0x0D,
			0x30, 0xB4, 0xB3, 0xB1, 0xAC, 0xAC, 0xAC, 0xAC, 0xAC, 0xB3, 0xB2, 0xB3, 0xB3, 0x3D, 0x3E, 0x0D,
			0x0D, 0xAD, 0xAF, 0xAF, 0xA6, 0xA6, 0xA7, 0x47, 0x48, 0xAE, 0xB3, 0xB4, 0xA8, 0xA9, 0xAC, 0x30,
			0x0D, 0xAD, 0xB0, 0xB1, 0xA7, 0xA6, 0xA7, 0x57, 0x58, 0xAF, 0xB3, 0xB4, 0xA8, 0xA9, 0xAC, 0x30
	};

	switch(prefs::palette)
	{
		case 0: return colour;
		case 1: return static_cast<Colour>(colour_table_cga[static_cast<unsigned int>(colour)]);
		case 2: return static_cast<Colour>(colour_table_nes[static_cast<unsigned int>(colour)]);
		case 3: return static_cast<Colour>(colour_table_cblinda[static_cast<unsigned int>(colour)]);
		case 4: return static_cast<Colour>(colour_table_cblindb[static_cast<unsigned int>(colour)]);
		default: return Colour::ERROR_COLOUR;
	}
}

// Prints a string in the Alagard font at the specified coordinates.
void alagard_print(string message, int x, int y, Colour colour, unsigned int flags)
{
	STACK_TRACE();
	for (unsigned int i = 0; i < message.size(); i++)
		alagard_print_at(message.at(i), x + (i * 3), y, colour, flags);
}

// Prints an Alagard font character at the specified coordinates.
void alagard_print_at(char letter, int x, int y, Colour colour, unsigned int flags)
{
	if (letter == ' ' || letter == '_') return;
	if (letter >= 'A' && letter <= 'Z') letter -= 65;
	else if (letter == '/') letter = 27;
	else if (letter == '.') letter = 28;
	else letter = 26;
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;
	const bool minus_eight_y = (flags & ALAGARD_FLAG_MINUS_EIGHT_Y) == ALAGARD_FLAG_MINUS_EIGHT_Y;
	terminal_color(rgb_string(colour).c_str());
	terminal_put(x, y - (minus_eight_y ? 8 : 0), 0x2000 + letter);
}

// Prints an ANSI string at the specified position.
void ansi_print(string msg, int x, int y, unsigned int print_flags, unsigned int dim)
{
	STACK_TRACE();

	msg = msg + "{10}";
	Colour colour = Colour::CGA_LGRAY;
	unsigned char r, g, b;
	int offset = 0;
	string::size_type pos = 0;
	string code;

	do
	{
		parse_colour(colour, r, g, b);
		if (dim)
		{
			float dim_m = 1.0f - static_cast<float>(dim) / 8.0f;
			r = round(static_cast<float>(r) * dim_m);
			g = round(static_cast<float>(g) * dim_m);
			b = round(static_cast<float>(b) * dim_m);
		}
		pos = msg.find((string)"{");
		if (pos != string::npos)
		{
			if (pos > 0)
			{
				offset += print(msg.substr(0, pos), x + offset, y, r, g, b, print_flags);
				msg = msg.substr(pos);
				x += pos;
			}
			code = msg.substr(1, 2);
			msg = msg.substr(4, msg.length() - 4);
			colour = static_cast<Colour>(strx::htoi(code));
		}
	} while(msg.size());
}

// Renders an ASCII box at the given coordinates.
void box(int x, int y, int w, int h, Colour colour, unsigned char flags, string title)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::WHITE;
	const bool dline = ((flags & BOX_FLAG_DOUBLE) == BOX_FLAG_DOUBLE);
	unsigned int print_flags = 0;
	if ((flags & BOX_FLAG_ALPHA) == BOX_FLAG_ALPHA) print_flags = PRINT_FLAG_ALPHA;
	else
	{
		unsigned int current_layer = get_layer();
		for (unsigned int i = 0; i <= 255; i++)
		{
			layer(i);
			clear(x, y, w, h);
		}
		layer(current_layer);
	}
	if ((flags & BOX_FLAG_OUTER_BORDER) == BOX_FLAG_OUTER_BORDER) rect(x - 1, y - 1, w + 2, h + 2, Colour::CGA_BLACK);


	for (int i = x; i < x + w; i++)
	{
		for (int j = y; j < y + h; j++)
		{
			Glyph glyph = static_cast<Glyph>(' ');
			if (i == x && j == y) glyph = (dline ? Glyph::LINE_DDRR : Glyph::LINE_DR);
			else if (i == x + w - 1 && j == y) glyph = (dline ? Glyph::LINE_DDLL : Glyph::LINE_DL);
			else if (i == x && j == y + h - 1) glyph = (dline ? Glyph::LINE_UURR : Glyph::LINE_UR);
			else if (i == x + w - 1 && j == y + h - 1) glyph = (dline ? Glyph::LINE_UULL : Glyph::LINE_UL);
			else if (j == y || j == y + h - 1) glyph = (dline ? Glyph::LINE_HH : Glyph::LINE_H);
			else if (i == x || i == x + w - 1) glyph = (dline ? Glyph::LINE_VV : Glyph::LINE_V);
			print_at(glyph, i, j, colour, print_flags);
		}
	}

	if (!title.size()) return;
	title = " " + title + " ";
	const unsigned int title_len = strx::ansi_strlen(title);
	const unsigned int title_x = (w / 2) - (title_len / 2) + x;
	ansi_print(title, title_x, y);
	print_at(Glyph::LINE_VL, title_x - 1, y, colour);
	print_at(Glyph::LINE_VR, title_x + title_len, y, colour);
}

// Clears a specified area of the current layer.
void clear(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	STACK_TRACE();
	terminal_clear_area(x, y, w, h);
}

// Clears the screen.
void cls()
{
	STACK_TRACE();
	terminal_layer(0);
	terminal_clear();
}

// Waits for a few milliseconds.
void delay(unsigned int ms)
{
	STACK_TRACE();
	terminal_delay(ms);
}

// Checks if the player clicked in a specified area.
bool did_mouse_click(unsigned short x, unsigned short y, unsigned short w, unsigned short h)
{
	if (mouse_clicked_x >= x && mouse_clicked_y >= y && mouse_clicked_x <= x + w - 1 && mouse_clicked_y <= y + h - 1)
	{
		mouse_clicked_x = mouse_clicked_y = 0;
		return true;
	}
	return false;
}

// This is where we clean up our shit.
void exit_functions()
{
	STACK_TRACE();
	if (cleaned_up) return;
	cleaned_up = true;
	guru::log("Running cleanup at level " + strx::itos(exit_func_level) + ".", GURU_INFO);

	if (exit_func_level >= 2)
	{
		terminal_close();
		guru::console_ready(false);
	}
	exit_func_level = 0;
}

// Redraws the display.
void flip()
{
	STACK_TRACE();
	terminal_refresh();
}

// Returns the number of columns on the screen.
unsigned short get_cols()
{
	return cols;
}

// Gets the current render layer.
unsigned char get_layer()
{
	STACK_TRACE();
	return terminal_state(TK_LAYER);
}

// Returns the number of rows on the screen.
unsigned short get_rows()
{
	return rows;
}

// Initializes the main terminal window.
void init()
{
	STACK_TRACE();
	guru::log("Duskfall v" + DUSKFALL_VERSION_STRING + " [build " + strx::itos(build_version()) + "]", GURU_STACK);
	guru::log("Main program entry. Let's do this.", GURU_INFO);
	exit_func_level = 1;

	if (!terminal_open()) guru::halt("Could not initialize terminal window.");
	exit_func_level = 2;

	screen_x = prefs::screen_x;
	screen_y = prefs::screen_y;
	const bool fullscreen = prefs::fullscreen;
	string window_title = "Duskfall " + DUSKFALL_VERSION_STRING + " [build " + strx::itos(build_version()) + "]";
	if (!terminal_set(("window: title = '" + window_title + "', size=" + strx::itos(screen_x / 16) + "x" + strx::itos(screen_y / 16) + ", resizeable=true, fullscreen=" + (fullscreen ? "true" : "false")).c_str()))
		guru::halt("Could not set up terminal window properties.");
	if (!terminal_set("input: filter='keyboard, mouse'")) guru::halt("Could not set input filters.");
	if (!terminal_set("U+0000: data/png/victoria.png, size=16x16")
			|| !terminal_set("U+1000: data/png/artos-serif.png, size=16x16")
			|| !terminal_set("U+2000: data/png/alagard.png, size=48x52, align=top-left")
			|| !terminal_set("U+3000: data/png/sprites.png, size=64x64, align=top-left")
			) guru::halt("Could not load bitmap fonts.");
	recalc_screen_size();
	exit_func_level = 3;
	guru::console_ready(true);
}

// Returns true if the key is a chosen 'cancel' key.
bool is_cancel(unsigned int key)
{
	STACK_TRACE();
	if (key == prefs::keybind(MENU_CANCEL) || key == KEY_ESCAPE) return true;
	return false;
}

// Returns true if the key is a chosen 'down' key.
bool is_down(unsigned int key)
{
	STACK_TRACE();
	if (key == KEY_DOWN || key == KEY_KP_2 || key == prefs::keybind(SOUTH)) return true;
	return false;
}

// Returns true if the key is a chosen 'left' key.
bool is_left(unsigned int key)
{
	STACK_TRACE();
	if (key == KEY_LEFT || key == KEY_KP_4 || key == prefs::keybind(WEST)) return true;
	return false;
}

// Returns true if the key is a chosen 'right' key.
bool is_right(unsigned int key)
{
	STACK_TRACE();
	if (key == KEY_RIGHT || key == KEY_KP_6 || key == prefs::keybind(EAST)) return true;
	return false;
}

// Returns true if the key is a chosen 'select' key.
bool is_select(unsigned int key)
{
	STACK_TRACE();
	if (key == prefs::keybind(Keys::MENU_OK) || key == prefs::keybind(Keys::MENU_OK_2)) return true;
	return false;
}

// Returns true if the key is a chosen 'up' key.
bool is_up(unsigned int key)
{
	STACK_TRACE();
	if (key == KEY_UP || key == KEY_KP_8 || key == prefs::keybind(NORTH)) return true;
	return false;
}

// Returns the name of a key.
string key_to_name(unsigned int key)
{
	STACK_TRACE();

	switch(key)
	{
		case 0: return "{5C}[unbound]";
		case ' ': return "Space Bar";
		case KEY_BACKSPACE: return "Backspace";
		case KEY_BREAK: return "Break";
		case KEY_DOWN: return "Arrow Down";
		case KEY_END: return "End";
		case KEY_ENTER: return "Enter";
		case KEY_ESCAPE: return "Escape";
		case KEY_HOME: return "Home";
		case KEY_INSERT: return "Insert";
		case KEY_KP_DIVIDE: return "Keypad /";
		case KEY_KP_ENTER: return "Keypad Enter";
		case KEY_KP_MINUS: return "Keypad -";
		case KEY_KP_MULTIPLY: return "Keypad *";
		case KEY_KP_PERIOD: return "Keypad .";
		case KEY_KP_PLUS: return "Keypad +";
		case KEY_LEFT: return "Arrow Left";
		case KEY_PAGEDOWN: return "Page Down";
		case KEY_PAGEUP: return "Page Up";
		case KEY_RIGHT: return "Arrow Right";
		case KEY_TAB: return "Tab";
		case KEY_UP: return "Arrow Up";
		case LMB_KEY: return "Left Mouse Button";
		case MOUSEWHEEL_DOWN_KEY: return "Mousewheel Up";
		case MOUSEWHEEL_UP_KEY: return "Mousewheel Up";
		case RMB_KEY: return "Right Mouse Button";
	}
	if (key >= KEY_F1 && key <= KEY_F12) return "F" + strx::itos(key - KEY_F1 + 1);
	else if (key >= KEY_KP_0 && key <= KEY_KP_9) return "Keypad " + strx::itos(key - KEY_KP_0);
	else if ((key >= '!' && key <= '@') || key == '`') return string(1, static_cast<char>(key));
	else if (key >= 'a' && key <= 'z') return string(1, static_cast<char>(key - 32));
	else if (key >= 'A' && key <= 'Z') return "Shift-" +string(1, static_cast<char>(key));
	else if (key >= 1000 && key < 10000) return "Shift-" + key_to_name(key - 1000);
	else if (key >= 10000 && key < 11000) return "Ctrl-" + key_to_name(key - 10000);
	else if (key >= 11000 && key < 20000) return "Ctrl-Shift-" + key_to_name(key - 11000);
	else if (key >= 100000 && key < 101000) return "Alt-" + key_to_name(key - 100000);
	else if (key >= 101000 && key < 110000) return "Shift-Alt-" + key_to_name(key - 101000);
	else if (key >= 110000 && key < 111000) return "Ctrl-Alt-" + key_to_name(key - 110000);
	else if (key >= 111000) return "Shift-Ctrl-Alt-" + key_to_name(key - 111000);
	else return "[unknown]";
}

// Select the rendering layer.
void layer(unsigned char new_layer)
{
	STACK_TRACE();
	terminal_layer(new_layer);
}

// Retrieves the middle column on the screen.
unsigned short midcol()
{
	return mid_col;
}

// Retrieves the middle row on the screen.
unsigned short midrow()
{
	return mid_row;
}

s_rgb nebula(int x, int y)
{
	STACK_TRACE();
	const string coord = strx::itos(x) + "," + strx::itos(y);
	if (nebula_cache.find(coord) != nebula_cache.end()) return nebula_cache.at(coord);

	const unsigned char value = mathx::perlin_rgb(x + mathx::prand_seed, y + mathx::prand_seed, 32.0, 0.5, 8);

	// Save a copy of the PRNG seed.
	const long prand_copy = mathx::prand_seed;

	s_rgb colour;
	colour.r = nebula_rgb(value, mathx::prand(4)) / 2;
	colour.g = nebula_rgb(value, mathx::prand(4)) / 2;
	colour.b = nebula_rgb(value, mathx::prand(4)) / 2;

	// Set the PRNG seed back again.
	mathx::prand_seed = prand_copy;

	nebula_cache.insert(std::pair<string, s_rgb>(coord, colour));
	return colour;
}

// Modifies an RGB value in the specified manner, used for rendering nebulae.
unsigned char nebula_rgb(unsigned char value, int modifier)
{
	STACK_TRACE();
	switch(modifier)
	{
		case 1: value /= 2; break;
		case 2: value = pow(value, 3) / 65025; break;
		case 3: value = pow(value, 2) / 255; break;
		case 4: value /= 5; break;
	}
	return value;
}

// Renders an OK box on a pop-up window.
void ok_box(int offset, Colour colour)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;

	box(mid_col - 1, mid_row + offset - 1, 3, 3, colour);
	print_at(static_cast<Glyph>(' '), mid_col, mid_row + offset - 1, colour);
	print_at(static_cast<Glyph>(' '), mid_col, mid_row + offset + 1, colour);
	print_at(Glyph::RETURN, mid_col, mid_row + offset, Colour::CGA_LGREEN);
	print_at(Glyph::LINE_VL, mid_col - 1, mid_row + offset, colour);
	print_at(Glyph::LINE_VR, mid_col + 1, mid_row + offset, colour);
}

// Parses a colour code into RGB.
void parse_colour(Colour colour, unsigned char &r, unsigned char &g, unsigned char &b)
{
	STACK_TRACE();
	static const unsigned char colour_table[(MAX_COLOUR + EXTRA_NES_COLOURS + EXTRA_COLOURBLIND_COLOURS + 1) * 3] = { 109,109,109, 0,44,150, 0,10,161, 50,0,132, 145,0,74, 188,0,26, 173,2,0, 122,13,0, 72,32,0, 8,51,0,
			2,55,0, 0,53,31, 0,47,82, 0,0,0, 89,58,34, 0,0,0, 188,188,188, 0,100,255, 20,66,255, 111,38,249, 231,32,167, 255,27,61, 255,21,0,
			205,34,0, 184,78,0, 37,109,0, 2,125,0, 0,119,66, 0,136,194, 20,20,20, 131,99,74, 0,0,0, 255,235,17, 8,207,255, 85,146,255, 203,109,255,
			255,51,240, 255,77,121, 255,117,35, 255,139,10, 249,175,20, 142,221,7, 28,237,37, 6,237,148, 2,250,255, 74,74,74, 184,161,143, 0,0,0,
			255,255,255, 150,251,255, 165,232,255, 210,156,231, 255,152,248, 255,156,165, 255,201,161, 255,235,150, 255,245,139, 207,227,131,
			150,233,160, 146,239,210, 136,255,251, 214,214,214, 40,40,40, 0,0,0,

			0,0,0, 0,0,128, 0,128,0, 0,128,128, 128,0,0, 128,0,128, 128,128,0, 192,192,192, 128,128,128, 0,0,255, 0,255,0, 0,255,255, 255,0,0, 255,0,255, 255,255,0, 255,255,255,
			0,0,0, 0,0,170, 0,170,0, 0,170,170, 170,0,0, 170,0,170, 170,85,0, 170,170,170, 85,85,85, 85,85,255, 85,255,85, 85,255,255, 255,85,85, 255,85,255, 255,255,85, 255,255,255,

			124,124,124, 0,0,252, 0,0,188, 68,40,188, 148,0,132, 168,0,32, 168,16,0, 136,20,0, 80,48,0, 0,120,0, 0,104,0, 0,88,0, 0,64,88,
			188,188,188, 0,120,248, 0,88,248, 104,68,252, 216,0,204, 228,0,88, 248,56,0, 228,92,16, 172,124,0, 0,184,0, 0,168,0, 0,168,68, 0,136,136,
			248,248,248, 60,188,252, 104,136,252, 152,120,248, 248,120,248, 248,88,152, 248,120,88, 252,160,68, 248,184,0, 184,248,24, 88,216,84, 88,248,152, 0,232,216, 120,120,120,
			164,228,252, 184,184,248, 216,184,248, 248,184,248, 248,164,192, 240,208,176, 252,224,168, 248,216,120, 216,248,120, 184,248,184, 184,248,216, 0,252,252, 248,216,248,

			70,62,15, 106,95,0, 125,111,18, 191,170,23, 213,190,25, 224,199,0, 255,232,122, 255,242,192, 0,44,93, 0,55,116, 0,90,189, 0,108,228, 72,123,232, 99,146,255, 155,174,244, 217,219,247,
			73,30,32, 122,13,8, 172,16,0, 253,24,0, 255,85,89, 239,100,107, 241,124,132, 246,167,179, 0,43,45, 0,86,89, 0,124,131, 29,163,176, 0,210,227, 85,225,244, 135,241,255, 193,244,255 };

	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;

	if (colour == Colour::ERROR_COLOUR)
	{
		unsigned long long seconds = time(NULL);
		if (seconds % 2) { r = 255; g = 235; b = 17; }
		else { r = 0; g = 100; b = 255; }
		return;
	}

	colour = adjust_palette(colour);

	r = colour_table[static_cast<unsigned int>(colour) * 3];
	g = colour_table[(static_cast<unsigned int>(colour) * 3) + 1];
	b = colour_table[(static_cast<unsigned int>(colour) * 3) + 2];
}

// Prints a message at the specified coordinates, in RGB colours.
int print(string message, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags)
{
	STACK_TRACE();
	int offset = 0;
	for (unsigned int i = 0; i < message.size(); i++)
	{
		// Check for special character tokens.
		if (message.at(i) == '^')
		{
			if (message.size() > i + 4)
			{
				if (message.at(i + 4) == '^')
				{
					const Glyph code = static_cast<Glyph>(atoi(message.substr(i + 1, 3).c_str()));
					if (static_cast<int>(code))	// Do not print ^000^ code.
					{
						print_at(code, x + i + offset, y, r, g, b, print_flags);
						offset -= 4;
					}
					else offset -= 5;
					i += 4;
				}
			}
		}
		else print_at(static_cast<Glyph>(message.at(i)), x + i + offset, y, r, g, b, print_flags);
	}
	return offset;
}

// Prints a message at the specified coordinates.
int print(string message, int x, int y, Colour colour, unsigned int print_flags)
{
	STACK_TRACE();
	unsigned char r, g, b;
	parse_colour(colour, r, g, b);
	return print(message, x, y, r, g, b, print_flags);
}

// Prints a character at a given coordinate on the screen, in specific RGB colours.
void print_at(Glyph letter, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags)
{
	STACK_TRACE();
	// Special cases for faux glyphs, which use other glyphs to draw special symbols.
	if (letter > static_cast<Glyph>(256) || letter == static_cast<Glyph>(0))
	{
		if (letter == static_cast<Glyph>(257))
		{
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags | PRINT_FLAG_PLUS_EIGHT_Y_B);
			return;
		}
		else if (letter == static_cast<Glyph>(258))
		{
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags);
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags | PRINT_FLAG_PLUS_EIGHT_Y_B | PRINT_FLAG_ALPHA);
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags | PRINT_FLAG_PLUS_EIGHT_X_B | PRINT_FLAG_PLUS_EIGHT_Y_B  | PRINT_FLAG_ALPHA);
			return;
		}
		else if (letter == static_cast<Glyph>(259))
		{
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags | PRINT_FLAG_PLUS_EIGHT_X_B | PRINT_FLAG_PLUS_EIGHT_Y_B  | PRINT_FLAG_ALPHA);
			return;
		}
		else if (letter == static_cast<Glyph>(260))
		{
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags);
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags | PRINT_FLAG_PLUS_EIGHT_X_B | PRINT_FLAG_ALPHA);
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags | PRINT_FLAG_PLUS_EIGHT_X_B | PRINT_FLAG_PLUS_EIGHT_Y_B | PRINT_FLAG_ALPHA);
			return;
		}
		else if (letter == static_cast<Glyph>(261))
		{
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags);
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags | PRINT_FLAG_PLUS_EIGHT_X_B | PRINT_FLAG_ALPHA);
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags | PRINT_FLAG_PLUS_EIGHT_Y_B | PRINT_FLAG_ALPHA);
			return;
		}
		else if (letter == static_cast<Glyph>(262))
		{
			print_at(Glyph::CORNER_BLOCK, x, y, r, g, b, print_flags | PRINT_FLAG_PLUS_EIGHT_X_B | PRINT_FLAG_ALPHA);
			return;
		}
		else guru::log("Attempt to print invalid glyph: " + strx::itos(static_cast<unsigned int>(letter)), GURU_WARN);
	}

	const bool no_nbsp = (print_flags & PRINT_FLAG_NO_NBSP) == PRINT_FLAG_NO_NBSP;
	if (!no_nbsp && letter == static_cast<Glyph>('`')) letter = static_cast<Glyph>(' ');
	const bool alpha = (print_flags & PRINT_FLAG_ALPHA) == PRINT_FLAG_ALPHA;
	const bool alpha_state = terminal_state(TK_COMPOSITION);
	if (alpha != alpha_state) terminal_composition(alpha);
	const bool sans = (print_flags & PRINT_FLAG_SANS) == PRINT_FLAG_SANS;
	if (sans) letter = static_cast<Glyph>(static_cast<unsigned int>(letter) + 0x1000);
	const bool plus_four_x = (print_flags & PRINT_FLAG_PLUS_FOUR_X) == PRINT_FLAG_PLUS_FOUR_X;
	const bool plus_four_y = (print_flags & PRINT_FLAG_PLUS_FOUR_Y) == PRINT_FLAG_PLUS_FOUR_Y;
	const bool plus_eight_x = (print_flags & PRINT_FLAG_PLUS_EIGHT_X) == PRINT_FLAG_PLUS_EIGHT_X;
	const bool plus_eight_y = (print_flags & PRINT_FLAG_PLUS_EIGHT_Y) == PRINT_FLAG_PLUS_EIGHT_Y;
	const bool plus_eight_x_b = (print_flags & PRINT_FLAG_PLUS_EIGHT_X_B) == PRINT_FLAG_PLUS_EIGHT_X_B;
	const bool plus_eight_y_b = (print_flags & PRINT_FLAG_PLUS_EIGHT_Y_B) == PRINT_FLAG_PLUS_EIGHT_Y_B;
	int off_x = 0, off_y = 0;
	if (plus_four_x) off_x += 4;
	if (plus_eight_x) off_x += 8;
	if (plus_eight_x_b) off_x += 8;
	if (plus_four_y) off_y += 4;
	if (plus_eight_y) off_y += 8;
	if (plus_eight_y_b) off_y += 8;
	terminal_color(rgb_string(r, g, b).c_str());
	if (!off_x && !off_y) terminal_put(x, y, static_cast<unsigned int>(letter));
	else terminal_put_ext(x, y, off_x, off_y, static_cast<unsigned int>(letter));
}

// As above, but given a char rather than a Glyph.
void print_at(char letter, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags)
{
	print_at(static_cast<Glyph>(letter), x, y, r, g, b, print_flags);
}

// Prints a character at a given coordinate on the screen.
void print_at(Glyph letter, int x, int y, Colour colour, unsigned int print_flags)
{
	STACK_TRACE();
	if ((print_flags & PRINT_FLAG_NO_SPACES) == PRINT_FLAG_NO_SPACES && letter == static_cast<Glyph>(' ')) return;
	unsigned char r, g, b;
	parse_colour(colour, r, g, b);
	print_at(letter, x, y, r, g, b, print_flags);
}

// As above, but given a char rather than a glyph.
void print_at(char letter, int x, int y, Colour colour, unsigned int print_flags)
{
	print_at(static_cast<Glyph>(letter), x, y, colour, print_flags);
}

// Recalculates the screen size variables.
void recalc_screen_size()
{
	STACK_TRACE();
	cols = terminal_state(TK_WIDTH);
	rows = terminal_state(TK_HEIGHT);
	screen_x = cols * 16;
	screen_y = rows * 16;
	mid_col = round(cols / 2.0f);
	mid_row = round(rows / 2.0f) - 1;
}

// Draws a coloured rectangle
void rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, Colour colour)
{
	STACK_TRACE();
	unsigned char r, g, b;
	parse_colour(colour, r, g, b);
	rect(x, y, w, h, r, g, b);
}

// As above, with RGB colour.
void rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char r, unsigned char g, unsigned char b)
{
	STACK_TRACE();
	if (get_layer() == 0)
	{
		terminal_bkcolor(rgb_string(r, g, b).c_str());
		terminal_clear_area(x, y, w, h);
		terminal_bkcolor(0U);
	}
	else
	{
		for (unsigned int cx = x; cx < x + w; cx++)
			for (unsigned int cy = y; cy < y + h; cy++)
				print_at(Glyph::BLOCK_SOLID, cx, cy, r, g, b);
	}
}

// Renders a nebula on the screen.
void render_nebula(unsigned short seed, int off_x, int off_y)
{
	STACK_TRACE();
	// If the currently-cached nebula is different from the one being requested, rebuild it.
	if (seed != nebula_cache_seed)
	{
		nebula_cache_seed = seed;
		nebula_cache.clear();
	}
	mathx::prand_seed = seed;
	for (int x = 0; x < cols + 1; x++)
	{
		for (int y = 0; y < rows + 1; y++)
		{
			s_rgb rgb = nebula(x + off_x, y + off_y);
			rect(x, y, 1, 1, rgb.r, rgb.g, rgb.b);
		}
	}
}

// Creates a BLT RGB string from RGB integers.
string rgb_string(unsigned char r, unsigned char g, unsigned char b)
{
	STACK_TRACE();
	return strx::itos(r) + "," + strx::itos(g) + "," + strx::itos(b);
}

// As above, but for a Colour integer.
string rgb_string(Colour colour)
{
	STACK_TRACE();
	unsigned char r, g, b;
	parse_colour(colour, r, g, b);
	return rgb_string(r, g, b);
}

// Prints a sprite at the given location.
void sprite_print(Sprite id, int x, int y, Colour colour)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;
	terminal_color(rgb_string(colour).c_str());
	terminal_put(x, y, 0x3000 + static_cast<unsigned int>(id));
}

// Polls SDL until a key is pressed. If a time is specified, it will abort after this time.
unsigned int wait_for_key(unsigned short max_ms)
{
	STACK_TRACE();
	const std::unordered_map<unsigned int, unsigned int> key_translation_table = { { TK_SPACE, ' ' }, { TK_MINUS, '-' }, { TK_EQUALS, '=' }, { TK_LBRACKET, '[' }, { TK_RBRACKET, ']' }, { TK_BACKSLASH, '\\' }, { TK_SEMICOLON, ';' },
		{ TK_APOSTROPHE, '\'' }, { TK_GRAVE, '`' }, { TK_COMMA, ',' }, { TK_PERIOD, '.' }, { TK_SLASH, '/' }, { TK_F1, KEY_F1 }, { TK_F2, KEY_F2 }, { TK_F3, KEY_F3 }, { TK_F4, KEY_F4 }, { TK_F5, KEY_F5 }, { TK_F6, KEY_F6 },
		{ TK_F7, KEY_F7 }, { TK_F8, KEY_F8 }, { TK_F9, KEY_F9 }, { TK_F10, KEY_F10 }, { TK_F11, KEY_F11 }, { TK_F12, KEY_F12 }, { TK_RETURN, KEY_ENTER }, { TK_ESCAPE, KEY_ESCAPE }, { TK_BACKSPACE, KEY_BACKSPACE },
		{ TK_TAB, KEY_TAB }, { TK_PAUSE, KEY_BREAK }, { TK_INSERT, KEY_INSERT }, { TK_HOME, KEY_HOME }, { TK_PAGEUP, KEY_PAGEUP }, { TK_DELETE, 0x7F }, { TK_END, KEY_END }, { TK_PAGEDOWN, KEY_PAGEDOWN }, { TK_RIGHT, KEY_RIGHT },
		{ TK_LEFT, KEY_LEFT }, { TK_UP, KEY_UP }, { TK_DOWN, KEY_DOWN }, { TK_KP_0, KEY_KP_0 }, { TK_KP_1, KEY_KP_1 }, { TK_KP_2, KEY_KP_2 }, { TK_KP_3, KEY_KP_3 }, { TK_KP_4, KEY_KP_4 }, { TK_KP_5, KEY_KP_5 },
		{ TK_KP_6, KEY_KP_6 }, { TK_KP_7, KEY_KP_7 }, { TK_KP_8, KEY_KP_8 }, { TK_KP_9, KEY_KP_9 }, { TK_KP_DIVIDE, KEY_KP_DIVIDE }, { TK_KP_MULTIPLY, KEY_KP_MULTIPLY }, { TK_KP_PLUS, KEY_KP_PLUS }, { TK_KP_MINUS, KEY_KP_MINUS },
		{ TK_KP_PERIOD, KEY_KP_PERIOD }, { TK_KP_ENTER, KEY_KP_ENTER } };

	auto timer = std::chrono::system_clock::now();
	unsigned int millis = 0;
	do
	{
		if (terminal_has_input())
		{
			unsigned int key = terminal_read();
			bool process_shift_ctrl_alt = false;

			if (key == TK_CLOSE || (key == TK_F4 && terminal_state(TK_ALT)))
			{
				exit_functions();
				guru::close_syslog();
				exit(0);
			}
			else if (key == TK_RESIZED)
			{
				recalc_screen_size();
				return RESIZE_KEY;
			}
			else if ((key >= TK_A && key <= TK_Z) || (key >= TK_1 && key <= TK_0))
			{
				if (key >= TK_1 && key <= TK_9) key = key - TK_1 + '1';
				else if (key == TK_0) key = '0';
				else if (key >= TK_A && key <= TK_Z) key = key - TK_A + 'a';
				process_shift_ctrl_alt = true;
			}
			else if (key == TK_MOUSE_LEFT || key == TK_MOUSE_RIGHT)
			{
				mouse_clicked_x = terminal_state(TK_MOUSE_X);
				mouse_clicked_y = terminal_state(TK_MOUSE_Y);
				if (key == TK_MOUSE_LEFT) return LMB_KEY;
				else return RMB_KEY;
			}
			else if (key == TK_MOUSE_SCROLL)
			{
				if (terminal_state(TK_MOUSE_WHEEL) > 0) return MOUSEWHEEL_UP_KEY;
				else return MOUSEWHEEL_DOWN_KEY;
			}
			else
			{
				auto found = key_translation_table.find(key);
				if (found != key_translation_table.end())
				{
					key = found->second;
					process_shift_ctrl_alt = true;
				}
			}

			if (process_shift_ctrl_alt)
			{
				const bool shift = terminal_state(TK_SHIFT);
				const bool ctrl = terminal_state(TK_CONTROL);
				const bool alt = terminal_state(TK_ALT);
				if (shift)
				{
					if (key >= 'a' && key <= 'z' && !alt && !ctrl) key -= 32;
					else key += 1000;
				}
				if (ctrl) key += 10000;
				if (alt) key += 100000;
				return key;
			}
		}
		delay(10);
		millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timer).count();
	} while(millis < max_ms || !max_ms);
	return 0;
}

// Renders a yes/no popup box and returns the result.
bool yes_no_query(string yn_strings, string title, Colour title_colour, unsigned int flags)
{
	STACK_TRACE();
	if (static_cast<int>(title_colour) > MAX_COLOUR) title_colour = Colour::ERROR_COLOUR;
	const bool ansi = ((flags & YES_NO_FLAG_ANSI) == YES_NO_FLAG_ANSI);
	vector<string> message = strx::string_explode(yn_strings, "|");
	unsigned int width = 0, height = 4 + message.size();
	for (auto str : message)
	{
		const unsigned int len = strx::ansi_strlen(str);
		if (len > width) width = len;
	}
	width += 4;
	const unsigned int box_start_x = mid_col - (width / 2);
	const unsigned int box_start_y = mid_row - (height / 2);
	box(box_start_x, box_start_y, width, height, UI_COLOUR_BOX);

	title = " " + title + " ";
	const unsigned int title_start = mid_col - (title.size() / 2);
	print(title, title_start, box_start_y, title_colour);
	print_at(Glyph::LINE_VL, title_start - 1, box_start_y, UI_COLOUR_BOX);
	print_at(Glyph::LINE_VR, title_start + title.size(), box_start_y, UI_COLOUR_BOX);

	for (unsigned int i = 0; i < message.size(); i++)
	{
		const string line = message.at(i);
		if (ansi) ansi_print(line, mid_col - (strx::ansi_strlen(line) / 2), box_start_y + 2 + i);
		else print(line, mid_col - (strx::ansi_strlen(line) / 2), box_start_y + 2 + i, Colour::CGA_WHITE, PRINT_FLAG_SANS);
	}

	const unsigned int bottom_row = box_start_y + height - 1;

	box(mid_col - 4, bottom_row - 1, 3, 3, UI_COLOUR_BOX);

	print_at(static_cast<Glyph>(' '), mid_col - 3, bottom_row - 1, UI_COLOUR_BOX);
	print_at(static_cast<Glyph>(' '), mid_col - 3, bottom_row + 1, UI_COLOUR_BOX);
	print_at(static_cast<Glyph>('Y'), mid_col - 3, bottom_row, Colour::CGA_LGREEN);
	print_at(Glyph::LINE_VL, mid_col - 4, bottom_row, UI_COLOUR_BOX);
	print_at(Glyph::LINE_VR, mid_col - 2, bottom_row, UI_COLOUR_BOX);

	box(mid_col + 2, bottom_row - 1, 3, 3, UI_COLOUR_BOX);
	print_at(static_cast<Glyph>(' '), mid_col + 3, bottom_row - 1, UI_COLOUR_BOX);
	print_at(static_cast<Glyph>('N'), mid_col + 3, bottom_row, Colour::CGA_LRED);
	print_at(static_cast<Glyph>(' '), mid_col + 3, bottom_row + 1, UI_COLOUR_BOX);
	print_at(Glyph::LINE_VL, mid_col + 2, bottom_row, UI_COLOUR_BOX);
	print_at(Glyph::LINE_VR, mid_col + 4, bottom_row, UI_COLOUR_BOX);

	do
	{
		flip();
		unsigned int key = wait_for_key();
		if (key == RESIZE_KEY) return false;
		if (key == LMB_KEY)
		{
			if (did_mouse_click(mid_col - 3, bottom_row)) return true;
			if (did_mouse_click(mid_col + 3, bottom_row)) return false;
		}
		if (key == 'Y' || key == 'y' || is_select(key)) return true;
		else if (key != LMB_KEY) return false;
	} while(true);
}

}	// namespace iocore
