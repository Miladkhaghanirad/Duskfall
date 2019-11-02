// controls.h -- The Controls class, which translates input from the player into game actions.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"
#include "ai.h"

class Controls : public AI
{
public:
			Controls(shared_ptr<Actor> new_owner) : AI(new_owner) { }
	bool	travel(short x_dir, short y_dir) override;	// Attempts to travel in a given direction.
};
