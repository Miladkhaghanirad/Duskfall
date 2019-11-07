// actor.h -- The Actor class, encompassing all items, NPCs, and other dynamic things in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

class AI;			// defined in ai.h
class Inventory;	// defined in inventory.h
enum class Colour : unsigned char;	// defined in iocore.h

#define ACTOR_FLAG_BLOCKER		(1 << 0)	// Does this Actor block movement?
#define ACTOR_FLAG_BLOCKS_LOS	(1 << 1)	// Is this Actor large enough to block line-of-sight?
#define ACTOR_FLAG_MONSTER		(1 << 2)	// Does this Actor count as a monster?
#define ACTOR_FLAG_ITEM			(1 << 3)	// Does this Actor count as an item?
#define ACTOR_FLAG_INVISIBLE	(1 << 4)	// Is this Actor invisible?
#define ACTOR_FLAG_DOOR			(1 << 5)	// Is this Actor a door?
#define ACTOR_FLAG_ANIMATED		(1 << 6)	// Does this Actor have an animated sprite?


class Actor
{
public:
					Actor(unsigned long long new_id, unsigned int new_dungeon_id = 0, unsigned long long new_owner_id = 0);
	virtual			~Actor();
	void			clear_flag(unsigned int flag);	// Clears a flag on this Actor.
	bool			has_low_priority_rendering() const;	// Does this Actor have lower-priority rendering (i.e. other Actors go on top)?
	bool			is_animated() const;	// Does this Actor have an animated sprite?
	bool			is_blocker() const;		// Does this Actor block movement into its tile?
	bool			is_door() const;		// Is this Actor some sort of door?
	bool			is_invisible() const;	// Is this Actor unable to be normally seen?
	bool			is_item() const;		// Is this Actor an item that can be picked up?
	bool			is_los_blocker() const;	// Does this Actor block line-of-sight?
	bool			is_monster() const;		// Is this Actor an NPC or monster?
	virtual void	load();	// Loads this Actor's data from disk.
	virtual void	save();	// Saves this Actor's data to disk.
	void			set_flag(unsigned int flag);	// Sets a flag on this Actor.
	virtual void	tile_react() { }	// Reacts to other Actors on the tile this Actor is standing on.

	shared_ptr<AI>	ai;			// If this Actor has AI, this is where its 'brain' is.
	unsigned int	dungeon_id;	// The ID of the dungeon level this Actor is on.
	unsigned char	flags;		// The Actor's individual flags.
	unsigned long long	id;		// The unique ID for this Actor.
	shared_ptr<Inventory>	inventory;	// If this Actor has an Inventory, it attaches here.
	string			name;		// The Actor's name.
	unsigned long long	owner_id;	// ID of an Actor that 'owns' this Actor (e.g. an inventory containing items)
	string			tile;		// The graphical tile used by this Actor.
	unsigned short	x, y;		// X,Y coordinates on the current dungeon level.
};
