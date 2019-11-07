// hero.h -- The Hero class, where the player data and other important stuff about the world is stored.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"
#include "actor.h"

class Controls;	// defined in controls.h

class Hero : public Actor
{
public:
			Hero(unsigned long long new_id, unsigned int new_dungeon_id = 0, unsigned long long new_owner_id = 0);
			~Hero();
	shared_ptr<Controls>	controls();	// Returns a pointer to this Hero's Controls AI.
	void	load() override;	// Loads the Hero's data from disk.
	void	recenter_camera();	// Recenters the camera on the Hero's position.
	void	recenter_camera_horiz();	// Recenters the camera horizontally on the Hero's position, not adjusting the vertical.
	void	recenter_camera_vert();		// Recenters the camera vertically on the Hero's position, not adjusting the horizontal.
	void	save() override;	// Saves the Hero's data to disk, along with the rest of the game world.
	void	tile_react() override;	// Reacts to other Actors on the tile this Actor is standing on.

	short			camera_off_x, camera_off_y;	// The 'camera' offset, allowing us to scroll around a multi-screen dungeon.
	unsigned char	difficulty;	// The current difficulty level.
	unsigned int	played;		// Time played.
	unsigned char	style;		// The hero's style.
};
