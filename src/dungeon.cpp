// dungeon.cpp -- The Dungeon class, which handles all the core functionality of a dungeon level, including generation, rendering, saving and loading.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "dungeon.h"
#include "guru.h"
#include "iocore.h"


Dungeon::Dungeon(unsigned short new_width, unsigned short new_height) : width(new_width), height(new_height)
{
	STACK_TRACE();
	tiles = new Tile[width * height]();

	// Set a default layout with basic floor tiles and an impassible wall.
	Tile indestructible_wall;
	indestructible_wall.name = 1;
	indestructible_wall.glyph = '#';
	indestructible_wall.colour = Colour::CGA_LGRAY;
	indestructible_wall.flags = TILE_FLAG_IMPASSIBLE | TILE_FLAG_OPAQUE;
	Tile basic_floor;
	basic_floor.name = 0;
	basic_floor.glyph = static_cast<unsigned short>(Glyph::MIDDOT);
	basic_floor.colour = Colour::CGA_GRAY;

	for (unsigned short y = 0; y < height; y++)
	{
		if (y == 0 || y == height - 1)
		{
			for (unsigned short x = 0; x < width; x++)
				tiles[x + y * width] = indestructible_wall;
		}
		else
		{
			for (unsigned short x = 0; x < width; x++)
			{
				if (x == 0 || x == width - 1) tiles[x + y * width] = indestructible_wall;
				else tiles[x + y * width] = basic_floor;
			}
		}
	}
}

Dungeon::~Dungeon()
{
	STACK_TRACE();
	delete[] tiles;
}

// Renders the dungeon on the screen.
void Dungeon::render()
{
	STACK_TRACE();
	for (unsigned int x = 0; x < width; x++)
	{
		for (unsigned int y = 0; y < height; y++)
		{
			Tile &here = tiles[x + y * width];
			iocore->print_at(static_cast<Glyph>(here.glyph), x, y, here.colour);
		}
	}
	iocore->flip();
}

// Sets a specified tile, with error checking.
void Dungeon::set_tile(unsigned short x, unsigned short y, Tile &tile)
{
	STACK_TRACE();
	if (x >= width || y >= height) guru->halt("Attempted to set out-of-bounds tile.");
	tiles[x + y * width] = tile;
}
