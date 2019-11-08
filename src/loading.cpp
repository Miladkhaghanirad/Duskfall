// loading.cpp -- Displays a loading screen with progress bar.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "iocore.h"
#include "loading.h"
#include "strx.h"
#include "title.h"
#include "version.h"

#include <cmath>


namespace loading
{

// Displays a loading screen with a specified percentage.
void loading_screen(unsigned int percentage, string str, bool floppy)
{
	STACK_TRACE();
	if (percentage > 100) percentage = 100;
	iocore::cls();
	iocore::render_nebula(57720, 0, 0);
	const int midrow = iocore::midrow(), midcol = iocore::midcol();
	title::redraw_static_logo(0);
	iocore::box(midcol - 20, midrow + (floppy ? 8 : 4), 40, 3, Colour::CGA_WHITE);
	const int size = round(38.0f * (percentage / 100.0f));
	for (int i = 0; i < size; i++)
		iocore::print_at(Glyph::BLOCK_SOLID, midcol - 19 + i, midrow + (floppy ? 9 : 5), Colour::CGA_WHITE);

	iocore::print(str, midcol - (str.size() / 2), midrow + (floppy ? 12 : 7), Colour::CGA_WHITE);

	if (floppy) title::render_floppy(midcol - 3, midrow, Colour::BLUE_LIGHT, true);

	iocore::flip();
	iocore::delay(1);
}

}	// namespace loading
