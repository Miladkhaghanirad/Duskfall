// iocore.h -- The render core, handling display and user interaction, as well as program initialization, shutdown and cleanup functionality.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"


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
enum class Glyph : unsigned short { FACE = 1, HALF_HEART, HEART, DIAMOND, CLUB, SPADE, BULLET, BULLET_INVERT, CIRCLE, CIRCLE_INVERT, MALE, FEMALE, MUSIC, MUSIC_DOUBLE, SUN, TRIANGLE_RIGHT, TRIANGLE_LEFT, ARROW_UD,
	DOUBLE_EXCLAIM, PILCROW, SECTION, GEOM_A, ARROW_UD_B, ARROW_UP, ARROW_DOWN, ARROW_RIGHT, ARROW_LEFT, RETURN, ARROW_LR, TRIANGLE_UP, TRIANGLE_DOWN, HOUSE = 127, C_CEDILLA_CAPS, U_DIAERESIS, E_ACUTE, A_CIRCUMFLEX, A_DIAERESIS,
	A_GRAVE, A_OVERRING, C_CEDILLA, E_CIRCUMFLEX, E_DIAERESIS, E_GRAVE, I_DIAERESIS, I_CIRCUMFLEX, I_GRAVE, A_DIAERESIS_CAPS, A_OVERRING_CAPS, E_ACUTE_CAPS, AE, AE_CAPS, O_CIRCUMFLEX, O_DIAERESIS, O_GRAVE, U_CIRCUMFLEX, U_GRAVE,
	Y_DIAERESIS, O_DIAERESIS_CAPS, U_DIAERESIS_CAPS, CENT, POUND, YEN, PESETA, F_HOOK, A_ACUTE, I_ACUTE, O_ACUTE, U_ACUTE, N_TILDE, N_TILDE_CAPS, ORDINAL_F, ORDINAL_M, QUESTION_INVERTED, NOT_REVERSE, NOT, HALF, QUARTER,
	EXCLAIM_INVERTED, GUILLEMET_OPEN, GUILLEMET_CLOSE, SHADE_LIGHT, SHADE_MEDIUM, SHADE_HEAVY, LINE_V, LINE_VL, LINE_VLL, LINE_VVL, LINE_DDL, LINE_DLL, LINE_VVLL, LINE_VV, LINE_DDLL, LINE_UULL, LINE_UUL, LINE_ULL, LINE_DL, LINE_UR,
	LINE_UH, LINE_DH, LINE_VR, LINE_H, LINE_VH, LINE_VRR, LINE_VVR, LINE_UURR, LINE_DDRR, LINE_UUHH, LINE_DDHH, LINE_VVRR, LINE_HH, LINE_VVHH, LINE_UHH, LINE_UUH, LINE_DHH, LINE_DDH, LINE_UUR, LINE_URR, LINE_DRR, LINE_DDR, LINE_VVH,
	LINE_VHH, LINE_UL, LINE_DR, BLOCK_SOLID, BLOCK_D, BLOCK_L, BLOCK_R, BLOCK_U, ALPHA, BETA, GAMMA_CAPS, PI_CAPS, SIGMA_CAPS, SIGMA, MU, TAU, PHI_CAPS, THETA_CAPS, OMEGA_CAPS, DELTA, INFINITY, PHI, EPSILON, INTERSECTION,
	TRIPLE_BAR, PLUS_MINUS, GEQ, LEQ, INTEGRAL, INTEGRAL_INVERTED, DIVISION, APPROXIMATION, DEGREE, BULLET_SMALL, INTERPUNCT, SQUARE_ROOT, N_SUPER, SQUARE, CORNER_BLOCK, COPYRIGHT, SKULL = 255 };

// Sprites used in sprites.png
enum class Sprite : unsigned short { DIFF_EASY = 0, DIFF_NORMAL, DIFF_HARD, LADY, GENT, ENBY, CURSOR };

// Keyboard definitions for mouse buttons/wheel and non-ASCII keys.
enum { LMB_KEY = 20000, RMB_KEY, MOUSEWHEEL_UP_KEY, MOUSEWHEEL_DOWN_KEY, KEY_BREAK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_INSERT, KEY_HOME, KEY_PAGEUP, KEY_END,
	KEY_PAGEDOWN, KEY_RIGHT, KEY_LEFT, KEY_UP, KEY_DOWN, KEY_KP_0, KEY_KP_1, KEY_KP_2, KEY_KP_3, KEY_KP_4, KEY_KP_5, KEY_KP_6, KEY_KP_7, KEY_KP_8, KEY_KP_9, KEY_KP_DIVIDE, KEY_KP_MULTIPLY, KEY_KP_PLUS, KEY_KP_MINUS, KEY_KP_PERIOD,
	KEY_KP_ENTER, RESIZE_KEY, KEY_ENTER, KEY_ESCAPE, KEY_BACKSPACE, KEY_TAB };

// alagard_print() flags
#define ALAGARD_FLAG_MINUS_EIGHT_Y	(1 << 0)

// box() flags
#define BOX_FLAG_DOUBLE				(1 << 0)
#define BOX_FLAG_ALPHA				(1 << 1)
#define BOX_FLAG_OUTER_BORDER		(1 << 2)

// print() flags
#define PRINT_FLAG_NO_NBSP			(1 << 0)
#define PRINT_FLAG_ALPHA			(1 << 1)
#define PRINT_FLAG_SANS				(1 << 2)
#define PRINT_FLAG_NO_SPACES		(1 << 3)
#define PRINT_FLAG_PLUS_EIGHT_X		(1 << 4)
#define PRINT_FLAG_PLUS_EIGHT_Y		(1 << 5)
#define PRINT_FLAG_PLUS_FOUR_X		(1 << 6)
#define PRINT_FLAG_PLUS_FOUR_Y		(1 << 7)
#define PRINT_FLAG_PLUS_EIGHT_X_B	(1 << 8)
#define PRINT_FLAG_PLUS_EIGHT_Y_B	(1 << 9)

// yes_no_query() flags
#define YES_NO_FLAG_ANSI		(1 << 0)


namespace iocore
{

Colour	adjust_palette(Colour colour);	// Adjusts the colour palette, if needed.
void	alagard_print(string message, int x, int y, Colour colour = Colour::CGA_WHITE, unsigned int flags = 0);	// Prints a string in the Alagard font at the specified coordinates.
void	alagard_print_at(char letter, int x, int y, Colour colour = Colour::CGA_WHITE, unsigned int flags = 0);	// Prints an Alagard font character at the specified coordinates.
void	ansi_print(string msg, int x, int y, unsigned int print_flags = 0, unsigned int dim = 0);	// Prints an ANSI string at the specified position.
void	box(int x, int y, int w, int h, Colour colour, unsigned char flags = 0, string title = "");	// Renders an ASCII box at the given coordinates.
void	clear(unsigned int x, unsigned int y, unsigned int w, unsigned int h);	// Clears a specified area of the current layer.
void	cls();					// Clears the screen.
void	delay(unsigned int ms);	// Waits for a few milliseconds.
bool	did_mouse_click(unsigned short x, unsigned short y, unsigned short w = 1, unsigned short h = 1);	// Checks if the player clicked in a specified area.
void	exit_functions();		// This is where we clean up our shit.
void	flip();					// Redraws the display.
unsigned short	get_cols();		// Returns the number of columns on the screen.
unsigned char	get_layer();	// Gets the current render layer.
unsigned short	get_rows();		// Returns the number of rows on the screen.
void	init();	// Initializes the main terminal window.
bool	is_cancel(unsigned int key);	// Returns true if the key is a chosen 'cancel' key.
bool	is_down(unsigned int key);		// Returns true if the key is a chosen 'down' key.
bool	is_left(unsigned int key);		// Returns true if the key is a chosen 'left' key.
bool	is_right(unsigned int key);		// Returns true if the key is a chosen 'right' key.
bool	is_select(unsigned int key);	// Returns true if the key is a chosen 'select' key.
bool	is_up(unsigned int key);		// Returns true if the key is a chosen 'up' key.
string	key_to_name(unsigned int key);	// Returns the name of a key.
void	layer(unsigned char new_layer);	// Select the rendering layer.
unsigned short	midcol();	// Retrieves the middle column on the screen.
unsigned short	midrow();	// Retrieves the middle row on the screen.
s_rgb	nebula(int x, int y);	// Determines the colour of a specific point in a nebula, based on X,Y coordinates.
unsigned char	nebula_rgb(unsigned char value, int modifier);	// Modifies an RGB value in the specified manner, used for rendering nebulae.
void	ok_box(int offset, Colour colour);	// Renders an OK box on a pop-up window.
void	parse_colour(Colour colour, unsigned char &r, unsigned char &g, unsigned char &b);	// Parses a colour code into RGB.
int		print(string message, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags = 0);	// Prints a message at the specified coordinates, in RGB colours.
int		print(string message, int x, int y, Colour colour, unsigned int print_flags = 0);	// Prints a message at the specified coordinates.
void	print_at(Glyph letter, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags = 0);	// Prints a character at a given coordinate on the screen, in RGB colours.
void	print_at(char letter, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned int print_flags = 0);	// As above, but given a char rather than a Glyph.
void	print_at(Glyph letter, int x, int y, Colour colour, unsigned int print_flags = 0);	// Prints a character at a given coordinate on the screen.
void	print_at(char letter, int x, int y, Colour colour, unsigned int print_flags = 0);	// As above, but given a char rather than a glyph.
void	recalc_screen_size();	// Recalculates the screen size variables.
void	rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, Colour colour);	// Draws a coloured rectangle
void	rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char r, unsigned char g, unsigned char b);	// As above, with RGB colour.
void	render_nebula(unsigned short seed, int off_x, int off_y);	// Renders a nebula on the screen.
string	rgb_string(unsigned char r, unsigned char g, unsigned char b);			// Creates a BLT RGB string from RGB integers.
string	rgb_string(Colour colour);	// As above, but for a Colour integer.
void	sprite_print(Sprite id, int x, int y, Colour colour = Colour::CGA_WHITE);	// Prints a sprite at the given location.
unsigned int	wait_for_key(unsigned short max_ms = 0);	// Polls BLT until a key is pressed. If a time is specified, it will abort after this time.
bool	yes_no_query(string yn_strings, string yn_title, Colour title_colour, unsigned int flags = 0);	// Renders a yes/no popup box and returns the result.

}	// namespace iocore
