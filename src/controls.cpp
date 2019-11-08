// controls.cpp -- The Controls class, which translates input from the player into game actions.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "controls.h"
#include "dungeon.h"
#include "hero.h"
#include "inventory.h"
#include "iocore.h"
#include "menu.h"
#include "message.h"
#include "prefs.h"
#include "static-data.h"
#include "strx.h"
#include "world.h"

#include "guru.h"

// Attempts to close a door.
void Controls::close()
{
	STACK_TRACE();
	message::msg("Close something in which direction? ");
	message::render();
	iocore::flip();
	int x_dir, y_dir;
	bool success = iocore::get_direction(x_dir, y_dir);
	if (!success)
	{
		message::amend("{5E}Nevermind.");
		return;
	}
	shared_ptr<Dungeon> dungeon = world::dungeon();
	Tile* tile = dungeon->tile(owner->x + x_dir, owner->y + y_dir);
	shared_ptr<Actor> door = nullptr;
	for (auto actor : *tile->actors())
	{
		if (actor->is_door() && !actor->is_blocker())
		{
			door = actor;
			break;
		}
	}

	if (door)
	{
		message::msg("You close the door.");
		door->set_flag(ACTOR_FLAG_BLOCKER);
		door->set_flag(ACTOR_FLAG_BLOCKS_LOS);
		door->sprite = door->sprite.substr(0, door->sprite.size() - 5);
		door->name = door->name.substr(0, door->name.size() - 9);
		world::dungeon()->recalc_lighting();
		world::queue_redraw();
	}
	else message::msg("That isn't something you can close.", MC::WARN);
}

// Picks an inventory item to drop.
void Controls::drop()
{
	STACK_TRACE();
	if (!owner->inventory->contents.size())
	{
		message::msg("You are not carrying anything.", MC::WARN);
		return;
	}
	auto inv_menu = std::make_shared<Menu>();
	inv_menu->set_title("DROP ITEM");
	for (auto item : owner->inventory->contents)
		inv_menu->add_item(item->name);
	int choice = inv_menu->render();
	if (choice >= 0) drop_item(choice);
}

// Drops an item on the ground.
void Controls::drop_item(unsigned int id)
{
	STACK_TRACE();
	auto item_ptr = owner->inventory->contents.at(id);
	owner->inventory->contents.erase(owner->inventory->contents.begin() + id);
	item_ptr->x = owner->x;
	item_ptr->y = owner->y;
	world::dungeon()->tile(owner->x, owner->y)->add_actor(item_ptr);
	message::msg("You drop the " + item_ptr->name + ".");
}

// Interacts with carried items.
void Controls::inventory()
{
	STACK_TRACE();
	if (!owner->inventory->contents.size())
	{
		message::msg("You are not carrying anything.", MC::WARN);
		return;
	}
	auto inv_menu = std::make_shared<Menu>();
	inv_menu->set_title("INVENTORY");
	for (auto item : owner->inventory->contents)
		inv_menu->add_item(item->name);
	int choice = inv_menu->render();
	if (choice >= 0) inventory_menu(choice);
}

// Item menu for a specified inventory item.
void Controls::inventory_menu(unsigned int id)
{
	STACK_TRACE();
	auto item = owner->inventory->contents.at(id);
	auto inv_menu = std::make_shared<Menu>();
	inv_menu->set_title(strx::str_toupper(item->name));
	inv_menu->add_item("Drop");
	int choice = inv_menu->render();
	switch(choice)
	{
		case 0: drop_item(choice); break;
	}
}

// Attempts to open a door.
void Controls::open()
{
	STACK_TRACE();
	message::msg("Open something in which direction? ");
	message::render();
	iocore::flip();
	int x_dir, y_dir;
	bool success = iocore::get_direction(x_dir, y_dir);
	if (!success)
	{
		message::amend("{5E}Nevermind.");
		return;
	}
	shared_ptr<Dungeon> dungeon = world::dungeon();
	Tile* tile = dungeon->tile(owner->x + x_dir, owner->y + y_dir);
	const unsigned int door_id = tile->has_door();
	if (door_id != UINT_MAX) open_door(tile->actors()->at(door_id));
	else message::msg("That isn't something you can open.", MC::WARN);
}

// Opens a specified door.
void Controls::open_door(shared_ptr<Actor> door)
{
	STACK_TRACE();
	message::msg("You open the door.");
	door->clear_flag(ACTOR_FLAG_BLOCKER);
	door->clear_flag(ACTOR_FLAG_BLOCKS_LOS);
	door->name += " (open)";
	door->sprite += "_OPEN";
	world::dungeon()->recalc_lighting();
	world::queue_redraw();
}

// Processes a keypress.
bool Controls::process_key(unsigned int key)
{
	STACK_TRACE();

	if (key == prefs::keybind(Keys::NORTH)) { travel(0, -1); return true; }
	if (key == prefs::keybind(Keys::SOUTH)) { travel(0, 1); return true; }
	if (key == prefs::keybind(Keys::EAST)) { travel(1, 0); return true; }
	if (key == prefs::keybind(Keys::WEST)) { travel(-1, 0); return true; }
	if (key == prefs::keybind(Keys::NORTHEAST)) { travel(1, -1); return true; }
	if (key == prefs::keybind(Keys::NORTHWEST)) { travel(-1, -1); return true; }
	if (key == prefs::keybind(Keys::SOUTHEAST)) { travel(1, 1); return true; }
	if (key == prefs::keybind(Keys::SOUTHWEST)) { travel(-1, 1); return true; }
	if (key == prefs::keybind(Keys::SCROLL_TOP) || key == prefs::keybind(Keys::SCROLL_BOTTOM) || key == prefs::keybind(Keys::SCROLL_PAGEUP) || key == prefs::keybind(Keys::SCROLL_PAGEDOWN) || key == MOUSEWHEEL_UP_KEY
			|| key == MOUSEWHEEL_DOWN_KEY || key == prefs::keybind(Keys::SCROLL_UP) || key == prefs::keybind(Keys::SCROLL_DOWN)) { message::process_input(key); return true; }
	if (key == prefs::keybind(Keys::OPEN)) { open(); return true; }
	if (key == prefs::keybind(Keys::CLOSE)) { close(); return true; }
	if (key == prefs::keybind(Keys::TAKE)) { take(); return true; }
	if (key == prefs::keybind(Keys::INVENTORY)) { inventory(); return true; }
	if (key == prefs::keybind(Keys::DROP)) { drop(); return true; }
	return false;
}

// Picks up nearby items.
void Controls::take()
{
	STACK_TRACE();
	vector<shared_ptr<Actor>> items_here;
	vector<unsigned int> item_ids;
	bool first_pick = true;
	do
	{
		items_here.clear();
		item_ids.clear();
		const auto tile = world::dungeon()->tile(owner->x, owner->y);
		const auto item_list = tile->items_here();
		for (unsigned int i = 0; i < item_list.size(); i++)
		{
			auto actor = tile->actors()->at(item_list.at(i));
			if (actor->is_invisible()) continue;
			items_here.push_back(actor);
			item_ids.push_back(i);
		}
		if (!items_here.size())
		{
			message::msg("There is nothing here that you can pick up!", MC::WARN);
			return;
		}
		else if (first_pick && items_here.size() == 1)
		{
			take_item(item_ids.at(0));
			first_pick = false;
		}
		else
		{
			shared_ptr<Menu> items_menu = std::make_shared<Menu>();
			for (auto item : items_here)
				items_menu->add_item(item->name);
			items_menu->set_title("TAKE ITEMS");
			int choice = items_menu->render();
			if (choice < 0) return;
			take_item(item_ids.at(choice));
			first_pick = false;
		}
	} while (items_here.size() > 1);
}

// Picks up a specific item.
void Controls::take_item(unsigned int item)
{
	STACK_TRACE();
	Tile* tile = world::dungeon()->tile(owner->x, owner->y);
	auto item_ptr = tile->actors()->at(item);
	tile->actors()->erase(tile->actors()->begin() + item);
	owner->inventory->contents.push_back(item_ptr);
	message::msg("You pick up the " + item_ptr->name + ".");
}

// Attempts to travel in a given direction.
bool Controls::travel(short x_dir, short y_dir)
{
	STACK_TRACE();
	if (AI::travel(x_dir, y_dir))
	{
		const int screen_x = world::hero()->x + world::hero()->camera_off_x;
		const int screen_y = world::hero()->y + world::hero()->camera_off_y;
		if (screen_x <= 5 || screen_x >= iocore::get_tile_cols() - 3) world::hero()->recenter_camera_horiz();
		if (screen_y <= 5 || screen_y >= iocore::get_tile_rows() - 3) world::hero()->recenter_camera_vert();
		return true;
	}
	else
	{
		shared_ptr<Dungeon> dungeon = world::dungeon();
		const auto tile = world::dungeon()->tile(owner->x + x_dir, owner->y + y_dir);
		for (auto actor : *tile->actors())
		{
			if (actor->is_blocker())
			{
				if (actor->is_door()) open_door(actor);
				else message::msg("The " + actor->name + " blocks your path!", MC::WARN);
				return false;
			}
		}
	}
	return false;
}
