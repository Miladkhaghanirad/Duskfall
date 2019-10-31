// controls.cpp -- The Controls class, which translates input from the player into game actions.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "controls.h"


// Attempts to travel in a given direction.
bool Controls::travel(short x_dir, short y_dir)
{
	if (AI::travel(x_dir, y_dir)) return true;
	return false;
}
