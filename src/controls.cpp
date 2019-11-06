// controls.cpp -- The Controls class, which translates input from the player into game actions.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "controls.h"
#include "dungeon.h"
#include "hero.h"
#include "iocore.h"
#include "message.h"
#include "sidebar.h"
#include "static-data.h"
#include "world.h"


// Attempts to close a door.
void Controls::close()
{
	STACK_TRACE();
	message::msg("Close something in which direction? ");
	message::render();
	iocore::flip();
	int x_dir, y_dir;
	bool success = iocore::get_direction(x_dir, y_dir);
	if (!success)
	{
		message::amend("{5E}Nevermind.");
		return;
	}
	shared_ptr<Dungeon> dungeon = world::dungeon();
	shared_ptr<Actor> door = nullptr;
	for (auto actor : dungeon->actors)
	{
		if (actor->x == world::hero()->x + x_dir && actor->y == world::hero()->y + y_dir && actor->door() && !actor->blocker())
		{
			door = actor;
			break;
		}
	}
	if (door)
	{
		message::msg("You close the door.");
		Tile new_tile = data::get_tile("BASIC_DOOR");
		world::dungeon()->set_tile(door->x, door->y, new_tile);
		door->set_flag(ACTOR_FLAG_BLOCKER);
		door->set_flag(ACTOR_FLAG_BLOCKS_LOS);
		world::dungeon()->recalc_lighting();
		world::queue_redraw();
	}
	else message::msg("That isn't something you can close.", MC::WARN);
}

// Attempts to open a door.
void Controls::open()
{
	STACK_TRACE();
	message::msg("Open something in which direction? ");
	message::render();
	iocore::flip();
	int x_dir, y_dir;
	bool success = iocore::get_direction(x_dir, y_dir);
	if (!success)
	{
		message::amend("{5E}Nevermind.");
		return;
	}
	shared_ptr<Dungeon> dungeon = world::dungeon();
	shared_ptr<Actor> door = nullptr;
	for (auto actor : dungeon->actors)
	{
		if (actor->x == world::hero()->x + x_dir && actor->y == world::hero()->y + y_dir && actor->door() && actor->blocker())
		{
			door = actor;
			break;
		}
	}
	if (door) open_door(door);
	else message::msg("That isn't something you can open.", MC::WARN);
}

// Opens a specified door.
void Controls::open_door(shared_ptr<Actor> door)
{
	STACK_TRACE();
	message::msg("You open the door.");
	Tile new_tile = data::get_tile("BASIC_DOOR_OPEN");
	world::dungeon()->set_tile(door->x, door->y, new_tile);
	door->clear_flag(ACTOR_FLAG_BLOCKER);
	door->clear_flag(ACTOR_FLAG_BLOCKS_LOS);
	world::dungeon()->recalc_lighting();
	world::queue_redraw();
}

// Attempts to travel in a given direction.
bool Controls::travel(short x_dir, short y_dir)
{
	STACK_TRACE();
	if (AI::travel(x_dir, y_dir))
	{
		const int screen_x = world::hero()->x + world::hero()->camera_off_x;
		const int screen_y = world::hero()->y + world::hero()->camera_off_y;
		if (screen_x <= 5 || screen_x >= iocore::get_tile_cols() - 3) world::hero()->recenter_camera_horiz();
		if (screen_y <= 5 || screen_y >= iocore::get_tile_rows() - 3) world::hero()->recenter_camera_vert();
		return true;
	}
	else
	{
		shared_ptr<Dungeon> dungeon = world::dungeon();
		for (auto actor : dungeon->actors)
		{
			if (actor->x == owner->x + x_dir && actor->y == owner->y + y_dir && actor->blocker())
			{
				if (actor->door()) open_door(actor);
				else message::msg("The " + actor->name + " blocks your path!", MC::WARN);
				return false;
			}
		}
	}
	return false;
}
