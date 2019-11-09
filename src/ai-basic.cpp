// ai-basic.cpp -- The BasicAI class, which handles standard, basic AI for monsters in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "ai-basic.h"
#include "attacker.h"
#include "dungeon.h"
#include "hero.h"
#include "mathx.h"
#include "world.h"


// The AI takes a turn.
void BasicAI::tick()
{
	STACK_TRACE();
	if (!owner->attacker) return;	// Do nothing if we can't attack.
	const unsigned int distance_to_player = mathx::grid_dist(owner->x, owner->y, world::hero()->x, world::hero()->y);
	if (distance_to_player == 1)
	{
		owner->attacker->attack(owner, world::hero().get());
		return;
	}
	const bool player_in_sight = world::dungeon()->los_check(owner->x, owner->y, world::hero()->x, world::hero()->y);
	if (player_in_sight || tracking_count)
	{
		if (player_in_sight) tracking_count = TRACKING_TURNS;
		else tracking_count--;
		int dx = 0, dy = 0;
		if (owner->x < world::hero()->x) dx = 1;
		else if (owner->x > world::hero()->x) dx = -1;
		if (owner->y < world::hero()->y) dy = 1;
		else if (owner->y > world::hero()->y) dy = -1;
		int dyo = dy;
		Tile* target_tile = world::dungeon()->tile(owner->x + dx, owner->y + dy);
		if (target_tile->is_impassible())
		{
			dy = 0;
			target_tile = world::dungeon()->tile(owner->x + dx, owner->y);
		}
		if (target_tile->is_impassible())
		{
			dy = dyo;
			dx = 0;
			target_tile = world::dungeon()->tile(owner->x, owner->y + dy);
		}
		if (target_tile->is_impassible()) return;
		travel(dx, dy);
	}
	else state = AIState::SLEEPING;	// If tracking_count runs out, the player has been out of sight for a long time - time to go inactive.
}
