
// sidebar.cpp -- The sidebar, displaying information about the nearby elements of the dungeon as well as player stats.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "actor.h"
#include "dungeon.h"
#include "iocore.h"
#include "sidebar.h"
#include "static-data.h"

#include "guru.h"


namespace sidebar
{

vector<shared_ptr<Actor>>	actors_in_sight;	// The list of visible Actors.
unsigned int				sidebar_row = 1;	// The current row we're writing on.
vector<shared_ptr<Tile>>	tiles_in_sight;		// The list of visible Tiles.


// Tells the sidebar system that an Actor is visible.
void actor_in_sight(shared_ptr<Actor> actor)
{
	STACK_TRACE();
	for (auto actor_check : actors_in_sight)
		if (actor_check->name == actor->name) return;
	for (unsigned int i = 0; i < actors_in_sight.size(); i++)
	{
		if (actor->name < actors_in_sight.at(i)->name)
		{
			actors_in_sight.insert(actors_in_sight.begin() + i, actor);
			return;
		}
	}
	actors_in_sight.push_back(actor);
}

// Writes a line of text on the sidebar.
void print(string text, unsigned int offset_x)
{
	STACK_TRACE();
	unsigned int x_pos = iocore::get_cols_narrow() - SIDEBAR_WIDTH_5X8 + offset_x;
	iocore::ansi_print(text, x_pos + offset_x, sidebar_row, PRINT_FLAG_NARROW);
	sidebar_row++;
}

// Renders the sidebar!
void render()
{
	STACK_TRACE();
	iocore::rect(iocore::get_cols() - SIDEBAR_WIDTH_8X8 - 1, 0, SIDEBAR_WIDTH_8X8 + 1, iocore::get_rows() - 1, Colour::BLACK);
	sidebar_row = 1;

	if (actors_in_sight.size())
	{
		bool monsters = false, items = false;
		for (auto actor : actors_in_sight)
		{
			if (actor->is_monster()) monsters = true;
			if (actor->is_item()) items = true;
			if (monsters && items) break;
		}

		if (monsters)
		{
			sidebar::print("{5F}Monsters nearby:");
			for (auto actor : actors_in_sight)
			{
				if (!actor->is_monster()) continue;
				iocore::print_at(actor->glyph, iocore::get_cols() - SIDEBAR_WIDTH_8X8 + 1, sidebar_row, actor->colour);
				sidebar::print("{57}" + actor->name, 1);
			}
			sidebar_row++;
		}

		if (items)
		{
			sidebar::print("{5F}Items nearby:");
			for (auto actor : actors_in_sight)
			{
				if (!actor->is_item()) continue;
				iocore::print_at(actor->glyph, iocore::get_cols() - SIDEBAR_WIDTH_8X8 + 1, sidebar_row, actor->colour);
				sidebar::print("{57}" + actor->name, 1);
			}
			sidebar_row++;
		}
	}

	if (tiles_in_sight.size())
	{
		sidebar::print("{5F}Features:");
		for (auto tile : tiles_in_sight)
		{
			iocore::print_at(static_cast<Glyph>(tile->glyph), iocore::get_cols() - SIDEBAR_WIDTH_8X8 + 1, sidebar_row, tile->colour);
			sidebar::print("{57}" + data::tile_name(tile->name), 1);
		}
		sidebar_row++;
	}
}

// Tells the sidebar system to forget the Actors and Tiles it has stored.
void reset_lists()
{
	STACK_TRACE();
	actors_in_sight.clear();
	tiles_in_sight.clear();
}

// Tells the sidebar system that a Tile is visible.
void tile_in_sight(Tile tile)
{
	STACK_TRACE();
	for (auto tile_check : tiles_in_sight)
		if (tile_check->name == tile.name) return;
	shared_ptr<Tile> tile_ptr = std::make_shared<Tile>(tile);
	for (unsigned int i = 0; i < tiles_in_sight.size(); i++)
	{
		if (data::tile_name(tile.name) < data::tile_name(tiles_in_sight.at(i)->name))
		{
			tiles_in_sight.insert(tiles_in_sight.begin() + i, tile_ptr);
			return;
		}
	}
	tiles_in_sight.push_back(tile_ptr);
}

}	// namespace sidebar
