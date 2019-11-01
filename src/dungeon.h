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
			Dungeon(unsigned short new_level, unsigned short new_width = 0, unsigned short new_height = 0);
			~Dungeon();
	void	generate();	// Generates a new dungeon level.
	void	load();		// Loadds this dungeon from disk.
	void	recalc_lighting();	// Clears the lighting array and recalculates all light sources.
	void	render();	// Renders the dungeon on the screen.
	void	save();		// Saves this dungeon to disk.
	void	set_tile(unsigned short x, unsigned short y, Tile &tile);	// Sets a specified tile, with error checking.
	Tile*	tile(unsigned short x, unsigned short y);	// Retrieves a specified tile pointer.

private:
	unsigned short	level;			// The vertical level of this dungeon.
	unsigned short	width, height;	// The width and height of this area.
	Tile			*tiles;			// An array of Tiles which make up this area.
	s_rgb			*lighting;		// An array of 8-bit integers defining the light level or visibility of tiles.

	void	cast_light(unsigned int x, unsigned int y, unsigned int radius, unsigned int row, float start_slope, float end_slope, unsigned int xx, unsigned int xy, unsigned int yx, unsigned int yy,  bool always_visible);
	s_rgb	diminish_light(s_rgb colour, float distance, float falloff);	// Dims a specified RGB colour.
	s_rgb	light_surface(s_rgb surface_colour, s_rgb light_colour);		// Applies a light source to a surface, affecting its colour.
	void	light_tile(unsigned short x, unsigned short y, s_rgb colour);	// Applies light to a specified tile.
	void	recalc_light_source(unsigned short x, unsigned short y, s_rgb colour, unsigned short radius, bool always_visible = false);	// Recalculates a specific light source.
};
