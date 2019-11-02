// actor.cpp -- The Actor class, encompassing all items, NPCs, and other dynamic things in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "actor.h"
#include "ai.h"
#include "iocore.h"


Actor::Actor() : ai(nullptr), colour(Colour::WHITE), glyph('?'), x(0), y(0) { }

Actor::~Actor() { }

// Loads this Actor's data from disk.
void Actor::load()
{
	STACK_TRACE();
}

// Saves this Actor's data to disk.
void Actor::save()
{
	STACK_TRACE();
}
