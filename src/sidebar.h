// sidebar.h -- The sidebar, displaying information about the nearby elements of the dungeon as well as player stats.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#pragma once
#include "duskfall.h"

class Actor;	// defined in actor.h
class Tile;		// defined in dungeon.h

#define SIDEBAR_WIDTH_8X8	16
#define SIDEBAR_WIDTH_5X8	24


namespace sidebar
{

void	actor_in_sight(shared_ptr<Actor> actor);	// Tells the sidebar system that an Actor is visible.
void	print(string text, unsigned int offset_x = 0);	// Writes a line of text on the sidebar.
void	render();					// Renders the sidebar!
void	reset_lists();				// Tells the sidebar system to forget the Actors and Tiles it has stored.
void	tile_in_sight(Tile tile);	// Tells the sidebar system that a Tile is visible.

}	// namespace sidebar
