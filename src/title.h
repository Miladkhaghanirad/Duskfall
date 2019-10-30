// title.h -- Animated title screen, based on animated title screen code from Krasten, which in turn was based on animated title screen code from a long-forgotten project.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

enum class Colour : unsigned char;	// defined in iocore.h

namespace title
{

void	animate_fire(bool render = true);	// Animates the flames. This can also be used to 'pre-ignite' the fire prior to rendering.
void	copyright_window();		// Display the copyright window.
void	glitch_warning();		// Displays the first-time glitch warning screen.
void	redraw_animated_logo();	// Redraws the animated logo every frame.
void	redraw_background();	// Redraws the background when needed.
void	redraw_menu();			// Redraws the title menu.
void	redraw_static_logo(int offset = 0);	// Redraws the static logo over the top of the flames.
void	render_floppy(int x, int y, Colour colour, bool front);	// Renders a floppy disk!
void	title_screen();			// Starts up the animated title screen!

}	// namespace title
