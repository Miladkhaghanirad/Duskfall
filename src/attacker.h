// attacker.h -- The Attacker class, which allows Actors to (attempt to) deal damage to other Actors with an attached Defender.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "duskfall.h"
#pragma once

class Actor;	// defined in actor.h

#define ATTACKER_FLAG_IS_HERO	(1 << 0)


class Attacker
{
public:
			Attacker(unsigned long long new_id) : flags(0), id(new_id), power(0) { }
	void	attack(Actor *owner, Actor *target);	// Attacks another Actor!
	bool	is_hero() const { return (flags & ATTACKER_FLAG_IS_HERO) == ATTACKER_FLAG_IS_HERO; }	// Checks if this Attacker is the Hero.
	void	load();	// Loads this Attacker from disk.
	void	save();	// Saves this Attacker to disk.

	unsigned int		flags;	// The flags for this Attacker.
	unsigned long long	id;		// The unique ID of this Attacker.
	unsigned short		power;	// This Attacker's attack power.
};
