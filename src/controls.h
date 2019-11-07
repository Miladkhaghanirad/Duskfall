// controls.h -- The Controls class, which translates input from the player into game actions.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"
#include "ai.h"


class Controls : public AI
{
public:
			Controls(Actor* new_owner) : AI(new_owner) { }
	void	close();	// Attempts to close a door.
	void	open();		// Attempts to open a door.
	bool	travel(short x_dir, short y_dir) override;	// Attempts to travel in a given direction.

private:
	static void	open_door(shared_ptr<Actor> door);	// Opens a specified door.
};
