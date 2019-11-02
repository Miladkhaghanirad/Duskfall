// actor.h -- The Actor class, encompassing all items, NPCs, and other dynamic things in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

class AI;	// defined in ai.h
enum class Colour : unsigned char;	// defined in iocore.h

#define ACTOR_FLAG_BLOCKER	(1 << 0)	// Does this Actor block movement?


class Actor
{
public:
					Actor();
	virtual			~Actor();
	bool			blocker() { return (flags & ACTOR_FLAG_BLOCKER) == ACTOR_FLAG_BLOCKER; }
	virtual void	load();		// Loads this Actor's data from disk.
	virtual void	save();		// Saves this Actor's data to disk.

	shared_ptr<AI>	ai;		// If this Actor has AI, this is where its 'brain' is.
	Colour			colour;	// The colour of this Actor's glyph.
	string			name;	// The Actor's name.
	unsigned char	flags;	// The Actor's individual flags.
	unsigned short	glyph;	// The glyph to represent this Actor in the world.
	unsigned short	x, y;	// X,Y coordinates on the current dungeon level.
};
