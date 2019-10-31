// hero.cpp -- The Hero class, where the player data and other important stuff about the world is stored.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "controls.h"
#include "dungeon.h"
#include "guru.h"
#include "hero.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"

#include <fstream>


Hero::Hero() : difficulty(1), style(1)
{
	ai = new Controls(this);
}

Hero::~Hero()
{
	STACK_TRACE();
}

// Loads the Hero's data from disk.
void Hero::load()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world()->save_db(), "SELECT * FROM hero");
		while (query.executeStep())
		{
			difficulty = query.getColumn("difficulty").getUInt();
			style = query.getColumn("style").getUInt();
			name = query.getColumn("name").getString();
			x = query.getColumn("x").getUInt();
			y = query.getColumn("y").getUInt();
		}
	}
	catch(std::exception &e)
	{
		guru->halt(e.what());
	}
}

// Saves the Hero's data to disk, along with the rest of the game world.
void Hero::save()
{
	STACK_TRACE();
	try
	{
		world()->save_db()->exec("DROP TABLE IF EXISTS hero; CREATE TABLE hero ( id INTEGER PRIMARY KEY AUTOINCREMENT, difficulty INTEGER NOT NULL, style INTEGER NOT NULL, name TEXT NOT NULL, x INTEGER NOT NULL, "
				"y INTEGER NOT NULL )");
		SQLite::Statement query(*world()->save_db(), "INSERT INTO hero (difficulty,style,name,x,y) VALUES (?,?,?,?,?)");
		query.bind(1, difficulty);
		query.bind(2, style);
		query.bind(3, name);
		query.bind(4, x);
		query.bind(5, y);
		query.exec();
	}
	catch(std::exception &e)
	{
		guru->halt(e.what());
	}

	std::ofstream tag_file("userdata/save/" + strx::itos(world()->slot()) + "/tag.dat");
	tag_file << name << std::endl;			// The character's name.
	tag_file << "00:00:00" << std::endl;	// The gameplay timer.
	tag_file << "1 Novice" << std::endl;	// The character's level and class.
	tag_file << "0" << std::endl;			// The number of times this character has died.
	tag_file << "20" << std::endl;			// The character's total health percentage (0 = empty, 20 = full).
	tag_file << "0" << std::endl;			// 1 if this character is poisoned, 0 if not.
	tag_file << strx::itos(difficulty) << std::endl;	// The current difficulty level.
	if (false) tag_file << 'X' << std::endl;	// X if this character is dead.
	tag_file.close();
}
