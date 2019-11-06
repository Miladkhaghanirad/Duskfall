// iocore.cpp -- The render core, handling display and user interaction, as well as program initialization, shutdown and cleanup functionality.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "filex.h"
#include "guru.h"
#include "iocore.h"
#include "mathx.h"
#include "prefs.h"
#include "strx.h"
#include "version.h"

#include "jsoncpp/json/json.h"
#include "lodepng/bmp2png.h"
#include "sdl_savejpeg/SDL_savejpeg.h"
#include "sdl2/SDL.h"
#include "sdl2/SDL_image.h"
#include "snes_ntsc/snes_ntsc.h"

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <unordered_map>


#define	SCREEN_MIN_X		1024	// Minimum X-resolution of the screen. Should not be lower than 1024.
#define SCREEN_MIN_Y		600		// Minimum Y-resolution of the screen. Should not be lower than 600.
#define SCREEN_MAX_X		4080	// Maximum X-resolution.
#define SCREEN_MAX_Y		4080	// Maximum Y-resolution.
#define SDL_RETRIES			100		// Amount of times to try re-acquiring the SDL window surface before giving up.
#define GLITCH_CHANCE		200		// The lower this number, the more often visual glitches occur.
#define NTSC_GLITCH_CHANCE	500		// The lower this number, the more often NTSC mode glitches occur.
#define NTSC_RESET_CHANCE	50		// The lower this number, the faster NTSC glitches go back to normal.
#define FOLDER_SCREENS		"userdata/screenshots"


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


namespace iocore
{

// Struct definitions
struct s_glitch
{
	unsigned int x, y, w, h;
	int off_x, off_y;
	bool black;
	SDL_Surface *surf;
};

SDL_Surface		*alagard = nullptr;		// The texture for the large bitmap font.
bool			current_animation_frame = false;	// This toggles on and off for two-frame animation.
bool			cleaned_up = false;		// Have we run the exit functions already?
unsigned short	cols = 0, rows = 0, mid_col = 0, mid_row = 0, narrow_cols = 0, mid_col_narrow = 0, tile_cols = 0, tile_rows = 0;	// The number of columns and rows available, and the middle column/row.
unsigned char	exit_func_level = 0;	// Keep track of what to clean up at exit.
SDL_Surface		*font = nullptr;		// The bitmap font texture.
SDL_Surface		*font_narrow = nullptr;	// The texture for the narrow bitmap font.
unsigned short	font_sheet_size = 0;	// The size of the font texture sheet, in glyphs.
unsigned short	font_sheet_size_narrow = 0;	// As above, for the narrow font.
unsigned int 	glitch_clear_countdown = 0;
SDL_Surface		*glitch_hz_surface = nullptr;	// Horizontal glitch surface.
unsigned char	glitch_multi = 0;		// Glitch intensity multiplier.
SDL_Surface		*glitch_sq_surface = nullptr;	// Square glitch surface.
std::vector<s_glitch>	glitch_vec;
SDL_Surface 	*glitched_main_surface = nullptr;	// A glitched version of the main render surface.
unsigned char	glitches_queued = 0;
bool			hold_glyph_glitches = false;	// Hold off on glyph glitching right now.
SDL_Surface		*main_surface = nullptr;	// The main render surface.
SDL_Window		*main_window = nullptr;		// The main (and only) SDL window.
unsigned short	mouse_clicked_x = 0, mouse_clicked_y = 0;	// Last clicked location for a mouse event.
std::unordered_map<std::string, s_rgb>	nebula_cache;	// Cache for the nebula() function.
unsigned short	nebula_cache_seed = 0;	// The seed for the nebula cache.
snes_ntsc_t		*ntsc = nullptr;		// Used by the NTSC filter.
bool			ntsc_filter = true;		// Whether or not the NTSC filter is enabled.
bool			ntsc_glitched = false;
vector<unsigned int>	queued_keys;	// Keypresses waiting to be processed.
int				screen_x = 0, screen_y = 0;	// Chosen screen resolution.
int				shade_mode = false;		// Are we rendering in shade mode?
SDL_Surface		*snes_surface = nullptr;	// The SNES surface, for rendering the CRT effect.
SDL_Surface		*sprites = nullptr;		// The texture for larger sprites.
unsigned char	surface_scale = 0;		// The surface scale modifier.
SDL_Surface		*temp_surface = nullptr;	// Temporary surface used for blitting glyphs.
SDL_Surface		**tileset = nullptr;	// The currently-loaded tileset.
unsigned int	tileset_file_count = 0;	// How many files are loaded for this tileset?
std::unordered_map<string, std::pair<unsigned int, unsigned int>>	tileset_map;	// The definitions map for the currently-loaded tileset.
unsigned int	tileset_pixel_size = 0;	// The size of the tiles in pixels (e.g. 16 = 16x16 tiles)
bool			tileset_supports_animation = false;	// Set to true is the currently-loaded tileset supports two-frame animation.
int				unscaled_x = 0, unscaled_y = 0;	// The unscaled resolution.
SDL_Surface		*window_surface = nullptr;	// The actual window's surface.


// Prints a string in the Alagard font at the specified coordinates.
void alagard_print(string message, int x, int y, Colour colour)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;

	for (unsigned int i = 0; i < message.size(); i++)
		alagard_print_at(message.at(i), x + (i * 24), y, colour);
}

// Prints an Alagard font character at the specified coordinates.
void alagard_print_at(char letter, int x, int y, Colour colour)
{
	STACK_TRACE();
	if (letter == ' ' || letter == '_') return;
	if (letter >= 'A' && letter <= 'Z') letter -= 65;
	else if (letter == '/') letter = 27;
	else if (letter == '.') letter = 28;
	else letter = 26;
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;

	unsigned char r, g, b;
	parse_colour(colour, r, g, b);

	// Parse the colour into SDL's native format.
	const unsigned int sdl_col = SDL_MapRGB(main_surface->format, r, g, b);

	// Determine the location of the character.
	const unsigned short loc_x = static_cast<unsigned short>(letter) * (ntsc_filter ? 24 : 48);
	SDL_Rect font_rect = { loc_x, 0, (ntsc_filter ? 24 : 48), (ntsc_filter ? 26 : 52) };

	// Draw a coloured square, then 'stamp' it with the font.
	SDL_Rect scr_rect = { x * (ntsc_filter ? 1 : 2), y * (ntsc_filter ? 1 : 2), (ntsc_filter ? 24 : 48), (ntsc_filter ? 26 : 52) };
	if (SDL_FillRect(main_surface, &scr_rect, sdl_col) < 0) guru::halt(SDL_GetError());
	if (SDL_BlitSurface(alagard, &font_rect, main_surface, &scr_rect) < 0) guru::halt(SDL_GetError());
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

// Calculates glitch positions.
void calc_glitches()
{
	STACK_TRACE();
	glitch_vec.clear();
	for (unsigned int i = 0; i < mathx::rnd(25); i++)
		if (mathx::rnd(5) == 1) glitch_square(); else glitch_horizontal();
}

// Like wait_for_key() below, but only checks if a key is queued; if not, it does nothing and doesn't wait.
unsigned int check_for_key()
{
	STACK_TRACE();
	if (queued_keys.size())
	{
		const unsigned int result = queued_keys.at(0);
		queued_keys.erase(queued_keys.begin());
		return result;
	}

	SDL_Event e;
	bool shift = false, ctrl = false, caps = false, alt = false;
	unsigned int key = 0;

	if (SDL_PollEvent(&e))
	{
		if (e.type == SDL_QUIT) { exit_functions(); exit(0); }
		else if (e.type == SDL_KEYDOWN)
		{
			const unsigned short mod = e.key.keysym.mod;
			if (!key)
			{
				key = e.key.keysym.sym;
				if ((mod & KMOD_LSHIFT) || (mod & KMOD_RSHIFT) || (mod & KMOD_SHIFT)) shift = true;
				if ((mod & KMOD_LCTRL) || (mod & KMOD_RCTRL) || (mod & KMOD_CTRL)) ctrl = true;
				if ((mod & KMOD_LALT) || (mod & KMOD_RALT) || (mod & KMOD_ALT)) alt = true;
				if (mod & KMOD_CAPS) caps = true;
			}
		}
		// Mouse controls!
		else if (e.type == SDL_MOUSEBUTTONDOWN)
		{
			if (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT)
			{
				if (ntsc_filter) mouse_clicked_x = SNES_NTSC_IN_WIDTH(e.button.x) / 8;
				else mouse_clicked_x = e.button.x / 16;
				mouse_clicked_y = e.button.y / 16;
				if (e.button.button == SDL_BUTTON_LEFT) return LMB_KEY; else return RMB_KEY;
			}
		}
		else if (e.type == SDL_MOUSEWHEEL)
		{
			if (e.wheel.y > 0) return MOUSEWHEEL_UP_KEY;
			else if (e.wheel.y < 0) return MOUSEWHEEL_DOWN_KEY;
		}
		else if (e.type == SDL_WINDOWEVENT)
		{
			if (e.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				glitch_vec.clear();
				window_surface = SDL_GetWindowSurface(main_window);
				if (!window_surface)
				{
					guru::console_ready(false);
					guru::halt(SDL_GetError());
				}
				if ((window_surface->w != main_surface->w || window_surface->h != main_surface->h) && surface_scale != 3)
				{
					SDL_FreeSurface(main_surface);
					SDL_FreeSurface(glitched_main_surface);
					if (ntsc_filter) SDL_FreeSurface(snes_surface);
					SDL_FreeSurface(glitch_hz_surface);
					if (!(main_surface = SDL_CreateRGBSurface(0, window_surface->w, window_surface->h, 16, 0, 0, 0, 0)))
					{
						guru::console_ready(false);
						guru::halt(SDL_GetError());
					}
					if (!(glitched_main_surface = SDL_CreateRGBSurface(0, window_surface->w, window_surface->h, 16, 0, 0, 0, 0)))
					{
						guru::console_ready(false);
						guru::halt(SDL_GetError());
					}
					if (ntsc_filter)
					{
						if (!(snes_surface = SDL_CreateRGBSurface(0, window_surface->w, window_surface->h, 16, 0, 0, 0, 0)))
						{
							guru::console_ready(false);
							guru::halt(SDL_GetError());
						}
					}
					if (!(glitch_hz_surface = SDL_CreateRGBSurface(0, window_surface->w + 16, 8, 16, 0, 0, 0, 0)))
					{
						guru::console_ready(false);
						guru::halt(SDL_GetError());
					}
					if (SDL_SetColorKey(glitch_hz_surface, SDL_TRUE, SDL_MapRGB(glitch_hz_surface->format, 1, 1, 1)) < 0)
					{
						guru::console_ready(false);
						guru::halt(SDL_GetError());
					}
					if (SDL_SetColorKey(glitch_sq_surface, SDL_TRUE, SDL_MapRGB(glitch_sq_surface->format, 1, 1, 1)) < 0)
					{
						guru::console_ready(false);
						guru::halt(SDL_GetError());
					}
				}
				else cls();
				if (surface_scale != 3)
				{
					screen_x = unscaled_x = window_surface->w; screen_y = unscaled_y = window_surface->h;
					if (surface_scale)
					{
						if (surface_scale == 1) screen_y = static_cast<int>(static_cast<float>(screen_y) / 1.333f);
						else if (surface_scale == 2) { screen_x /= 2; screen_y /= 2; }
					}
					if (ntsc_filter)
					{
						cols = SNES_NTSC_IN_WIDTH(screen_x) / 8;
						narrow_cols = SNES_NTSC_IN_WIDTH(screen_x) / 5;
						tile_cols = SNES_NTSC_IN_WIDTH(unscaled_x) / tileset_pixel_size;
						tile_rows = (unscaled_y - 112) / (tileset_pixel_size * 2);
					}
					else
					{
						cols = screen_x / 16;
						narrow_cols = screen_x / 10;
						tile_cols = screen_x / tileset_pixel_size;
						tile_rows = screen_y / tileset_pixel_size;
					}
					rows = screen_y / 16;
					mid_col = cols / 2;
					mid_row = rows / 2;
					mid_col_narrow = narrow_cols / 2;
				}
				return RESIZE_KEY;
			}
			else if (e.window.event == SDL_WINDOWEVENT_CLOSE) { exit_functions(); exit(0); }
		}
	}
	if (key == SDLK_LCTRL || key == SDLK_RCTRL || key == SDLK_LALT || key == SDLK_RALT || key == SDLK_LSHIFT || key == SDLK_RSHIFT) key = 0;
	else if (key >= 'a' && key <= 'z')
	{
		if ((shift || caps) && ctrl && alt) key += (1 << 15) - 32;	// shift-ctrl-alt
		else if (ctrl && alt) key += (1 << 15);						// ctrl-alt
		else if ((shift || caps) && ctrl) key += (1 << 16) - 32;	// shift-ctrl
		else if ((shift || caps) && alt) key += (1 << 17) - 32;		// shift-alt
		else if (shift || caps) key -= 32;							// shift
		else if (ctrl) key -= 96;									// ctrl
		else if (alt) key += (1 << 17);								// alt
	}
	else if ((key == SDLK_LEFT || key == SDLK_KP_4) && shift) key = SHIFT_LEFT;
	else if ((key == SDLK_RIGHT || key == SDLK_KP_6) && shift) key = SHIFT_RIGHT;
	else if ((key == SDLK_UP || key == SDLK_KP_8) && shift) key = SHIFT_UP;
	else if ((key == SDLK_DOWN || key == SDLK_KP_2) && shift) key = SHIFT_DOWN;
	else if (key == prefs::keybind(Keys::SCREENSHOT))
	{
		// Create screenshot folder if needed.
		filex::make_dir(FOLDER_SCREENS);

		// Determine the filename for the screenshot.
		int sshot = 0;
		string filename;
		while(true)
		{
			filename = (string)FOLDER_SCREENS + "/duskfall" + strx::itos(++sshot);
			if (!(filex::file_exists(filename + ".png") || filex::file_exists(filename + ".bmp") || filex::file_exists(filename + ".jpg") || filex::file_exists(filename + ".tmp"))) break;
			if (sshot > 1000000) return key;	// Just give up if we have an absurd amount of files.
		}
		if (prefs::screenshot_type == 2) SDL_SaveJPG((ntsc_filter ? snes_surface : main_surface), (filename + ".jpg").c_str(), -1);
		else SDL_SaveBMP((ntsc_filter ? snes_surface : main_surface), (filename + (prefs::screenshot_type > 0 ? ".tmp" : ".bmp")).c_str());
		if (prefs::screenshot_type == 1) std::thread(convert_png, filename).detach();
	}
	return key;
}

// Clears 'shade mode' entirely.
void clear_shade()
{
	shade_mode = 0;
}

// Clears the screen.
void cls()
{
	STACK_TRACE();
	if (SDL_FillRect(main_surface, &main_surface->clip_rect, SDL_MapRGBA(main_surface->format, 0, 0, 0, 255)) < 0) guru::halt(SDL_GetError());
}

// Calls SDL_Delay but also handles visual glitches.
void delay(unsigned int ms)
{
	STACK_TRACE();
	SDL_Delay(ms);

	// Check to see if we're still on a countdown for the glitch-clear.
	if (glitch_clear_countdown)
	{
		if (ms >= glitch_clear_countdown)	// Glitches are done, revert to normal display.
		{
			glitch_clear_countdown = 0;
			glitch_vec.clear();
			if (!glitches_queued) flip();
		}
		else glitch_clear_countdown -= ms;	// Just count the timer down for now.
		return;
	}

	// If glitches are disabled, then just exit quietly now. It's okay to leave the code above, as that counts down to *removing* glitches.
	if (!prefs::visual_glitches || !glitch_multi) return;

	// If we're due for more glitches, get 'em started.
	if (glitches_queued)
	{
		glitches_queued--;
		glitch_clear_countdown = mathx::rnd(75) + 25;
		calc_glitches();
		flip();
		return;
	}

	// See if any new glitches are due to start.
	int glitch_chance = 0;
	if (glitch_multi) glitch_chance = GLITCH_CHANCE / glitch_multi;
	if (prefs::visual_glitches == 1) glitch_chance *= 3;
	if (mathx::rnd(glitch_chance) == 1)
	{
		glitches_queued = 1;
		if (mathx::rnd(1000) == 1) glitches_queued = 5;
		else if (mathx::rnd(20) == 1) glitches_queued = 3;
		else if (mathx::rnd(3) == 1) glitches_queued = 2;
		glitch_clear_countdown = mathx::rnd(75) + 25;
		calc_glitches();
		flip();
	}
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

	if (exit_func_level >= 3)
	{
		SDL_FreeSurface(main_surface);
		SDL_FreeSurface(glitched_main_surface);
		SDL_FreeSurface(window_surface);
		if (ntsc_filter) SDL_FreeSurface(snes_surface);
		SDL_FreeSurface(temp_surface);
#ifndef TARGET_LINUX	// Not sure why, but these cause some nasty console errors on Linux.
		SDL_FreeSurface(glitch_hz_surface);
		SDL_FreeSurface(glitch_sq_surface);
#endif
		if (ntsc_filter) free(ntsc);
		main_surface = window_surface = snes_surface = temp_surface = glitch_hz_surface = glitch_sq_surface = glitched_main_surface = nullptr;
		ntsc = nullptr;
		guru::console_ready(false);

		if (exit_func_level >= 4) SDL_FreeSurface(font);

		if (filex::directory_exists(FOLDER_SCREENS))
		{
			vector<string> files = filex::files_in_dir(FOLDER_SCREENS);
			int converted = 0;
			for (auto file : files)
			{
				const string filename = (string)FOLDER_SCREENS + "/" + file.substr(0, file.length() - 4);
				const string ext = file.substr(file.length() - 3);
				if (ext == "tmp")
				{
					converted++;
					rename((filename + ".tmp").c_str(), (filename + ".bmp").c_str());
				}
			}
			if (converted) guru::log("Rescued " + strx::itos(converted) + " unconverted screenshots as BMP format.", GURU_INFO);
		}
	}

	if (exit_func_level >= 2)
	{
#ifndef TARGET_LINUX	// Also not sure why, but this can be problematic on Linux.
		guru::log("Shutting SDL down cleanly.", GURU_INFO);
		SDL_Quit();
#endif
	}
	exit_func_level = 0;
}

// Redraws the display.
void flip()
{
	STACK_TRACE();
	bool glitching = (prefs::visual_glitches && glitch_multi > 0);

	if (glitching && mathx::rnd(NTSC_GLITCH_CHANCE * glitch_multi) == 1 && prefs::ntsc_mode != 3 && prefs::visual_glitches >= 3 && !ntsc_glitched)
	{
		update_ntsc_mode(mathx::rnd(3));	// Don't do shader mode 0, it's too 'clean' for a glitch.
		ntsc_glitched = true;
	}
	else if (ntsc_glitched && mathx::rnd(NTSC_RESET_CHANCE) == 1)
	{
		update_ntsc_mode();
		ntsc_glitched = false;
	}

	SDL_Surface *render_surf = main_surface, *output_surf = snes_surface;
	if (prefs::visual_glitches && glitch_clear_countdown)
	{
		render_surf = glitched_main_surface;
		SDL_BlitSurface(main_surface, nullptr, glitched_main_surface, nullptr);
		render_glitches();
	}

	if (ntsc_filter)
	{
		if (SDL_LockSurface(snes_surface) < 0)
		{
			guru::console_ready(false);
			guru::halt(SDL_GetError());
		}
		unsigned char *output_pixels = (unsigned char*)snes_surface->pixels;
		const long output_pitch = snes_surface->pitch;
		snes_ntsc_blit(ntsc, (unsigned short*)render_surf->pixels, render_surf->pitch / 2, 0, render_surf->w, render_surf->h, output_pixels, output_pitch);
		for (int y = snes_surface->h / 2; --y >= 0; )
		{
			unsigned char const* in = output_pixels + y * output_pitch;
			unsigned char* out = output_pixels + y * 2 * output_pitch;
			int n;
			for (n = render_surf->w; n; --n)
			{
				const unsigned prev = *(unsigned short*) in;
				const unsigned next = *(unsigned short*) (in + output_pitch);
				// mix 16-bit rgb without losing low bits
				const unsigned mixed = prev + next + ((prev ^ next) & 0x0821);
				// darken by 12%
				*(unsigned short*) out = prev;
				*(unsigned short*) (out + output_pitch) = (mixed >> 1) - (mixed >> 4 & 0x18E3);
				in += 2;
				out += 2;
			}
		}
		SDL_UnlockSurface(snes_surface);

		if (!(window_surface = SDL_GetWindowSurface(main_window)))
		{
			guru::console_ready(false);
			guru::halt(SDL_GetError());
		}
	}
	else output_surf = main_surface;

	if (surface_scale)
	{
		SDL_Rect the_rect = { 0, 0, 0, 0 };
		switch(surface_scale)
		{
			case 1: the_rect = { 0, 0, output_surf->w, static_cast<int>(output_surf->h * 1.333f) }; break;
			case 2: the_rect = { 0, 0, output_surf->w * 2, output_surf->h * 2 }; break;
			case 3: the_rect = { 0, 0, window_surface->w, window_surface->h }; break;
		}
		if (SDL_BlitScaled(output_surf, nullptr, window_surface, &the_rect) < 0)
		{
			guru::console_ready(false);
			guru::halt(SDL_GetError());
		}
	}
	else if (SDL_BlitSurface(output_surf, nullptr, window_surface, nullptr) < 0)
	{
		guru::console_ready(false);
		guru::halt(SDL_GetError());
	}

	if (SDL_UpdateWindowSurface(main_window) < 0)	// This can fail once in a blue moon. We'll retry a few times, then give up.
	{
		guru::log("Having trouble updating the main window surface. Trying to fix this...", GURU_WARN);
		bool got_there_in_the_end = false;
		int tries = 0;
		for (int i = 0; i < SDL_RETRIES; i++)
		{
			if (!(window_surface = SDL_GetWindowSurface(main_window)))
			{
				guru::console_ready(false);
				guru::halt(SDL_GetError());
			}
			if (!SDL_UpdateWindowSurface(main_window)) { got_there_in_the_end = true; tries = i + 1; break; }
			delay(10);
		}
		if (!got_there_in_the_end)
		{
			guru::console_ready(false);
			guru::halt(SDL_GetError());
		}
		else guru::log("...Reacquired access to the window surface after " + strx::itos(tries) + (tries == 1 ? " try." : " tries."), GURU_WARN);
	}
}

// Returns the number of columns on the screen.
unsigned short get_cols()
{
	return cols;
}

// As above, for the narrow font.
unsigned short get_cols_narrow()
{
	return narrow_cols;
}

// Gets a direction key, or returns false if an invalid key is pressed.
bool get_direction(int &x_dir, int &y_dir)
{
	STACK_TRACE();
	x_dir = y_dir = 0;
	const unsigned int key = wait_for_key();
	if (key == prefs::keybind(Keys::NORTH) || key == prefs::keybind(Keys::NORTHEAST) || key == prefs::keybind(Keys::NORTHWEST)) y_dir = -1;
	else if (key == prefs::keybind(Keys::SOUTH) || key == prefs::keybind(Keys::SOUTHEAST) || key == prefs::keybind(Keys::SOUTHWEST)) y_dir = 1;
	if (key == prefs::keybind(Keys::WEST) || key == prefs::keybind(Keys::NORTHWEST) || key == prefs::keybind(Keys::SOUTHWEST)) x_dir = -1;
	else if (key == prefs::keybind(Keys::EAST) || key == prefs::keybind(Keys::NORTHEAST) || key == prefs::keybind(Keys::SOUTHEAST)) x_dir = 1;
	if (x_dir || y_dir) return true;
	else return false;
}

// Check if we're using an NTSC screen filter or not.
bool get_ntsc_filter()
{
	return ntsc_filter;
}

// Gets a pixel from the main surface.
s_rgb get_pixel(int x, int y)
{
	STACK_TRACE();
	int bpp = main_surface->format->BytesPerPixel;
	uint8_t *p = (uint8_t*)main_surface->pixels + y * main_surface->pitch + x * bpp;
	uint32_t pixel = 0;
	switch(bpp)
	{
		case 1: pixel = *p; break;
		case 2: pixel = *(uint16_t*)p; break;
		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN) pixel = p[0] << 16 | p[1] << 8 | p[2];
			else pixel = p[0] | p[1] << 8 | p[2] << 16;
			break;
		case 4: pixel = *(uint32_t*)p; break;
	}
	s_rgb rgb;
	SDL_GetRGB(pixel, main_surface->format, &rgb.r, &rgb.g, &rgb.b);
	return rgb;
}

// Returns the number of rows on the screen.
unsigned short get_rows()
{
	return rows;
}

// Returns the number of columns available for tile rendering.
unsigned short get_tile_cols()
{
	STACK_TRACE();
	return tile_cols;
}

// Returns the number of rows available for tile rendering.
unsigned short get_tile_rows()
{
	STACK_TRACE();
	return tile_rows;
}

// Offsets part of the display.
void glitch(int glitch_x, int glitch_y, int glitch_w, int glitch_h, int glitch_off_x, int glitch_off_y, bool black, SDL_Surface *surf)
{
	STACK_TRACE();
	if ((surf == glitch_hz_surface && (glitch_w > glitched_main_surface->w || glitch_h > 8)) || (surf == glitch_sq_surface && (glitch_w > 70 || glitch_h > 70))) guru::halt("Invalid parameters given to glitch()");
	SDL_Rect clear = { 0, 0, surf->w, surf->h };
	SDL_FillRect(surf, &clear, SDL_MapRGB(surf->format, 1, 1, 1));
	SDL_Rect source = { glitch_x, glitch_y, glitch_w, glitch_h };
	SDL_Rect dest = { glitch_x + glitch_off_x, glitch_y + glitch_off_y, glitch_w, glitch_h };
	SDL_BlitSurface(glitched_main_surface, &source, surf, nullptr);
	if (black) SDL_FillRect(glitched_main_surface, &source, SDL_MapRGB(glitched_main_surface->format, 0, 0, 0));
	SDL_BlitSurface(surf, nullptr, glitched_main_surface, &dest);
}

// Horizontal displacement visual glitch.
void glitch_horizontal()
{
	STACK_TRACE();
	s_glitch horiz_glitch;
	horiz_glitch.x = 0; horiz_glitch.w = glitched_main_surface->w;	// Horizontal glitches always fill the entire screen, left to right.
	horiz_glitch.y = mathx::rnd(glitched_main_surface->h);	// Random Y coordinate.
	horiz_glitch.h = mathx::rnd(8);	// Varying glitch sizes.
	horiz_glitch.off_x = mathx::rnd(30) - 15;	// The horizontal direction can glitch either way.
	horiz_glitch.off_y = 0;	// It doesn't glitch vertically.
	horiz_glitch.black = (mathx::rnd(2) == 1);
	horiz_glitch.surf = glitch_hz_surface;
	glitch_vec.push_back(horiz_glitch);
}

// Sets the glitch intensity level.
void glitch_intensity(unsigned char value)
{
	glitch_multi = value;
}

// Square displacement glitch.
void glitch_square()
{
	STACK_TRACE();
	s_glitch square_glitch;
	square_glitch.x = mathx::rnd(glitched_main_surface->w); square_glitch.y = mathx::rnd(glitched_main_surface->h);	// Random X,Y coordinates.
	square_glitch.w = mathx::rnd(60) + 10; square_glitch.h = mathx::rnd(60) + 10;	// Random width and height.
	square_glitch.off_x = mathx::rnd(30) - 15; square_glitch.off_y = mathx::rnd(30) - 15;	// Can glitch either horizontally, vertically, or both.
	square_glitch.black = false;
	square_glitch.surf = glitch_sq_surface;
	glitch_vec.push_back(square_glitch);
}

// Converts a Glyph into an ansi_print() compatible glyph string.
string glyph_string(Glyph glyph)
{
	STACK_TRACE();
	string result = strx::itos(static_cast<unsigned int>(glyph));
	while (result.size() < 3) result = "0" + result;
	return "^" + result + "^";
}

// Initializes SDL and gets the ball rolling.
void init()
{
	STACK_TRACE();
	guru::log("Duskfall v" + DUSKFALL_VERSION_STRING + " [build " + strx::itos(build_version()) + "]", GURU_STACK);
	guru::log("Main program entry. Let's do this.", GURU_INFO);

	// Check for necessary CPU features.
	bool has_mmx = SDL_HasMMX(), has_sse = SDL_HasSSE(), has_sse2 = SDL_HasSSE2(), has_sse3 = SDL_HasSSE3(), has_multicore = (SDL_GetCPUCount() > 1);
	vector<string> missing_cpu;
	if (!has_mmx) missing_cpu.push_back("MMX");
	if (!has_sse) missing_cpu.push_back("SSE");
	if (!has_sse2) missing_cpu.push_back("SSE2");
	if (!has_sse3) missing_cpu.push_back("SSE3");
	if (!has_multicore) missing_cpu.push_back("multi-core");
	if (missing_cpu.size()) guru::log("Missing CPU features may degrade performance: " + strx::comma_list(missing_cpu), GURU_WARN);

	// Start the ball rolling.
	guru::log("Initializing SDL core systems: video, timer, events.", GURU_INFO);
	const unsigned int sdl_flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS;
	if (SDL_Init(sdl_flags) < 0) guru::halt(SDL_GetError());
	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) guru::halt(IMG_GetError());
	exit_func_level = 2;

	// This is messy. Set up all the surfaces we'll be using for rendering, and exit out if anything goes wrong.
	guru::log("Initializing SDL window and surfaces.", GURU_INFO);
	ntsc_filter = prefs::ntsc_filter;
	screen_x = unscaled_x = prefs::screen_x;
	screen_y = unscaled_y = prefs::screen_y;
	const bool fullscreen = prefs::fullscreen;
	surface_scale = prefs::scale_mod;
	if (surface_scale == 1) screen_y = static_cast<int>(screen_y * 1.3333) + 1;
	else if (surface_scale == 2) { screen_x *= 2; screen_y *= 2; }
	if (screen_x < SCREEN_MIN_X) screen_x = SCREEN_MIN_X;
	else if (screen_x > SCREEN_MAX_X) screen_x = SCREEN_MAX_X;
	if (screen_y < SCREEN_MIN_Y) screen_y = SCREEN_MIN_Y;
	else if (screen_y > SCREEN_MAX_Y) screen_y = SCREEN_MAX_Y;
	string window_title = "Duskfall " + DUSKFALL_VERSION_STRING + " [build " + strx::itos(build_version()) + "]";
	main_window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_x, screen_y, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
	if (!main_window) guru::halt(SDL_GetError());
	SDL_SetWindowMinimumSize(main_window, SCREEN_MIN_X, SCREEN_MIN_Y);
	if (ntsc_filter)
	{
		cols = SNES_NTSC_IN_WIDTH(unscaled_x) / 8;
		rows = unscaled_y / 16;
		narrow_cols = SNES_NTSC_IN_WIDTH(unscaled_x) / 5;
	}
	else
	{
		cols = unscaled_x / 16;
		rows = unscaled_y / 16;
		narrow_cols = unscaled_x / 10;
	}
	mid_col = cols / 2;
	mid_row = rows / 2;
	mid_col_narrow = narrow_cols / 2;
	if (!(window_surface = SDL_GetWindowSurface(main_window))) guru::halt(SDL_GetError());
	if (!(main_surface = SDL_CreateRGBSurface(0, window_surface->w, window_surface->h, 16, 0, 0, 0, 0))) guru::halt(SDL_GetError());
	if (!(glitched_main_surface = SDL_CreateRGBSurface(0, window_surface->w, window_surface->h, 16, 0, 0, 0, 0))) guru::halt(SDL_GetError());
	if (ntsc_filter) { if (!(snes_surface = SDL_CreateRGBSurface(0, window_surface->w + 16, window_surface->h + 16, 16, 0, 0, 0, 0))) guru::halt(SDL_GetError()); }
	SDL_RaiseWindow(main_window);
	if (!(temp_surface = SDL_CreateRGBSurface(0, 32, 32, 16, 0, 0, 0, 0))) guru::halt(SDL_GetError());
	if (!(glitch_hz_surface = SDL_CreateRGBSurface(0, window_surface->w + 16, 8, 16, 0, 0, 0, 0))) guru::halt(SDL_GetError());
	if (SDL_SetColorKey(glitch_hz_surface, SDL_TRUE, SDL_MapRGB(glitch_hz_surface->format, 1, 1, 1)) < 0) guru::halt(SDL_GetError());
	if (!(glitch_sq_surface = SDL_CreateRGBSurface(0, 70, 70, 16, 0, 0, 0, 0))) guru::halt(SDL_GetError());
	if (SDL_SetColorKey(glitch_sq_surface, SDL_TRUE, SDL_MapRGB(glitch_sq_surface->format, 1, 1, 1)) < 0) guru::halt(SDL_GetError());
	exit_func_level = 3;

	// Sets up the PCG PRNG.
	guru::log("Initializing pseudorandom number generator.", GURU_INFO);
	mathx::init();

	// Set up the SNES renderer.
	if (ntsc_filter)
	{
		if (!(ntsc = static_cast<snes_ntsc_t*>(calloc(1, sizeof(snes_ntsc_t))))) guru::halt("Unable to initialize NTSC shader.");
		update_ntsc_mode();
	}

	// Blank the screen.
	cls();
	flip();

	// Load the bitmap fonts and other PNGs into memory.
	guru::log("Attempting to load bitmap fonts.", GURU_INFO);
	load_and_optimize_png("png/fonts.png", &font);
	load_and_optimize_png("png/alagard.png", &alagard);
	load_and_optimize_png("png/sprites.png", &sprites);
	load_and_optimize_png("png/ruthenia.png", &font_narrow);
	font_sheet_size = (font->w * font->h) / 8;
	font_sheet_size_narrow = (font_narrow->w / 5) * (font_narrow->h / 8);
	if (!ntsc_filter)
	{
		font_sheet_size /= 2;
		font_sheet_size_narrow /= 2;
	}
	load_tileset("dawnlike");
	exit_func_level = 4;

	// Now that the font is loaded and SDL is initialized, we can activate Guru's error screen.
	guru::console_ready(true);
}

// Returns true if the key is a chosen 'cancel' key.
bool is_cancel(unsigned int key)
{
	STACK_TRACE();
	if (key == prefs::keybind(MENU_CANCEL) || key == SDLK_ESCAPE) return true;
	return false;
}

// Returns true if the key is a chosen 'down' key.
bool is_down(unsigned int key)
{
	STACK_TRACE();
	if (key == SDLK_DOWN || key == SDLK_KP_2 || key == prefs::keybind(SOUTH)) return true;
	return false;
}

// Returns true if the key is a chosen 'left' key.
bool is_left(unsigned int key)
{
	STACK_TRACE();
	if (key == SDLK_LEFT || key == SDLK_KP_4 || key == prefs::keybind(WEST)) return true;
	return false;
}

// Returns true if the key is a chosen 'right' key.
bool is_right(unsigned int key)
{
	STACK_TRACE();
	if (key == SDLK_RIGHT || key == SDLK_KP_6 || key == prefs::keybind(EAST)) return true;
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
	if (key == SDLK_UP || key == SDLK_KP_8 || key == prefs::keybind(NORTH)) return true;
	return false;
}

// Returns the name of a key.
string key_to_name(unsigned int key)
{
	STACK_TRACE();
	if (!key) return "{5C}[unbound]";
	else if (key == 8) return "Backspace";
	else if (key == 9) return "Tab";
	else if (key == 13) return "Enter";
	else if ((key >= '!' && key <= '@') || key == '`') return string(1, static_cast<char>(key));
	else if (key >= 'a' && key <= 'z') return string(1, static_cast<char>(key - 32));
	else if (key >= 'A' && key <= 'Z') return "Shift-" +string(1, static_cast<char>(key));
	else if (key >= 1 && key <= 26) return "Ctrl-" + string(1, static_cast<char>(key + 64));
	else if (key >= ((1 << 17) + 'a') && key <= ((1 << 17) + 'z')) return "Alt-" + string(1, static_cast<char>(key - (1 << 17) - 32));
	else if (key >= ((1 << 15) + 'A') && key <= ((1 << 15) + 'Z')) return "Shift-Ctrl-Alt-" + string(1, static_cast<char>(key - (1 << 15)));
	else if (key >= ((1 << 15) + 'a') && key <= ((1 << 15) + 'z')) return "Ctrl-Alt-" + string(1, static_cast<char>(key - (1 << 15) - 32));
	else if (key >= ((1 << 16) + 'A') && key <= ((1 << 16) + 'Z')) return "Shift-Ctrl-" + string(1, static_cast<char>(key - (1 << 16)));
	else if (key >= ((1 << 17) + 'A') && key <= ((1 << 17) + 'Z')) return "Shift-Alt-" + string(1, static_cast<char>(key - (1 << 17)));
	else if (key == MOUSEWHEEL_UP_KEY) return "Mousewheel Up";
	else if (key == MOUSEWHEEL_DOWN_KEY) return "Mousewheel Up";
	else if (key == LMB_KEY) return "Left Mouse Button";
	else if (key == RMB_KEY) return "Right Mouse Button";

	if ((key > 0x7F && key < 0x40000039) || key > 0x4000011A) return "{5C}[unknown]";
	return SDL_GetKeyName(key);
}

// Loads a PNG into memory and optimizes it for the main render surface.
void load_and_optimize_png(string filename, SDL_Surface **dest)
{
	STACK_TRACE();
	SDL_Surface *surf_temp = IMG_Load(("data/" + filename).c_str());
	if (!surf_temp) guru::halt(IMG_GetError());

	// Double the size of the loaded graphics if we're not using the NTSC filter.
	if (!ntsc_filter)
	{
		SDL_Surface *surface_temp_opt = SDL_ConvertSurface(surf_temp, main_surface->format, 0);
		if (!surface_temp_opt) guru::halt(SDL_GetError());
		SDL_FreeSurface(surf_temp);
		SDL_Surface *surface_temp_scaled = SDL_CreateRGBSurface(0, surface_temp_opt->w * 2, surface_temp_opt->h * 2, 16, 0, 0, 0, 0);
		if (!surface_temp_scaled) guru::halt(SDL_GetError());
		if (SDL_BlitScaled(surface_temp_opt, nullptr, surface_temp_scaled, nullptr) < 0) guru::halt(SDL_GetError());
		SDL_FreeSurface(surface_temp_opt);
		*dest = surface_temp_scaled;
	}
	else
	{
		*dest = SDL_ConvertSurface(surf_temp, main_surface->format, 0);
		if (!dest) guru::halt(SDL_GetError());
		SDL_FreeSurface(surf_temp);
	}
	if (SDL_SetColorKey(*dest, SDL_TRUE, SDL_MapRGB((*dest)->format, 255, 255, 255)) < 0) guru::halt(SDL_GetError());
}

// Loads a specified tileset into memory, discarding the previous tileset.
void load_tileset(string dir)
{
	STACK_TRACE();
	if (tileset_file_count)
	{
		for (unsigned int i = 0; i < tileset_file_count; i++)
			SDL_FreeSurface(tileset[i]);
		delete[] tileset;
	}
	Json::Value json = filex::load_json("tilesets/" + dir + "/tileset");
	const Json::Value::Members jmem = json.getMemberNames();
	tileset_file_count = json["TILESET_FILES"].asUInt();
	tileset = new SDL_Surface*[tileset_file_count];
	tileset_pixel_size = json["TILESET_PIXEL_SIZE"].asUInt();
	string tileset_alpha_unparsed = json["TILESET_ALPHA"].asString();
	tileset_supports_animation = json["TILESET_SUPPORTS_ANIMATION"].asBool();
	if (tileset_alpha_unparsed.size() != 6) guru::halt("Invalid tileset alpha string.");
	unsigned char alpha_r = strx::htoi(tileset_alpha_unparsed.substr(0, 2));
	unsigned char alpha_g = strx::htoi(tileset_alpha_unparsed.substr(2, 2));
	unsigned char alpha_b = strx::htoi(tileset_alpha_unparsed.substr(4, 2));
	if (!ntsc_filter) tileset_pixel_size *= 2;
	for (unsigned int i = 0; i < tileset_file_count; i++)
	{
		load_and_optimize_png("tilesets/" + dir + "/" + strx::itos(i) + ".png", &tileset[i]);
		if (SDL_SetColorKey(tileset[i], SDL_TRUE, SDL_MapRGB(tileset[i]->format, alpha_r, alpha_g, alpha_b)) < 0) guru::halt(SDL_GetError());
	}
	for (unsigned int i = 0; i < jmem.size(); i++)
	{
		string def_id = jmem.at(i);
		if (def_id.substr(0, 7) == "TILESET") continue;
		const string def_unparsed = json[def_id].asString();
		vector<string> def_parsed = strx::string_explode(def_unparsed, ":");
		if (def_parsed.size() != 2) guru::halt("Formatting error in " + dir + " tileset: " + def_id);
		std::pair<unsigned int, unsigned int> new_pair = std::pair<unsigned int, unsigned int>(atoi(def_parsed.at(0).c_str()), atoi(def_parsed.at(1).c_str()));
		tileset_map.insert(std::pair<string, std::pair<unsigned int, unsigned int>>(def_id, new_pair));
	}
	if (ntsc_filter)
	{
		tile_cols = SNES_NTSC_IN_WIDTH(unscaled_x) / tileset_pixel_size;
		tile_rows = (unscaled_y - 112) / (tileset_pixel_size * 2);
	}
	else
	{
		tile_cols = screen_x / tileset_pixel_size;
		tile_rows = screen_y / tileset_pixel_size;
	}
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

// As above, for the narrow font.
unsigned short midcol_narrow()
{
	return mid_col_narrow;
}

// Determines the colour of a specific point in a nebula, based on X,Y coordinates.
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

void parse_colour(Colour colour, unsigned char &r, unsigned char &g, unsigned char &b)
{
	STACK_TRACE();
	static const unsigned char colour_table[(MAX_COLOUR + 1) * 3] = { 109,109,109, 0,44,150, 0,10,161, 50,0,132, 145,0,74, 188,0,26, 173,2,0, 122,13,0, 72,32,0, 8,51,0,
			2,55,0, 0,53,31, 0,47,82, 0,0,0, 89,58,34, 0,0,0, 188,188,188, 0,100,255, 20,66,255, 111,38,249, 231,32,167, 255,27,61, 255,21,0,
			205,34,0, 184,78,0, 37,109,0, 2,125,0, 0,119,66, 0,136,194, 20,20,20, 131,99,74, 0,0,0, 255,235,17, 8,207,255, 85,146,255, 203,109,255,
			255,51,240, 255,77,121, 255,117,35, 255,139,10, 249,175,20, 142,221,7, 28,237,37, 6,237,148, 2,250,255, 74,74,74, 184,161,143, 0,0,0,
			255,255,255, 150,251,255, 165,232,255, 210,156,231, 255,152,248, 255,156,165, 255,201,161, 255,235,150, 255,245,139, 207,227,131,
			150,233,160, 146,239,210, 136,255,251, 214,214,214, 40,40,40, 0,0,0,

			0,0,0, 0,0,128, 0,128,0, 0,128,128, 128,0,0, 128,0,128, 128,128,0, 192,192,192, 128,128,128, 0,0,255, 0,255,0, 0,255,255, 255,0,0, 255,0,255, 255,255,0, 255,255,255,
			0,0,0, 0,0,170, 0,170,0, 0,170,170, 170,0,0, 170,0,170, 170,85,0, 170,170,170, 85,85,85, 85,85,255, 85,255,85, 85,255,255, 255,85,85, 255,85,255, 255,255,85, 255,255,255 };

	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;

	if (colour == Colour::ERROR_COLOUR)
	{
		unsigned long long seconds = time(NULL);
		if (seconds % 2) { r = 255; g = 235; b = 17; }
		else { r = 0; g = 100; b = 255; }
		return;
	}

	r = colour_table[static_cast<unsigned int>(colour) * 3];
	g = colour_table[(static_cast<unsigned int>(colour) * 3) + 1];
	b = colour_table[(static_cast<unsigned int>(colour) * 3) + 2];
}

// Prints a message at the specified coordinates.
int print(string message, int x, int y, Colour colour, unsigned int print_flags)
{
	STACK_TRACE();
	unsigned char r, g, b;
	parse_colour(colour, r, g, b);
	return print(message, x, y, r, g, b, print_flags);
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

// Prints a character at a given coordinate on the screen.
void print_at(Glyph letter, int x, int y, Colour colour, unsigned int print_flags)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;

	// Parse the colour into RGB values.
	unsigned char r, g, b;
	parse_colour(colour, r, g, b);
	print_at(letter, x, y, r, g, b, print_flags);
}

// As above, but with a char instead of a glyph.
void print_at(char letter, int x, int y, Colour colour, unsigned int print_flags)
{
	STACK_TRACE();
	print_at(static_cast<Glyph>(letter), x, y, colour, print_flags);
}

// Prints a character at a given coordinate on the screen, in specific RGB colours.
void print_at(Glyph letter, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags)
{
	STACK_TRACE();
	const bool narrow_font = (print_flags & PRINT_FLAG_NARROW) == PRINT_FLAG_NARROW;
	int glyph_width = (narrow_font ? 5 : 8), glyph_height = 8;
	if (!ntsc_filter) { glyph_width *= 2; glyph_height *= 2; }
	int cols_available = (narrow_font ? narrow_cols : cols), rows_available = rows;
	unsigned int sheet_size = (narrow_font ? font_sheet_size_narrow : font_sheet_size);
	SDL_Surface *chosen_font = (narrow_font ? font_narrow : font);

	// Just exit quietly if drawing off-screen. This shouldn't normally happen.
	if (mathx::check_flag(print_flags, PRINT_FLAG_ABSOLUTE))
	{
		if (x < 0 || y < 0 || x > cols_available * glyph_width || y > rows_available * glyph_height) return;
	}
	else
	{
		if (x < 0 || y < 0 || x > cols_available || y > rows_available) return;
	}

	// If we're using the alternate font, adjust the glyph now.
	if (mathx::check_flag(print_flags, PRINT_FLAG_ALT_FONT))
	{
		const unsigned int letter_int = static_cast<unsigned int>(letter);
		if ((letter_int >= 'A' && letter_int <= 'Z') || (letter_int >= 'a' && letter_int <= 'z'))
			letter = static_cast<Glyph>(letter_int + 192);
	}

	// Are we in shade mode? If so, dim the colours.
	if (shade_mode > 0)
	{
		r /= 2;
		g /= 2;
		b /= 2;
	}

	// Check for no-NBSP print flag.
	bool no_nbsp = false;
	if (mathx::check_flag(print_flags, PRINT_FLAG_NO_NBSP)) no_nbsp = true;

	// Check for no-spaces print flag.
	if (mathx::check_flag(print_flags, PRINT_FLAG_NO_SPACES) && letter == static_cast<Glyph>(' ')) return;

	if (letter == static_cast<Glyph>('`') && !no_nbsp) letter = static_cast<Glyph>(' ');	// We can use ` to put non-break spaces in strings.

	// Parse the colour into SDL's native format.
	const unsigned int sdl_col = SDL_MapRGB(main_surface->format, r, g, b);

	// Determine the location of the character in the grid.
	if (narrow_font) letter = static_cast<Glyph>(static_cast<unsigned short>(letter) - 32);
	if (static_cast<unsigned short>(letter) >= sheet_size) letter = static_cast<Glyph>('?');
	unsigned short loc_x = static_cast<unsigned short>(letter) * glyph_width, loc_y = 0;
	while (loc_x >= chosen_font->w) { loc_y += glyph_height; loc_x -= chosen_font->w; }
	SDL_Rect font_rect = {loc_x, loc_y, glyph_width, glyph_height};

	// Draw a coloured square, then 'stamp' it with the font.
	int x_pos = x, y_pos = y;
	if (!mathx::check_flag(print_flags, PRINT_FLAG_ABSOLUTE)) { x_pos *= glyph_width; y_pos *= glyph_height; }
	if (mathx::check_flag(print_flags, PRINT_FLAG_PLUS_FOUR_X)) x_pos += (ntsc_filter ? 2 : 4);
	if (mathx::check_flag(print_flags, PRINT_FLAG_PLUS_FOUR_Y)) y_pos += (ntsc_filter ? 2 : 4);
	if (mathx::check_flag(print_flags, PRINT_FLAG_PLUS_EIGHT_X)) x_pos += (ntsc_filter ? 4 : 8);
	if (mathx::check_flag(print_flags, PRINT_FLAG_PLUS_EIGHT_Y)) y_pos += (ntsc_filter ? 4 : 8);
	SDL_Rect scr_rect = {x_pos, y_pos, glyph_width, glyph_height};
	if (mathx::check_flag(print_flags, PRINT_FLAG_ALPHA))
	{
		SDL_Rect temp_rect = {0, 0, glyph_width, glyph_height};
		if (SDL_FillRect(temp_surface, &temp_rect, sdl_col) < 0) guru::halt(SDL_GetError());
		if (SDL_BlitSurface(chosen_font, &font_rect, temp_surface, &temp_rect) < 0) guru::halt(SDL_GetError());
		if (SDL_SetColorKey(temp_surface, SDL_TRUE, SDL_MapRGB(temp_surface->format, 0, 0, 0)) < 0) guru::halt(SDL_GetError());
		if (SDL_BlitSurface(temp_surface, &temp_rect, main_surface, &scr_rect) < 0) guru::halt(SDL_GetError());
	}
	else
	{
		if (SDL_FillRect(main_surface, &scr_rect, sdl_col) < 0) guru::halt(SDL_GetError());
		if (SDL_BlitSurface(chosen_font, &font_rect, main_surface, &scr_rect) < 0) guru::halt(SDL_GetError());
	}
}

// As above, but with a char instead of a glyph.
void print_at(char letter, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags)
{
	STACK_TRACE();
	print_at(static_cast<Glyph>(letter), x, y, r, g, b, print_flags);
}

// Renders a tile from the active tileset on the screen at the specified location.
void print_tile(string tile, int x, int y, unsigned char brightness, bool animated)
{
	STACK_TRACE();
	if (!brightness)
	{
		rect(x * tileset_pixel_size, y * tileset_pixel_size, tileset_pixel_size, tileset_pixel_size, Colour::BLACK);
		return;
	}
	if (!tileset_supports_animation || !prefs::animation) animated = false;

	// Sanity checks to ensure the tilesheet data is valid and we're not trying to load something that doesn't exist.
	auto found = tileset_map.find(tile);
	if (found == tileset_map.end())
	{
		guru::log("Missing tile: " + tile, GURU_ERROR);
		if (tile != "ERROR") print_tile("ERROR", x, y, brightness);
		else rect(x, y, tileset_pixel_size, tileset_pixel_size, Colour::ERROR_COLOUR);
		return;
	}
	unsigned int sheet = found->second.first;
	unsigned int tile_pos = found->second.second;
	if (sheet >= tileset_file_count || tile_pos * tileset_pixel_size > static_cast<unsigned int>(tileset[sheet]->w * tileset[sheet]->h))
	{
		guru::log("Invalid tilesheet definition: " + tile, GURU_ERROR);
		rect(x, y, tileset_pixel_size, tileset_pixel_size, Colour::ERROR_COLOUR);
		return;
	}

	// If we're trying to draw off-screen, just exit quietly.
	if (x < 0 || y < 0 || static_cast<signed int>(x * tileset_pixel_size) >= unscaled_x || static_cast<signed int>(y * tileset_pixel_size) >= unscaled_y) return;

	// Determine the location of the sprite on the grid.
	SDL_Surface *chosen_sheet = tileset[sheet];
	unsigned int loc_x = tile_pos * tileset_pixel_size, loc_y = 0;
	if (animated && current_animation_frame) loc_x += tileset_pixel_size;
	while (loc_x >= static_cast<unsigned int>(chosen_sheet->w)) { loc_y += tileset_pixel_size; loc_x -= chosen_sheet->w; }
	SDL_Rect tile_rect = {static_cast<signed int>(loc_x), static_cast<signed int>(loc_y), static_cast<signed int>(tileset_pixel_size), static_cast<signed int>(tileset_pixel_size)};

	// Print the sprite on the screen!
	SDL_Rect scr_rect = {static_cast<signed int>(x * tileset_pixel_size), static_cast<signed int>(y * tileset_pixel_size), static_cast<signed int>(tileset_pixel_size), static_cast<signed int>(tileset_pixel_size)};
	if (SDL_BlitSurface(chosen_sheet, &tile_rect, main_surface, &scr_rect) < 0) guru::halt(SDL_GetError());

	// If the brightness is not maximum, edit the pixels to dim it.
	if (brightness == 255) return;
	float ratio = static_cast<float>(brightness) / 255.0f;
	if (SDL_MUSTLOCK(main_surface)) SDL_LockSurface(main_surface);
	for (int px = scr_rect.x; px < scr_rect.x + scr_rect.w; px++)
	{
		for (int py = scr_rect.y; py < scr_rect.y + scr_rect.h; py++)
		{
			s_rgb rgb = get_pixel(px, py);
			rgb.r = round(static_cast<float>(rgb.r) * ratio);
			rgb.g = round(static_cast<float>(rgb.g) * ratio);
			rgb.b = round(static_cast<float>(rgb.b) * ratio);
			put_pixel(rgb, px, py);
		}
	}
	if (SDL_MUSTLOCK(main_surface)) SDL_UnlockSurface(main_surface);
}

// Writes a pixel to the main surface.
void put_pixel(s_rgb rgb, int x, int y)
{
	STACK_TRACE();
	uint32_t pixel = SDL_MapRGB(main_surface->format, rgb.r, rgb.g, rgb.b);
	int bpp = main_surface->format->BytesPerPixel;
	uint8_t *p = (uint8_t*)main_surface->pixels + y * main_surface->pitch + x * bpp;
	switch(bpp)
	{
		case 1: *p = pixel; break;
		case 2: *(uint16_t*)p = pixel; break;
		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			{
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			}
			else
			{
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;
		case 4: *(uint32_t*)p = pixel; break;
	}
}

// Draws a coloured rectangle.
void rect(int x, int y, int w, int h, Colour colour)
{
	STACK_TRACE();
	const unsigned int glyph_size = (ntsc_filter ? 8 : 16);
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;
	rect_fine(x * glyph_size, y * glyph_size, w * glyph_size, h * glyph_size, colour);
}

// Draws a rectangle at very specific coords.
void rect_fine(int x, int y, int w, int h, Colour colour)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;

	s_rgb rgb_col;
	parse_colour(colour, rgb_col.r, rgb_col.g, rgb_col.b);
	rect_fine(x, y, w, h, rgb_col);
}

// As above, but with direct RGB input.
void rect_fine(int x, int y, int w, int h, s_rgb colour)
{
	STACK_TRACE();

	// Are we in shade mode? If so, dim the colours.
	if (shade_mode > 0)
	{
		colour.r /= 2;
		colour.g /= 2;
		colour.b /= 2;
	}
	SDL_Rect dest = { x, y, w, h };
	const unsigned int sdl_col = SDL_MapRGB(main_surface->format, colour.r, colour.g, colour.b);
	if (SDL_FillRect(main_surface, &dest, sdl_col) < 0) guru::halt(SDL_GetError());
}

// Renders pre-calculated glitches.
void render_glitches()
{
	STACK_TRACE();
	for (auto g : glitch_vec)
		glitch(g.x, g.y, g.w, g.h, g.off_x, g.off_y, g.black, g.surf);
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
			print_at(Glyph::BLOCK_SOLID, x, y, rgb.r, rgb.g, rgb.b);
		}
	}
}

// Do absolutely nothing for a little while.
void sleep_for(unsigned int amount)
{
	STACK_TRACE();
	if (!amount) return;
	const int cycles = amount / 10;
	for (int i = 0; i < cycles; i++)
	{
		const unsigned int key = wait_for_key(10);
		if (key && cycles < 100) queued_keys.push_back(key);
	}
}

// Prints a sprite at the given location.
void sprite_print(Sprite id, int x, int y, Colour colour, unsigned char flags)
{
	STACK_TRACE();
	const bool plus_four = ((flags & SPRITE_FLAG_PLUS_FOUR) == SPRITE_FLAG_PLUS_FOUR);
	const bool quad = ((flags & SPRITE_FLAG_QUAD) == SPRITE_FLAG_QUAD);
	const int sprite_size = (ntsc_filter ? 16 : 32);
	if (quad)
	{
		for (unsigned int i = 0; i < 2; i++)
		{
			for (unsigned int j = 0; j < 2; j++)
			{
				Sprite new_id = id;
				if (i == 1) new_id = static_cast<Sprite>(static_cast<int>(new_id) + 1);
				if (j == 1) new_id = static_cast<Sprite>(static_cast<int>(new_id) + 24);
				sprite_print(new_id, x + (i * 2), y + (j * 2), colour, flags ^ SPRITE_FLAG_QUAD);
			}
		}
		return;
	}
	unsigned short loc_x = static_cast<unsigned short>(id) * sprite_size, loc_y = 0;
	while (loc_x >= sprites->w) { loc_y += sprite_size; loc_x -= sprites->w; }
	SDL_Rect sprite_rect = {loc_x, loc_y, sprite_size, sprite_size};

	// Parse the colour into SDL's native format.
	unsigned char r, g, b;
	parse_colour(colour, r, g, b);
	const unsigned int sdl_col = SDL_MapRGB(main_surface->format, r, g, b);

	// Draw a coloured square, then 'stamp' it with the sprite.
	SDL_Rect scr_rect = { (x * (sprite_size / 2)) + (plus_four ? (sprite_size / 4) : 0), y * (sprite_size / 2), sprite_size, sprite_size };
	SDL_Rect temp_rect = {0, 0, sprite_size * 2, sprite_size * 2};
	if (SDL_FillRect(temp_surface, &temp_rect, sdl_col) < 0) guru::halt(SDL_GetError());
	if (SDL_BlitSurface(sprites, &sprite_rect, temp_surface, &temp_rect) < 0) guru::halt(SDL_GetError());
	if (SDL_SetColorKey(temp_surface, SDL_TRUE, SDL_MapRGB(temp_surface->format, 0, 0, 0)) < 0) guru::halt(SDL_GetError());
	if (SDL_BlitSurface(temp_surface, &temp_rect, main_surface, &scr_rect) < 0) guru::halt(SDL_GetError());
}

// Returns the pixel size of the loaded tileset's individual tiles.
unsigned int tile_pixel_size()
{
	return tileset_pixel_size;
}

// Toggles the two-step animations.
void toggle_animation_frame()
{
	current_animation_frame = !current_animation_frame;
}

// Unlocks the mutexes, if they're locked. Only for use by the Guru system.
void unlock_surfaces()
{
	STACK_TRACE();
	if (ntsc_filter) SDL_UnlockSurface(snes_surface);
}

// Updates the NTSC filter.
void update_ntsc_mode(int force)
{
	STACK_TRACE();
	if (!ntsc_filter) return;
	snes_ntsc_setup_t setup = snes_ntsc_composite;
	if (force == -1)
	{
		if (prefs::ntsc_mode == 1) setup = snes_ntsc_svideo;
		else if (prefs::ntsc_mode == 0) setup = snes_ntsc_rgb;
		else if (prefs::ntsc_mode == 3) setup = snes_ntsc_monochrome;
	}
	else
	{
		if (force == 1) setup = snes_ntsc_svideo;
		else if (force == 0) setup = snes_ntsc_rgb;
		else if (force == 3) setup = snes_ntsc_monochrome;
	}
	setup.merge_fields = 1;
	if (force != -1 && mathx::rnd(3) == 1) setup.merge_fields = 0;
	snes_ntsc_init(ntsc, &setup);
}

// Polls SDL until a key is pressed. If a time is specified, it will abort after this time.
unsigned int wait_for_key(unsigned short max_ms)
{
	STACK_TRACE();
	if (queued_keys.size())
	{
		const unsigned int result = queued_keys.at(0);
		queued_keys.erase(queued_keys.begin());
		return result;
	}

	unsigned int elapsed = 0, key = 0;
	while (elapsed < max_ms || (!max_ms && !key))
	{
		if ((key = check_for_key()))
		{
			SDL_FlushEvent(SDL_KEYDOWN);
			return key;
		}
		if (!max_ms || max_ms >= 10) delay(10);
		elapsed += 10;
	}
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
		else print(line, mid_col - (strx::ansi_strlen(line) / 2), box_start_y + 2 + i, Colour::CGA_WHITE, PRINT_FLAG_ALT_FONT);
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
