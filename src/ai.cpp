// ai.cpp -- The AI classes, which handle both NPC 'intelligence', and objects that update their own state.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "actor.h"
#include "ai.h"
#include "dungeon.h"
#include "guru.h"
#include "hero.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"


// Loads this AI from disk.
void AI::load()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement ai_query(*world::save_db(), "SELECT * FROM ai WHERE id = ?");
		ai_query.bind(1, static_cast<signed long long>(id));
		if (ai_query.executeStep())
		{
			state = static_cast<AIState>(ai_query.getColumn("state").getUInt());
			tracking_count = ai_query.getColumn("tracking_count").getUInt();
		}
		else guru::halt("Could not load AI data from save file!");
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Reacts to being attacked by something!
void AI::react_to_attack(Actor*)
{
	STACK_TRACE();
	if (state == AIState::SLEEPING)
	{
		state = AIState::AGGRO;
		world::dungeon()->add_active_ai(owner->ai);
	}
}

// Saves this AI to disk.
void AI::save()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement ai_delete(*world::save_db(), "DELETE FROM ai WHERE id = ?");
		ai_delete.bind(1, static_cast<signed long long>(id));
		ai_delete.exec();
		SQLite::Statement ai_statement(*world::save_db(), "INSERT INTO ai (id, type, state, tracking_count) VALUES (?,?,?,?)");
		ai_statement.bind(1, static_cast<signed long long>(id));
		ai_statement.bind(2, owner->ai_type);
		ai_statement.bind(3, static_cast<unsigned int>(state));
		ai_statement.bind(4, tracking_count);
		ai_statement.exec();
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Attempts to travel in a given direction.
bool AI::travel(short x_dir, short y_dir)
{
	STACK_TRACE();
	shared_ptr<Dungeon> dungeon = world::dungeon();
	Tile* target_tile = dungeon->tile(owner->x + x_dir, owner->y + y_dir);
	if (target_tile->is_impassible()) return false;
	for (auto actor : *target_tile->actors())
		if (actor->is_blocker()) return false;

	Tile* current_tile = dungeon->tile(owner->x, owner->y);
	const bool is_hero = (owner == world::hero().get());
	bool removed_from_old_tile = false;
	shared_ptr<Actor> owner_shared = nullptr;
	if (!is_hero)
	{
		for (unsigned int i = 0; i < current_tile->actors()->size(); i++)
		{
			if (current_tile->actors()->at(i).get() == owner)
			{
				owner_shared = current_tile->actors()->at(i);
				current_tile->actors()->erase(current_tile->actors()->begin() + i);
				removed_from_old_tile = true;
				continue;
			}
		}
		if (!removed_from_old_tile) guru::nonfatal("Could not remove " + owner->get_name(false) + " from dungeon tile!", GURU_ERROR);
	}
	owner->x += x_dir;
	owner->y += y_dir;
	if (!is_hero && removed_from_old_tile) target_tile->actors()->push_back(owner_shared);
	world::queue_redraw();
	world::queue_recalc_lighting();
	owner->tile_react();
	return true;
}
