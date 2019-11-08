// inventory.h -- The Inventory class, which can be attached to an Actor and contains other Actors.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#pragma once
#include "duskfall.h"

class Actor;	// defined in actor.h


class Inventory
{
public:
			Inventory(unsigned long long new_id) : id(new_id) { }
	void	load();	// Loads this Inventory from disk.
	void	save();	// Saves this Inventory to disk.

	vector<shared_ptr<Actor>>	contents;	// The Actors stored in this Inventory.
	unsigned long long	id;		// The unique ID for this Inventory.
};
