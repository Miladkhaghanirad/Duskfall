// ai.h -- The AI classes, which handle both NPC 'intelligence', and objects that update their own state.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

class Actor;	// defined in actor.h


class AI
{
public:
					AI(shared_ptr<Actor> new_owner) : owner(new_owner) { }
	virtual			~AI() { }
	virtual bool	travel(short x_dir, short y_dir);	// Attempts to travel in a given direction.

protected:
	shared_ptr<Actor>	owner;	// The Actor that owns this AI.
};
