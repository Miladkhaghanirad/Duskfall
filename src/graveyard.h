// graveyard.h -- Keeps track of Actors, Dungeons and other things which are due to be deleted from SQLite during the next game-save.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#pragma once
#include "duskfall.h"


namespace graveyard
{

// The destroy_* functions mark objects for removal from the database.
void	destroy_actor(unsigned long long id);
void	destroy_ai(unsigned long long id);
void	destroy_attacker(unsigned long long id);
void	destroy_defender(unsigned long long id);
void	destroy_inventory(unsigned long long id);

void	purge();	// Called during a game-save, purges all the doomed data from the database.

}
