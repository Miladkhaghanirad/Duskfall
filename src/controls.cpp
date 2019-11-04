// controls.cpp -- The Controls class, which translates input from the player into game actions.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "controls.h"
#include "dungeon.h"
#include "hero.h"
#include "iocore.h"
#include "message.h"
#include "world.h"


// Attempts to travel in a given direction.
bool Controls::travel(short x_dir, short y_dir)
{
	STACK_TRACE();
	if (AI::travel(x_dir, y_dir))
	{
		const int screen_x = world()->hero()->x + world()->hero()->camera_off_x;
		const int screen_y = world()->hero()->y + world()->hero()->camera_off_y;
		if (screen_x <= 5 || screen_x >= iocore::get_cols() - MESSAGE_LOG_SIZE - 5) world()->hero()->recenter_camera_horiz();
		if (screen_y <= 5 || screen_y >= iocore::get_rows() - MESSAGE_LOG_SIZE - 5) world()->hero()->recenter_camera_vert();
		return true;
	}
	else
	{
		shared_ptr<Dungeon> dungeon = world()->dungeon();
		for (auto actor : dungeon->actors)
		{
			if (actor->x == owner->x + x_dir && actor->y == owner->y + y_dir && actor->blocker())
			{
				msglog->msg("The " + actor->name + " blocks your path!", MC::WARN);
				return false;
			}
		}
	}
	return false;
}
