// ai.h -- The AI classes, which handle both NPC 'intelligence', and objects that update their own state.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

class Actor;	// defined in actor.h

enum class AIState : uint8_t { NONE, DEAD, SLEEPING, AGGRO };

#define TRACKING_TURNS	20	// After this many turns without line-of-sight, an NPC will stop tracking the player.


class AI
{
public:
					AI(Actor *new_owner, unsigned long long new_id) : id(new_id), state(AIState::SLEEPING), tracking_count(0), owner(new_owner) { }
	virtual			~AI() { }
	virtual void	load();	// Loads this AI from disk.
	virtual void	react_to_attack(Actor *assailant);	// Reacts to being attacked by something!
	virtual void	save();	// Saves this AI to disk.
	virtual void	tick() = 0;	// The AI takes a turn.
	virtual bool	travel(short x_dir, short y_dir);	// Attempts to travel in a given direction.

	unsigned long long	id;		// The unique ID for this AI.
	AIState				state;	// The current state of this AI.
	unsigned char		tracking_count;	// The NPC will give up tracking the player after a while.

protected:
	Actor*	owner;	// The Actor that owns this AI. This should NOT be a shared_ptr, as that creates some nasty issues during object destruction.
};
