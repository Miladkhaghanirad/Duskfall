// dungeon.cpp -- The Dungeon class, which handles all the core functionality of a dungeon level, including generation, rendering, saving and loading.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "ai.h"
#include "dungeon.h"
#include "guru.h"
#include "hero.h"
#include "iocore.h"
#include "loading.h"
#include "mathx.h"
#include "message.h"
#include "prefs.h"
#include "static-data.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"

#include <algorithm>
#include <chrono>
#include <cmath>


// Used by the shadowcasting system below.
static signed char shadowcast_multipliers[4][8] = {
	{ 1, 0, 0, -1, -1, 0, 0, 1 },
	{ 0, 1, -1, 0, 0, -1, 1, 0 },
	{ 0, 1, 1, 0, 0, -1, -1, 0 },
	{ 1, 0, 0, 1, -1, 0, 0, -1 }
};


Dungeon::Dungeon(unsigned short new_id, unsigned short new_width, unsigned short new_height) : height(new_height), id(new_id), region(nullptr), width(new_width)
{
	STACK_TRACE();
	if (!new_width || !new_height) return;
	tiles = new Tile[width * height]();
	lighting = new unsigned char[width * height]();
}

Dungeon::~Dungeon()
{
	STACK_TRACE();
	delete[] tiles;
	delete[] lighting;
}

// Adds an Actor's AI to the active AI list.
void Dungeon::add_active_ai(shared_ptr<AI> new_ai)
{
	STACK_TRACE();
	if (std::find(active_ai.begin(), active_ai.end(), new_ai) != active_ai.end())
	{
		guru::nonfatal("Attempt to add re-add existing active AI!", GURU_WARN);
		return;
	}
	active_ai.push_back(new_ai);
}

// Carves out a square room.
void Dungeon::carve_room(unsigned short x, unsigned short y, unsigned short w, unsigned short h, unsigned int new_region)
{
	STACK_TRACE();

	Tile basic_floor = data::get_tile("BASIC_FLOOR");
	for (unsigned int rx = x; rx < x + w; rx++)
	{
		for (unsigned int ry = y; ry < y + h; ry++)
		{
			set_tile(rx, ry, basic_floor);
			if (region) region[rx + ry * width] = new_region;
		}
	}

	// Put things in this room!
	if (w <= 1 || h <= 1) return;
	unsigned int monsters_here = mathx::rnd(4) - 1;
	unsigned int items_here = mathx::rnd(3) - 1;
	monsters_here = items_here = 2;
	while (monsters_here)
	{
		monsters_here--;
		string new_mob = "ORC";
		if (mathx::rnd(10) >= 8) new_mob = "TROLL";
		else if (mathx::rnd(10000) == 1) new_mob = "PLATINO";
		auto success = find_empty_tile(x, y, w, h);
		if (success.first >= width || success.second >= height) break;
		tile(success.first, success.second)->add_actor(data::get_mob(new_mob));
	}
	while (items_here)
	{
		items_here--;
		string new_item = "JACKET_POTATO";
		if (mathx::rnd(2) == 1) new_item = "SQUIDDLYBOX";
		auto success = find_empty_tile(x, y, w, h);
		if (success.first >= width || success.second >= height) break;
		tile(success.first, success.second)->add_actor(data::get_item(new_item));
	}
}

// Shadowcasting engine, thanks to RogueBasin's C++ implementation of Bj�rn Bergstr�m's recursive shadowcasting FOV algorithm.
void Dungeon::cast_light(unsigned int x, unsigned int y, unsigned int radius, unsigned int row, float start_slope, float end_slope, unsigned int xx, unsigned int xy, unsigned int yx, unsigned int yy, bool always_visible)
{
	STACK_TRACE();
	if (start_slope < end_slope) return;
	float next_start_slope = start_slope;
	for (unsigned int i = row; i <= radius; i++)
	{
		bool blocked = false;
		for (int dx = -i, dy = -i; dx <= 0; dx++)
		{
			float l_slope = (dx - 0.5) / (dy + 0.5);
			float r_slope = (dx + 0.5) / (dy - 0.5);
			if (start_slope < r_slope) continue;
			else if (end_slope > l_slope) break;

			int sax = dx * xx + dy * xy;
			int say = dx * yx + dy * yy;
			if ((sax < 0 && static_cast<unsigned int>(abs(sax)) > x) || (say < 0 && static_cast<unsigned int>(abs(say)) > y)) continue;
			unsigned int ax = x + sax;
			unsigned int ay = y + say;
			if (ax >= width || ay >= height) continue;

			unsigned int radius2 = radius * radius;
			if (static_cast<unsigned int>(dx * dx + dy * dy) < radius2)
			{
				const unsigned char existing_light = lighting[ax + ay * width];
				if (always_visible || existing_light)
				{
					std::pair<unsigned short, unsigned short> xy = { ax, ay };
					if (dynamic_light_temp.find(xy) == dynamic_light_temp.end()) dynamic_light_temp.insert(xy);
				}
			}

			Tile* the_tile = tile(ax, ay);
			if (blocked)
			{
				if (the_tile->is_opaque() || the_tile->contains_los_blocker())
				{
					next_start_slope = r_slope;
					continue;
				}
				else
				{
					blocked = false;
					start_slope = next_start_slope;
				}
			}
			else if (the_tile->is_opaque() || the_tile->contains_los_blocker())
			{
				blocked = true;
				next_start_slope = r_slope;
				cast_light(x, y, radius, i + 1, start_slope, l_slope, xx, xy, yx, yy, always_visible);
			}
		}
		if (blocked) break;
	}
}

// Dims a specified light source.
unsigned char Dungeon::diminish_light(float distance, float falloff) const
{
	STACK_TRACE();
	float divisor = pow(distance, falloff) - distance + 1;
	return static_cast<unsigned char>(round(255.0f / divisor));
}

// Marks a given tile as explored.
void Dungeon::explore(unsigned short x, unsigned short y)
{
	STACK_TRACE();
	tiles[x + y * width].flags |= TILE_FLAG_EXPLORED;
}

// Attempts to find an empty tile within the specified space, aborts after too many failures.
std::pair<unsigned short, unsigned short> Dungeon::find_empty_tile(unsigned short x, unsigned short y, unsigned short w, unsigned short h) const
{
	STACK_TRACE();
	unsigned int attempts = 0;
	while (attempts++ < 100)
	{
		unsigned int tx = mathx::rnd(w) - 1 + x;
		unsigned int ty = mathx::rnd(h) - 1 + y;
		if (tile(tx, ty)->is_impassible() || tile(tx, ty)->actors()->size()) continue;
		return std::pair<unsigned short, unsigned short>(tx, ty);
	}
	return std::pair<unsigned short, unsigned short>(USHRT_MAX, USHRT_MAX);
}

// Generates a new dungeon level.
void Dungeon::generate()
{
	STACK_TRACE();

	// Set a default layout with basic floor tiles and an impassible wall.
	Tile indestructible_wall = data::get_tile("BOUNDARY_WALL");
	Tile regular_wall = data::get_tile("BASIC_WALL");

	for (unsigned short y = 0; y < height; y++)
	{
		if (y == 0 || y == height - 1)
		{
			for (unsigned short x = 0; x < width; x++)
				set_tile(x, y, indestructible_wall);
		}
		else
		{
			for (unsigned short x = 0; x < width; x++)
			{
				if (x == 0 || x == width - 1) set_tile(x, y, indestructible_wall);
				else set_tile(x, y, regular_wall);
			}
		}
	}

	// For now, there is only one type of dungeon level generator.
	generate_type_a();
}

// Generates a type A dungeon level. This is based roughly on the procedural dungeon generator by Bob Nystrom in Hauberk, (c) 2000-2014.
void Dungeon::generate_type_a()
{
	STACK_TRACE();
	region = new unsigned int[width * height]();
	unsigned int current_region = 1;

	// First, place a number of randomly-sized rooms in random positions. Any rooms which overlap other rooms are removed.
	const unsigned int attempts = (width * height) / 25;	// a 100x100 dungeon would have 400 attempts. I have not tested if this scales well for very large dungeons, so be careful.

	for (unsigned int i = 0; i < attempts; i++)
	{
		const unsigned int room_width = (mathx::rnd(3) * 2) + 3, room_height = (mathx::rnd(3) * 2) + 3;	// Room size varies from 5x5 to 9x9.
		const unsigned int room_x = (mathx::rnd((width - room_width) / 2) * 2);
		const unsigned int room_y = (mathx::rnd((height - room_height) / 2) * 2);
		if (!viable_room_position(room_x, room_y, room_width, room_height)) continue;	// Abort if it overlaps anything  else.
		carve_room(room_x, room_y, room_width, room_height, current_region++);	// We're good to go, place the room!
	}

	// Now fill the remaining solid areas in with mazes. I'm adapting the Growing Tree algorithm from here: http://www.astrolog.org/labyrnth/algrithm.htm
	// Pretty sure I didn't implement it quite right, but it's working okay for now.
	vector<std::pair<unsigned short, unsigned short>> viable_maze_cells;
	for (unsigned short x = 2; x < width - 2; x += 2)
		for (unsigned short y = 2; y < height - 2; y += 2)
			if (viable_maze_position(x, y)) viable_maze_cells.push_back(std::pair<unsigned short, unsigned short>(x, y));

	while (viable_maze_cells.size())
	{
		unsigned int choice = mathx::rnd(viable_maze_cells.size() - 1);
		unsigned short x = viable_maze_cells.at(choice).first;
		unsigned short y = viable_maze_cells.at(choice).second;
		viable_maze_cells.erase(viable_maze_cells.begin() + choice);
		if (!viable_maze_position(x, y)) continue;
		current_region++;
		while(true)
		{
			carve_room(x, y, 1, 1, current_region);
			vector<std::pair<signed char, signed char>> viable_directions;
			if (viable_maze_position(x + 1, y)) viable_directions.push_back(std::pair<signed char, signed char>(1, 0));
			if (viable_maze_position(x - 1, y)) viable_directions.push_back(std::pair<signed char, signed char>(-1, 0));
			if (viable_maze_position(x, y + 1)) viable_directions.push_back(std::pair<signed char, signed char>(0, 1));
			if (viable_maze_position(x, y - 1)) viable_directions.push_back(std::pair<signed char, signed char>(0, -1));
			if (!viable_directions.size()) break;
			else
			{
				std::pair<signed char, signed char> dir = viable_directions.at(mathx::rnd(viable_directions.size()) - 1);
				x += dir.first;
				y += dir.second;
			}
			if (!viable_maze_position(x, y)) break;
		}
	}

	// Link all the regions together.
	vector<std::pair<unsigned short, unsigned short>> region_connectors;
	for (unsigned short x = 2; x < width - 2; x++)
		for (unsigned short y = 2; y < width - 2; y++)
			if (touches_two_regions(x, y)) region_connectors.push_back(std::pair<unsigned short, unsigned short>(x, y));
	while(region_connectors.size())
	{
		unsigned int choice = mathx::rnd(region_connectors.size()) - 1;
		std::pair<unsigned short, unsigned short> xy = region_connectors.at(choice);
		region_connectors.erase(region_connectors.begin() + choice);

		vector<std::pair<signed char, signed char>> viable_directions;
		unsigned int current_region = region[xy.first + xy.second * width];
		if (region[(xy.first + 2) + xy.second * width] != current_region && region[(xy.first + 2) + xy.second * width] != 0) viable_directions.push_back(std::pair<signed char, signed char>(1, 0));
		if (region[(xy.first - 2) + xy.second * width] != current_region && region[(xy.first - 2) + xy.second * width] != 0) viable_directions.push_back(std::pair<signed char, signed char>(-1, 0));
		if (region[xy.first + (xy.second + 2) * width] != current_region && region[xy.first + (xy.second + 2) * width] != 0) viable_directions.push_back(std::pair<signed char, signed char>(0, 1));
		if (region[xy.first + (xy.second - 2) * width] != current_region && region[xy.first + (xy.second - 2) * width] != 0) viable_directions.push_back(std::pair<signed char, signed char>(0, -1));
		if (!viable_directions.size()) continue;

		while (viable_directions.size())
		{
			choice = mathx::rnd(viable_directions.size()) - 1;
			std::pair<signed char, signed char> dir = viable_directions.at(choice);
			viable_directions.erase(viable_directions.begin() + choice);
			unsigned int target_region = region[(xy.first + (dir.first * 2)) + (xy.second + (dir.second * 2)) * width];
			carve_room(xy.first + dir.first, xy.second + dir.second, 1, 1, target_region);
			region_floodfill(xy.first + dir.first, xy.second + dir.second, current_region);
			if (mathx::rnd(3) != 1) break;
		}
	}

	// Dead ends are kinda bad. Let's get rid of as many as we can, while keeping a few to make things interesting.
	vector<std::pair<unsigned short, unsigned short>> dead_ends;
	for (unsigned short x = 2; x < width - 2; x++)
		for (unsigned short y = 2; y < height - 2; y++)
			if (is_dead_end(x, y)) dead_ends.push_back(std::pair<unsigned short, unsigned short>(x, y));
	while (dead_ends.size())
	{
		std::pair<unsigned short, unsigned short> xy = dead_ends.at(0);
		dead_ends.erase(dead_ends.begin());
		if (!is_dead_end(xy.first, xy.second) || mathx::rnd(3) != 1) continue;
		vector<std::pair<signed char, signed char>> viable_directions;
		if (tile(xy.first + 1, xy.second)->is_destroyable_wall()) viable_directions.push_back(std::pair<signed char, signed char>(1, 0));
		if (tile(xy.first - 1, xy.second)->is_destroyable_wall()) viable_directions.push_back(std::pair<signed char, signed char>(-1, 0));
		if (tile(xy.first, xy.second + 1)->is_destroyable_wall()) viable_directions.push_back(std::pair<signed char, signed char>(0, 1));
		if (tile(xy.first, xy.second - 1)->is_destroyable_wall()) viable_directions.push_back(std::pair<signed char, signed char>(0, -1));
		if (!viable_directions.size()) continue;
		unsigned int choice = mathx::rnd(viable_directions.size()) - 1;
		carve_room(xy.first + viable_directions.at(choice).first, xy.second + viable_directions.at(choice).second, 1, 1, 0);
	}

	// Attempt to place doors in room entrances.
	for (unsigned short x = 2; x < width - 2; x++)
	{
		for (unsigned short y = 2; y < height - 2; y++)
		{
			const int door_type = viable_doorway(x, y);
			if (door_type && mathx::rnd(3) == 1)
			{
				shared_ptr<Actor> door = data::get_tile_feature("DOOR");
				tile(x, y)->add_actor(door);
				if (door_type == 2) door->sprite += "_HORIZ";
			}
		}
	}

	delete[] region;
}

// Check to see if this tile is a dead-end.
bool Dungeon::is_dead_end(unsigned short x, unsigned short y) const
{
	STACK_TRACE();
	unsigned int surrounding_walls = 0;
	if (tile(x, y)->is_wall()) return false;
	if (tile(x + 1, y)->is_wall()) surrounding_walls++;
	if (tile(x - 1, y)->is_wall()) surrounding_walls++;
	if (tile(x, y + 1)->is_wall()) surrounding_walls++;
	if (tile(x, y - 1)->is_wall()) surrounding_walls++;
	if (surrounding_walls == 3) return true;
	else return false;
}

// Loads this dungeon from disk.
void Dungeon::load()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world::save_db(), "SELECT width, height FROM dungeon WHERE id = ?");
		query.bind(1, static_cast<signed long long>(id));
		if (query.executeStep())
		{
			width = query.getColumn("width").getUInt();
			height = query.getColumn("height").getUInt();
			tiles = new Tile[width * height]();
			lighting = new unsigned char[width * height]();
		}
		else guru::halt("Could not load data for dungeon ID " + strx::uitos(id));

		auto last_redraw = std::chrono::system_clock::now();
		loading::loading_screen(0, "Loading Dungeon...");
		SQLite::Statement tile_query(*world::save_db(), "SELECT * FROM tiles WHERE dungeon_id = ?");
		tile_query.bind(1, static_cast<signed long long>(id));
		unsigned int tile_count = 0;
		while (tile_query.executeStep())
		{
			const auto time_now = std::chrono::system_clock::now();
			const std::chrono::duration<float> elapsed_seconds = time_now - last_redraw;
			if (elapsed_seconds.count() >= 0.1f)
			{
				last_redraw = time_now;
				const unsigned int current_percent = round((static_cast<float>(tile_count) / static_cast<float>(width * height)) * 100);
				loading::loading_screen(current_percent, "Loading Dungeon...");
			}
			unsigned short x = tile_query.getColumn("x").getUInt();
			unsigned short y = tile_query.getColumn("y").getUInt();
			tile(x, y)->x = x;
			tile(x, y)->y = y;
			tile(x, y)->load(tile_query, id);
			tile_count++;
			for (auto actor : *tile(x, y)->actors())
				if (actor->ai && actor->ai->state != AIState::NONE && actor->ai->state != AIState::SLEEPING && actor->ai->state != AIState::DEAD)
					add_active_ai(actor->ai);
		}
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Checks to see if a given tile is within the player's line of sight; optional x2/y2 coordinates can specify another non-player origin.
// Yes, x2/y2 is used as the origin for the player, but it shouldn't matter either way - if all is working correctly, a LoS check should be symmetrical.
// Largely adapted from Bresenham's Line Algorithm on RogueBasin.
bool Dungeon::los_check(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2) const
{
	if (x2 == USHRT_MAX || y2 == USHRT_MAX)
	{
		x2 = world::hero()->x;
		y2 = world::hero()->y;
	}

	int delta_x(x2 - x1);
	// if x1 == x2, then it does not matter what we set here
	signed char const ix((delta_x > 0) - (delta_x < 0));
	delta_x = std::abs(delta_x) << 1;

	int delta_y(y2 - y1);
	// if y1 == y2, then it does not matter what we set here
	signed char const iy((delta_y > 0) - (delta_y < 0));
	delta_y = std::abs(delta_y) << 1;

	if (delta_x >= delta_y)
	{
		// error may go below zero
		int error(delta_y - (delta_x >> 1));

		while (x1 != x2)
		{
			// reduce error, while taking into account the corner case of error == 0
			if ((error > 0) || (!error && (ix > 0)))
			{
				error -= delta_x;
				y1 += iy;
			}
			// else do nothing

			error += delta_y;
			x1 += ix;

			// We're not calling contains_los_blocker() here, as this function is used to see if a tile would be within a player's
			// line of sight, for calculating things like dynamic lighting.
			if (tile(x1, y1)->is_opaque()) return false;
		}
	}
	else
	{
		// error may go below zero
		int error(delta_x - (delta_y >> 1));

		while (y1 != y2)
		{
			// reduce error, while taking into account the corner case of error == 0
			if ((error > 0) || (!error && (iy > 0)))
			{
				error -= delta_y;
				x1 += ix;
			}
			// else do nothing

			error += delta_x;
			y1 += iy;

			if (tile(x1, y1)->is_opaque()) return false;
		}
	}

	return true;
}

// View the dungeon map in its entirety. see_all should only be used in debugging/testing code.
void Dungeon::map_view(bool see_all)
{
	STACK_TRACE();
	recalc_lighting();
	while(true)
	{
		iocore::cls();
		render(see_all);
		iocore::flip();
		const unsigned int key = iocore::wait_for_key();
		if (key == prefs::keybind(Keys::NORTH) || key == prefs::keybind(Keys::NORTHEAST) || key == prefs::keybind(Keys::NORTHWEST)) world::hero()->camera_off_y++;
		else if (key == prefs::keybind(Keys::SOUTH) || key == prefs::keybind(Keys::SOUTHEAST) || key == prefs::keybind(Keys::SOUTHWEST)) world::hero()->camera_off_y--;
		if (key == prefs::keybind(Keys::EAST) || key == prefs::keybind(Keys::SOUTHEAST) || key == prefs::keybind(Keys::NORTHEAST)) world::hero()->camera_off_x--;
		else if (key == prefs::keybind(Keys::WEST) || key == prefs::keybind(Keys::SOUTHWEST) || key == prefs::keybind(Keys::NORTHWEST)) world::hero()->camera_off_x++;
	}
}

// Picks a viable random starting location.
void Dungeon::random_start_position(unsigned short &x, unsigned short &y) const
{
	STACK_TRACE();
	while(true)
	{
		x = mathx::rnd(width - 4) + 2;
		y = mathx::rnd(height - 4) + 2;
		if (tile(x, y)->is_floor()) return;
	}
}

// Recalculates a specific light source.
void Dungeon::recalc_light_source(unsigned short x, unsigned short y, unsigned short radius, bool always_visible)
{
	STACK_TRACE();
	for (unsigned int i = 0; i < 8; i++)
		cast_light(x, y, radius, 1, 1.0, 0.0, shadowcast_multipliers[0][i], shadowcast_multipliers[1][i], shadowcast_multipliers[2][i], shadowcast_multipliers[3][i], always_visible);
	lighting[x + y * width] = 255;

	while (dynamic_light_temp.size())
	{
		auto iterator = dynamic_light_temp.begin();
		std::pair<unsigned short, unsigned short> xy = *iterator;
		dynamic_light_temp.erase(iterator);

		if (!always_visible && tile(xy.first, xy.second)->is_opaque())
		{
			if (los_check(xy.first, xy.second)) dynamic_light_temp_walls.insert(xy);
		} else
		{
			float distance = mathx::grid_dist(x, y, xy.first, xy.second);
			unsigned int total_light = lighting[xy.first + xy.second * width] + diminish_light(distance, 1.05f);
			if (total_light > 255) total_light = 255;
			lighting[xy.first + xy.second * width] = total_light;
		}
	}

	// Brute-force check: look at all the *visible* (to the player) tiles surrounding the wall, and pick the brightest.
	for (auto xy : dynamic_light_temp_walls)
	{
		unsigned char brightest = 30;
		for (short dx = -1; dx <= 1; dx++)
		{
			if (dx + xy.first < 0 || dx + xy.first >= width) continue;
			for (short dy = -1; dy <= 1; dy++)
			{
				if ((dx == 0 && dy == 0) || dy + xy.second < 0 || dy + xy.second >= height) continue;
				if (tile(xy.first + dx, xy.second + dy)->is_opaque()) continue;
				if (!los_check(xy.first + dx, xy.second + dy)) continue;
				unsigned char light = lighting[xy.first + dx + (xy.second + dy) * width];
				if (light > brightest) brightest = light;
			}
		}
		lighting[xy.first + xy.second * width] = brightest;
	}

	dynamic_light_temp.clear();
	dynamic_light_temp_walls.clear();
}

// Clears the lighting array and recalculates all light sources.
void Dungeon::recalc_lighting()
{
	STACK_TRACE();
	memset(lighting, 0, sizeof(unsigned char) * width * height);
	recalc_light_source(world::hero()->x, world::hero()->y, 100, true);
}

// Flood-fills a specified area with a new region ID.
void Dungeon::region_floodfill(unsigned short x, unsigned short y, unsigned int new_region)
{
	STACK_TRACE();
	if (region[x + y * width] == new_region) return;
	if (tile(x, y)->is_wall()) return;

	region[x + y * width] = new_region;
	for (short dx = -1; dx <= 1; dx++)
	{
		for (short dy = -1; dy <= 1; dy++)
		{
			if (dx != 0 && dy != 0) continue;
			if (region[x + dx + (y + dy) * width] != new_region) region_floodfill(x + dx, y + dy, new_region);
		}
	}
}

// Renders the dungeon on the screen.
void Dungeon::render(bool see_all)
{
	STACK_TRACE();
	iocore::cls();
	for (unsigned int x = 0; x < width; x++)
	{
		int screen_x = static_cast<signed int>(x) + world::hero()->camera_off_x;
		if (screen_x < 0 || static_cast<unsigned int>(screen_x) >= iocore::get_tile_cols()) continue;
		for (unsigned int y = 0; y < height; y++)
		{
			int screen_y = static_cast<signed int>(y) + world::hero()->camera_off_y;
			if (screen_y < 0 || static_cast<unsigned int>(screen_y) >= iocore::get_tile_rows()) continue;
#
			Tile* here = tile(x, y);
			unsigned char here_brightness = lighting[x + y * width];
			if (see_all && here_brightness < 50) here_brightness = 50;
			if (here_brightness >= 50)
			{
				shared_ptr<Actor> actor_here = nullptr;
				const auto actors = here->actors();
				if (actors)
				{
					for (auto actor : *actors)
					{
						if (actor->is_invisible()) continue;
						if (actor_here)
						{
							if (actor_here->has_low_priority_rendering() && !actor->has_low_priority_rendering()) actor_here = actor;
						}
						else actor_here = actor;
					}
				}
				iocore::print_tile(here->get_sprite(), screen_x, screen_y, here_brightness);
				if (x == world::hero()->x && y == world::hero()->y) iocore::print_tile(world::hero()->sprite, screen_x, screen_y, here_brightness, true);
				else if (actor_here) iocore::print_tile(actor_here->sprite, screen_x, screen_y, here_brightness, actor_here->is_animated());
				explore(x, y);
			}
			else if (here->is_explored()) iocore::print_tile(here->get_sprite(), screen_x, screen_y, 50);
		}
	}
}

// Saves this dungeon to disk.
void Dungeon::save()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement clear_dungeon(*world::save_db(), "DELETE FROM dungeon WHERE id = ?");
		clear_dungeon.bind(1, static_cast<signed long long>(id));
		clear_dungeon.exec();
		SQLite::Statement clear_tiles(*world::save_db(), "DELETE FROM tiles WHERE dungeon_id = ?");
		clear_tiles.bind(1, static_cast<signed long long>(id));
		clear_tiles.exec();

		SQLite::Statement statement(*world::save_db(), "INSERT INTO dungeon (id, width, height) VALUES (?, ?, ?)");
		statement.bind(1, static_cast<signed long long>(id));
		statement.bind(2, width);
		statement.bind(3, height);
		statement.exec();
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}
	for (unsigned int x = 0; x < width; x++)
		for (unsigned int y = 0; y < height; y++)
			tile(x, y)->save(id);
}

// Sets a specified tile, with error checking.
void Dungeon::set_tile(unsigned short x, unsigned short y, Tile &new_tile)
{
	STACK_TRACE();
	if (x >= width || y >= height)
	{
		guru::nonfatal("Attempted to set out-of-bounds tile.", GURU_CRITICAL);
		return;
	}

	Tile* old_tile = tile(x, y);
	vector<shared_ptr<Actor>> old_actors;
	for (auto actor : *old_tile->actors())
		old_actors.push_back(actor);

	tiles[x + y * width] = new_tile;
	tiles[x + y * width].x = x;
	tiles[x + y * width].y = y;

	for (auto actor : old_actors)
		tiles[x + y * width].actors()->push_back(actor);
}

// Runs any active AI in this Dungeon.
void Dungeon::tick_ai()
{
	STACK_TRACE();
	for (unsigned int i = 0; i < active_ai.size(); i++)
	{
		shared_ptr<AI> ai = active_ai.at(i);
		if (ai->state == AIState::NONE || ai->state == AIState::SLEEPING || ai->state == AIState::DEAD)
		{
			// If the AI is inactive or destroyed, just quietly remove it from the list and continue.
			active_ai.erase(active_ai.begin() + i);
			i--;
			continue;
		}
		ai->tick();
	}
}

// Retrieves a specified tile pointer.
Tile* Dungeon::tile(unsigned short x, unsigned short y) const
{
	STACK_TRACE();
	if (x >= width || y >= height) guru::halt("Attempted to retrieve out-of-bounds tile.");
	return &tiles[x + y * width];
}

// Checks if this tile touches a different region.
bool Dungeon::touches_two_regions(unsigned short x, unsigned short y) const
{
	STACK_TRACE();
	if (x < 2 || y < 2 || x >= width - 2 || y >= height - 2) return false;
	unsigned int current_region = region[x + y * width];
	if (current_region == 0) return false;
	if (region[(x + 2) + y * width] != current_region && region[(x + 2) + y * width] > 0) return true;
	if (region[(x - 2) + y * width] != current_region && region[(x - 2) + y * width] > 0) return true;
	if (region[x + (y + 2) * width] != current_region && region[x + (y + 2) * width] > 0) return true;
	if (region[x + (y - 2) * width] != current_region && region[x + (y - 2) * width] > 0) return true;
	return false;
}

// Checks if this tile is a viable doorway.
int Dungeon::viable_doorway(unsigned short x, unsigned short y) const
{
	STACK_TRACE();
	if (x < 2 || y < 2 || x >= width - 2 || y >= height - 2) return false;
	if (!tile(x, y)->is_floor()) return false;	// Only basic floor can become a door.
	if (tile(x, y)->has_door() != UINT_MAX) return false;	// Don't place a door where one already exists.

	if (tile(x - 1, y)->is_wall() && tile(x + 1, y)->is_wall())
	{
		if (tile(x, y + 1)->is_wall()) return 0;
		if (tile(x, y - 1)->is_wall()) return 0;
		if (!tile(x - 1, y + 1)->is_wall() && !tile(x + 1, y + 1)->is_wall()) return 1;
		if (!tile(x - 1, y - 1)->is_wall() && !tile(x + 1, y - 1)->is_wall()) return 1;
	}
	else if (tile(x, y - 1)->is_wall() && tile(x, y + 1)->is_wall())
	{
		if (tile(x - 1, y)->is_wall()) return 0;
		if (tile(x + 1, y)->is_wall()) return 0;
		if (!tile(x + 1, y - 1)->is_wall() && !tile(x + 1, y + 1)->is_wall()) return 2;
		if (!tile(x - 1, y - 1)->is_wall() && !tile(x - 1, y + 1)->is_wall()) return 2;
	}

	return 0;
}

// Checks if this tile is a viable position to build a maze corridor.
bool Dungeon::viable_maze_position(unsigned short x, unsigned short y) const
{
	STACK_TRACE();
	if (x < 2 || y < 2 || x >= width - 2 || y >= height - 2) return false;

	unsigned short current_exits = 0;
	if (!tile(x, y)->is_wall()) return false;
	for (int ox = -1; ox <= 1; ox++)
	{
		for (int oy = -1; oy <= 1; oy++)
		{
			if (ox == 0 && oy == 0) continue;
			if (!tile(x + ox, y + oy)->is_wall()) current_exits++;
		}
	}
	if (current_exits <= 1) return true;
	else return false;
}

// Checks if this is a viable position to place a new room.
bool Dungeon::viable_room_position(unsigned short x, unsigned short y, unsigned short w, unsigned short h) const
{
	STACK_TRACE();
	for (unsigned short rx = x - 1; rx < x + w + 2; rx++)
		for (unsigned short ry = y - 1; ry < y + h + 2; ry++)
			if (!tile(rx, ry)->is_destroyable_wall()) return false;
	return true;
}

// Adds an Actor to this Tile.
void Tile::add_actor(shared_ptr<Actor> actor)
{
	STACK_TRACE();
	contained_actors.push_back(actor);
	actor->x = x;
	actor->y = y;
}

// Checks nearby tiles to modify floor and wall sprites.
string Tile::check_neighbours(int x, int y, bool wall) const
{
	STACK_TRACE();
	const unsigned char wall_map[16] = { 5, 4, 2, 15, 2, 15, 2, 15, 4, 4, 11, 14, 11, 12, 11, 13 };

	unsigned char neighbours = 0;
	if (neighbour_identical(x, y - 1)) neighbours += 1;
	if (neighbour_identical(x - 1, y)) neighbours += 2;
	if (neighbour_identical(x + 1, y)) neighbours += 4;
	if (neighbour_identical(x, y + 1)) neighbours += 8;

	if (wall) return strx::itos(wall_map[neighbours]);
	else return "5";
}

// Checks if this Tile contains an Actor that blocks line-of-sight.
bool Tile::contains_los_blocker() const
{
	STACK_TRACE();
	for (auto actor : contained_actors)
		if (actor->is_los_blocker()) return true;
	return false;
}

// Returns the sprite name for rendering this tile.
string Tile::get_sprite() const
{
	STACK_TRACE();
	string sprite_name = sprite;

	if (sprite_name.size() >= 7 && sprite_name.substr(0, 6) == "FLOOR_") return sprite_name.substr(0, 7) + "_" + check_neighbours(x, y, false);
	else if (sprite_name.size() >= 6 && sprite_name.substr(0, 5) == "WALL_") return sprite_name.substr(0, 6) + "_" + check_neighbours(x, y, true);
	return sprite_name;
}

// Checks if a door is present here, and returns the Actor vector ID if so.
unsigned int Tile::has_door() const
{
	STACK_TRACE();
	for (unsigned int i = 0; i < contained_actors.size(); i++)
		if (contained_actors.at(i)->is_door()) return i;
	return UINT_MAX;
}

// Is this Tile a wall that can be destroyed?
bool Tile::is_destroyable_wall() const
{
	return is_wall() && !is_permawall();
}

// Has this Tile been explored?
bool Tile::is_explored() const
{
	return (flags & TILE_FLAG_EXPLORED) == TILE_FLAG_EXPLORED;
}

// Is this Tile a floor of some kind?
bool Tile::is_floor() const
{
	return (flags & TILE_FLAG_FLOOR) == TILE_FLAG_FLOOR;
}

// Is this Tile something that blocks movement?
bool Tile::is_impassible() const
{
	return (flags & TILE_FLAG_IMPASSIBLE) == TILE_FLAG_IMPASSIBLE;
}

// Does this Tile block line-of-sight?
bool Tile::is_opaque() const
{
	return (flags & TILE_FLAG_OPAQUE) == TILE_FLAG_OPAQUE;
}

// Is this Tile a wall that can never be destroyed under any circumstances?
bool Tile::is_permawall() const
{
	return (flags & TILE_FLAG_PERMAWALL) == TILE_FLAG_PERMAWALL;
}

// Is this Tile a wall of some kind?
bool Tile::is_wall() const
{
	return (flags & TILE_FLAG_WALL) == TILE_FLAG_WALL;
}

// Returns a list of all contained Actors with the ACTOR_FLAG_ITEM flag.
vector<unsigned int> Tile::items_here() const
{
	STACK_TRACE();
	vector<unsigned int> result;
	for (unsigned int i = 0; i < contained_actors.size(); i++)
		if (contained_actors.at(i)->is_item()) result.push_back(i);
	return result;
}

// Loads this Tile from disk.
void Tile::load(SQLite::Statement &query, unsigned long long dungeon_id)
{
	STACK_TRACE();
	try
	{
		name = query.getColumn("name").getString();
		flags = query.getColumn("flags").getUInt();
		sprite = query.getColumn("sprite").getString();

		SQLite::Statement actor_query(*world::save_db(), "SELECT id FROM actors WHERE owner = ? AND x = ? AND y = ?");
		actor_query.bind(1, static_cast<signed long long>(dungeon_id));
		actor_query.bind(2, x);
		actor_query.bind(3, y);
		while (actor_query.executeStep())
		{
			unsigned long long new_id = actor_query.getColumn("id").getInt64();
			auto new_actor = std::make_shared<Actor>(new_id);
			new_actor->load(dungeon_id);
			contained_actors.push_back(new_actor);
		}
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Returns a list of all contained Actors with the ACTOR_FLAG_MONSTER flag.
vector<unsigned int> Tile::mobs_here() const
{
	STACK_TRACE();
	vector<unsigned int> result;
	for (unsigned int i = 0; i < contained_actors.size(); i++)
		if (contained_actors.at(i)->is_monster()) result.push_back(i);
	return result;
}

// Check if a neighbour is an identical tile.
bool Tile::neighbour_identical(int x, int y) const
{
	STACK_TRACE();
	if (x < 0 || y < 0 || x >= world::dungeon()->get_width() || y >= world::dungeon()->get_height()) return false;
	Tile* neighbour = world::dungeon()->tile(x, y);
	if (neighbour->sprite == sprite) return true;
	else return false;
}

// Removes an Actor from this Tile.
void Tile::remove_actor(unsigned int id)
{
	STACK_TRACE();
	if (contained_actors.size() <= id)
	{
		guru::nonfatal("Out-of-bounds Actor ID in removal request.", GURU_CRITICAL);
		return;
	}
	contained_actors.erase(contained_actors.begin() + id);
}

// Saves this Tile to disk.
void Tile::save(unsigned long long dungeon_id)
{
	STACK_TRACE();
	try
	{
		SQLite::Statement statement(*world::save_db(), "INSERT INTO tiles (dungeon_id, x, y, name, sprite, flags) VALUES (?, ?, ?, ?, ?, ?)");
		statement.bind(1, static_cast<signed long long>(dungeon_id));
		statement.bind(2, x);
		statement.bind(3, y);
		statement.bind(4, name);
		statement.bind(5, sprite);
		statement.bind(6, flags);
		statement.exec();
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
	for (auto actor : contained_actors)
		actor->save(dungeon_id);
}
