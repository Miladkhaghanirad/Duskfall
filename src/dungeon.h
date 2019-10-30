// dungeon.h -- The Dungeon class, which handles all the core functionality of a dungeon level, including generation, rendering, saving and loading.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

enum class Colour : unsigned char;	// defined in iocore.h

#define TILE_FLAG_IMPASSIBLE	(1 << 0)
#define TILE_FLAG_OPAQUE		(1 << 1)


struct Tile
{
	unsigned short	glyph;	// The symbol used to represent this tile.
	Colour			colour;	// The base colour of this tile, before any lighting effects.
	unsigned char	flags;	// Various properties of this tile.
	unsigned char	name;	// An integer for the tile's name, which is decoded elsewhere.

			Tile() : glyph('?'), colour(static_cast<Colour>(0x30)), flags(0) { }
	bool	impassible() { return (flags & TILE_FLAG_IMPASSIBLE) == TILE_FLAG_IMPASSIBLE; }
	bool	opaque() { return (flags & TILE_FLAG_OPAQUE) == TILE_FLAG_OPAQUE; }
};

class Dungeon
{
public:
			Dungeon(unsigned short new_width, unsigned short new_height);
			~Dungeon();
	void	render();	// Renders the dungeon on the screen.
	void	set_tile(unsigned short x, unsigned short y, Tile &tile);	// Sets a specified tile, with error checking.

private:
	unsigned short	width, height;	// The width and height of this area.
	Tile			*tiles;		// An array of Tiles which make up this area.
};
