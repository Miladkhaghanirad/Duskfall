// world.cpp -- The World object, containing the main game loop and 'heartbeat' system, keeping track of in-game time and triggering events.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "ai.h"
#include "controls.h"
#include "dungeon.h"
#include "guru.h"
#include "hero.h"
#include "iocore.h"
#include "message.h"
#include "prefs.h"
#include "sidebar.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"

#include <chrono>
#include <cmath>
#include <set>


namespace world
{

bool				db_ready = false;		// Is the database available for reading/writing?
unsigned short		level = 0;				// The current dungeon level depth.
bool				recalc_lighting = true;	// Recalculate the dynamic lighting at the start of the next turn.
bool				redraw_full = true;		// Redraw the dungeon entirely at the start of the next turn.
SQLite::Database	*save_db_ptr = nullptr;	// SQLite handle for the save game file.
unsigned short		save_slot = 0;			// The save file slot.
shared_ptr<Dungeon>	the_dungeon = nullptr;	// The current dungeon level.
shared_ptr<Hero>	the_hero = nullptr;		// The main Hero object.


// Returns a pointer to the Dungeon object.
shared_ptr<Dungeon>	dungeon()
{
	return the_dungeon;
}

// Returns a pointer to the Hero object.
shared_ptr<Hero> hero()
{
	return the_hero;
}

// Loads the game from disk.
void load()
{
	STACK_TRACE();
	db_ready = true;
	try
	{
		SQLite::Statement query(*save_db_ptr, "SELECT * FROM world");
		if (query.executeStep())
		{
			level = query.getColumn("level").getUInt();
		}
		else guru::halt("Saved game file is damaged!");
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}

	the_dungeon = std::make_shared<Dungeon>(level);
	the_dungeon->load();
	the_hero->load();
	the_hero->recenter_camera();
	message::load();
}

// Creates a new World in the specified save slot.
void new_world(unsigned short slot, bool new_save)
{
	STACK_TRACE();
	save_slot = slot;
	try
	{
		save_db_ptr = new SQLite::Database("userdata/save/" + strx::itos(save_slot) + "/save.dat", SQLite::OPEN_READWRITE | (new_save ? SQLite::OPEN_CREATE : 0));
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}
	the_hero = std::make_shared<Hero>(1, 0, 0);
}

// Queues up a recalculation of the game's dynamic lighting.
void queue_recalc_lighting()
{
	recalc_lighting = true;
}

// The main game loop!
void main_loop()
{
	STACK_TRACE();
	auto game_clock = std::chrono::system_clock::now();
	auto animation_timer = std::chrono::system_clock::now();
	guru::game_output(true);
	while(true)
	{
		std::chrono::duration<float> elapsed_seconds = std::chrono::system_clock::now() - animation_timer;
		if (elapsed_seconds.count() > 1)
		{
			animation_timer = std::chrono::system_clock::now();
			iocore::toggle_animation_frame();
			redraw_full = true;
		}
		if (recalc_lighting)
		{
			the_dungeon->recalc_lighting();
			recalc_lighting = false;
		}
		if (redraw_full)
		{
			iocore::cls();
			the_dungeon->render();
			message::render();
			sidebar::render();
			iocore::flip();
			redraw_full = false;
		}

		const unsigned int key = iocore::check_for_key();
		if (!key) iocore::delay(1);
		bool action_taken = true;
		if (key == RESIZE_KEY)
		{
			redraw_full = true;
			action_taken = false;
		}
		else if (key == prefs::keybind(Keys::SAVE))
		{
			std::chrono::duration<float> elapsed_seconds = std::chrono::system_clock::now() - game_clock;
			the_hero->played += round(elapsed_seconds.count());
			game_clock = std::chrono::system_clock::now();
			save();
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
				|| key == MOUSEWHEEL_DOWN_KEY || key == prefs::keybind(Keys::SCROLL_UP) || key == prefs::keybind(Keys::SCROLL_DOWN)) message::process_input(key);
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
		else if (key == prefs::keybind(Keys::OPEN)) Controls::open();
		else if (key == prefs::keybind(Keys::CLOSE)) Controls::close();
		else action_taken = false;
		if (action_taken) message::reset_count();
	}
}

// Sets up a new game.
void new_game()
{
	STACK_TRACE();
	level = 1;
	hero()->x = hero()->y = 5;
	the_dungeon = std::make_shared<Dungeon>(1, 100, 100);
	the_dungeon->generate();
	the_dungeon->random_start_position(hero()->x, hero()->y);
	the_hero->recenter_camera();
	message::msg("It is very dark. You are likely to be eaten by a grue.");
	save(true);
}

// Queues up a full redraw of the game world.
void queue_redraw()
{
	redraw_full = true;
}

// Saves the game to disk.
void save(bool first_time)
{
	STACK_TRACE();
	if (!first_time) message::msg("Saving game...", MC::INFO);
	try
	{
		SQLite::Transaction transaction(*save_db_ptr);
		if (first_time)
			save_db_ptr->exec("CREATE TABLE world ( id INTEGER PRIMARY KEY AUTOINCREMENT, level INTEGER NOT NULL ); "
					"CREATE TABLE hero ( id INTEGER PRIMARY KEY AUTOINCREMENT, difficulty INTEGER NOT NULL, style INTEGER NOT NULL, played INTEGER NOT NULL );"
					"CREATE TABLE actors ( aid PRUMARY KEY INTEGER UNIQUE NOT NULL, did INTEGER NOT NULL, owner INTEGER NOT NULL, name TEXT, flags INTEGER NOT NULL, x INTEGER NOT NULL, y INTEGER NOT NULL, inventory INTEGER, "
					"tile TEXT NOT NULL ); "
					"CREATE TABLE id_seq (next_id INTEGER PRIMARY KEY AUTOINCREMENT); INSERT INTO id_seq DEFAULT VALUES; UPDATE sqlite_sequence SET seq = " + strx::itos(unique_id()) + " WHERE name='id_seq';" );

		save_db_ptr->exec("DELETE FROM world; DELETE FROM hero; DELETE FROM actors;");
		SQLite::Statement query(*save_db_ptr, "INSERT INTO world (level) VALUES (?)");
		query.bind(1, level);
		query.exec();
		db_ready = true;

		hero()->save();
		the_dungeon->save();
		message::save();
		transaction.commit();
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
	if (!first_time) message::amend(" done!");
}

// Returns a pointer to the save file handle.
SQLite::Database* save_db()
{
	return save_db_ptr;
}

// Read-only access to the save slot currently in use.
unsigned short slot()
{
	return save_slot;
}

// Gets a unique item ID for a SQLite save file.
unsigned long long unique_id()
{
	STACK_TRACE();
	if (!db_ready)
	{
		// Use this simple system for the initial dungeon generation, then we can rely on SQLite for the rest.
		static unsigned long long temporary_id = 1000;
		return ++temporary_id;
	}
	try
	{
		save_db_ptr->exec("DELETE FROM id_seq; INSERT INTO id_seq DEFAULT VALUES;");
		SQLite::Statement query(*save_db_ptr, "SELECT last_insert_rowid();");
		if (query.executeStep()) return query.getColumn("last_insert_rowid()").getInt64();
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
	return 0;
}

}	// namespace world
