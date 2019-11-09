// actor.h -- The Actor class, encompassing all items, NPCs, and other dynamic things in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

class AI;			// defined in ai.h
class Attacker;		// defined in attacker.h
class Defender;		// defined in defender.h
class Inventory;	// defined in inventory.h
enum class Colour : unsigned char;	// defined in iocore.h

#define ACTOR_FLAG_BLOCKER		(1 << 0)	// Does this Actor block movement?
#define ACTOR_FLAG_BLOCKS_LOS	(1 << 1)	// Is this Actor large enough to block line-of-sight?
#define ACTOR_FLAG_MONSTER		(1 << 2)	// Does this Actor count as a monster?
#define ACTOR_FLAG_ITEM			(1 << 3)	// Does this Actor count as an item?
#define ACTOR_FLAG_INVISIBLE	(1 << 4)	// Is this Actor invisible?
#define ACTOR_FLAG_DOOR			(1 << 5)	// Is this Actor a door?
#define ACTOR_FLAG_ANIMATED		(1 << 6)	// Does this Actor have an animated sprite?
#define ACTOR_FLAG_PROPER_NOUN	(1 << 7)	// Does this Actor have a proper noun for a name (e.g. 'David', rather than 'the orc').


class Actor
{
public:
					Actor(unsigned long long new_id);
	virtual			~Actor();
	void			add_ai(string type, unsigned long long new_id);	// Adds AI to this Actor.
	void			clear_flag(unsigned int flag);	// Clears a flag on this Actor.
	string			get_name(bool first_letter_caps) const;	// Gets the name of this Actor, with 'the' at the start if it doesn't have a proper noun name.
	bool			has_low_priority_rendering() const;	// Does this Actor have lower-priority rendering (i.e. other Actors go on top)?
	bool			has_proper_noun() const;	// Does this Actor have a proper noun for a name?
	bool			is_animated() const;	// Does this Actor have an animated sprite?
	bool			is_blocker() const;		// Does this Actor block movement into its tile?
	bool			is_door() const;		// Is this Actor some sort of door?
	bool			is_invisible() const;	// Is this Actor unable to be normally seen?
	bool			is_item() const;		// Is this Actor an item that can be picked up?
	bool			is_los_blocker() const;	// Does this Actor block line-of-sight?
	bool			is_monster() const;		// Is this Actor an NPC or monster?
	virtual void	load(unsigned long long owner_id);	// Loads this Actor's data from disk.
	virtual void	save(unsigned long long owner_id);	// Saves this Actor's data to disk.
	void			set_flag(unsigned int flag);	// Sets a flag on this Actor.
	virtual void	tile_react() { }	// Reacts to other Actors on the tile this Actor is standing on.

	shared_ptr<AI>	ai;			// If this Actor has AI, this is where its 'brain' is.
	string			ai_type;	// The type of AI attached to this Actor.
	shared_ptr<Attacker>	attacker;	// If this Actor is an Attacker, it attaches here.
	shared_ptr<Defender>	defender;	// If this Actor is a Defender, it attaches here.
	unsigned char	flags;		// The Actor's individual flags.
	unsigned long long	id;		// The unique ID for this Actor.
	shared_ptr<Inventory>	inventory;	// If this Actor has an Inventory, it attaches here.
	string			name;		// The Actor's name.
	string			sprite;		// The graphical tile used by this Actor.
	unsigned short	x, y;		// X,Y coordinates on the current dungeon level.
};
