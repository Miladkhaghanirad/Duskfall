// inventory.cpp -- The Inventory class, which can be attached to an Actor and contains other Actors.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "actor.h"
#include "guru.h"
#include "inventory.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"


// Loads this Inventory from disk.
void Inventory::load()
{
	STACK_TRACE();
	try
	{
		const unsigned int item_count = world::save_db()->execAndGet("SELECT COUNT(*) FROM actors WHERE owner = " + strx::uitos(id));
		contents.reserve(item_count);
		SQLite::Statement items_query(*world::save_db(), "SELECT * FROM actors WHERE owner = ?");
		items_query.bind(1, static_cast<long long>(id));
		while (items_query.executeStep())
		{
			shared_ptr<Actor> new_item = std::make_shared<Actor>(0);
			new_item->id = items_query.getColumn("aid").getInt64();
			new_item->dungeon_id = items_query.getColumn("did").getInt64();	// This is entirely moot and unneeded, but let's just retrieve the proper data anyway, why not?
			new_item->owner_id = id;
			new_item->load();
			contents.push_back(new_item);
		}
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Saves this Inventory to disk.
void Inventory::save()
{
	STACK_TRACE();
	for (auto actor : contents)
		actor->save();
}
