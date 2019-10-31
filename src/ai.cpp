// ai.cpp -- The AI classes, which handle both NPC 'intelligence', and objects that update their own state.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "actor.h"
#include "ai.h"
#include "dungeon.h"
#include "world.h"


// Attempts to travel in a given direction.
bool AI::travel(short x_dir, short y_dir)
{
	Dungeon *dungeon = world()->dungeon();
	Tile *target_tile = dungeon->tile(owner->x + x_dir, owner->y + y_dir);
	if (target_tile->impassible()) return false;
	owner->x += x_dir;
	owner->y += y_dir;
	world()->queue_redraw();
	world()->queue_recalc_lighting();
	return true;
}
