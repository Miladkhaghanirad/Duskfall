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


Actor::Actor(unsigned long long new_id, unsigned int new_dungeon_id, unsigned long long new_owner_id) : ai(nullptr), colour(Colour::WHITE), dungeon_id(new_dungeon_id), flags(0), glyph('?'), id(new_id), inventory(nullptr),
	owner_id(new_owner_id), x(0), y(0) { }

Actor::~Actor() { }

// Clears a flag on this Actor.
void Actor::clear_flag(unsigned int flag)
{
	if ((flags & flag) == flag) flags ^= flag;
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
			colour = static_cast<Colour>(query.getColumn("colour").getUInt());
			name = query.getColumn("name").getString();
			flags = query.getColumn("flags").getUInt();
			glyph = query.getColumn("glyph").getUInt();
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
		SQLite::Statement query(*world::save_db(), "INSERT INTO actors (aid, did, owner, colour, name, flags, glyph, x, y, inventory) VALUES (?,?,?,?,?,?,?,?,?,?)");
		query.bind(1, static_cast<long long>(id));
		query.bind(2, dungeon_id);
		query.bind(3, static_cast<long long>(owner_id));
		query.bind(4, static_cast<unsigned int>(colour));
		query.bind(5, name);
		query.bind(6, flags);
		query.bind(7, glyph);
		query.bind(8, x);
		query.bind(9, y);
		if (inventory) query.bind(10, static_cast<long long>(inventory->id));
		else query.bind(10);
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
