// inventory.h -- The Inventory class, which can be attached to an Actor and contains other Actors.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#pragma once
#include "duskfall.h"

class Actor;	// defined in actor.h


class Inventory
{
public:
			Inventory(Actor *new_owner, unsigned long long new_id) : id(new_id), owner(new_owner) { }
	void	load();	// Loads this Inventory from disk.
	void	save();	// Saves this Inventory to disk.

	unsigned long long	id;		// The unique ID for this Inventory.

private:
	vector<shared_ptr<Actor>>	contents;
	Actor*				owner;	// The Actor that owns this Inventory. This should NOT be a shared_ptr, as that creates some nasty issues during object destruction.
};
