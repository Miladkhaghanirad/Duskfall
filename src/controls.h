// controls.h -- The Controls class, which translates input from the player into game actions.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"
#include "ai.h"


class Controls : public AI
{
public:
			Controls(Actor* new_owner, unsigned long long new_id) : AI(new_owner, new_id) { }
	void	close();		// Attempts to close a door.
	void	drop();			// Picks an inventory item to drop.
	void	inventory();	// Interacts with carried items.
	void	react_to_attack(Actor*) override { }	// Do nothing! This is to override the default AI behaviour; the player chooses what they do, so we don't want any automated behaviour here.
	void	open();			// Attempts to open a door.
	bool	process_key(unsigned int key);	// Processes a keypress.
	void	take();			// Picks up nearby items.
	void	tick() override { }	// This will eventually be used for things like poison effects on the player, but for now, it does nothing.
	bool	travel(short x_dir, short y_dir) override;	// Attempts to travel in a given direction.

private:
	void	drop_item(unsigned int id);	// Drops an item on the ground.
	void	inventory_menu(unsigned int id);	// Item menu for a specified inventory item.
	void	open_door(shared_ptr<Actor> door);	// Opens a specified door.
	void	take_item(unsigned int id);	// Picks up a specific item.
};
