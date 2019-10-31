// iocore.cpp -- The render core, handling display and user interaction, as well as program initialization, shutdown and cleanup functionality.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "filex.h"
#include "guru.h"
#include "iocore.h"
#include "mathx.h"
#include "prefs.h"
#include "strx.h"
#include "version.h"

#include "lodepng/bmp2png.h"
#include "sdl_savejpeg/SDL_savejpeg.h"
#include "SDL2/SDL_image.h"

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <thread>

#define	SCREEN_MIN_X		1024	// Minimum X-resolution of the screen. Should not be lower than 1024.
#define SCREEN_MIN_Y		600		// Minimum Y-resolution of the screen. Should not be lower than 600.
#define SCREEN_MAX_X		4080	// Maximum X-resolution.
#define SCREEN_MAX_Y		4080	// Maximum Y-resolution.
#define SDL_RETRIES			100		// Amount of times to try re-acquiring the SDL window surface before giving up.
#define GLITCH_CHANCE		200		// The lower this number, the more often visual glitches occur.
#define NTSC_GLITCH_CHANCE	500		// The lower this number, the more often NTSC mode glitches occur.
#define NTSC_RESET_CHANCE	50		// The lower this number, the faster NTSC glitches go back to normal.
#define EXTRA_NES_COLOURS			53	// How many extra colours are added from the NES palette.
#define EXTRA_COLOURBLIND_COLOURS	32	// How many extra colours are added from the colourblind palettes.
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

IOCore*	iocore = nullptr;	// The main IOCore object.

IOCore::IOCore() : nebula_cache_seed(0), shade_mode(0), exit_func_level(1), hold_glyph_glitches(false), glitch_multi(1), mouse_clicked_x(0), mouse_clicked_y(0), glitch_clear_countdown(0), glitches_queued(0), ntsc_glitched(false),
		cleaned_up(false)
{
	STACK_TRACE();
	guru->log("Duskfall v" + DUSKFALL_VERSION_STRING + " [build " + strx::itos(build_version()) + "]", GURU_STACK);
	guru->log("Main program entry. Let's do this.", GURU_INFO);

	// Check for necessary CPU features.
	bool has_mmx = SDL_HasMMX(), has_sse = SDL_HasSSE(), has_sse2 = SDL_HasSSE2(), has_sse3 = SDL_HasSSE3(), has_multicore = (SDL_GetCPUCount() > 1);
	vector<string> missing_cpu;
	if (!has_mmx) missing_cpu.push_back("MMX");
	if (!has_sse) missing_cpu.push_back("SSE");
	if (!has_sse2) missing_cpu.push_back("SSE2");
	if (!has_sse3) missing_cpu.push_back("SSE3");
	if (!has_multicore) missing_cpu.push_back("multi-core");
	if (missing_cpu.size()) guru->log("Missing CPU features may degrade performance: " + strx::comma_list(missing_cpu), GURU_WARN);

	// Start the ball rolling.
	guru->log("Initializing SDL core systems: video, timer, events.", GURU_INFO);
	const unsigned int sdl_flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS;
	if (SDL_Init(sdl_flags) < 0) guru->halt(SDL_GetError());
	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) guru->halt(IMG_GetError());
	exit_func_level = 2;

	// This is messy. Set up all the surfaces we'll be using for rendering, and exit out if anything goes wrong.
	guru->log("Initializing SDL window and surfaces.", GURU_INFO);
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
	if (!main_window) guru->halt(SDL_GetError());
	SDL_SetWindowMinimumSize(main_window, SCREEN_MIN_X, SCREEN_MIN_Y);
	cols = SNES_NTSC_IN_WIDTH(unscaled_x) / 8; rows = unscaled_y / 16; mid_col = cols / 2; mid_row = rows / 2;
	if (!(window_surface = SDL_GetWindowSurface(main_window))) guru->halt(SDL_GetError());
	if (!(main_surface = SDL_CreateRGBSurface(0, window_surface->w, window_surface->h, 16, 0, 0, 0, 0))) guru->halt(SDL_GetError());
	if (!(glitched_main_surface = SDL_CreateRGBSurface(0, window_surface->w, window_surface->h, 16, 0, 0, 0, 0))) guru->halt(SDL_GetError());
	if (!(snes_surface = SDL_CreateRGBSurface(0, window_surface->w + 16, window_surface->h + 16, 16, 0, 0, 0, 0))) guru->halt(SDL_GetError());
	SDL_RaiseWindow(main_window);
	if (!(temp_surface = SDL_CreateRGBSurface(0, 32, 32, 16, 0, 0, 0, 0))) guru->halt(SDL_GetError());
	if (!(glitch_hz_surface = SDL_CreateRGBSurface(0, window_surface->w + 16, 8, 16, 0, 0, 0, 0))) guru->halt(SDL_GetError());
	if (SDL_SetColorKey(glitch_hz_surface, SDL_TRUE, SDL_MapRGB(glitch_hz_surface->format, 1, 1, 1)) < 0) guru->halt(SDL_GetError());
	if (!(glitch_sq_surface = SDL_CreateRGBSurface(0, 70, 70, 16, 0, 0, 0, 0))) guru->halt(SDL_GetError());
	if (SDL_SetColorKey(glitch_sq_surface, SDL_TRUE, SDL_MapRGB(glitch_sq_surface->format, 1, 1, 1)) < 0) guru->halt(SDL_GetError());
	exit_func_level = 3;

	// Sets up the PCG PRNG.
	guru->log("Initializing pseudorandom number generator.", GURU_INFO);
	mathx::init();

	// Set up the SNES renderer.
	if (!(ntsc = static_cast<snes_ntsc_t*>(calloc(1, sizeof(snes_ntsc_t))))) guru->halt("Unable to initialize NTSC shader.");
	update_ntsc_mode();

	// Blank the screen.
	cls();
	flip();

	// Load the CP437 font into memory.
	guru->log("Attempting to load bitmap fonts.", GURU_INFO);
	auto load_and_optimize_png = [] (string filename, SDL_Surface **dest, SDL_Surface *main_surface)
	{
		SDL_Surface *surf_temp = IMG_Load(("data/png/" + filename).c_str());
		if (!surf_temp) guru->halt(IMG_GetError());
		*dest = SDL_ConvertSurface(surf_temp, main_surface->format, 0);
		if (!dest) guru->halt(SDL_GetError());
		SDL_FreeSurface(surf_temp);
		if (SDL_SetColorKey(*dest, SDL_TRUE, SDL_MapRGB((*dest)->format, 255, 255, 255)) < 0) guru->halt(SDL_GetError());
	};

	load_and_optimize_png("cp437.png", &font, main_surface);
	load_and_optimize_png("alagard.png", &alagard, main_surface);
	font_sheet_size = (font->w * font->h) / 8;
	exit_func_level = 4;

	// Now that the font is loaded and SDL is initialized, we can activate Guru's error screen.
	guru->activate();
}

IOCore::~IOCore()
{
	exit_functions();
}

// Adjusts the colour palette, if needed.
Colour IOCore::adjust_palette(Colour colour)
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
void IOCore::alagard_print(string message, int x, int y, Colour colour)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;

	for (unsigned int i = 0; i < message.size(); i++)
		alagard_print_at(message.at(i), x + (i * 24), y, colour);
}

// Prints an Alagard font character at the specified coordinates.
void IOCore::alagard_print_at(char letter, int x, int y, Colour colour)
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
	const unsigned short loc_x = static_cast<unsigned short>(letter) * 24;
	SDL_Rect font_rect = { loc_x, 0, 24, 26 };

	// Draw a coloured square, then 'stamp' it with the font.
	SDL_Rect scr_rect = { x, y, 24, 26 };
	if (SDL_FillRect(main_surface, &scr_rect, sdl_col) < 0) guru->halt(SDL_GetError());
	if (SDL_BlitSurface(alagard, &font_rect, main_surface, &scr_rect) < 0) guru->halt(SDL_GetError());
}

// Prints an ANSI string at the specified position.
void IOCore::ansi_print(string msg, int x, int y, unsigned int print_flags)
{
	STACK_TRACE();

	msg = msg + "{10}";
	Colour colour = Colour::CGA_LGRAY;
	int offset = 0;
	string::size_type pos = 0;
	string code;

	do
	{
		pos = msg.find((string)"{");
		if (pos != string::npos)
		{
			if (pos > 0)
			{
				offset += print(msg.substr(0, pos), x + offset, y, colour, print_flags);
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
void IOCore::box(int x, int y, int w, int h, Colour colour, unsigned char flags, string title)
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
	iocore->ansi_print(title, title_x, y);
	iocore->print_at(Glyph::LINE_VL, title_x - 1, y, colour);
	iocore->print_at(Glyph::LINE_VR, title_x + title_len, y, colour);
}

// Calculates glitch positions.
void IOCore::calc_glitches()
{
	STACK_TRACE();
	glitch_vec.clear();
	for (unsigned int i = 0; i < mathx::rnd(25); i++)
		if (mathx::rnd(5) == 1) glitch_square(); else glitch_horizontal();
}

// Clears the screen.
void IOCore::cls()
{
	STACK_TRACE();
	if (SDL_FillRect(main_surface, &main_surface->clip_rect, SDL_MapRGBA(main_surface->format, 0, 0, 0, 255)) < 0) guru->halt(SDL_GetError());
}

// Calls SDL_Delay but also handles visual glitches.
void IOCore::delay(unsigned int ms)
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
bool IOCore::did_mouse_click(unsigned short x, unsigned short y, unsigned short w, unsigned short h)
{
	if (mouse_clicked_x >= x && mouse_clicked_y >= y && mouse_clicked_x <= x + w - 1 && mouse_clicked_y <= y + h - 1)
	{
		mouse_clicked_x = mouse_clicked_y = 0;
		return true;
	}
	return false;
}

// This is where we clean up our shit.
void IOCore::exit_functions()
{
	STACK_TRACE();
	if (cleaned_up) return;
	cleaned_up = true;
	guru->log("Running cleanup at level " + strx::itos(exit_func_level) + ".", GURU_INFO);

	if (exit_func_level >= 3)
	{
		SDL_FreeSurface(main_surface);
		SDL_FreeSurface(glitched_main_surface);
		SDL_FreeSurface(window_surface);
		SDL_FreeSurface(snes_surface);
		SDL_FreeSurface(temp_surface);
#ifndef TARGET_LINUX	// Not sure why, but these cause some nasty console errors on Linux.
		SDL_FreeSurface(glitch_hz_surface);
		SDL_FreeSurface(glitch_sq_surface);
#endif
		free(ntsc);
		main_surface = window_surface = snes_surface = temp_surface = glitch_hz_surface = glitch_sq_surface = glitched_main_surface = nullptr;
		ntsc = nullptr;
		guru->deactivate();

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
			if (converted) guru->log("Rescued " + strx::itos(converted) + " unconverted screenshots as BMP format.", GURU_INFO);
		}
	}

	if (exit_func_level >= 2)
	{
#ifndef TARGET_LINUX	// Also not sure why, but this can be problematic on Linux.
		guru->log("Shutting SDL down cleanly.", GURU_INFO);
		SDL_Quit();
#endif
	}
}

// Redraws the display.
void IOCore::flip()
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

	SDL_Surface *render_surf = main_surface;
	if (prefs::visual_glitches && glitch_clear_countdown)
	{
		render_surf = glitched_main_surface;
		SDL_BlitSurface(main_surface, nullptr, glitched_main_surface, nullptr);
		render_glitches();
	}

	if (SDL_LockSurface(snes_surface) < 0)
	{
		guru->deactivate();
		guru->halt(SDL_GetError());
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
		guru->deactivate();
		guru->halt(SDL_GetError());
	}

	if (surface_scale)
	{
		SDL_Rect the_rect = { 0, 0, 0, 0 };
		switch(surface_scale)
		{
			case 1: the_rect = { 0, 0, snes_surface->w, static_cast<int>(snes_surface->h * 1.333f) }; break;
			case 2: the_rect = { 0, 0, snes_surface->w * 2, snes_surface->h * 2 }; break;
			case 3: the_rect = { 0, 0, window_surface->w, window_surface->h }; break;
		}
		if (SDL_BlitScaled(snes_surface, nullptr, window_surface, &the_rect) < 0)
		{
			guru->deactivate();
			guru->halt(SDL_GetError());
		}
	}
	else if (SDL_BlitSurface(snes_surface, nullptr, window_surface, nullptr) < 0)
	{
		guru->deactivate();
		guru->halt(SDL_GetError());
	}

	if (SDL_UpdateWindowSurface(main_window) < 0)	// This can fail once in a blue moon. We'll retry a few times, then give up.
	{
		guru->log("Having trouble updating the main window surface. Trying to fix this...", GURU_WARN);
		bool got_there_in_the_end = false;
		int tries = 0;
		for (int i = 0; i < SDL_RETRIES; i++)
		{
			if (!(window_surface = SDL_GetWindowSurface(main_window)))
			{
				guru->deactivate();
				guru->halt(SDL_GetError());
			}
			if (!SDL_UpdateWindowSurface(main_window)) { got_there_in_the_end = true; tries = i + 1; break; }
			delay(10);
		}
		if (!got_there_in_the_end)
		{
			guru->deactivate();
			guru->halt(SDL_GetError());
		}
		else guru->log("...Reacquired access to the window surface after " + strx::itos(tries) + (tries == 1 ? " try." : " tries."), GURU_WARN);
	}
}

// Offsets part of the display.
void IOCore::glitch(int glitch_x, int glitch_y, int glitch_w, int glitch_h, int glitch_off_x, int glitch_off_y, bool black, SDL_Surface *surf)
{
	STACK_TRACE();
	if ((surf == glitch_hz_surface && (glitch_w > glitched_main_surface->w || glitch_h > 8)) || (surf == glitch_sq_surface && (glitch_w > 70 || glitch_h > 70))) guru->halt("Invalid parameters given to glitch()");
	SDL_Rect clear = { 0, 0, surf->w, surf->h };
	SDL_FillRect(surf, &clear, SDL_MapRGB(surf->format, 1, 1, 1));
	SDL_Rect source = { glitch_x, glitch_y, glitch_w, glitch_h };
	SDL_Rect dest = { glitch_x + glitch_off_x, glitch_y + glitch_off_y, glitch_w, glitch_h };
	SDL_BlitSurface(glitched_main_surface, &source, surf, nullptr);
	if (black) SDL_FillRect(glitched_main_surface, &source, SDL_MapRGB(glitched_main_surface->format, 0, 0, 0));
	SDL_BlitSurface(surf, nullptr, glitched_main_surface, &dest);
}

// Horizontal displacement visual glitch.
void IOCore::glitch_horizontal()
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

// Square displacement glitch.
void IOCore::glitch_square()
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

// Returns true if the key is a chosen 'cancel' key.
bool IOCore::is_cancel(unsigned int key)
{
	STACK_TRACE();
	if (key == prefs::keybind(MENU_CANCEL) || key == SDLK_ESCAPE) return true;
	return false;
}

// Returns true if the key is a chosen 'down' key.
bool IOCore::is_down(unsigned int key)
{
	STACK_TRACE();
	if (key == SDLK_DOWN || key == SDLK_KP_2 || key == prefs::keybind(SOUTH)) return true;
	return false;
}

// Returns true if the key is a chosen 'left' key.
bool IOCore::is_left(unsigned int key)
{
	STACK_TRACE();
	if (key == SDLK_LEFT || key == SDLK_KP_4 || key == prefs::keybind(WEST)) return true;
	return false;
}

// Returns true if the key is a chosen 'right' key.
bool IOCore::is_right(unsigned int key)
{
	STACK_TRACE();
	if (key == SDLK_RIGHT || key == SDLK_KP_6 || key == prefs::keybind(EAST)) return true;
	return false;
}

// Returns true if the key is a chosen 'select' key.
bool IOCore::is_select(unsigned int key)
{
	STACK_TRACE();
	if (key == prefs::keybind(Keys::MENU_OK) || key == prefs::keybind(Keys::MENU_OK_2)) return true;
	return false;
}

// Returns true if the key is a chosen 'up' key.
bool IOCore::is_up(unsigned int key)
{
	STACK_TRACE();
	if (key == SDLK_UP || key == SDLK_KP_8 || key == prefs::keybind(NORTH)) return true;
	return false;
}

// Returns the name of a key.
string IOCore::key_to_name(unsigned int key)
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

// Determines the colour of a specific point in a nebula, based on X,Y coordinates.
s_rgb IOCore::nebula(int x, int y)
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
unsigned char IOCore::nebula_rgb(unsigned char value, int modifier)
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
void IOCore::ok_box(int offset, Colour colour)
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

void IOCore::parse_colour(Colour colour, unsigned char &r, unsigned char &g, unsigned char &b)
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

// Prints a message at the specified coordinates.
int IOCore::print(string message, int x, int y, Colour colour, unsigned int print_flags)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;
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
						print_at(code, x + i + offset, y, colour, print_flags);
						offset -= 4;
					}
					else offset -= 5;
					i += 4;
				}
			}
		}
		else print_at(static_cast<Glyph>(message.at(i)), x + i + offset, y, colour, print_flags);
	}
	return offset;
}

// Prints a character at a given coordinate on the screen.
void IOCore::print_at(Glyph letter, int x, int y, Colour colour, unsigned int print_flags)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;

	// Parse the colour into RGB values.
	unsigned char r, g, b;
	parse_colour(colour, r, g, b);
	print_at(letter, x, y, r, g, b, print_flags);
}

// Prints a character at a given coordinate on the screen, in specific RGB colours.
void IOCore::print_at(Glyph letter, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags)
{
	STACK_TRACE();
	// Just exit quietly if drawing off-screen. This shouldn't normally happen.
	if (mathx::check_flag(print_flags, PRINT_FLAG_ABSOLUTE))
	{
		if (x < 0 || y < 0 || x > cols * 8 || y > rows * 8) return;
	}
	else
	{
		if (x < 0 || y < 0 || x > cols || y > rows) return;
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
	if (static_cast<unsigned short>(letter) >= font_sheet_size) letter = static_cast<Glyph>('?');
	unsigned short loc_x = static_cast<unsigned short>(letter) * 8, loc_y = 0;
	while (loc_x >= font->w) { loc_y += 8; loc_x -= font->w; }
	SDL_Rect font_rect = {loc_x, loc_y, 8, 8};

	// Draw a coloured square, then 'stamp' it with the font.
	int x_pos = x, y_pos = y;
	if (!mathx::check_flag(print_flags, PRINT_FLAG_ABSOLUTE)) { x_pos *= 8; y_pos *= 8; }
	if (mathx::check_flag(print_flags, PRINT_FLAG_PLUS_FOUR_X)) x_pos += 2;
	if (mathx::check_flag(print_flags, PRINT_FLAG_PLUS_FOUR_Y)) y_pos += 2;
	if (mathx::check_flag(print_flags, PRINT_FLAG_PLUS_EIGHT_X)) x_pos += 4;
	if (mathx::check_flag(print_flags, PRINT_FLAG_PLUS_EIGHT_Y)) y_pos += 4;
	SDL_Rect scr_rect = {x_pos, y_pos, 8, 8};
	if (mathx::check_flag(print_flags, PRINT_FLAG_ALPHA))
	{
		SDL_Rect temp_rect = {0, 0, 8, 8};
		if (SDL_FillRect(temp_surface, &temp_rect, sdl_col) < 0) guru->halt(SDL_GetError());
		if (SDL_BlitSurface(font, &font_rect, temp_surface, &temp_rect) < 0) guru->halt(SDL_GetError());
		if (SDL_SetColorKey(temp_surface, SDL_TRUE, SDL_MapRGB(temp_surface->format, 0, 0, 0)) < 0) guru->halt(SDL_GetError());
		if (SDL_BlitSurface(temp_surface, &temp_rect, main_surface, &scr_rect) < 0) guru->halt(SDL_GetError());
	}
	else
	{
		if (SDL_FillRect(main_surface, &scr_rect, sdl_col) < 0) guru->halt(SDL_GetError());
		if (SDL_BlitSurface(font, &font_rect, main_surface, &scr_rect) < 0) guru->halt(SDL_GetError());
	}
}

// Draws a coloured rectangle.
void IOCore::rect(int x, int y, int w, int h, Colour colour)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;
	rect_fine(x * 8, y * 8, w * 8, h * 8, colour);
}

// Draws a rectangle at very specific coords.
void IOCore::rect_fine(int x, int y, int w, int h, Colour colour)
{
	STACK_TRACE();
	if (static_cast<int>(colour) > MAX_COLOUR) colour = Colour::ERROR_COLOUR;

	s_rgb rgb_col;
	parse_colour(colour, rgb_col.r, rgb_col.g, rgb_col.b);
	rect_fine(x, y, w, h, rgb_col);
}

// As above, but with direct RGB input.
void IOCore::rect_fine(int x, int y, int w, int h, s_rgb colour)
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
	if (SDL_FillRect(main_surface, &dest, sdl_col) < 0) guru->halt(SDL_GetError());
}

// Renders pre-calculated glitches.
void IOCore::render_glitches()
{
	STACK_TRACE();
	for (auto g : glitch_vec)
		glitch(g.x, g.y, g.w, g.h, g.off_x, g.off_y, g.black, g.surf);
}

// Renders a nebula on the screen.
void IOCore::render_nebula(unsigned short seed, int off_x, int off_y)
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

// Unlocks the mutexes, if they're locked. Only for use by the Guru system.
void IOCore::unlock_surfaces()
{
	STACK_TRACE();
	SDL_UnlockSurface(snes_surface);
}

// Updates the NTSC filter.
void IOCore::update_ntsc_mode(int force)
{
	STACK_TRACE();
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
unsigned int IOCore::wait_for_key(unsigned short max_ms)
{
	STACK_TRACE();
	if (queued_keys.size())
	{
		const unsigned int result = queued_keys.at(0);
		queued_keys.erase(queued_keys.begin());
		return result;
	}

	SDL_Event e;
	unsigned int elapsed = 0, key = 0;
	bool shift = false, ctrl = false, caps = false, alt = false;
	while (elapsed < max_ms || (!max_ms && !key))
	{
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
					mouse_clicked_x = SNES_NTSC_IN_WIDTH(e.button.x) / 8;
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
						guru->deactivate();
						guru->halt(SDL_GetError());
					}
					if ((window_surface->w != main_surface->w || window_surface->h != main_surface->h) && surface_scale != 3)
					{
						SDL_FreeSurface(main_surface);
						SDL_FreeSurface(glitched_main_surface);
						SDL_FreeSurface(snes_surface);
						SDL_FreeSurface(glitch_hz_surface);
						if (!(main_surface = SDL_CreateRGBSurface(0, window_surface->w, window_surface->h, 16, 0, 0, 0, 0)))
						{
							guru->deactivate();
							guru->halt(SDL_GetError());
						}
						if (!(glitched_main_surface = SDL_CreateRGBSurface(0, window_surface->w, window_surface->h, 16, 0, 0, 0, 0)))
						{
							guru->deactivate();
							guru->halt(SDL_GetError());
						}
						if (!(snes_surface = SDL_CreateRGBSurface(0, window_surface->w, window_surface->h, 16, 0, 0, 0, 0)))
						{
							guru->deactivate();
							guru->halt(SDL_GetError());
						}
						if (!(glitch_hz_surface = SDL_CreateRGBSurface(0, window_surface->w + 16, 8, 16, 0, 0, 0, 0)))
						{
							guru->deactivate();
							guru->halt(SDL_GetError());
						}
						if (SDL_SetColorKey(glitch_hz_surface, SDL_TRUE, SDL_MapRGB(glitch_hz_surface->format, 1, 1, 1)) < 0)
						{
							guru->deactivate();
							guru->halt(SDL_GetError());
						}
						if (SDL_SetColorKey(glitch_sq_surface, SDL_TRUE, SDL_MapRGB(glitch_sq_surface->format, 1, 1, 1)) < 0)
						{
							guru->deactivate();
							guru->halt(SDL_GetError());
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
						cols = SNES_NTSC_IN_WIDTH(screen_x) / 8;
						rows = screen_y / 16;
						mid_col = cols / 2;
						mid_row = rows / 2;
					}
					return RESIZE_KEY;
				}
				else if (e.window.event == SDL_WINDOWEVENT_CLOSE) { exit_functions(); exit(0); }
			}
		}
		if (key == SDLK_LCTRL || key == SDLK_RCTRL || key == SDLK_LALT || key == SDLK_RALT || key == SDLK_LSHIFT || key == SDLK_RSHIFT) key = 0;
		if (!max_ms || max_ms >= 10) delay(10);
		elapsed += 10;
	}
	if (key >= 'a' && key <= 'z')
	{
		if ((shift || caps) && ctrl && alt) key += (1 << 15) - 32;	// shift-ctrl-alt
		else if (ctrl && alt) key += (1 << 15);						// ctrl-alt
		else if ((shift || caps) && ctrl) key += (1 << 16) - 32;	// shift-ctrl
		else if ((shift || caps) && alt) key += (1 << 17) - 32;		// shift-alt
		else if (shift || caps) key -= 32;							// shift
		else if (ctrl) key -= 96;									// ctrl
		else if (alt) key += (1 << 17);								// alt
	}
	if ((key == SDLK_LEFT || key == SDLK_KP_4) && shift) key = SHIFT_LEFT;
	else if ((key == SDLK_RIGHT || key == SDLK_KP_6) && shift) key = SHIFT_RIGHT;
	else if ((key == SDLK_UP || key == SDLK_KP_8) && shift) key = SHIFT_UP;
	else if ((key == SDLK_DOWN || key == SDLK_KP_2) && shift) key = SHIFT_DOWN;
	if (key == prefs::keybind(Keys::SCREENSHOT))
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
		if (prefs::screenshot_type == 2) SDL_SaveJPG(snes_surface, (filename + ".jpg").c_str(), -1);
		else SDL_SaveBMP(snes_surface, (filename + (prefs::screenshot_type > 0 ? ".tmp" : ".bmp")).c_str());
		if (prefs::screenshot_type == 1) std::thread(convert_png, filename).detach();
	}

	return key;
}
