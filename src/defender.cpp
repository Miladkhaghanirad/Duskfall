// defender.cpp -- The Defender class, which allows Actors to take damage from Attackers or other sources.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "defender.h"
#include "guru.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"


// Loads this Defender from disk.
void Defender::load()
{
	STACK_TRACE();
	try
	{

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
		SQLite::Statement defender(*world::save_db(), "INSERT INTO defenders (id, armour, hp, hp_max) VALUES (?,?,?,?)");
		defender.bind(1, static_cast<signed long long>(id));
		defender.bind(2, armour);
		defender.bind(3, hp);
		defender.bind(4, hp_max);
		defender.exec();
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}
