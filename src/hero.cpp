// hero.cpp -- The Hero class, where the player data and other important stuff about the world is stored.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "controls.h"
#include "dungeon.h"
#include "guru.h"
#include "iocore.h"
#include "hero.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"

#include <fstream>


Hero::Hero() : camera_off_x(0), camera_off_y(0), difficulty(1), played(0), style(1)
{
	ai = std::make_shared<Controls>(shared_ptr<Hero>(this));
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
			played = query.getColumn("played").getUInt();
		}
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Recenters the camera on the Hero's position.
void Hero::recenter_camera()
{
	STACK_TRACE();
	recenter_camera_horiz();
	recenter_camera_vert();
}

// Recenters the camera horizontally on the Hero's position, not adjusting the vertical.
void Hero::recenter_camera_horiz()
{
	STACK_TRACE();
	camera_off_x = (iocore->get_cols() / 2) - x;
}

// Recenters the camera vertically on the Hero's position, not adjusting the horizontal.
void Hero::recenter_camera_vert()
{
	STACK_TRACE();
	camera_off_y = (iocore->get_rows() / 2) - y;
}

// Saves the Hero's data to disk, along with the rest of the game world.
void Hero::save()
{
	STACK_TRACE();
	try
	{
		world()->save_db()->exec("DROP TABLE IF EXISTS hero; CREATE TABLE hero ( id INTEGER PRIMARY KEY AUTOINCREMENT, difficulty INTEGER NOT NULL, style INTEGER NOT NULL, name TEXT NOT NULL, x INTEGER NOT NULL, "
				"y INTEGER NOT NULL, played INTEGER NOT NULL )");
		SQLite::Statement query(*world()->save_db(), "INSERT INTO hero (difficulty,style,name,x,y,played) VALUES (?,?,?,?,?,?)");
		query.bind(1, difficulty);
		query.bind(2, style);
		query.bind(3, name);
		query.bind(4, x);
		query.bind(5, y);
		query.bind(6, played);
		query.exec();
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}

	std::ofstream tag_file("userdata/save/" + strx::itos(world()->slot()) + "/tag.dat");
	tag_file << name << std::endl;			// The character's name.

	// The gameplay timer.
	unsigned int hours = 0, minutes = 0, seconds = played;
	if (seconds >= 3600)
	{
		hours = seconds / 3600;
		seconds -= (hours * 3600);
	}
	if (seconds >= 60)
	{
		minutes = seconds / 60;
		seconds -= (minutes * 60);
	}
	string hours_string = strx::itos(hours); if (hours_string.size() < 2) hours_string = "0" + hours_string;
	string minutes_string = strx::itos(minutes); if (minutes_string.size() < 2) minutes_string = "0" + minutes_string;
	string seconds_string = strx::itos(seconds); if (seconds_string.size() < 2) seconds_string = "0" + seconds_string;
	tag_file << hours_string + ":" + minutes_string + ":" + seconds_string << std::endl;

	tag_file << "1 Novice" << std::endl;	// The character's level and class.
	tag_file << "0" << std::endl;			// The number of times this character has died.
	tag_file << "20" << std::endl;			// The character's total health percentage (0 = empty, 20 = full).
	tag_file << "0" << std::endl;			// 1 if this character is poisoned, 0 if not.
	tag_file << strx::itos(difficulty) << std::endl;	// The current difficulty level.
	if (false) tag_file << 'X' << std::endl;	// X if this character is dead.
	tag_file.close();
}
