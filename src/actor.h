// actor.h -- The Actor class, encompassing all items, NPCs, and other dynamic things in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

class AI;	// defined in ai.h
enum class Colour : unsigned char;	// defined in iocore.h

#define ACTOR_FLAG_BLOCKER		(1 << 0)	// Does this Actor block movement?
#define ACTOR_FLAG_BLOCKS_LOS	(1 << 1)	// Is this Actor large enough to block line-of-sight?
#define ACTOR_FLAG_MONSTER		(1 << 2)	// Does this Actor count as a monster?
#define ACTOR_FLAG_ITEM			(1 << 3)	// Does this Actor count as an item?
#define ACTOR_FLAG_INVISIBLE	(1 << 4)	// Is this Actor invisible?
#define ACTOR_FLAG_DOOR			(1 << 5)	// Is this Actor a door?


class Actor
{
public:
					Actor();
	virtual			~Actor();
	bool			blocker() { return (flags & ACTOR_FLAG_BLOCKER) == ACTOR_FLAG_BLOCKER; }
	bool			blocks_los() { return (flags & ACTOR_FLAG_BLOCKS_LOS) == ACTOR_FLAG_BLOCKS_LOS; }
	bool			door() { return (flags & ACTOR_FLAG_DOOR) == ACTOR_FLAG_DOOR; }
	void			clear_flag(unsigned int flag);	// Clears a flag on this Actor.
	bool			invisible() { return (flags & ACTOR_FLAG_INVISIBLE) == ACTOR_FLAG_INVISIBLE; }
	bool			is_item() { return (flags & ACTOR_FLAG_ITEM) == ACTOR_FLAG_ITEM; }
	bool			is_monster() { return (flags & ACTOR_FLAG_MONSTER) == ACTOR_FLAG_MONSTER; }
	bool			low_priority_rendering() { return is_item(); }
	virtual void	load(unsigned int actor_id, unsigned int dungeon_id);		// Loads this Actor's data from disk.
	virtual void	save(unsigned int actor_id, unsigned int dungeon_id);		// Saves this Actor's data to disk.
	void			set_flag(unsigned int flag);	// Sets a flag on this Actor.

	AI*				ai;	// If this Actor has AI, this is where its 'brain' is.
	Colour			colour;	// The colour of this Actor's glyph.
	string			name;	// The Actor's name.
	unsigned char	flags;	// The Actor's individual flags.
	unsigned short	glyph;	// The glyph to represent this Actor in the world.
	unsigned short	x, y;	// X,Y coordinates on the current dungeon level.
};
