// hero.h -- The Hero class, where the player data and other important stuff about the world is stored.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

namespace SQLite { class Database; }	// defined in SQLiteCpp/SQLiteCpp.h
class Dungeon;	// defined in dungeon.h


class Hero
{
public:
			Hero(unsigned int slot);
			~Hero();
	void	load();	// Loads the Hero's data from disk, along with the rest of the game world.
	void	save();	// Saves the Hero's data to disk, along with the rest of the game world.

	SQLite::Database	*save_db;	// SQLite handle for the save game file.
	Dungeon				*dungeon;	// The dungeon level.
	unsigned char		difficulty;	// The current difficulty level.
	unsigned char		style;		// The hero's style.
	string				name;		// The hero's name.
	unsigned short		x, y;		// X,Y coordinates on the current dungeon level.
	unsigned short		level;		// The current dungeon level depth.

private:
	unsigned short		save_slot;	// The save file slot.
};

extern Hero*	hero;	// External access to the Hero object.
