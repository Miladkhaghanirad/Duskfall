// actor.cpp -- The Actor class, encompassing all items, NPCs, and other dynamic things in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "actor.h"
#include "ai.h"
#include "guru.h"
#include "iocore.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"


Actor::Actor() : ai(nullptr), colour(Colour::WHITE), flags(0), glyph('?'), x(0), y(0) { }

Actor::~Actor() { }

// Loads this Actor's data from disk.
void Actor::load(unsigned int actor_id, unsigned int dungeon_id)
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world::save_db(), "SELECT * FROM actors WHERE aid = ?  AND did = ?");
		query.bind(1, actor_id);
		query.bind(2, dungeon_id);
		while (query.executeStep())
		{
			colour = static_cast<Colour>(query.getColumn("colour").getUInt());
			name = query.getColumn("name").getString();
			flags = query.getColumn("flags").getUInt();
			glyph = query.getColumn("glyph").getUInt();
			x = query.getColumn("x").getUInt();
			y = query.getColumn("y").getUInt();
		}
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Saves this Actor's data to disk.
void Actor::save(unsigned int actor_id, unsigned int dungeon_id)
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world::save_db(), "INSERT INTO actors (aid,did,colour,name,flags,glyph,x,y) VALUES (?,?,?,?,?,?,?,?)");
		query.bind(1, actor_id);
		query.bind(2, dungeon_id);
		query.bind(3, static_cast<unsigned int>(colour));
		query.bind(4, name);
		query.bind(5, flags);
		query.bind(6, glyph);
		query.bind(7, x);
		query.bind(8, y);
		query.exec();
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}
}
