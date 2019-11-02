// world.h -- The World object, containing the main game loop and 'heartbeat' system, keeping track of in-game time and triggering events.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

namespace SQLite { class Database; }	// defined in SQLiteCpp/SQLiteCpp.h
class Dungeon;	// defined in dungeon.h
class Hero;		// defined in hero.h


class World
{
public:
						World(unsigned int new_slot, bool new_save);
						~World();
	shared_ptr<Dungeon>	dungeon() { return the_dungeon; }	// Returns a pointer to the Dungeon object.
	shared_ptr<Hero>	hero() { return the_hero; }			// Returns a pointer to the Hero object.
	void				load();			// Loads the game from disk.
	void				main_loop();	// The main game loop!
	void				new_game();		// Sets up a new game.
	void				queue_recalc_lighting() { recalc_lighting = true; }	// Queues up a recalculation of the game's dynamic lighting.
	void				queue_redraw() { redraw_full = true; }				// Queues up a full redraw of the game world.
	void				save(bool announce);	// Saves the game to disk.
	SQLite::Database*	save_db() { return save_db_ptr; }	// Returns a pointer to the save file handle.
	unsigned short	slot() { return save_slot; }	// Read-only access to the save slot currently in use.

private:
	bool				recalc_lighting;	// Recalculate the dynamic lighting at the start of the next turn.
	bool				redraw_full;	// Redraw the dungeon entirely at the start of the next turn.
	SQLite::Database	*save_db_ptr;	// SQLite handle for the save game file.
	shared_ptr<Hero>	the_hero;		// The main Hero object.
	shared_ptr<Dungeon>	the_dungeon;	// The current dungeon level.
	unsigned short		save_slot;		// The save file slot.
	unsigned short		level;			// The current dungeon level depth.
};

void				new_world(unsigned short slot, bool new_save);	// Creates a new World in the specified save slot.
shared_ptr<World>	world();		// Returns a pointer to the main World object, or creates a new one if none exists.
