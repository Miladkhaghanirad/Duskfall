// controls.cpp -- The Controls class, which translates input from the player into game actions.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "controls.h"
#include "hero.h"
#include "iocore.h"
#include "world.h"

// Attempts to travel in a given direction.
bool Controls::travel(short x_dir, short y_dir)
{
	if (AI::travel(x_dir, y_dir))
	{
		const int screen_x = world()->hero()->x + world()->hero()->camera_off_x;
		const int screen_y = world()->hero()->y + world()->hero()->camera_off_y;
		if (screen_x <= 5 || screen_x >= iocore->get_cols() - 5) world()->hero()->recenter_camera_horiz();
		if (screen_y <= 5 || screen_y >= iocore->get_rows() - 5) world()->hero()->recenter_camera_vert();
		return true;
	}
	return false;
}
