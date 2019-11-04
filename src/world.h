// world.h -- The World object, containing the main game loop and 'heartbeat' system, keeping track of in-game time and triggering events.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

namespace SQLite { class Database; }	// defined in SQLiteCpp/SQLiteCpp.h
class Dungeon;	// defined in dungeon.h
class Hero;		// defined in hero.h


namespace world
{

shared_ptr<Dungeon>	dungeon();		// Returns a pointer to the Dungeon object.
shared_ptr<Hero>	hero();			// Returns a pointer to the Hero object.
void				load();			// Loads the game from disk.
void				main_loop();	// The main game loop!
void				new_game();		// Sets up a new game.
void				new_world(unsigned short slot, bool new_save);	// Creates a new World in the specified save slot.
void				queue_recalc_lighting();	// Queues up a recalculation of the game's dynamic lighting.
void				queue_redraw();				// Queues up a full redraw of the game world.
void				save(bool first_time = false);	// Saves the game to disk.
SQLite::Database*	save_db();		// Returns a pointer to the save file handle.
unsigned short		slot();			// Read-only access to the save slot currently in use.

}	// namespace world
