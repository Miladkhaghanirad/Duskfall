// dungeon.h -- The Dungeon class, which handles all the core functionality of a dungeon level, including generation, rendering, saving and loading.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"
#include <set>

class Actor;	// defined in actor.h

#define TILE_FLAG_IMPASSIBLE	(1 << 0)
#define TILE_FLAG_OPAQUE		(1 << 1)
#define TILE_FLAG_WALL			(1 << 2)
#define TILE_FLAG_PERMAWALL		(1 << 3)
#define TILE_FLAG_EXPLORED		(1 << 4)
#define TILE_FLAG_FLOOR			(1 << 5)


class Tile
{
public:
	unsigned char	flags;	// Various properties of this tile.
	unsigned int	name;	// An integer for the tile's hashed name, which is decoded elsewhere.
	unsigned int	sprite;	// An integer for the tile's hashed tile sprite, which is decoded elsewhere.

			Tile() : flags(0), name(0), sprite(0) { }
	bool	destroyable_wall() { return wall() && !permawall(); }
	bool	explored() { return (flags & TILE_FLAG_EXPLORED) == TILE_FLAG_EXPLORED; }
	string	get_name();		// Returns the name of this tile.
	string	get_sprite(int x, int y);	// Returns the sprite name for rendering this tile.
	bool	impassible() { return (flags & TILE_FLAG_IMPASSIBLE) == TILE_FLAG_IMPASSIBLE; }
	bool	opaque() { return (flags & TILE_FLAG_OPAQUE) == TILE_FLAG_OPAQUE; }
	bool	permawall() { return (flags & TILE_FLAG_PERMAWALL) == TILE_FLAG_PERMAWALL; }
	bool	wall() { return (flags & TILE_FLAG_WALL) == TILE_FLAG_WALL; }

private:
	string	check_neighbours(int x, int y, bool wall);	// Checks nearby tiles to modify floor and wall sprites.
	bool	neighbour_identical(int x, int y);	// Check if a neighbour is an identical tile.
};

class Dungeon
{
public:
			Dungeon(unsigned short new_level, unsigned short new_width = 0, unsigned short new_height = 0);
			~Dungeon();
	void	generate();	// Generates a new dungeon level.
	void	generate_type_a();	// Generates a type A dungeon level.
	unsigned short	get_height() { return height; }	// Read-only access to the dungeon height.
	unsigned short	get_width() { return width; }	// Read-only access to the dungeon width.
	void	load();		// Loadds this dungeon from disk.
	bool	los_check(unsigned short x1, unsigned short y1);	// Checks to see if a given tile is within the player's line of sight.
	void	map_view(bool see_all = false);	// View the dungeon map in its entirety.
	void	random_start_position(unsigned short &x, unsigned short &y);	// Picks a viable random starting location.
	void	recalc_lighting();	// Clears the lighting array and recalculates all light sources.
	void	render(bool see_all = false);	// Renders the dungeon on the screen.
	void	save();		// Saves this dungeon to disk.
	void	set_tile(unsigned short x, unsigned short y, Tile &tile);	// Sets a specified tile, with error checking.
	shared_ptr<Tile>	tile(unsigned short x, unsigned short y);	// Retrieves a specified tile pointer.

	vector<shared_ptr<Actor>>	actors;	// The Actors stored in this dungeon level.

private:
	unsigned short	level;			// The vertical level of this dungeon.
	unsigned short	width, height;	// The width and height of this area.
	Tile			*tiles;			// An array of Tiles which make up this area.
	unsigned char	*lighting;		// An array of 8-bit integers defining the light level or visibility of tiles.
	unsigned int	*region;		// The region the current tile belongs to.
	std::set<std::pair<unsigned short, unsigned short>> dynamic_light_temp, dynamic_light_temp_walls;	// Temporary data used by the dynamic lighting system.

	void	carve_room(unsigned short x, unsigned short y, unsigned short w, unsigned short h, unsigned int new_region);	// Carves out a square room.
	void	cast_light(unsigned int x, unsigned int y, unsigned int radius, unsigned int row, float start_slope, float end_slope, unsigned int xx, unsigned int xy, unsigned int yx, unsigned int yy,  bool always_visible);
	unsigned char	diminish_light(float distance, float falloff);					// Dims a specified light source.
	void	explore(unsigned short x, unsigned short y);					// Marks a given tile as explored.
	bool	find_empty_tile(unsigned short x, unsigned short y, unsigned short w, unsigned short h, unsigned short &rx, unsigned short &ry);	// Attempts to find an empty tile within the specified space.
	bool	is_dead_end(unsigned short x, unsigned short y);				// Check to see if this tile is a dead-end.
	s_rgb	light_surface(s_rgb surface_colour, s_rgb light_colour);		// Applies a light source to a surface, affecting its colour.
	void	recalc_light_source(unsigned short x, unsigned short y, unsigned short radius, bool always_visible = false);	// Recalculates a specific light source.
	void	region_floodfill(unsigned short x, unsigned short y, unsigned int new_region);		// Flood-fills a specified area with a new region ID.
	bool	tile_contains_los_blocker(unsigned short x, unsigned short y);	// Checks if a tile contains an Actor that blocks line-of-sight.
	bool	touches_two_regions(unsigned short x, unsigned short y);	// Checks if this tile touches a different region.
	bool	viable_doorway(unsigned short x, unsigned short y);			// Checks if this tile is a viable doorway.
	bool	viable_maze_position(unsigned short x, unsigned short y);	// Checks if this tile is a viable position to build a maze corridor.
	bool	viable_room_position(unsigned short x, unsigned short y, unsigned short w, unsigned short h);	// Checks if this is a viable position to place a new room.
};
