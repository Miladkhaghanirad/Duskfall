// defender.h -- The Defender class, which allows Actors to take damage from Attackers or other sources.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "duskfall.h"
#pragma once


class Defender
{
public:
			Defender(unsigned long long new_id) : armour(0), hp(0), hp_max(0), id(new_id) { }
	void	load();	// Loads this Defender from disk.
	void	save();	// Saves this Defender to disk.

	unsigned short		armour;		// The armour rating of this Defender.
	unsigned short		hp, hp_max;	// The current and maximum hit points of this Defender.
	unsigned long long	id;		// The unique ID of this Defender.
};
