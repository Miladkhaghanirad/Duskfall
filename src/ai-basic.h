// ai-basic.h -- The BasicAI class, which handles standard, basic AI for monsters in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "ai.h"

class Actor;	// defined in actor.h


class BasicAI : public AI
{
public:
			BasicAI(Actor* new_owner, unsigned long long new_id) : AI(new_owner, new_id) { }
	void	tick() override;	// The AI takes a turn.
};
