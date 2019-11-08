// inventory.cpp -- The Inventory class, which can be attached to an Actor and contains other Actors.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "actor.h"
#include "graveyard.h"
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
		items_query.bind(1, static_cast<signed long long>(id));
		while (items_query.executeStep())
		{
			const unsigned long long new_id = items_query.getColumn("id").getInt64();
			shared_ptr<Actor> new_item = std::make_shared<Actor>(new_id);
			new_item->load(id);
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
		actor->save(id);
}
