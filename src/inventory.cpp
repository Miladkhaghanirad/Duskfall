// inventory.cpp -- The Inventory class, which can be attached to an Actor and contains other Actors.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "actor.h"
#include "guru.h"
#include "inventory.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"


// Adds an item to this Inventory.
void Inventory::add_item(shared_ptr<Actor> item)
{
	STACK_TRACE();
	if (item->owner_id) guru::log("Relocating item that has existing owner!", GURU_ERROR);
	item->owner_id = id;
	item->dungeon_id = 0;
	contents.push_back(item);
}

// Removes an item from this Inventory. WARNING: This will leave the item ownerless and without a dungeon ID.
void Inventory::remove_item(unsigned int id)
{
	STACK_TRACE();
	auto item = contents.at(id);
	contents.erase(contents.begin() + id);
	item->owner_id = 0;
}

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
			new_item->dungeon_id = 0;
			new_item->owner_id = id;
			new_item->load();
			contents.push_back(new_item);
			world::actors()->push_back(new_item);
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
