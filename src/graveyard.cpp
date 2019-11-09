// graveyard.cpp -- Keeps track of Actors, Dungeons and other things which are due to be deleted from SQLite during the next game-save.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "graveyard.h"
#include "guru.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"
#include "strx.h"


namespace graveyard
{

// Vectors of objects due to be removed from the saved-game database during the next save. They've already been removed from memory.
vector<unsigned long long>	dead_actors, dead_attackers, dead_defenders, dead_dungeons;


// The destroy_* functions mark objects for removal from the database.
void destroy_actor(unsigned long long id)
{
	STACK_TRACE();
	// This is tricky, as Actor can have other bits attached. We'll check the database to see what needs to be purged.
	try
	{
		SQLite::Statement actors_query(*world::save_db(), "SELECT inventory, attacker, defender FROM actors WHERE id = ?");
		actors_query.bind(1, static_cast<signed long long>(id));
		while (actors_query.executeStep())
		{
			if (!actors_query.isColumnNull("inventory")) destroy_inventory(static_cast<unsigned long long>(actors_query.getColumn("inventory").getInt64()));
			if (!actors_query.isColumnNull("attacker")) destroy_attacker(static_cast<unsigned long long>(actors_query.getColumn("attacker").getInt64()));
			if (!actors_query.isColumnNull("defender")) destroy_defender(static_cast<unsigned long long>(actors_query.getColumn("defender").getInt64()));
			dead_actors.push_back(id);	// Yes, this goes here; if the Actor isn't found in the DB (i.e. if they were created some time *after* the last save), there's nothing to purge from the DB.
		}
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

void destroy_attacker(unsigned long long id)
{
	STACK_TRACE();
	dead_attackers.push_back(id);
}

void destroy_defender(unsigned long long id)
{
	STACK_TRACE();
	dead_defenders.push_back(id);
}

void destroy_dungeon(unsigned long long id)
{
	STACK_TRACE();
	dead_dungeons.push_back(id);
}

void destroy_inventory(unsigned long long id)
{
	STACK_TRACE();
	try
	{
		SQLite::Statement inventory_query(*world::save_db(), "SELECT id FROM actors WHERE owner = ?");
		inventory_query.bind(1, static_cast<signed long long>(id));
		while (inventory_query.executeStep())
			destroy_actor(static_cast<unsigned long long>(inventory_query.getColumn("id").getInt64()));
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Called during a game-save, purges all the doomed data from the database.
void purge()
{
	STACK_TRACE();

	auto purge_db = [](string table, vector<unsigned long long> &vec)
	{
		for (auto id : vec)
		{
			SQLite::Statement statement(*world::save_db(), "DELETE FROM " + table + " WHERE id = ?");
			statement.bind(1, static_cast<signed long long>(id));
			statement.exec();
		}
		vec.clear();
	};

	try
	{
		for (auto id : dead_dungeons)
		{
			// Delete the entry from the dungeon table. This is the easy part.
			SQLite::Statement dungeon_statement(*world::save_db(), "DELETE FROM dungeon WHERE id = ?");
			dungeon_statement.bind(1, static_cast<signed long long>(id));
			dungeon_statement.exec();

			// Delete all tiles that match this dungeon. This is a lot of deletions, but should be pretty fast as it's a single batch.
			SQLite::Statement tiles_statement(*world::save_db(), "DELETE FROM tiles WHERE dungeon_id = ?");
			tiles_statement.bind(1, static_cast<signed long long>(id));
			tiles_statement.exec();

			// Wipe out all the Actors in this Dungeon. We have to be sure to add Inventories, Attackers and Defendeers too -- destroy_actor() will handle that part.
			SQLite::Statement actors_query(*world::save_db(), "SELECT id FROM actors WHERE owner = ?");
			actors_query.bind(1, static_cast<signed long long>(id));
			while (actors_query.executeStep())
				destroy_actor(static_cast<unsigned long long>(actors_query.getColumn("id").getInt64()));
		}

		purge_db("actors", dead_actors);
		purge_db("attackers", dead_attackers);
		purge_db("defenders", dead_defenders);
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

}
