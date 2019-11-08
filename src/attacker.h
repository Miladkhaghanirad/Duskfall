// attacker.h -- The Attacker class, which allows Actors to (attempt to) deal damage to other Actors with an attached Defender.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "duskfall.h"
#pragma once


class Attacker
{
public:
			Attacker(unsigned long long new_id) : id(new_id), power(0) { }
	void	load();	// Loads this Attacker from disk.
	void	save();	// Saves this Attacker to disk.

	unsigned long long	id;		// The unique ID of this Attacker.
	unsigned short		power;	// This Attacker's attack power.
};
