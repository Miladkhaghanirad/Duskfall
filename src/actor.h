// actor.h -- The Actor class, encompassing all items, NPCs, and other dynamic things in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

class AI;	// defined in ai.h


class Actor
{
public:
					Actor();
	virtual			~Actor();
	virtual void	load() = 0;	// All Actors should be able to load themselves from disk.
	virtual void	save() = 0;	// All Actors should be able to save themselves to disk.

	AI				*ai;	// If this Actor has AI, this is where its 'brain' is.
	unsigned short	x, y;	// X,Y coordinates on the current dungeon level.
};
