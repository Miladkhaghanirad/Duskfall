// death.cpp -- Handles death, respawning, death reports, and the game-over screen.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "death.h"
#include "iocore.h"
#include "prefs.h"
#include "title.h"


namespace death
{

// Triggered when the player dies.
void die(string)
{
	STACK_TRACE();
	const Colour death_sprite[49] = {
		Colour::CGA_BLACK, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_BLACK,
		Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED,
		Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_BLACK, Colour::CGA_RED, Colour::CGA_BLACK, Colour::CGA_RED, Colour::CGA_RED,
		Colour::CGA_BLACK, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_BLACK,
		Colour::CGA_BLACK, Colour::CGA_BLACK, Colour::CGA_RED, Colour::CGA_BLACK, Colour::CGA_RED, Colour::CGA_BLACK, Colour::CGA_BLACK,
		Colour::CGA_BLACK, Colour::CGA_RED, Colour::CGA_BLACK, Colour::CGA_RED, Colour::CGA_BLACK, Colour::CGA_RED, Colour::CGA_BLACK,
		Colour::CGA_BLACK, Colour::CGA_BLACK, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_RED, Colour::CGA_BLACK, Colour::CGA_BLACK };

	iocore::cls();
	static int midcol = iocore::midcol(), midrow = iocore::midrow();
	for (int y = midrow - 10, count = 0; y <= midrow + 2; y += 2)
	{
		for (int x = midcol - 7; x <= midcol + 5; x += 2)
		{
			iocore::print_at(Glyph::BLOCK_SOLID, x, y, (Colour)death_sprite[count]);
			iocore::print_at(Glyph::BLOCK_SOLID, x + 1, y, (Colour)death_sprite[count]);
			iocore::print_at(Glyph::BLOCK_SOLID, x, y + 1, (Colour)death_sprite[count]);
			iocore::print_at(Glyph::BLOCK_SOLID, x + 1, y + 1, (Colour)death_sprite[count++]);
		}
	}
	const bool can_respawn = false;
	if (can_respawn) iocore::print("YOU HAVE DIED... BUT THE ADVENTURE IS NOT YET OVER", midcol - 25, midrow - 15, Colour::CGA_WHITE);
	else iocore::print("YOU HAVE DIED... YOUR ADVENTURE HAS COME TO AN END", midcol - 25, midrow - 15, Colour::CGA_WHITE);
	if (prefs::death_reports) iocore::print("DEATH REPORT CREATED - PRESS ANY KEY TO CONTINUE", midcol - 24, midrow - 13, Colour::CGA_WHITE);
	else iocore::print("PRESS ANY KEY TO CONTINUE", midcol - 12, midrow - 13, Colour::CGA_WHITE);
	iocore::print("THANKS FOR PLAYING", midcol - 9, midrow + 6, Colour::CGA_WHITE);
	for (int i = 0; i < 9; i++)
		iocore::print(title::static_title[i], midcol - 23, midrow + 8 + i, Colour::CGA_RED, PRINT_FLAG_NO_SPACES);
	iocore::flip();
	iocore::wait_for_key();

	if (can_respawn)
	{
		// respawn code goes here
	}
	else
	{
		iocore::exit_functions();
		exit(0);
	}
}

}	// namespace death
