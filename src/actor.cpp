// actor.cpp -- The Actor class, encompassing all items, NPCs, and other dynamic things in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "actor.h"
#include "ai.h"
#include "guru.h"
#include "inventory.h"
#include "iocore.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"


Actor::Actor(unsigned long long new_id, unsigned int new_dungeon_id, unsigned long long new_owner_id) : ai(nullptr), dungeon_id(new_dungeon_id), flags(0), id(new_id), inventory(nullptr), owner_id(new_owner_id), x(0), y(0) { }

Actor::~Actor() { }

// Clears a flag on this Actor.
void Actor::clear_flag(unsigned int flag)
{
	if ((flags & flag) == flag) flags ^= flag;
}

// Does this Actor have lower-priority rendering (i.e. other Actors go on top)?
bool Actor::has_low_priority_rendering() const
{
	return is_item();
}

// Does this Actor have an animated sprite?
bool Actor::is_animated() const
{
	return (flags & ACTOR_FLAG_ANIMATED) == ACTOR_FLAG_ANIMATED;
}

// Does this Actor block movement into its tile?
bool Actor::is_blocker() const
{
	return (flags & ACTOR_FLAG_BLOCKER) == ACTOR_FLAG_BLOCKER;
}

// Is this Actor some sort of door?
bool Actor::is_door() const
{
	return (flags & ACTOR_FLAG_DOOR) == ACTOR_FLAG_DOOR;
}

// Is this Actor unable to be normally seen?
bool Actor::is_invisible() const
{
	return (flags & ACTOR_FLAG_INVISIBLE) == ACTOR_FLAG_INVISIBLE;
}

// Is this Actor an item that can be picked up?
bool Actor::is_item() const
{
	return (flags & ACTOR_FLAG_ITEM) == ACTOR_FLAG_ITEM;
}

// Does this Actor block line-of-sight?
bool Actor::is_los_blocker() const
{
	return (flags & ACTOR_FLAG_BLOCKS_LOS) == ACTOR_FLAG_BLOCKS_LOS;
}

// Is this Actor an NPC or monster?
bool Actor::is_monster() const
{
	return (flags & ACTOR_FLAG_MONSTER) == ACTOR_FLAG_MONSTER;
}

// Loads this Actor's data from disk.
void Actor::load()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world::save_db(), "SELECT * FROM actors WHERE aid = ? AND did = ? AND owner = ?");
		query.bind(1, static_cast<long long>(id));
		query.bind(2, dungeon_id);
		query.bind(3, static_cast<long long>(owner_id));
		while (query.executeStep())
		{
			name = query.getColumn("name").getString();
			flags = query.getColumn("flags").getUInt();
			tile = query.getColumn("tile").getString();
			x = query.getColumn("x").getUInt();
			y = query.getColumn("y").getUInt();
			if (!query.isColumnNull("inventory"))
			{
				unsigned long long inventory_id = query.getColumn("inventory").getInt64();
				shared_ptr<Inventory> new_inv = std::make_shared<Inventory>(this, 0);
				new_inv->id = inventory_id;
				new_inv->load();
			}
		}
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}

	if (inventory) inventory->load();
}

// Saves this Actor's data to disk.
void Actor::save()
{
	STACK_TRACE();
	try
	{
		world::save_db()->exec("DELETE FROM actors WHERE aid = " + strx::uitos(id));
		SQLite::Statement query(*world::save_db(), "INSERT INTO actors (aid, did, owner, name, flags, x, y, inventory, tile) VALUES (?,?,?,?,?,?,?,?,?)");
		query.bind(1, static_cast<long long>(id));
		query.bind(2, dungeon_id);
		query.bind(3, static_cast<long long>(owner_id));
		query.bind(4, name);
		query.bind(5, flags);
		query.bind(6, x);
		query.bind(7, y);
		if (inventory) query.bind(8, static_cast<long long>(inventory->id));
		else query.bind(8);
		query.bind(9, tile);
		query.exec();
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}

	if (inventory) inventory->save();
}

// Sets a flag on this Actor.
void Actor::set_flag(unsigned int flag)
{
	flags |= flag;
}
