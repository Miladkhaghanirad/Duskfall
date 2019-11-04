// title.cpp -- Animated title screen, based on animated title screen code from Krasten, which in turn was based on animated title screen code from a long-forgotten project.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "filex.h"
#include "guru.h"
#include "hero.h"
#include "iocore.h"
#include "mathx.h"
#include "prefs.h"
#include "strx.h"
#include "title.h"
#include "version.h"
#include "wiki.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"

#include <chrono>
#include <cmath>
#include <fstream>

#include "dungeon.h"	// temp

namespace title
{

#define MAX_SAVE_SLOTS	100	// How many save slots to allow?

unsigned char	menu_pos = 0;	// The current menu position.

static const string static_title[8] = {
		"^219^^219^^219^^219^^287^^222^^219^  ^286^  ^315^^219^^219^^219^^219^  ^285^ ^315^^286^  ^285^^219^^219^^219^^221^ ^315^^219^^219^^219^^219^ ^285^    ^285^",
		"^219^^221^ ^284^^219^^222^^219^  ^219^^221^ ^219^^221^ ^222^^219^ ^222^^219^^315^^219^^283^ ^222^^219^  ^219^^221^ ^219^^221^ ^222^^219^^222^^219^   ^222^^219^",
		"^219^^221^ ^222^^219^^222^^219^  ^219^^221^ ^219^^221^ ^222^^316^ ^222^^219^^222^^283^  ^222^^219^  ^283^  ^219^^221^ ^222^^219^^222^^219^   ^222^^219^",
		"^219^^221^ ^222^^219^^222^^219^  ^219^^221^ ^219^^221^    ^285^^219^^219^^316^  ^285^^219^^220^^287^   ^219^^221^ ^222^^219^^222^^219^   ^222^^219^",
		"^219^^221^ ^222^^219^^222^^219^  ^219^^221^^284^^219^^219^^219^^219^^219^^317^^284^^219^^219^^287^ ^317^^284^^219^^223^^316^  ^284^^219^^219^^219^^219^^219^^222^^219^   ^222^^219^",
		"^219^^221^ ^222^^219^^222^^219^  ^219^^221^    ^222^^219^ ^222^^219^^222^^286^  ^222^^219^     ^219^^221^ ^222^^219^^222^^219^   ^222^^219^",
		"^219^^221^ ^285^^219^^222^^219^  ^219^^221^ ^315^^221^ ^222^^219^ ^222^^219^^317^^219^^286^ ^222^^219^     ^219^^221^ ^222^^219^^222^^219^^221^ ^315^^222^^219^^221^ ^315^",
		"^219^^219^^219^^219^^316^^222^^219^^219^^219^^283^ ^315^^219^^219^^219^^219^^316^ ^222^^219^ ^317^^283^ ^222^^219^     ^219^^221^ ^222^^316^^222^^219^^219^^219^^219^^222^^219^^219^^219^^219^" };

// Animated flames effect.
#define ANIMATED_FLAMES_W	106
#define ANIMATED_FLAMES_H	24
const s_rgb flame_colour_blend[10] = { {0x00,0x00,0x00}, {0x40,0x00,0x00}, {0x80,0x00,0x00}, {0xBF,0x00,0x00}, {0xFF,0x33,0x00}, {0xFF,0x99,0x00}, {0xFF,0xD9,0x00},
		{0xFF,0xF2,0x00}, {0xFF,0xFF,0x40}, {0xFF,0xFF,0x60} };
unsigned char *heat;


// Animates the flames. This can also be used to 'pre-ignite' the fire prior to rendering.
void animate_fire(bool render)
{
	STACK_TRACE();
	const int midrow = iocore::midrow(), midcol = iocore::midcol();
	const bool ntsc_filter = iocore::get_ntsc_filter();
	for (int x = 0; x < 106; x++)
	{
		if (mathx::rnd(5) == 1)	// 1 in 3 chance of flames getting colder.
		{
			if (heat[x] > 1) heat[x]--;
		}
		else if (heat[x] < 9) heat[x]++;	// Otherwise it gets hotter.

		for (int y = 0; y < 24; y++)
		{
			if (y > 0)
			{
				int minus_y = mathx::rnd(3);
				if (y - minus_y < 0) minus_y = y;
				int x_off = mathx::rnd(4) - 2;
				if (x + x_off < 0) x_off = -x;
				else if (x + x_off > 105) x_off = 105 - x;
				unsigned char source = heat[((y - minus_y) * ANIMATED_FLAMES_W) + (x + x_off)];
				if (source > 0) heat[(y * ANIMATED_FLAMES_W) + x] = source - (mathx::rnd(10) == 1 ? 0 : 1);
				else heat[(y * ANIMATED_FLAMES_W) + x] = 0;
			}
			if (render) iocore::rect_fine(((x * 4) + ((midcol - 26) * 8)) * (ntsc_filter ? 1 : 2), (((23 - y) * 4) + ((midrow - 15) * 8)) * (ntsc_filter ? 1 : 2), (ntsc_filter ? 4 : 8), (ntsc_filter ? 4 : 8),
					flame_colour_blend[heat[(y * ANIMATED_FLAMES_W) + x]]);
		}
	}
}

// How much do you hate yourself?
void choose_difficulty()
{
	STACK_TRACE();
	bool redraw = true;
	unsigned short midrow = iocore::midrow(), midcol = iocore::midcol();
	unsigned int selected = 1;
	while(true)
	{
		if (redraw)
		{
			redraw = false;
			midrow = iocore::midrow();
			midcol = iocore::midcol();
			redraw_logo_and_bg();

			const string choose1 = "Choose the DIFFICULTY you would like to play on.";
			const string choose2 = "Please note, this can NOT be changed after you";
			const string choose3 = "have started the game, so pick wisely!";
			iocore::print(choose1, midcol - (choose1.size() / 2), midrow - 6, Colour::AQUA_BRIGHT, PRINT_FLAG_PLUS_FOUR_X | PRINT_FLAG_ALT_FONT);
			iocore::print(choose2, midcol - (choose2.size() / 2), midrow - 5, Colour::AQUA_BRIGHT, PRINT_FLAG_PLUS_FOUR_X | PRINT_FLAG_ALT_FONT);
			iocore::print(choose3, midcol - (choose3.size() / 2), midrow - 4, Colour::AQUA_BRIGHT, PRINT_FLAG_ALT_FONT);

			iocore::sprite_print(Sprite::DIFF_EASY, midcol - 8, midrow - 2, Colour::CGA_LGREEN, SPRITE_FLAG_QUAD);
			iocore::sprite_print(Sprite::DIFF_NORMAL, midcol - 2, midrow - 2, Colour::AQUA_BRIGHT, SPRITE_FLAG_QUAD);
			iocore::sprite_print(Sprite::DIFF_HARD, midcol + 4, midrow - 2, Colour::CGA_LRED, SPRITE_FLAG_QUAD);
			iocore::sprite_print(Sprite::CURSOR, midcol - 6 + (selected * 6), midrow + 2, Colour::CGA_WHITE, SPRITE_FLAG_QUAD);

			string desc;
			switch(selected)
			{
				case 0: desc = "{3D}Playing on {5A}Adventurer difficulty {3D}does not affect the toughness of your foes. Instead, you will retain all your equipment, "
						"items and money upon death, though you will still be sent back to the surface. {5A}Your score will not be counted for the highscore table."; break;
				case 1: desc = "{3D}Playing on {21}Hero difficulty{3D}, death is not the end: upon dying, your equipment, items and part of your soul will remain with your corpse and "
						"you will be revived on the surface. To retrieve your lost belongings, you will have to find your corpse - but if you die again before that happens, your death is final. "
						"{21}This is the recommended difficulty."; break;
				case 2: desc = "{3D}Playing on {5C}Legend difficulty {3D}does not affect the toughness of your foes. Instead, death is permanent, there are no second chances! However, your "
						"final score is doubled in this mode. {5C}This difficulty is unfair and recommended only for veteran players looking for a challenge!"; break;
			}
			vector<string> split = strx::ansi_vector_split(desc, 49);
			iocore::print("Use the LEFT and RIGHT keys to select the game's", midcol - 24, midrow + 6, Colour::CGA_WHITE);
			iocore::print("difficulty, then SPACE or ENTER key to decide.", midcol - 24, midrow + 7, Colour::CGA_WHITE);
			for (unsigned int i = 0; i < split.size(); i++)
			{
				string line = split.at(i);
				iocore::ansi_print(line, midcol - 24, midrow + 9 + i);
			}

			iocore::flip();
		}

		const unsigned int key = iocore::wait_for_key();
		if (key == RESIZE_KEY) redraw = true;
		else if (iocore::is_left(key))
		{
			if (selected > 0) selected--; else selected = 2;
			redraw = true;
		}
		else if (iocore::is_right(key))
		{
			if (selected < 2) selected++; else selected = 0;
			redraw = true;
		}
		else if (iocore::is_select(key))
		{
			world()->hero()->difficulty = selected;
			return;
		}
		else if (key == LMB_KEY)
		{
			bool done = false;
			if (iocore::did_mouse_click(midcol - 8, midrow - 2, 4, 4))
			{
				world()->hero()->difficulty = 0;
				done = true;
			}
			else if (iocore::did_mouse_click(midcol - 2, midrow - 2, 4, 4))
			{
				world()->hero()->difficulty = 1;
				done = true;
			}
			else if (iocore::did_mouse_click(midcol + 4, midrow - 2, 4, 4))
			{
				world()->hero()->difficulty = 2;
				done = true;
			}
			if (done) return;
		}

	}
}

// Picks a name for the character.
void choose_name()
{
	STACK_TRACE();
	bool redraw = true, done = false;
	string name;

	do
	{
		if (redraw)
		{
			redraw = false;
			const unsigned short midrow = iocore::midrow(), midcol = iocore::midcol();
			redraw_logo_and_bg();

			const string choose1 = "Choose a NAME for your new character. If you don't";
			const string choose2 = "want to choose or can't think of anything, just hit";
			const string choose3 = "ENTER and the game will pick a random name for you.";
			iocore::print(choose1, midcol - (choose1.size() / 2), midrow - 6, Colour::AQUA_BRIGHT, PRINT_FLAG_PLUS_FOUR_X | PRINT_FLAG_ALT_FONT);
			iocore::print(choose2, midcol - (choose2.size() / 2), midrow - 5, Colour::AQUA_BRIGHT, PRINT_FLAG_PLUS_FOUR_X | PRINT_FLAG_ALT_FONT);
			iocore::print(choose3, midcol - (choose3.size() / 2), midrow - 4, Colour::AQUA_BRIGHT, PRINT_FLAG_ALT_FONT);
			iocore::print("Use the keyboard to type a name for your", midcol - 24, midrow + 6, Colour::GRAY_PALE);
			iocore::print("character, and hit the ENTER key when done. You", midcol - 24, midrow + 7, Colour::GRAY_PALE);
			iocore::print("can use BACKSPACE to erase letters.", midcol - 24, midrow + 8, Colour::GRAY_PALE);

			const int pos = (iocore::get_cols() * 4) - (name.size() * 12);
			iocore::alagard_print(name, pos, midrow * 8, Colour::CGA_LCYAN);

			iocore::flip();
		}

		unsigned int key = iocore::wait_for_key();
		if (key >= 'a' && key <= 'z' && name.size() < 16) { name += string(1, key - 32); redraw = true; }
		else if (((key >= 'A' && key <= 'Z') || key == ' ') && name.size() < 16) { name += string(1, key); redraw = true; }
		else if (key == SDLK_BACKSPACE && name.size() > 0) { name = name.substr(0, name.size() - 1); redraw = true; }
		else if (key == RESIZE_KEY) redraw = true;
		else if (iocore::is_select(key))
		{
			done = true;
			name = strx::trim_excess_spaces(name);
			if (name.size()) world()->hero()->name = name;
			else
			{
				string new_name;
				unsigned char style = 1;	// replace with check for which style was chosen.
				switch(style)
				{
					case 0: new_name = filex::random_line("data/names/female.txt", 1000); break;
					case 1: new_name = filex::random_line("data/names/surnames.txt", 1997); break;
					case 2: new_name = filex::random_line("data/names/male.txt", 1000); break;
				}
				new_name = strx::str_toupper(new_name);
				const int pos = (iocore::get_cols() * 4) - (new_name.size() * 12);
				iocore::alagard_print(new_name, pos, iocore::midrow() * 8, Colour::CGA_LCYAN);
				iocore::flip();
				iocore::sleep_for(1000);
				world()->hero()->name = new_name;
			}
		}
	} while(!done);
}

// Choose your character's style.
void choose_style()
{
	STACK_TRACE();
	bool redraw = true;
	unsigned short midrow = iocore::midrow(), midcol = iocore::midcol();
	unsigned char chosen_style = 1;
	while(true)
	{
		if (redraw)
		{
			redraw = false;
			midrow = iocore::midrow();
			midcol = iocore::midcol();
			redraw_logo_and_bg();

			const string choose1 = "Choose a STYLE for your new character or NEUTRAL if";
			const string choose2 = "you prefer not to choose either. Your character's";
			const string choose3 = "style will not affect their stats or skills.";
			iocore::print(choose1, midcol - (choose1.size() / 2), midrow - 6, Colour::AQUA_BRIGHT, PRINT_FLAG_PLUS_FOUR_X | PRINT_FLAG_ALT_FONT);
			iocore::print(choose2, midcol - (choose2.size() / 2), midrow - 5, Colour::AQUA_BRIGHT, PRINT_FLAG_PLUS_FOUR_X | PRINT_FLAG_ALT_FONT);
			iocore::print(choose3, midcol - (choose3.size() / 2), midrow - 4, Colour::AQUA_BRIGHT, PRINT_FLAG_ALT_FONT);

			iocore::sprite_print(Sprite::LADY, midcol - 8, midrow - 2, Colour::MAGENTA_PALE, SPRITE_FLAG_QUAD);
			iocore::sprite_print(Sprite::ENBY, midcol - 2, midrow - 2, Colour::CGA_LGREEN, SPRITE_FLAG_QUAD);
			iocore::sprite_print(Sprite::GENT, midcol + 4, midrow - 2, Colour::AQUA_BRIGHT, SPRITE_FLAG_QUAD);
			iocore::sprite_print(Sprite::CURSOR, midcol - 6 + (chosen_style * 6), midrow + 2, Colour::CGA_WHITE, SPRITE_FLAG_QUAD);

			string desc;
			switch(chosen_style)
			{
				case 0: desc = "{3D}While style has no bearing on your character's stats, skills or abilities, choosing a {34}FEMININE {3D}style will have "
						"characters in the game refer to you with feminine pronouns and terms."; break;
				case 1: desc = "{3D}Selecting a {5A}NEUTRAL {3D}style means that people you interact with in the game world will not use gendered pronouns, "
						"and will instead refer to you by neutral terms such as 'adventurer'."; break;
				case 2: desc = "{3D}While style has no bearing on your character's stats, skills or abilities, choosing a {21}MASCULINE {3D}style will have "
						"characters in the game refer to you with masculine pronouns and terms."; break;
			}
			vector<string> split = strx::ansi_vector_split(desc, 49);
			iocore::print("Use the LEFT and RIGHT keys to select your", midcol - 24, midrow + 6, Colour::CGA_WHITE);
			iocore::print("chosen style, then SELECT or ENTER to decide.", midcol - 24, midrow + 7, Colour::CGA_WHITE);
			for (unsigned int i = 0; i < split.size(); i++)
			{
				string line = split.at(i);
				iocore::ansi_print(line, midcol - 24, midrow + 9 + i);
			}

			iocore::flip();
		}
		const unsigned int key = iocore::wait_for_key();
		if (key == RESIZE_KEY) redraw = true;
		if (iocore::is_down(key) || iocore::is_right(key) || key == MOUSEWHEEL_DOWN_KEY)
		{
			if (++chosen_style > 2) chosen_style = 0;
			redraw = true;
		}
		else if (iocore::is_up(key) || iocore::is_left(key) || key == MOUSEWHEEL_UP_KEY)
		{
			if (!chosen_style) chosen_style = 2; else chosen_style--;
			redraw = true;
		}
		else if (iocore::is_select(key))
		{
			world()->hero()->style = chosen_style;
			return;
		}
		else if (key == LMB_KEY || key == RMB_KEY)
		{
			const unsigned short midrow = iocore::midrow(), midcol = iocore::midcol();
			if (iocore::did_mouse_click(midcol - 8, midrow - 2, 4, 4))
			{
				if (key == LMB_KEY)
				{
					world()->hero()->style = 0;
					return;
				}
				else redraw = true;
			}
			if (iocore::did_mouse_click(midcol - 2, midrow - 2, 4, 4))
			{
				if (key == LMB_KEY)
				{
					world()->hero()->style = 1;
					return;
				}
				else redraw = true;
			}
			if (iocore::did_mouse_click(midcol + 4, midrow - 2, 4, 4))
			{
				if (key == LMB_KEY)
				{
					world()->hero()->style = 2;
					return;
				}
			}
		}
	}
}

// Display the copyright window.
void copyright_window()
{
	STACK_TRACE();

	vector<string> copyright;
	std::ifstream copyright_file("copyright.txt");
	if (!copyright_file.is_open()) guru::halt("Could not load copyright.txt");
	string line;
	while (getline(copyright_file, line))
	{
		if (!line.size()) copyright.push_back("");
		else if (line == "-") copyright.push_back("{{PB}}");
		else
		{
			strx::find_and_replace(line, "é", "^130^");
			vector<string> line_vec = strx::ansi_vector_split(line, 53);
			copyright.insert(copyright.end(), line_vec.begin(), line_vec.end());
		}
	}
	copyright_file.close();

	iocore::render_nebula(12811, 0, 0);
	string copyright_title = "COPYRIGHT & LICENSE INFORMATION";
	const int midrow = iocore::midrow(), midcol = iocore::midcol();
	const int title_x = midcol - (copyright_title.size() / 2);
	iocore::box(midcol - 27, midrow - 18, 55, 37, UI_COLOUR_BOX);
	iocore::ok_box(18, UI_COLOUR_BOX);
	iocore::print(copyright_title, title_x, midrow - 18, UI_COLOUR_LABEL);
	iocore::print_at(Glyph::LINE_VL, title_x - 1, midrow - 18, UI_COLOUR_BOX);
	iocore::print_at(Glyph::LINE_VR, title_x + copyright_title.size(), midrow - 18, UI_COLOUR_BOX);

	unsigned int screen_line = 1, copyright_line = 0, next_break = 0;
	do
	{
		if (screen_line == 1 && !copyright.at(copyright_line).size()) copyright_line++;	// Don't start with a blank line.
		if (copyright.at(copyright_line).compare("{{PB}}")) iocore::ansi_print(copyright.at(copyright_line), midcol - 26, midrow - 18 + screen_line++, PRINT_FLAG_ALT_FONT);

		// Figure out the best place to stop for the next screen.
		if (!next_break || next_break < copyright_line)
		{
			for (int i = 0; i <= 34; i++)
			{
				if (copyright_line + i >= copyright.size()) { next_break = copyright.size(); break; }
				const string line = copyright.at(copyright_line + i);
				if (!line.size()) next_break = copyright_line + i;
				else if (!line.compare("{{PB}}")) { next_break = copyright_line + i; break; }
			}
		}

		if (++copyright_line >= copyright.size() || next_break == copyright_line)
		{
			iocore::flip();
			iocore::wait_for_key();
			iocore::rect(midcol - 26, midrow - 17, 53, 34, Colour::CGA_BLACK);
			screen_line = 1;
		}

		if (copyright_line >= copyright.size()) break;
	} while(true);
}

// Displays the first-time glitch warning screen.
void glitch_warning()
{
	STACK_TRACE();
	while(true)
	{
		const int midrow = iocore::midrow(), midcol = iocore::midcol();
		iocore::box(midcol - 27, midrow - 18, 55, 37, UI_COLOUR_BOX);
		vector<string> message = strx::ansi_vector_split("Duskfall is a text-based game and with the following exception, does not include flashing colours or images. However: {nl} "
				"The game by default uses a 'glitching' screen effect, which causes the display to sometimes flicker and distort, to imitate an unreliable old computer screen. {nl} "
				"{5F}This can occasionally result in flickering or flashing colours which may affect players who have epilepsy. Some players may also simply dislike this effect. {nl} "
				"{5F}This message will only be shown once. Please decide now if you would like to enable or disable visual glitches, by {5F}pressing the {5A}Y {5F}key to enable glitches, or the "
				"{5C}N {5F}key to disable glitches. {nl} {57}This option can be changed again later at any time via the in-game preferences menu.", 53);
		for (unsigned int i = 0; i < 8; i++)
		{
			string line_colour;
			switch(i)
			{
				case 0: case 7: line_colour = "{41}"; break;
				case 1: case 6: line_colour = "{02}"; break;
				case 4: line_colour = "{22}"; break;
				default: line_colour = "{12}"; break;
			}
			iocore::ansi_print("^000^   " + line_colour + static_title[i], midcol - 26, midrow - 17 + i, PRINT_FLAG_PLUS_EIGHT_X);
		}
		iocore::ansi_print("       {5B}* * * {5A}IMPORTANT EPILEPSY WARNING {5B}* * *", midcol - 26, midrow - 6);
		for (unsigned int i = 0; i < message.size(); i++)
			iocore::ansi_print(message.at(i), midcol - 26, midrow - 4 + i);
		iocore::flip();
		const unsigned int key = iocore::wait_for_key();
		if (key == RESIZE_KEY) glitch_warning();
		if (key == 'Y' || key == 'y')
		{
			prefs::visual_glitches = 2;
			prefs::glitch_warn = true;
			prefs::save();
			return;
		}
		else if (key == 'N' || key == 'n')
		{
			prefs::visual_glitches = 0;
			prefs::glitch_warn = true;
			prefs::save();
			return;
		}
	}
}

// Loads a saved game.
void load_game(int slot)
{
	STACK_TRACE();
	const string save_dir = "userdata/save/" + strx::itos(slot);
	const string save_file = save_dir + "/" + "save.dat";

	new_world(slot, false);
	world()->load();
	world()->main_loop();
}

// Starts a new game!
bool new_game(int slot, bool start_over)
{
	STACK_TRACE();
	if (start_over && !iocore::yes_no_query("{5F}Choosing this option will {35}permanently|{35}erase your existing save file {5F}and any|{5F}progress you may have made. "
			"Are you|{5F}sure this is what you want to do?", "WARNING", Colour::CGA_LRED, YES_NO_FLAG_ANSI)) return false;

	const string save_dir = "userdata/save/" + strx::itos(slot);
	const string save_file = save_dir + "/" + "save.dat";

	// Delete any old save files, then create a new save directory.
	filex::remove_directory(save_dir);
	if (start_over) return true;
	filex::make_dir("userdata/save");
	filex::make_dir(save_dir.c_str());

	// Create the World object!
	new_world(slot, true);

	// Character creation.
	choose_difficulty();
	choose_style();
	choose_name();

	// Clean up memory used by the animated flames.
	delete[] heat; heat = nullptr;

	// Start a new game and begin the loop!
	world()->new_game();
	world()->main_loop();
	return true;
}

// Redraws the animated logo every frame.
void redraw_animated_logo()
{
	STACK_TRACE();
	const int midrow = iocore::midrow(), midcol = iocore::midcol();
	iocore::box(midcol - 27, midrow - 16, 55, 14, UI_COLOUR_BOX);
	iocore::print(" Damn the torpedoes! Full speed ahead! ", midcol - 19, midrow - 16, Colour::AQUA_PALE, PRINT_FLAG_NO_NBSP | PRINT_FLAG_ALT_FONT);
	iocore::print_at(Glyph::LINE_VL, midcol - 20, midrow - 16, UI_COLOUR_BOX);
	iocore::print_at(Glyph::LINE_VR, midcol + 20, midrow - 16, UI_COLOUR_BOX);

	const string edition_str = " " + strx::itos(DUSKFALL_VERSION_EPOCH) + "." + strx::itos(DUSKFALL_VERSION_MAJOR) + " ^196^ " + DUSKFALL_EDITION + " ";
	const unsigned int edition_len = strx::ansi_strlen(edition_str);
	const unsigned int edition_pos = midcol - (edition_len / 2);
	const bool offset = !mathx::is_odd(edition_len);
	iocore::print(edition_str, edition_pos, midrow - 3, Colour::AQUA_PALE, PRINT_FLAG_NO_NBSP | PRINT_FLAG_PLUS_FOUR_Y | (offset ? PRINT_FLAG_PLUS_EIGHT_X : 0) | PRINT_FLAG_ALT_FONT);
	iocore::print_at(Glyph::LINE_VL, edition_pos - 1, midrow - 3, UI_COLOUR_BOX, (offset ? PRINT_FLAG_PLUS_EIGHT_X : 0));
	iocore::print_at(Glyph::LINE_VR, edition_pos + edition_len, midrow - 3, UI_COLOUR_BOX, (offset ? PRINT_FLAG_PLUS_EIGHT_X : 0));

	// Render the flames!
	animate_fire(true);

	// Draw the logo on top of said flames.
	redraw_static_logo(0);
}

// Redraws the background when needed.
void redraw_background()
{
	STACK_TRACE();
	iocore::cls();
	iocore::render_nebula(57720, 0, 0);
	const int midrow = iocore::midrow(), midcol = iocore::midcol();
	string version = "Welcome to          v" + DUSKFALL_VERSION_STRING;
	bool shunt_back = false;
	if (version.size() == 32) { version.erase(21, 1); shunt_back = true; }
	const string copyright = "Copyright C 2016-2019 R.Simmons";
	const string copyright2 = "and the Duskfall contributors";
	const string copyright3 = "See Copyright/License for all  ";
	if (version.size() < 31 && !mathx::is_odd(version.size())) version += "!";
	const int version_pos = midcol - (version.size() / 2);
	const int copyright_pos = midcol - (copyright.size() / 2);
	const int copyright2_pos = midcol - (copyright2.size() / 2);
	const int copyright3_pos = midcol - (copyright3.size() / 2);

	iocore::box(midcol - 16, midrow - 1, 33, 6, UI_COLOUR_BOX);
	iocore::print(version, version_pos, midrow, Colour::CGA_WHITE, PRINT_FLAG_ALT_FONT);
	iocore::print("DUSKFALL", version_pos + 11, midrow, Colour::CGA_LGREEN);
	iocore::print(DUSKFALL_VERSION_STRING, version_pos + 21 - (shunt_back ? 1 : 0), midrow, Colour::CGA_LGREEN, PRINT_FLAG_ALT_FONT);
	iocore::print(copyright, copyright_pos, midrow + 1, Colour::CGA_WHITE, PRINT_FLAG_ALT_FONT);
	iocore::print(copyright2, copyright2_pos, midrow + 2, Colour::CGA_WHITE, PRINT_FLAG_ALT_FONT);
	iocore::print(copyright3, copyright3_pos, midrow + 3, Colour::CGA_WHITE, PRINT_FLAG_ALT_FONT);
	iocore::print_at(Glyph::COPYRIGHT, copyright_pos + 10, midrow + 1, Colour::CGA_WHITE);
	iocore::print_at(Glyph::COPYRIGHT, copyright3_pos + 30, midrow + 3, Colour::CGA_WHITE);

	redraw_menu();
}

// Does exactly what it says on the tin.
void redraw_logo_and_bg()
{
	STACK_TRACE();
	const unsigned short midrow = iocore::midrow(), midcol = iocore::midcol();
	iocore::cls();
	iocore::render_nebula(57720, 0, 0);
	iocore::box(midcol - 27, midrow - 18, 55, 37, UI_COLOUR_BOX);
	redraw_static_logo(-3);
	iocore::box(midcol - 25, midrow + 5, 51, 13, UI_COLOUR_BOX);
}

// Redraws the title menu.
void redraw_menu()
{
	STACK_TRACE();
	const int midrow = iocore::midrow(), midcol = iocore::midcol();
	iocore::box(midcol - 18, midrow + 8, 9, 9, UI_COLOUR_BOX);
	iocore::box(midcol + 10, midrow + 8, 9, 9, UI_COLOUR_BOX);
	iocore::box(midcol - 25, midrow - 1, 9, 9, UI_COLOUR_BOX);
	iocore::box(midcol + 17, midrow - 1, 9, 9, UI_COLOUR_BOX);
	iocore::box(midcol - 27, midrow + 8, 9, 9, UI_COLOUR_BOX);
	iocore::box(midcol + 19, midrow + 8, 9, 9, UI_COLOUR_BOX);
	iocore::print_at(Glyph::LINE_VR, midcol - 10, midrow + 12, UI_COLOUR_BOX, PRINT_FLAG_PLUS_FOUR_Y);
	iocore::print_at(Glyph::LINE_VL, midcol + 10, midrow + 12, UI_COLOUR_BOX, PRINT_FLAG_PLUS_FOUR_Y);
	iocore::print_at(Glyph::LINE_VR, midcol - 19, midrow + 12, UI_COLOUR_BOX, PRINT_FLAG_PLUS_FOUR_Y);
	iocore::print_at(Glyph::LINE_VL, midcol + 19, midrow + 12, UI_COLOUR_BOX, PRINT_FLAG_PLUS_FOUR_Y);
	iocore::print_at(Glyph::LINE_VL, midcol - 18, midrow + 12, UI_COLOUR_BOX, PRINT_FLAG_PLUS_FOUR_Y);
	iocore::print_at(Glyph::LINE_VR, midcol + 18, midrow + 12, UI_COLOUR_BOX, PRINT_FLAG_PLUS_FOUR_Y);
	iocore::print_at(Glyph::LINE_UH, midcol - 22, midrow + 8, UI_COLOUR_BOX);
	iocore::print_at(Glyph::LINE_DH, midcol - 22, midrow + 7, UI_COLOUR_BOX);
	iocore::print_at(Glyph::LINE_UH, midcol + 22, midrow + 8, UI_COLOUR_BOX);
	iocore::print_at(Glyph::LINE_DH, midcol + 22, midrow + 7, UI_COLOUR_BOX);

	render_floppy(midcol - 17, midrow + 9, Colour::BLUE_LIGHT, (menu_pos == 0));
	render_floppy(midcol + 11, midrow + 9, Colour::BLUE_LIGHT, (menu_pos == 0));
	render_floppy(midcol - 26, midrow + 9, Colour::RED, (menu_pos >= 1 && menu_pos <= 3));
	render_floppy(midcol + 20, midrow + 9, Colour::RED, (menu_pos >= 1 && menu_pos <= 3));
	render_floppy(midcol - 24, midrow, Colour::GREEN_LIGHT, (menu_pos == 4));
	render_floppy(midcol + 18, midrow, Colour::GREEN_LIGHT, (menu_pos == 4));

	iocore::box(midcol - 9, midrow + 6, 19, 13, UI_COLOUR_BOX);

	iocore::print(menu_pos == 0 ? "BEGIN ADVENTURE" : "Begin Adventure", midcol - 7, midrow + 8, menu_pos == 0 ? Colour::YELLOW_PURE : Colour::CGA_LGRAY);
	iocore::print(menu_pos == 1 ? "GAME MANUAL" : "Game Manual", midcol - 5, midrow + 10, menu_pos == 1 ? Colour::YELLOW_PURE : Colour::CGA_LGRAY);
	iocore::print(menu_pos == 2 ? "PREFS/KEYBINDINGS" : "Prefs/KeyBindings", midcol - 8, midrow + 12, menu_pos == 2 ? Colour::YELLOW_PURE : Colour::CGA_LGRAY);
	iocore::print(menu_pos == 3 ? "COPYRIGHT/LICENSE" : "Copyright/License", midcol - 8, midrow + 14, menu_pos == 3 ? Colour::YELLOW_PURE : Colour::CGA_LGRAY);
	iocore::print(menu_pos == 4 ? "QUIT TO DESKTOP" : "Quit to Desktop", midcol - 7, midrow + 16, menu_pos == 4 ? Colour::YELLOW_PURE : Colour::CGA_LGRAY);
	iocore::print_at(Glyph::LINE_VL, midcol - 9, midrow + 12, UI_COLOUR_BOX, PRINT_FLAG_PLUS_FOUR_Y);
	iocore::print_at(Glyph::LINE_VR, midcol + 9, midrow + 12, UI_COLOUR_BOX, PRINT_FLAG_PLUS_FOUR_Y);
	iocore::print_at(Glyph::LINE_VL, midcol - 3, midrow + 18, UI_COLOUR_BOX);
	iocore::print_at(Glyph::ARROW_UP, midcol - 2, midrow + 18, (menu_pos ? Colour::CGA_LGREEN : Colour::CGA_GRAY));
	iocore::print_at(static_cast<Glyph>('/'), midcol - 1, midrow + 18, Colour::CGA_LGRAY);
	iocore::print_at(Glyph::ARROW_DOWN, midcol, midrow + 18, (menu_pos < 4 ? Colour::CGA_LGREEN : Colour::CGA_GRAY));
	iocore::print_at(static_cast<Glyph>('/'), midcol + 1, midrow + 18, Colour::CGA_LGRAY);
	iocore::print_at(Glyph::RETURN, midcol + 2, midrow + 18, Colour::CGA_LGREEN);
	iocore::print_at(Glyph::LINE_VR, midcol + 3, midrow + 18, UI_COLOUR_BOX);
	iocore::flip();
}

// Redraws the static logo over the top of the flames.
void redraw_static_logo(int offset)
{
	STACK_TRACE();
	const int midrow = iocore::midrow(), midcol = iocore::midcol();
	for (int i = 3; i <= 4; i++)
	{
		for (int x = 1; x < 54; x++)
		{
			if (i == 3) iocore::print_at(Glyph::BLOCK_U, midcol - 27 + x, midrow - 14 + i + offset, Colour::BLACK_LIGHT, PRINT_FLAG_PLUS_EIGHT_Y);
			else iocore::print_at(Glyph::BLOCK_D, midcol - 27 + x, midrow - 13 + i + offset, Colour::BLACK_LIGHT, PRINT_FLAG_PLUS_EIGHT_Y);
			iocore::print_at(Glyph::BLOCK_SOLID, midcol - 27 + x, midrow - 13 + i + offset, Colour::TERM_BLUE);
		}
	}
	for (int i = 0; i < 8; i++)
	{
		iocore::print(static_title[i], midcol - 23, midrow - 13 + i + offset, Colour::BLACK_LIGHT, PRINT_FLAG_NO_SPACES | PRINT_FLAG_ALPHA);
		iocore::print(static_title[i], midcol - 22, midrow - 13 + i + offset, Colour::BLACK_LIGHT, PRINT_FLAG_NO_SPACES | PRINT_FLAG_ALPHA);
		iocore::print(static_title[i], midcol - 23, midrow - 13 + i + offset, Colour::BLACK_LIGHT, PRINT_FLAG_NO_SPACES | PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_EIGHT_Y  | PRINT_FLAG_PLUS_EIGHT_X);
		iocore::print(static_title[i], midcol - 23, midrow - 14 + i + offset, Colour::BLACK_LIGHT, PRINT_FLAG_NO_SPACES | PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_EIGHT_Y  | PRINT_FLAG_PLUS_EIGHT_X);
	}
	for (int i = 0; i < 8; i++)
	{
		Colour col;
		switch(i)
		{
			case 0: case 7: case 8: col = Colour::TERM_BLUE; break;
			case 1: case 6: col = Colour::BLUE; break;
			case 2: case 5: col = Colour::BLUE_LIGHT; break;
			case 3: case 4: col = Colour::BLUE_BRIGHT; break;
		}
		iocore::print(static_title[i], midcol - 23, midrow - 13 + i + offset, col, PRINT_FLAG_NO_SPACES | PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_EIGHT_X);
	}
}

// Renders a floppy disk!
void render_floppy(int x, int y, Colour colour, bool front)
{
	STACK_TRACE();

	auto dark_col = colour;
	if (colour == Colour::BLUE_LIGHT) dark_col = Colour::TERM_LBLUE;
	else if (colour == Colour::RED) dark_col = Colour::TERM_RED;
	else if (colour == Colour::GREEN_LIGHT) dark_col = Colour::LIME_LIGHT;

	// The plastic disk body.
	for (int sx = 0; sx < 7; sx++)
		for (int sy = 0; sy < 7; sy++)
			iocore::print_at(Glyph::BLOCK_SOLID, x + sx, y + sy, colour);

	// The metal disk cover.
	for (int sx = 1; sx < 5; sx++)
		for (int sy = 4; sy < 7; sy++)
			iocore::print_at(Glyph::BLOCK_SOLID, x + sx, y + sy, Colour::GRAY_PALE, PRINT_FLAG_PLUS_EIGHT_X);

	// The hole in the disk cover.
	iocore::print_at(Glyph::BLOCK_SOLID, x + (front ? 2 : 4), y + 4, dark_col, PRINT_FLAG_PLUS_EIGHT_Y);
	iocore::print_at(Glyph::BLOCK_SOLID, x + (front ? 2 : 4), y + 5, dark_col, PRINT_FLAG_PLUS_EIGHT_Y);

	// The indent next to the slide.
	iocore::print_at(Glyph::BLOCK_SOLID, x + (front ? 5 : 0), y + 4, dark_col, PRINT_FLAG_PLUS_EIGHT_X);
	iocore::print_at(Glyph::BLOCK_SOLID, x + (front ? 5 : 0), y + 5, dark_col, PRINT_FLAG_PLUS_EIGHT_X);
	iocore::print_at(Glyph::BLOCK_SOLID, x + (front ? 5 : 0), y + 6, dark_col, PRINT_FLAG_PLUS_EIGHT_X);

	// The read/write tab at the corner.
	iocore::print_at(Glyph::BLOCKS_4, x + (front ? 6 : 0), y, dark_col, PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_FOUR_X);
	iocore::print_at(Glyph::BLOCKS_4, x + (front ? 0 : 6), y, Colour::BLACK_LIGHT, PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_FOUR_X);

	// The insert arrow at the top front, and the HD logo.
	if (front)
	{
		iocore::print_at(Glyph::ARROW_DOWN, x + 6, y + 6, dark_col, PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_FOUR_X);
		iocore::print_at(Glyph::UPSIDE_DOWN_HD, x, y + 6, Colour::CGA_LGRAY, PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_FOUR_X);
	}

	// The cipped corner.
	iocore::print_at((front ? Glyph::CORNER_CLIP_DL : Glyph::CORNER_CLIP_DR), x + (front ? 0 : 6), y + 6, Colour::BLACK_LIGHT, PRINT_FLAG_ALPHA);

	// The disk label.
	int label_max = (front ? 3 : 1);
	for (int sx = 1; sx < 6; sx++)
	{
		for (int sy = 0; sy < label_max; sy++)
		{
			iocore::print_at(Glyph::BLOCK_SOLID, x + sx, y + sy, Colour::WHITE);
		}
	}
	if (front)
	{
		iocore::print("____", x + 1, y - 1, Colour::GRAY_PALE, PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_EIGHT_X | PRINT_FLAG_PLUS_EIGHT_Y);
		iocore::print("____", x + 1, y, Colour::GRAY_PALE, PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_EIGHT_X | PRINT_FLAG_PLUS_EIGHT_Y);
		iocore::print("____", x + 1, y + 1, Colour::GRAY_PALE, PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_EIGHT_X | PRINT_FLAG_PLUS_EIGHT_Y);
	}

	// The rear metal disc.
	if (!front)
	{
		iocore::print_at(Glyph::CURVE_DR, x + 2, y + 1, Colour::GRAY_PALE, PRINT_FLAG_PLUS_EIGHT_X | PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_EIGHT_Y);
		iocore::print_at(Glyph::CURVE_DL, x + 3, y + 1, Colour::GRAY_PALE, PRINT_FLAG_PLUS_EIGHT_X | PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_EIGHT_Y);
		iocore::print_at(Glyph::CURVE_UR, x + 2, y + 2, Colour::GRAY_PALE, PRINT_FLAG_PLUS_EIGHT_X | PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_EIGHT_Y);
		iocore::print_at(Glyph::CURVE_UL, x + 3, y + 2, Colour::GRAY_PALE, PRINT_FLAG_PLUS_EIGHT_X | PRINT_FLAG_ALPHA | PRINT_FLAG_PLUS_EIGHT_Y);
		iocore::print_at(Glyph::BLOCK_SOLID, x + 3, y + 2, Colour::GRAY_PALE, PRINT_FLAG_ALPHA);
		iocore::print_at(Glyph::FLOPPY_DISK_METAL_HOLE, x + 3, y + 2, colour, PRINT_FLAG_ALPHA);
	}
}

// Selects which slot to load, or start a new game.
void select_save_slot()
{
	STACK_TRACE();

	vector<string> slot_names, slot_times, slot_tiers;
	vector<int> slot_hearts, slot_poisoned, slot_respawns, slot_dead, slot_difficulty;
	for (int i = 0; i < MAX_SAVE_SLOTS; i++)
	{
		string tag_file = "userdata/save/" + strx::itos(i) + "/tag.dat";
		if (filex::file_exists(tag_file))
		{
			std::ifstream file_tag(tag_file);
			string line;
			getline(file_tag, line); slot_names.push_back(line);
			getline(file_tag, line); slot_times.push_back(line);
			getline(file_tag, line); slot_tiers.push_back(line);
			getline(file_tag, line); slot_respawns.push_back(std::stoi(line));
			getline(file_tag, line); slot_hearts.push_back(std::stoi(line));
			getline(file_tag, line); slot_poisoned.push_back(std::stoi(line));
			getline(file_tag, line); slot_difficulty.push_back(std::stoi(line));
			getline(file_tag, line); slot_dead.push_back(line.size());
			file_tag.close();
		}
		else
		{
			slot_names.push_back("EMPTY SAVE SLOT");
			slot_times.push_back("--:--:--");
			slot_tiers.push_back("");
			slot_respawns.push_back(0);
			slot_hearts.push_back(0);
			slot_poisoned.push_back(0);
			slot_difficulty.push_back(3);
			slot_dead.push_back(0);
		}
	}

	unsigned int slot_pos = 0, slot_offset = 0;
	unsigned int slots_visible = (iocore::get_rows() - 1) / 12;
	unsigned int midcol = iocore::midcol();

	while(true)
	{
		iocore::cls();
		iocore::box(midcol - 27, 0, 54, iocore::get_rows(), UI_COLOUR_BOX);

		for (unsigned int i = slot_offset; i < slot_offset + slots_visible; i++)
		{
			const unsigned int y_pos = ((i - slot_offset) * 12) + 1;
			iocore::box(midcol - 26, y_pos, 52, 10, (i == slot_pos ? Colour::YELLOW_PURE : Colour::GRAY_LIGHT));
			iocore::alagard_print(slot_names.at(i), (midcol - 25) * 8, (y_pos + 1) * 8, (i == slot_pos ? Colour::WHITE : Colour::GRAY_LIGHT));
			iocore::print(slot_times.at(i), midcol + 25 - slot_times.at(i).size(), y_pos + 7, (i == slot_pos ? Colour::WHITE : Colour::GRAY_LIGHT));
			if (slot_tiers.at(i).size())
			{
				string death_str, tier_colour = "{21}";
				Sprite diff_sprite = Sprite::DIFF_NORMAL;
				Colour diff_col = Colour::AQUA_BRIGHT;
				switch(slot_difficulty.at(i))
				{
					case 0: diff_sprite = Sprite::DIFF_EASY; tier_colour = "{5A}"; diff_col = Colour::CGA_LGREEN; break;
					case 2: diff_sprite = Sprite::DIFF_HARD; tier_colour = "{5C}"; diff_col = Colour::CGA_LRED; break;
				}

				if (slot_respawns.at(i))
					death_str = (i == slot_pos ? " {5F}^329^x" : " {57}^329^x") + strx::itos(slot_respawns.at(i));
				iocore::ansi_print(tier_colour + "Rank " + slot_tiers.at(i) + death_str, midcol - 20, y_pos + 5);
				if (slot_difficulty.at(i) < 3) iocore::sprite_print(diff_sprite, midcol - 25, y_pos + 5, diff_col, SPRITE_FLAG_QUAD);

				if (slot_dead.at(i)) iocore::print("^042^ DECEASED", midcol - 20, y_pos + 7, Colour::CGA_LRED);
				else
				{
					const int hero_hearts = slot_hearts.at(i);
					for (int j = 0; j < 10; j++)
					{
						Colour heart_colour = Colour::CGA_LRED;
						if (slot_poisoned.at(i)) heart_colour = Colour::CGA_LGREEN;
						const int qual = (j + 1) * 2;
						bool full_gray = true, full_red = false, half_red = false;
						if (hero_hearts >= qual) { full_red = true; full_gray = false; }
						else if (hero_hearts >= qual - 1) half_red = true;

						// Ensure it's not all gray if some health remains.
						if (j == 0 && full_gray && !half_red) half_red = true;

						if (full_gray) iocore::print_at(Glyph::HEART, midcol - 20 + j, y_pos + 7, Colour::GRAY_DARK);
						else if (full_red) iocore::print_at(Glyph::HEART, midcol - 20 + j, y_pos + 7, heart_colour);
						if (half_red) iocore::print_at(Glyph::HALF_HEART, midcol - 20 + j, y_pos + 7, heart_colour, PRINT_FLAG_ALPHA);
					}
				}
			}
		}

		iocore::flip();
		unsigned int key = iocore::wait_for_key();
		if (key == RESIZE_KEY)
		{
			slots_visible = (iocore::get_rows() - 1) / 12;
			slot_pos = slot_offset = 0;
			midcol = iocore::midcol();
		}
		if (iocore::is_up(key) || key == MOUSEWHEEL_UP_KEY)
		{
			if (slot_pos > 0)
			{
				slot_pos--;
				if (static_cast<signed int>(slot_pos) - static_cast<signed int>(slot_offset) < 0) slot_offset = slot_pos;
			}
		}
		else if (iocore::is_down(key) || key == MOUSEWHEEL_DOWN_KEY)
		{
			if (slot_pos < (MAX_SAVE_SLOTS - 1))
			{
				slot_pos++;
				if (slot_pos - slot_offset >= slots_visible) slot_offset++;
			}
		}
		else if (iocore::is_cancel(key) || key == RMB_KEY) return;
		else if (iocore::is_select(key) || key == LMB_KEY)
		{
			bool valid_selection = true;
			if (key == LMB_KEY)
			{
				valid_selection = false;
				for (unsigned int i = slot_offset; i < slot_offset + slots_visible; i++)
				{
					const unsigned int y_pos = ((i - slot_offset) * 12) + 1;
					if (iocore::did_mouse_click(midcol - 26, y_pos, 52, 10))
					{
						valid_selection = true;
						slot_pos = i;
						break;
					}
				}
			}
			if (valid_selection)
			{
				if (slot_tiers.at(slot_pos).size())
				{
					const unsigned int y_pos = ((slot_pos - slot_offset) * 12) + 2;
					const unsigned int box_height = 5;
					const int max_choice = 1;
					iocore::box(midcol + 15, y_pos, 10, box_height, Colour::WHITE);
					int sub_choice = 0;
					bool done = false;
					bool dead = slot_dead.at(slot_pos);
					bool always_prune = false;
					while(!done)
					{
						const Colour continue_highlight = (dead ? Colour::CGA_LRED : (always_prune ? Colour::CGA_LGREEN : Colour::YELLOW_PURE));
						const Colour continue_dark = (dead ? Colour::CGA_RED : Colour::GRAY_LIGHT);
						iocore::print(sub_choice == 0 ? "CONTINUE" : "Continue", midcol + 16, y_pos + 1, sub_choice == 0 ? continue_highlight : continue_dark);
						iocore::print(sub_choice == 1 ? "DELETE" : "Delete", midcol + 17, y_pos + 3, sub_choice == 1 ? Colour::YELLOW_PURE : Colour::GRAY_LIGHT);
						iocore::flip();
						unsigned int menu_key = iocore::wait_for_key();
						if (iocore::is_up(menu_key) || menu_key == MOUSEWHEEL_UP_KEY)
						{
							if (sub_choice) sub_choice--; else sub_choice = max_choice;
						}
						else if (iocore::is_down(menu_key) || menu_key == MOUSEWHEEL_DOWN_KEY)
						{
							if (sub_choice < max_choice) sub_choice++; else sub_choice = 0;
						}
						else if (iocore::is_select(menu_key) || menu_key == LMB_KEY)
						{
							bool valid_keypress = true;
							if (menu_key == LMB_KEY)
							{
								valid_keypress = false;
								if (iocore::did_mouse_click(midcol + 16, y_pos + 1, 8))
								{
									valid_keypress = true;
									sub_choice = 0;
								}
								else if (iocore::did_mouse_click(midcol + 16, y_pos + 3, 8))
								{
									valid_keypress = true;
									sub_choice = 1;
								}
							}

							if (valid_keypress)
							{
								done = true;
								if (sub_choice == 0)
								{
									if (dead) done = false;
									else
									{
										load_game(slot_pos);
										return;
									}
								}
								else if (sub_choice == 1)
								{
									bool result = new_game(slot_pos, true);
									if (result)
									{
										slot_names.at(slot_pos) = "EMPTY SAVE SLOT";
										slot_times.at(slot_pos) = "--:--:--";
										slot_tiers.at(slot_pos) = "";
										slot_hearts.at(slot_pos) = slot_respawns.at(slot_pos) = slot_poisoned.at(slot_pos) = 0;
									}
								}
							}
						}
						else if (iocore::is_cancel(menu_key)) done = true;
					}
				}
				else
				{
					new_game(slot_pos, false);
					return;
				}
			}	// valid_selection
		}
	}
}

// Starts up the animated title screen!
void title_screen()
{
	STACK_TRACE();

	// Glitch warning screen.
	if (!prefs::glitch_warn) glitch_warning();

	// Set up the animated flames.
	heat = new unsigned char[ANIMATED_FLAMES_W * ANIMATED_FLAMES_H];
	memset(heat, 0, sizeof(unsigned char) * ANIMATED_FLAMES_W * ANIMATED_FLAMES_H);
	memset(heat, 9, sizeof(unsigned char) * ANIMATED_FLAMES_W);
	for (int i = 0; i < 20; i++) animate_fire(false);	// Pre-ignite the fire.

	// We don't need to redraw the background very often.
	redraw_background();
	iocore::flip();
	auto last_flame_redraw = std::chrono::system_clock::now();

	do
	{
		const auto start = std::chrono::system_clock::now();
		std::chrono::duration<float> time_since_last_redraw = std::chrono::system_clock::now() - last_flame_redraw;
		if (time_since_last_redraw.count() >= 0.1f)
		{
			last_flame_redraw = std::chrono::system_clock::now();
			redraw_animated_logo();
		}
		iocore::flip();
		const auto end = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsed_seconds = end - start;
		const int delay = round(elapsed_seconds.count() * 1000);
		unsigned int key = iocore::wait_for_key(delay < 50 ? 100 - delay : 50);
		if (!key) continue;

		if (key == RESIZE_KEY) { redraw_background(); continue; }
		else if (iocore::is_down(key) && menu_pos < 4) { menu_pos++; redraw_menu(); continue; }
		else if (iocore::is_up(key) && menu_pos > 0) { menu_pos--; redraw_menu(); continue; }

		else if (iocore::is_select(key))
		{
			if (menu_pos == 0)	// Begin Adventure
			{
				select_save_slot();
				redraw_background();
			}
			else if (menu_pos == 1)	// Game Manual
			{
				wiki->wiki("HELP");
				redraw_background();
			}
			else if (menu_pos == 2)	// Prefs
			{
				prefs::prefs_window();
				redraw_background();
				redraw_menu();
			}
			else if (menu_pos == 3)	// Copyright
			{
				copyright_window();
				redraw_background();
				redraw_menu();
			}
			else if (menu_pos == 4) return;	// Quit
		}

		else if (key == LMB_KEY)
		{
			unsigned short midcol = iocore::midcol(), midrow = iocore::midrow();
			if (iocore::did_mouse_click(midcol - 7, midrow + 8, 15))	// Begin Adventure
			{
				menu_pos = 0;
				redraw_menu();
				select_save_slot();
				redraw_background();
			}
			else if (iocore::did_mouse_click(midcol - 5, midrow + 10, 11))	// Game Manual
			{
				menu_pos = 1;
				redraw_menu();
				wiki->wiki("HELP");
				redraw_background();
			}
			else if (iocore::did_mouse_click(midcol - 8, midrow + 12, 17))	// Prefs
			{
				menu_pos = 2;
				redraw_menu();
				prefs::prefs_window();
				redraw_background();
				redraw_menu();
			}
			else if (iocore::did_mouse_click(midcol - 8, midrow + 14, 17))	// Copyright
			{
				menu_pos = 3;
				redraw_menu();
				copyright_window();
				redraw_background();
				redraw_menu();
			}
			else if (iocore::did_mouse_click(midcol - 7, midrow + 16, 15))	// Quit to Desktop
			{
				menu_pos = 4;
				redraw_menu();
				return;
			}
		}
	} while(true);
}

}	// namespace title
