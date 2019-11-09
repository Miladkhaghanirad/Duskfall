// defender.cpp -- The Defender class, which allows Actors to take damage from Attackers or other sources.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "actor.h"
#include "ai.h"
#include "defender.h"
#include "dungeon.h"
#include "graveyard.h"
#include "guru.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"


// It's just a flesh wound!
void Defender::die(Actor *owner)
{
	STACK_TRACE();
	hp = 0;
	if (owner->ai) owner->ai->state = AIState::DEAD;
	if (is_hero()) { }	// todo: Handle player death here!
	else
	{
		Tile* owner_tile = world::dungeon()->tile(owner->x, owner->y);
		unsigned int tile_vector_id = UINT_MAX;
		for (unsigned int i = 0; i < owner_tile->actors()->size(); i++)
		{
			const shared_ptr<Actor> actor = owner_tile->actors()->at(i);
			if (actor.get() == owner)
			{
				tile_vector_id = 0;
				break;
			}
		}
		if (tile_vector_id == UINT_MAX)
		{
			guru::nonfatal("Could not determine tile vector position for " + owner->get_name(false) + "!", GURU_CRITICAL);
			return;
		}
		owner_tile->actors()->erase(owner_tile->actors()->begin() + tile_vector_id);
		graveyard::destroy_actor(owner->id);
		world::queue_redraw();
	}
}

// Loads this Defender from disk.
void Defender::load()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world::save_db(), "SELECT * FROM defenders WHERE id = ?");
		query.bind(1, static_cast<signed long long>(id));
		if (query.executeStep())
		{
			armour = query.getColumn("armour").getUInt();
			hp = query.getColumn("hp").getUInt();
			hp_max = query.getColumn("hp_max").getUInt();
			if (!query.isColumnNull("flags")) flags = query.getColumn("flags").getUInt();
		}
		else guru::halt("Could not load Defender data.");
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Saves this Defender to disk.
void Defender::save()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement delete_defender(*world::save_db(), "DELETE FROM defenders WHERE id = ?");
		delete_defender.bind(1, static_cast<signed long long>(id));
		delete_defender.exec();
		SQLite::Statement defender(*world::save_db(), "INSERT INTO defenders (id, armour, hp, hp_max, flags) VALUES (?,?,?,?,?)");
		defender.bind(1, static_cast<signed long long>(id));
		defender.bind(2, armour);
		defender.bind(3, hp);
		defender.bind(4, hp_max);
		if (flags) defender.bind(5, flags); else defender.bind(5);
		defender.exec();
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Tells this Defender to take damage, and die if necessary.
void Defender::take_damage(unsigned int damage, Actor *owner)
{
	STACK_TRACE();
	if (damage >= hp) die(owner);
	else hp -= damage;
}
