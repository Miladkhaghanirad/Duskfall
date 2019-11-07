// hero.cpp -- The Hero class, where the player data and other important stuff about the world is stored.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "controls.h"
#include "dungeon.h"
#include "guru.h"
#include "inventory.h"
#include "iocore.h"
#include "hero.h"
#include "message.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"

#include <fstream>


Hero::Hero(unsigned long long new_id, unsigned int new_dungeon_id, unsigned long long new_owner_id) : Actor(new_id, new_dungeon_id, new_owner_id), camera_off_x(0), camera_off_y(0), difficulty(1), played(0), style(1)
{
	STACK_TRACE();
	ai = std::make_shared<Controls>(this);
	inventory = std::make_shared<Inventory>(this, world::unique_id());
	tile = "PLAYER";
	id = new_id;
	dungeon_id = new_dungeon_id;
	owner_id = new_owner_id;
	flags |= ACTOR_FLAG_ANIMATED;
}

Hero::~Hero() { }

// Returns a pointer to this Hero's Controls AI.
shared_ptr<Controls> Hero::controls()
{
	STACK_TRACE();
	return std::dynamic_pointer_cast<Controls>(ai);
}

// Loads the Hero's data from disk.
void Hero::load()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world::save_db(), "SELECT * FROM hero");
		while (query.executeStep())
		{
			difficulty = query.getColumn("difficulty").getUInt();
			style = query.getColumn("style").getUInt();
			played = query.getColumn("played").getUInt();
		}
		Actor::load();
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
	camera_off_x = (iocore::get_tile_cols() / 2) - x;
}

// Recenters the camera vertically on the Hero's position, not adjusting the horizontal.
void Hero::recenter_camera_vert()
{
	STACK_TRACE();
	camera_off_y = (iocore::get_tile_rows() / 2) - y;
}

// Saves the Hero's data to disk, along with the rest of the game world.
void Hero::save()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world::save_db(), "INSERT INTO hero (difficulty,style,played) VALUES (?,?,?)");
		query.bind(1, difficulty);
		query.bind(2, style);
		query.bind(3, played);
		query.exec();
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}
	Actor::save();

	std::ofstream tag_file("userdata/save/" + strx::itos(world::slot()) + "/tag.dat");
	tag_file << name << std::endl;	// The character's name.

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

// Reacts to other Actors on the tile this Actor is standing on.
void Hero::tile_react()
{
	STACK_TRACE();
	vector<shared_ptr<Actor>> items_here;
	for (auto actor : *world::actors())
	{
		if (actor->x != x || actor->y != y || actor->is_invisible()) continue;
		if (actor->is_item()) items_here.push_back(actor);
	}
	if (items_here.size())
	{
		vector<string> item_names;
		for (auto item : items_here)
			item_names.push_back(item->name);
		message::msg("You see " + strx::comma_list(item_names, true) + " here.");
	}
}
