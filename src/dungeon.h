// dungeon.h -- The Dungeon class, which handles all the core functionality of a dungeon level, including generation, rendering, saving and loading.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"
#include <set>

class Actor;	// defined in actor.h
namespace SQLite { class Statement; }	// defined in SQLiteCpp/SQLiteCpp.h

#define TILE_FLAG_IMPASSIBLE	(1 << 0)
#define TILE_FLAG_OPAQUE		(1 << 1)
#define TILE_FLAG_WALL			(1 << 2)
#define TILE_FLAG_PERMAWALL		(1 << 3)
#define TILE_FLAG_EXPLORED		(1 << 4)
#define TILE_FLAG_FLOOR			(1 << 5)


class Tile
{
public:
			Tile() : flags(0), x(0), y(0) { }
	vector<shared_ptr<Actor>>*	actors() { return &contained_actors; }	// Return a pointer to the contained_actors vector.
	void	add_actor(shared_ptr<Actor> actor);	// Adds an Actor to this Tile.
	bool	contains_los_blocker() const;	// Checks if this Tile contains an Actor that blocks line-of-sight.
	string	get_sprite() const;		// Returns the sprite name for rendering this tile.
	unsigned int	has_door() const;	// Checks if a door is present here, and returns the Actor vector ID if so.
	bool	is_destroyable_wall() const;	// Is this Tile a wall that can be destroyed?
	bool	is_explored() const;	// Has this Tile been explored?
	bool	is_floor() const;		// Is this Tile a floor of some kind?
	bool	is_impassible() const;	// Is this Tile something that blocks movement?
	bool	is_opaque() const;		// Does this Tile block line-of-sight?
	bool	is_permawall() const;	// Is this Tile a wall that can never be destroyed under any circumstances?
	bool	is_wall() const;		// Is this Tile a wall of some kind?
	vector<unsigned int>	items_here() const;	// Returns a list of all contained Actors with the ACTOR_FLAG_ITEM flag.
	void	load(SQLite::Statement &query, unsigned long long dungeon_id);	// Loads this Tile from disk.
	vector<unsigned int>	mobs_here() const;	// Returns a list of all contained Actors with the ACTOR_FLAG_MONSTER flag.
	void	remove_actor(unsigned int id);		// Removes an Actor from this Tile.
	void	save(unsigned long long dungeon_id);	// Saves this Tile to disk.
	void	set_sprite(string new_sprite) { sprite = new_sprite; }	// Sets the sprite for this Tile.

	unsigned char	flags;	// Various properties of this tile.
	string			name;	// The name of this Tile.
	unsigned short	x, y;	// The X,Y coordinates for this Tile.

private:
	string	check_neighbours(int x, int y, bool wall) const;	// Checks nearby tiles to modify floor and wall sprites.
	bool	neighbour_identical(int x, int y) const;			// Check if a neighbour is an identical tile.

	vector<shared_ptr<Actor>>	contained_actors;	// Actors who are present within this Tile.
	string	sprite;			// The sprite representing this tile. Private, since we should be using get_sprite() to access it.
};

class Dungeon
{
public:
			Dungeon(unsigned short new_id, unsigned short new_width = 0, unsigned short new_height = 0);
			~Dungeon();
	void	generate();	// Generates a new dungeon level.
	void	generate_type_a();	// Generates a type A dungeon level.
	unsigned short	get_height() const { return height; }	// Read-only access to the dungeon height.
	unsigned int	get_id() const { return id; }		// Read-only access to the dungeon ID.
	unsigned short	get_width() const { return width; }	// Read-only access to the dungeon width.
	void	load();		// Loadds this dungeon from disk.
	bool	los_check(unsigned short x1, unsigned short y1) const;	// Checks to see if a given tile is within the player's line of sight.
	void	map_view(bool see_all = false);	// View the dungeon map in its entirety.
	void	random_start_position(unsigned short &x, unsigned short &y) const;	// Picks a viable random starting location.
	void	recalc_lighting();	// Clears the lighting array and recalculates all light sources.
	void	render(bool see_all = false);	// Renders the dungeon on the screen.
	void	save();		// Saves this dungeon to disk.
	void	set_tile(unsigned short x, unsigned short y, Tile &new_tile);	// Sets a specified tile, with error checking.
	Tile*	tile(unsigned short x, unsigned short y) const;	// Retrieves a specified tile pointer.

private:
	std::set<std::pair<unsigned short, unsigned short>> dynamic_light_temp, dynamic_light_temp_walls;	// Temporary data used by the dynamic lighting system.
	unsigned short		height;			// The height of the dungeon (Y).
	unsigned long long	id;				// The unique ID of this Dungeon.
	unsigned char		*lighting;		// An array of 8-bit integers defining the light level or visibility of tiles.
	unsigned int		*region;		// The region the current tile belongs to (used during dungeon generation).
	Tile				*tiles;			// An array of Tiles which make up this area.
	unsigned short		width;			// The width of the dungeon (X).

	void	carve_room(unsigned short x, unsigned short y, unsigned short w, unsigned short h, unsigned int new_region);	// Carves out a square room.
	void	cast_light(unsigned int x, unsigned int y, unsigned int radius, unsigned int row, float start_slope, float end_slope, unsigned int xx, unsigned int xy, unsigned int yx, unsigned int yy,  bool always_visible);
	unsigned char	diminish_light(float distance, float falloff) const;	// Dims a specified light source.
	void	explore(unsigned short x, unsigned short y);					// Marks a given tile as explored.
	std::pair<unsigned short, unsigned short>	find_empty_tile(unsigned short x, unsigned short y, unsigned short w, unsigned short h) const;	// Attempts to find an empty tile within the specified space.
	bool	is_dead_end(unsigned short x, unsigned short y) const;			// Check to see if this tile is a dead-end.
	void	recalc_light_source(unsigned short x, unsigned short y, unsigned short radius, bool always_visible = false);	// Recalculates a specific light source.
	void	region_floodfill(unsigned short x, unsigned short y, unsigned int new_region);		// Flood-fills a specified area with a new region ID.
	bool	touches_two_regions(unsigned short x, unsigned short y) const;	// Checks if this tile touches a different region.
	int		viable_doorway(unsigned short x, unsigned short y) const;		// Checks if this tile is a viable doorway.
	bool	viable_maze_position(unsigned short x, unsigned short y) const;	// Checks if this tile is a viable position to build a maze corridor.
	bool	viable_room_position(unsigned short x, unsigned short y, unsigned short w, unsigned short h) const;	// Checks if this is a viable position to place a new room.
};
