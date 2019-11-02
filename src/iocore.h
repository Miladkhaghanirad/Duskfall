// iocore.h -- The render core, handling display and user interaction, as well as program initialization, shutdown and cleanup functionality.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once

class Guru;	// defined in guru.h

#include "duskfall.h"
#include <unordered_map>
#include "sdl2/sdl.h"
#include "snes_ntsc/snes_ntsc.h"

// Colour code definitions.
enum class Colour : unsigned char { GRAY = 0x00, AQUA, BLUE, PURPLE, MAGENTA, PINK, RED, ORANGE, YELLOW, LIME, GREEN, TURQ, CYAN, BLACK, BROWN, ERROR_COLOUR, GRAY_LIGHT, AQUA_LIGHT, BLUE_LIGHT, PURPLE_LIGHT, MAGENTA_LIGHT,
	PINK_LIGHT, RED_LIGHT, ORANGE_LIGHT, YELLOW_LIGHT, LIME_LIGHT, GREEN_LIGHT, TURQ_LIGHT, CYAN_LIGHT, BLACK_LIGHT, BROWN_LIGHT, _1F, YELLOW_PURE, AQUA_BRIGHT, BLUE_BRIGHT, PURPLE_BRIGHT, MAGENTA_BRIGHT, PINK_BRIGHT,
	RED_BRIGHT, ORANGE_BRIGHT, YELLOW_BRIGHT, LIME_BRIGHT, GREEN_BRIGHT, TURQ_BRIGHT, CYAN_BRIGHT, GRAY_DARK, BROWN_PALE, _2F, WHITE, AQUA_PALE, BLUE_PALE, PURPLE_PALE, MAGENTA_PALE, PINK_PALE, RED_PALE, ORANGE_PALE,
	YELLOW_PALE, LIME_PALE, GREEN_PALE, TURQ_PALE, CYAN_PALE, GRAY_PALE, BLACK_PALE, _3F,

	TERM_BLACK = 0x40, TERM_BLUE, TERM_GREEN, TERM_CYAN, TERM_RED, TERM_MAGENTA, TERM_YELLOW, TERM_LGRAY, TERM_GRAY, TERM_LBLUE, TERM_LGREEN, TERM_LCYAN, TERM_LRED, TERM_LMAGENTA, TERM_LYELLOW, TERM_WHITE,

	CGA_BLACK = 0x50, CGA_BLUE, CGA_GREEN, CGA_CYAN, CGA_RED, CGA_MAGENTA, CGA_YELLOW, CGA_LGRAY, CGA_GRAY, CGA_LBLUE, CGA_LGREEN, CGA_LCYAN, CGA_LRED, CGA_LMAGENTA, CGA_LYELLOW, CGA_WHITE };
#define MAX_COLOUR	0x5F	// The highest palette colour available.

#define UI_COLOUR_BOX	Colour::CGA_LGRAY
#define UI_COLOUR_LABEL	Colour::CGA_WHITE

// Codes for the high-ASCII characters.
enum class Glyph : unsigned short { FACE_BLACK = 1, FACE_WHITE, HEART, DIAMOND, CLUB, SPADE, BULLET, BULLET_INVERT, CIRCLE, CIRCLE_INVERT, MALE, FEMALE, MUSIC, MUSIC_DOUBLE, SUN, TRIANGLE_RIGHT, TRIANGLE_LEFT, ARROW_UD,
	DOUBLE_EXCLAIM, PILCROW, SECTION, GEOM_A, ARROW_UD_B, ARROW_UP, ARROW_DOWN, ARROW_RIGHT, ARROW_LEFT, BRACKET, ARROW_LR, TRIANGLE_UP, TRIANGLE_DOWN, HOUSE = 127, C_CEDILLA_CAPS, U_DIAERESIS, E_ACUTE, A_CIRCUMFLEX, A_DIAERESIS,
	A_GRAVE, A_OVERRING, C_CEDILLA, E_CIRCUMFLEX, E_DIAERESIS, E_GRAVE, I_DIAERESIS, I_CIRCUMFLEX, I_GRAVE, A_DIAERESIS_CAPS, A_OVERRING_CAPS, E_ACUTE_CAPS, AE, AE_CAPS, O_CIRCUMFLEX, O_DIAERESIS, O_GRAVE, U_CIRCUMFLEX, U_GRAVE,
	Y_DIAERESIS, O_DIAERESIS_CAPS, U_DIAERESIS_CAPS, CENT, POUND, YEN, PESETA, F_HOOK, A_ACUTE, I_ACUTE, O_ACUTE, U_ACUTE, N_TILDE, N_TILDE_CAPS, ORDINAL_F, ORDINAL_M, QUESTION_INVERTED, NOT_REVERSE, NOT, HALF, QUARTER,
	EXCLAIM_INVERTED, GUILLEMET_OPEN, GUILLEMET_CLOSE, SHADE_LIGHT, SHADE_MEDIUM, SHADE_HEAVY, LINE_V, LINE_VL, LINE_VLL, LINE_VVL, LINE_DDL, LINE_DLL, LINE_VVLL, LINE_VV, LINE_DDLL, LINE_UULL, LINE_UUL, LINE_ULL, LINE_DL, LINE_UR,
	LINE_UH, LINE_DH, LINE_VR, LINE_H, LINE_VH, LINE_VRR, LINE_VVR, LINE_UURR, LINE_DDRR, LINE_UUHH, LINE_DDHH, LINE_VVRR, LINE_HH, LINE_VVHH, LINE_UHH, LINE_UUH, LINE_DHH, LINE_DDH, LINE_UUR, LINE_URR, LINE_DRR, LINE_DDR, LINE_VVH,
	LINE_VHH, LINE_UL, LINE_DR, BLOCK_SOLID, BLOCK_D, BLOCK_L, BLOCK_R, BLOCK_U, ALPHA, BETA, GAMMA_CAPS, PI_CAPS, SIGMA_CAPS, SIGMA, MU, TAU, PHI_CAPS, THETA_CAPS, OMEGA_CAPS, DELTA, INFINITY, PHI, EPSILON, INTERSECTION,
	TRIPLE_BAR, PLUS_MINUS, GEQ, LEQ, INTEGRAL, INTEGRAL_INVERTED, DIVISION, APPROXIMATION, DEGREE, BULLET_SMALL, INTERPUNCT, SQUARE_ROOT, N_SUPER, SQUARE, MIDBLOCK, HALF_HEART, COPYRIGHT, BLOCKS_7 = 283, BLOCKS_11, BLOCKS_14,
	BLOCKS_13, BLOCKS_4, UPSIDE_DOWN_HD, BLOCKS_8 = 315, BLOCKS_1, BLOCKS_2, CORNER_CLIP_DL, CORNER_CLIP_DR, CURVE_DL, CURVE_UR, CURVE_UL, CURVE_DR, FLOPPY_DISK_METAL_HOLE, RETURN, TICK, MIDDOT, MIDCOMMA, SKULL };

// Sprites used in sprites.png
enum class Sprite : unsigned short { DIFF_EASY = 0, DIFF_NORMAL = 2, DIFF_HARD = 4, LADY = 6, GENT = 8, ENBY = 10, CURSOR = 12 };

// box() flags
#define BOX_FLAG_DOUBLE			(1 << 0)
#define BOX_FLAG_ALPHA			(1 << 1)
#define BOX_FLAG_OUTER_BORDER	(1 << 2)

// print() flags
#define PRINT_FLAG_NO_NBSP		(1 << 0)
#define PRINT_FLAG_ALPHA		(1 << 1)
#define PRINT_FLAG_PLUS_FOUR_X	(1 << 2)
#define PRINT_FLAG_PLUS_FOUR_Y	(1 << 3)
#define PRINT_FLAG_NO_SPACES	(1 << 4)
#define PRINT_FLAG_PLUS_EIGHT_X	(1 << 5)
#define PRINT_FLAG_PLUS_EIGHT_Y	(1 << 6)
#define PRINT_FLAG_ABSOLUTE		(1 << 7)
#define PRINT_FLAG_ALT_FONT		(1 << 8)

// sprite_print() flags
#define SPRITE_FLAG_PLUS_FOUR	(1 << 0)
#define SPRITE_FLAG_QUAD		(1 << 1)

// yes_no_query() flags
#define YES_NO_FLAG_ANSI			(1 << 0)

// Control keys.
enum { CTRL_A = 1, CTRL_B, CTRL_C, CTRL_D, CTRL_E, CTRL_F, CTRL_G, CTRL_H, CTRL_I, CTRL_J, CTRL_K, CTRL_L, CTRL_M, CTRL_N, CTRL_O, CTRL_P, CTRL_Q, CTRL_R, CTRL_S, CTRL_T, CTRL_U, CTRL_V, CTRL_W, CTRL_X, CTRL_Y, CTRL_Z };

// Shift-arrows.
enum { SHIFT_LEFT = 27, SHIFT_RIGHT, SHIFT_UP, SHIFT_DOWN };

#define RESIZE_KEY			(277 | (1<<30))
#define LMB_KEY				(UINT_MAX - 1)
#define RMB_KEY				(UINT_MAX - 2)
#define MOUSEWHEEL_UP_KEY	(UINT_MAX - 3)
#define MOUSEWHEEL_DOWN_KEY	(UINT_MAX - 4)

// Struct definitions
struct s_glitch
{
	unsigned int x, y, w, h;
	int off_x, off_y;
	bool black;
	SDL_Surface *surf;
};

class IOCore
{
public:
			IOCore();
			~IOCore();				// This is where we clean up our shit.
	void	alagard_print(string message, int x, int y, Colour colour = Colour::CGA_WHITE);	// Prints a string in the Alagard font at the specified coordinates.
	void	alagard_print_at(char letter, int x, int y, Colour colour = Colour::CGA_WHITE);	// Prints an Alagard font character at the specified coordinates.
	void	ansi_print(string msg, int x, int y, unsigned int print_flags = 0, unsigned int dim = 0);	// Prints an ANSI string at the specified position.
	void	box(int x, int y, int w, int h, Colour colour, unsigned char flags = 0, string title = "");	// Renders an ASCII box at the given coordinates.
	void	clear_shade() { shade_mode = 0; }	// Clears 'shade mode' entirely.
	void	cls();					// Clears the screen.
	void	delay(unsigned int ms);	// Calls SDL_Delay but also handles visual glitches.
	bool	did_mouse_click(unsigned short x, unsigned short y, unsigned short w = 1, unsigned short h = 1);	// Checks if the player clicked in a specified area.
	void	exit_functions();		// This is where we clean up our shit.
	void	flip();					// Redraws the display.
	unsigned short	get_cols() { return cols; }		// Returns the number of columns on the screen.
	unsigned short	get_rows() { return rows; }		// Returns the number of rows on the screen.
	void	glitch_intensity(unsigned char value) { glitch_multi = value; }	// Sets the glitch intensity level.
	bool	is_cancel(unsigned int key);	// Returns true if the key is a chosen 'cancel' key.
	bool	is_down(unsigned int key);		// Returns true if the key is a chosen 'down' key.
	bool	is_left(unsigned int key);		// Returns true if the key is a chosen 'left' key.
	bool	is_right(unsigned int key);		// Returns true if the key is a chosen 'right' key.
	bool	is_select(unsigned int key);	// Returns true if the key is a chosen 'select' key.
	bool	is_up(unsigned int key);		// Returns true if the key is a chosen 'up' key.
	string	key_to_name(unsigned int key);	// Returns the name of a key.
	unsigned short	midcol() { return mid_col; }	// Retrieves the middle column on the screen.
	unsigned short	midrow() { return mid_row; }	// Retrieves the middle row on the screen.
	void	ok_box(int offset, Colour colour);	// Renders an OK box on a pop-up window.
	void	parse_colour(Colour colour, unsigned char &r, unsigned char &g, unsigned char &b);	// Parses a colour code into RGB.
	int		print(string message, int x, int y, Colour colour, unsigned int print_flags = 0);	// Prints a message at the specified coordinates.
	int		print(string message, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags = 0);	// Prints a message at the specified coordinates, in RGB colours.
	void	print_at(Glyph letter, int x, int y, Colour colour, unsigned int print_flags = 0);	// Prints a character at a given coordinate on the screen.
	void	print_at(char letter, int x, int y, Colour colour, unsigned int print_flags = 0) { print_at(static_cast<Glyph>(letter), x, y, colour, print_flags); }
	void	print_at(Glyph letter, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags = 0);	// Prints a character at a given coordinate on the screen, in RGB colours.
	void	print_at(char letter, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags = 0) { print_at(static_cast<Glyph>(letter), x, y, r, g, b, print_flags); }
	void	rect(int x, int y, int w, int h, Colour colour);		// Draws a coloured rectangle
	void	rect_fine(int x, int y, int w, int h, Colour colour);	// Draws a rectangle at very specific coords.
	void	rect_fine(int x, int y, int w, int h, s_rgb colour);	// As above, but with direct RGB input.
	void	render_nebula(unsigned short seed, int off_x, int off_y);	// Renders a nebula on the screen.
	void	sleep_for(unsigned int amount);	// Do absolutely nothing for a little while.
	void	sprite_print(Sprite id, int x, int y, Colour colour = Colour::CGA_WHITE, unsigned char print_flags = 0);	// Prints a sprite at the given location.
	void	unlock_surfaces();		// Unlocks the mutexes, if they're locked. Only for use by the Guru system.
	void	update_ntsc_mode(int force = -1);	// Updates the NTSC filter.
	unsigned int	wait_for_key(unsigned short max_ms = 0);	// Polls SDL until a key is pressed. If a time is specified, it will abort after this time.
	bool	yes_no_query(string yn_strings, string yn_title, Colour title_colour, unsigned int flags = 0);	// Renders a yes/no popup box and returns the result.

private:
	SDL_Surface		*font;					// The bitmap font texture.
	SDL_Surface		*alagard;				// The texture for the large bitmap font.
	SDL_Surface		*sprites;				// The texture for larger sprites.
	unsigned short	font_sheet_size;		// The size of the font texture sheet, in glyphs.
	SDL_Window		*main_window;			// The main (and only) SDL window.
	SDL_Surface		*main_surface;			// The main render surface.
	SDL_Surface 	*glitched_main_surface;	// A glitched version of the main render surface.
	SDL_Surface		*window_surface;		// The actual window's surface.
	SDL_Surface		*snes_surface;			// The SNES surface, for rendering the CRT effect.
	SDL_Surface		*temp_surface;			// Temporary surface used for blitting glyphs.
	SDL_Surface		*glitch_hz_surface;		// Horizontal glitch surface.
	SDL_Surface		*glitch_sq_surface;		// Square glitch surface.
	unsigned short	cols, rows, mid_col, mid_row;	// The number of columns and rows available, and the middle column/row.
	std::unordered_map<std::string, s_rgb>	nebula_cache;	// Cache for the nebula() function.
	unsigned short	nebula_cache_seed;		// The seed for the nebula cache.
	int				shade_mode;				// Are we rendering in shade mode?
	int				screen_x, screen_y;		// Chosen screen resolution.
	int				unscaled_x, unscaled_y;	// The unscaled resolution.
	snes_ntsc_t		*ntsc;					// Used by the NTSC filter.
	unsigned char	exit_func_level;		// Keep track of what to clean up at exit.
	bool			hold_glyph_glitches;	// Hold off on glyph glitching right now.
	unsigned char	glitch_multi;			// Glitch intensity multiplier.
	unsigned char	surface_scale;			// The surface scale modifier.
	unsigned short	mouse_clicked_x, mouse_clicked_y;	// Last clicked location for a mouse event.
	std::vector<s_glitch>	glitch_vec;
	unsigned int 	glitch_clear_countdown;
	unsigned char	glitches_queued;
	bool			ntsc_glitched;
	vector<unsigned int>	queued_keys;	// Keypresses waiting to be processed.
	bool			cleaned_up;				// Have we run the exit functions already?

	Colour	adjust_palette(Colour colour);	// Adjusts the colour palette, if needed.
	void	calc_glitches();		// Calculates glitch positions.
	void	glitch(int glitch_x, int glitch_y, int glitch_w, int glitch_h, int glitch_off_x, int glitch_off_y, bool black, SDL_Surface *surf);	// Offsets part of the display.
	void	glitch_horizontal();	// Horizontal displacement visual glitch.
	void	glitch_square();		// Square displacement glitch.
	s_rgb	nebula(int x, int y);	// Determines the colour of a specific point in a nebula, based on X,Y coordinates.
	unsigned char	nebula_rgb(unsigned char value, int modifier);	// Modifies an RGB value in the specified manner, used for rendering nebulae.
	void	render_glitches();		// Renders pre-calculated glitches.
};

extern shared_ptr<IOCore>	iocore;	// External access to the IOCore object.
