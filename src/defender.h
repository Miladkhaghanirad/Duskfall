// defender.h -- The Defender class, which allows Actors to take damage from Attackers or other sources.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "duskfall.h"
#pragma once

class Actor;	// defined in actor.h

#define DEFENDER_FLAG_IS_HERO	(1 << 0)


class Defender
{
public:
			Defender(unsigned long long new_id) : armour(0), flags(0), hp(0), hp_max(0), id(new_id) { }
	void	die(shared_ptr<Actor> owner);	// It's just a flesh wound!
	bool	is_hero() const { return (flags & DEFENDER_FLAG_IS_HERO) == DEFENDER_FLAG_IS_HERO; }	// Checks if this Defender is the Hero.
	void	load();	// Loads this Defender from disk.
	void	save();	// Saves this Defender to disk.
	void	take_damage(unsigned int damage, shared_ptr<Actor> owner);	// Tells this Defender to take damage, and die if necessary.

	unsigned short		armour;		// The armour rating of this Defender.
	unsigned int		flags;		// The flags for this Defender.
	unsigned short		hp, hp_max;	// The current and maximum hit points of this Defender.
	unsigned long long	id;		// The unique ID of this Defender.
};
