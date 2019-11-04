// world.cpp -- The World object, containing the main game loop and 'heartbeat' system, keeping track of in-game time and triggering events.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "ai.h"
#include "dungeon.h"
#include "guru.h"
#include "hero.h"
#include "iocore.h"
#include "message.h"
#include "prefs.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"

#include <chrono>
#include <cmath>
#include <set>


shared_ptr<World>	the_world = nullptr;	// The main World object.

World::World(unsigned int new_slot, bool new_save) : recalc_lighting(true), redraw_full(true), the_dungeon(nullptr), save_slot(new_slot), level(0)
{
	STACK_TRACE();
	try
	{
		save_db_ptr = new SQLite::Database("userdata/save/" + strx::itos(save_slot) + "/save.dat", SQLite::OPEN_READWRITE | (new_save ? SQLite::OPEN_CREATE : 0));
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}
	the_hero = std::make_shared<Hero>();
	msglog = std::make_shared<MessageLog>();
}

World::~World()
{
	STACK_TRACE();
	the_hero = nullptr;
	if (save_db_ptr) delete save_db_ptr;
}

// Loads the game from disk.
void World::load()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*save_db_ptr, "SELECT * FROM world");
		if (query.executeStep())
		{
			level = query.getColumn("level").getUInt();
		}
		else guru::halt("Saved game file is damaged!");

		the_dungeon = std::make_shared<Dungeon>(level);
		the_dungeon->load();
		the_hero->load();
		msglog->load();
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}

	the_dungeon = std::make_shared<Dungeon>(level);
	the_dungeon->load();
	the_hero->recenter_camera();
}

// The main game loop!
void World::main_loop()
{
	STACK_TRACE();
	std::chrono::time_point<std::chrono::system_clock> game_clock = std::chrono::system_clock::now();
	while(true)
	{
		if (recalc_lighting)
		{
			the_dungeon->recalc_lighting();
			recalc_lighting = false;
		}
		if (redraw_full)
		{
			iocore::cls();
			the_dungeon->render();
			msglog->render();
			redraw_full = false;
		}
		iocore::flip();

		const unsigned int key = iocore::wait_for_key();
		if (key == RESIZE_KEY) redraw_full = true;
		else if (key == prefs::keybind(Keys::SAVE))
		{
			std::chrono::duration<float> elapsed_seconds = std::chrono::system_clock::now() - game_clock;
			the_hero->played += round(elapsed_seconds.count());
			game_clock = std::chrono::system_clock::now();
			save(true);
		}
		else if (key == prefs::keybind(Keys::NORTH)) hero()->ai->travel(0, -1);
		else if (key == prefs::keybind(Keys::SOUTH)) hero()->ai->travel(0, 1);
		else if (key == prefs::keybind(Keys::EAST)) hero()->ai->travel(1, 0);
		else if (key == prefs::keybind(Keys::WEST)) hero()->ai->travel(-1, 0);
		else if (key == prefs::keybind(Keys::NORTHEAST)) hero()->ai->travel(1, -1);
		else if (key == prefs::keybind(Keys::NORTHWEST)) hero()->ai->travel(-1, -1);
		else if (key == prefs::keybind(Keys::SOUTHEAST)) hero()->ai->travel(1, 1);
		else if (key == prefs::keybind(Keys::SOUTHWEST)) hero()->ai->travel(-1, 1);
		else if (key == prefs::keybind(Keys::SCROLL_TOP) || key == prefs::keybind(Keys::SCROLL_BOTTOM) || key == prefs::keybind(Keys::SCROLL_PAGEUP) || key == prefs::keybind(Keys::SCROLL_PAGEDOWN) || key == MOUSEWHEEL_UP_KEY
				|| key == MOUSEWHEEL_DOWN_KEY || key == prefs::keybind(Keys::SCROLL_UP) || key == prefs::keybind(Keys::SCROLL_DOWN)) msglog->process_input(key);
		else if (key == prefs::keybind(Keys::OPTIONS_WINDOW))
		{
			prefs::prefs_window();
			redraw_full = true;
		}
		else if (key == prefs::keybind(Keys::QUIT_GAME))
		{
			iocore::exit_functions();
			exit(0);
		}
	}
}

// Sets up a new game.
void World::new_game()
{
	STACK_TRACE();
	level = 1;
	hero()->x = hero()->y = 5;
	the_dungeon = std::make_shared<Dungeon>(1, 100, 100);
	the_dungeon->generate();
	the_dungeon->random_start_position(hero()->x, hero()->y);
	the_hero->recenter_camera();
	msglog->msg("It is very dark. You are likely to be eaten by a grue.");
	save(false);
}

// Saves the game to disk.
void World::save(bool announce)
{
	STACK_TRACE();
	if (announce) msglog->msg("Saving game...", MC::INFO);
	try
	{
		SQLite::Transaction transaction(*world()->save_db());
		save_db_ptr->exec("DROP TABLE IF EXISTS world; CREATE TABLE world ( id INTEGER PRIMARY KEY AUTOINCREMENT, level INTEGER NOT NULL );");
		SQLite::Statement query(*save_db_ptr, "INSERT INTO world (level) VALUES (?)");
		query.bind(1, level);
		query.exec();

		hero()->save();
		the_dungeon->save();
		msglog->save();
		transaction.commit();
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
	if (announce) msglog->amend(" done!");
}

// Creates a new World in the specified save slot.
void new_world(unsigned short slot, bool new_save)
{
	STACK_TRACE();
	if (the_world) guru::halt("World object already exists!");
	the_world = std::make_shared<World>(slot, new_save);
}

// Returns a pointer to the main World object, if one exists.
shared_ptr<World> world()
{
	STACK_TRACE();
	if (!the_world) guru::halt("No world object defined!");
	return the_world;
}
