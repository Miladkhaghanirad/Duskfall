// hero.h -- The Hero class, where the player data and other important stuff about the world is stored.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"
#include "actor.h"

class Hero : public Actor
{
public:
			Hero();
			~Hero();
	void	load();	// Loads the Hero's data from disk.
	void	save();	// Saves the Hero's data to disk, along with the rest of the game world.

	unsigned char		difficulty;	// The current difficulty level.
	unsigned char		style;		// The hero's style.
	string				name;		// The hero's name.
};
