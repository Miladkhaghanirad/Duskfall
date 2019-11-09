// attacker.cpp -- The Attacker class, which allows Actors to (attempt to) deal damage to other Actors with an attached Defender.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "attacker.h"
#include "guru.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"


// Loads this Attacker from disk.
void Attacker::load()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world::save_db(), "SELECT * FROM attackers WHERE id = ?");
		query.bind(1, static_cast<signed long long>(id));
		if (query.executeStep())
		{
			power = query.getColumn("power").getUInt();
		}
		else guru::halt("Could not load Attacker data.");
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Saves this Attacker to disk.
void Attacker::save()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement delete_attacker(*world::save_db(), "DELETE FROM attackers WHERE id = ?");
		delete_attacker.bind(1, static_cast<signed long long>(id));
		delete_attacker.exec();
		SQLite::Statement attacker(*world::save_db(), "INSERT INTO attackers (id, power) VALUES (?,?)");
		attacker.bind(1, static_cast<signed long long>(id));
		attacker.bind(2, power);
		attacker.exec();
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}
