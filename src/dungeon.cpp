// dungeon.cpp -- The Dungeon class, which handles all the core functionality of a dungeon level, including generation, rendering, saving and loading.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "dungeon.h"
#include "guru.h"
#include "hero.h"
#include "iocore.h"
#include "mathx.h"
#include "prefs.h"
#include "static-data.h"
#include "world.h"

#include "strx.h"

#include "SQLiteCpp/SQLiteCpp.h"

#include <cmath>


// Used by the shadowcasting system below.
static signed char shadowcast_multipliers[4][8] = {
	{ 1, 0, 0, -1, -1, 0, 0, 1 },
	{ 0, 1, -1, 0, 0, -1, 1, 0 },
	{ 0, 1, 1, 0, 0, -1, -1, 0 },
	{ 1, 0, 0, 1, -1, 0, 0, -1 }
};


Dungeon::Dungeon(unsigned short new_level, unsigned short new_width, unsigned short new_height) : level(new_level), width(new_width), height(new_height), region(nullptr)
{
	STACK_TRACE();
	if (!new_width || !new_height) return;
	tiles = new Tile[width * height]();
	lighting = new s_rgb[width * height]();
}

Dungeon::~Dungeon()
{
	STACK_TRACE();
	delete[] tiles;
	delete[] lighting;
}

// Carves out a square room.
void Dungeon::carve_room(unsigned short x, unsigned short y, unsigned short w, unsigned short h, unsigned int new_region)
{
	STACK_TRACE();

	Tile basic_floor = static_data()->get_tile("BASIC_FLOOR");
	for (unsigned int rx = x; rx < x + w; rx++)
	{
		for (unsigned int ry = y; ry < y + h; ry++)
		{
			tiles[rx + ry * width] = basic_floor;
			if (region) region[rx + ry * width] = new_region;
		}
	}
}

// Shadowcasting engine, thanks to RogueBasin's C++ implementation of Björn Bergström's recursive shadowcasting FOV algorithm.
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
				const s_rgb existing_light = lighting[ax + ay * width];
				if (always_visible || existing_light.r || existing_light.g || existing_light.b)
				{
					std::pair<unsigned short, unsigned short> xy = { ax, ay };
					if (dynamic_light_temp.find(xy) == dynamic_light_temp.end()) dynamic_light_temp.insert(xy);
				}
			}

			Tile *tile = &tiles[ax + ay * width];
			if (blocked)
			{
				if (tile->opaque())
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
			else if (tile->opaque())
			{
				blocked = true;
				next_start_slope = r_slope;
				cast_light(x, y, radius, i + 1, start_slope, l_slope, xx, xy, yx, yy, always_visible);
			}
		}
		if (blocked) break;
	}
}

// Dims a specified RGB colour.
s_rgb Dungeon::diminish_light(s_rgb colour, float distance, float falloff)
{
	STACK_TRACE();
	float divisor = pow(distance, falloff) - distance + 1;
	colour.r = static_cast<unsigned char>(round(static_cast<float>(colour.r) / divisor));
	colour.g = static_cast<unsigned char>(round(static_cast<float>(colour.g) / divisor));
	colour.b = static_cast<unsigned char>(round(static_cast<float>(colour.b) / divisor));
	return colour;
}

// Marks a given tile as explored.
void Dungeon::explore(unsigned short x, unsigned short y)
{
	STACK_TRACE();
	tiles[x + y * width].flags |= TILE_FLAG_EXPLORED;
}

// Generates a new dungeon level.
void Dungeon::generate()
{
	STACK_TRACE();

	// Set a default layout with basic floor tiles and an impassible wall.
	Tile indestructible_wall = static_data()->get_tile("BOUNDARY_WALL");
	Tile regular_wall = static_data()->get_tile("BASIC_WALL");

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
				else tiles[x + y * width] = regular_wall;
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
	{
		for (unsigned short y = 2; y < height - 2; y += 2)
		{
			if (viable_maze_position(x, y)) viable_maze_cells.push_back(std::pair<unsigned short, unsigned short>(x, y));
		}
	}

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
		if (tiles[xy.first + 1 + xy.second * width].destroyable_wall()) viable_directions.push_back(std::pair<signed char, signed char>(1, 0));
		if (tiles[xy.first - 1 + xy.second * width].destroyable_wall()) viable_directions.push_back(std::pair<signed char, signed char>(-1, 0));
		if (tiles[xy.first + (xy.second + 1) * width].destroyable_wall()) viable_directions.push_back(std::pair<signed char, signed char>(0, 1));
		if (tiles[xy.first + (xy.second - 1) * width].destroyable_wall()) viable_directions.push_back(std::pair<signed char, signed char>(0, -1));
		if (!viable_directions.size()) continue;
		unsigned int choice = mathx::rnd(viable_directions.size()) - 1;
		carve_room(xy.first + viable_directions.at(choice).first, xy.second + viable_directions.at(choice).second, 1, 1, 0);
	}

	// Attempt to place doors in room entrances.
	Tile regular_door = static_data()->get_tile("BASIC_DOOR");
	for (unsigned short x = 2; x < width - 2; x++)
		for (unsigned short y = 2; y < height - 2; y++)
			if (viable_doorway(x, y) && mathx::rnd(3) == 1) tiles[x + y * width] = regular_door;

	delete[] region;
}

// Check to see if this tile is a dead-end.
bool Dungeon::is_dead_end(unsigned short x, unsigned short y)
{
	STACK_TRACE();
	unsigned int surrounding_walls = 0;
	if (tiles[x + y * width].wall()) return false;
	if (tiles[x + 1 + y * width].wall()) surrounding_walls++;
	if (tiles[x - 1 + y * width].wall()) surrounding_walls++;
	if (tiles[x + (y + 1) * width].wall()) surrounding_walls++;
	if (tiles[x + (y - 1) * width].wall()) surrounding_walls++;
	if (surrounding_walls == 3) return true;
	else return false;
}

// Applies a light source to a surface, affecting its colour.
s_rgb Dungeon::light_surface(s_rgb surface_colour, s_rgb light_colour)
{
	STACK_TRACE();
	if (!light_colour.r && !light_colour.g && !light_colour.b) return { 0, 0, 0};
	auto light_with = [](unsigned char surf, unsigned char light) -> unsigned char
	{
		float light_perc = static_cast<float>(light) / 255.0f;
		unsigned char modified_surf = static_cast<unsigned char>(round(static_cast<float>(surf) * light_perc));
		//unsigned char modified_surf = (surf / 2) + (light / 2);
		if (modified_surf < 20 && surf > 0 && light > 0)
		{
			if (surf >= 20) modified_surf = 20;
			else modified_surf = 20;
		}
		return modified_surf;
	};

	s_rgb lit_surface_colour;
	lit_surface_colour.r = light_with(surface_colour.r, light_colour.r);
	lit_surface_colour.g = light_with(surface_colour.g, light_colour.g);
	lit_surface_colour.b = light_with(surface_colour.b, light_colour.b);

	return lit_surface_colour;
}

// Applies light to a specified tile.
void Dungeon::light_tile(unsigned short x, unsigned short y, s_rgb colour)
{
	STACK_TRACE();
	s_rgb *existing_rgb = &lighting[x + y * width];

	auto merge_light = [] (unsigned char &current, unsigned char extra)
	{
		if (static_cast<unsigned int>(current) + static_cast<unsigned int>(extra) > 255) current = 255;
		else current += extra;
	};

	merge_light(existing_rgb->r, colour.r);
	merge_light(existing_rgb->g, colour.g);
	merge_light(existing_rgb->b, colour.b);
}

// Loads this dungeon from disk.
void Dungeon::load()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world()->save_db(), "SELECT * FROM dungeon WHERE level = ?");
		query.bind(1, level);
		while (query.executeStep())
		{
			width = query.getColumn("width").getUInt();
			height = query.getColumn("height").getUInt();
			tiles = new Tile[width * height]();
			lighting = new s_rgb[width * height]();
			const void* blob = query.getColumn("tiles").getBlob();
			memcpy(tiles, blob, sizeof(Tile) * width * height);
		}
	}
	catch(std::exception &e)
	{
		guru->halt(e.what());
	}
}

// Checks to see if a given tile is within the player's line of sight.
// Largely adapted from Bresenham's Line Algorithm on RogueBasin.
bool Dungeon::los_check(unsigned short x1, unsigned short y1)
{
	unsigned short x2 = world()->hero()->x, y2 = world()->hero()->y;

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

			if (tiles[x1 + y1 * width].opaque()) return false;
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

			if (tiles[x1 + y1 * width].opaque()) return false;
		}
	}

	return true;
}

// View the dungeon map in its entirety. see_all should only be used in debugging/testing code.
void Dungeon::map_view(bool see_lighting, bool see_all)
{
	STACK_TRACE();
	recalc_lighting();
	while(true)
	{
		iocore->cls();
		render(see_lighting, see_all);
		iocore->flip();
		const unsigned int key = iocore->wait_for_key();
		if (key == prefs::keybind(Keys::NORTH) || key == prefs::keybind(Keys::NORTHEAST) || key == prefs::keybind(Keys::NORTHWEST)) world()->hero()->camera_off_y++;
		else if (key == prefs::keybind(Keys::SOUTH) || key == prefs::keybind(Keys::SOUTHEAST) || key == prefs::keybind(Keys::SOUTHWEST)) world()->hero()->camera_off_y--;
		if (key == prefs::keybind(Keys::EAST) || key == prefs::keybind(Keys::SOUTHEAST) || key == prefs::keybind(Keys::NORTHEAST)) world()->hero()->camera_off_x--;
		else if (key == prefs::keybind(Keys::WEST) || key == prefs::keybind(Keys::SOUTHWEST) || key == prefs::keybind(Keys::NORTHWEST)) world()->hero()->camera_off_x++;
	}
}

// Picks a viable random starting location.
void Dungeon::random_start_position(unsigned short &x, unsigned short &y)
{
	STACK_TRACE();
	while(true)
	{
		x = mathx::rnd(width - 4) + 2;
		y = mathx::rnd(height - 4) + 2;
		if (tiles[x + y * width].name == strx::hash("floor")) return;
	}
}

// Recalculates a specific light source.
void Dungeon::recalc_light_source(unsigned short x, unsigned short y, s_rgb colour, unsigned short radius, bool always_visible)
{
	STACK_TRACE();
	for (unsigned int i = 0; i < 8; i++)
		cast_light(x, y, radius, 1, 1.0, 0.0, shadowcast_multipliers[0][i], shadowcast_multipliers[1][i], shadowcast_multipliers[2][i], shadowcast_multipliers[3][i], always_visible);

	while (dynamic_light_temp.size())
	{
		auto iterator = dynamic_light_temp.begin();
		std::pair<unsigned short, unsigned short> xy = *iterator;
		dynamic_light_temp.erase(iterator);

		if (!always_visible && tiles[xy.first + xy.second * width].opaque())
		{
			if (los_check(xy.first, xy.second)) dynamic_light_temp_walls.insert(xy);
		} else
		{
			float distance = mathx::grid_dist(x, y, xy.first, xy.second);
			light_tile(xy.first, xy.second, diminish_light(colour, distance, 1.05f));
		}
	}

	// Brute-force check: look at all the *visible* (to the player) tiles surrounding the wall, and pick the brightest.
	for (auto xy : dynamic_light_temp_walls)
	{
		s_rgb brightest = { 30, 30, 30 };
		int brightest_total = 90;
		for (short dx = -1; dx <= 1; dx++)
		{
			if (dx + xy.first < 0 || dx + xy.first >= width) continue;
			for (short dy = -1; dy <= 1; dy++)
			{
				if ((dx == 0 && dy == 0) || dy + xy.second < 0 || dy + xy.second >= height) continue;
				if (tiles[xy.first + dx + (xy.second + dy) * width].opaque()) continue;
				if (!los_check(xy.first + dx, xy.second + dy)) continue;
				s_rgb light = lighting[xy.first + dx + (xy.second + dy) * width];
				if (light.r + light.g + light.b > brightest_total)
				{
					brightest_total = light.r + light.g + light.b;
					brightest = light;
				}
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
	memset(lighting, 0, sizeof(struct s_rgb) * width * height);
	recalc_light_source(world()->hero()->x, world()->hero()->y, { 255, 255, 200 }, 100, true);
}

// Flood-fills a specified area with a new region ID.
void Dungeon::region_floodfill(unsigned short x, unsigned short y, unsigned int new_region)
{
	STACK_TRACE();
	if (region[x + y * width] == new_region) return;
	if (tiles[x + y * width].wall()) return;

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
void Dungeon::render(bool render_lighting, bool see_all)
{
	STACK_TRACE();
	iocore->cls();
	for (unsigned int x = 0; x < width; x++)
	{
		int screen_x = static_cast<signed int>(x) + world()->hero()->camera_off_x;
		if (screen_x < 0 || screen_x > iocore->get_cols()) continue;
		for (unsigned int y = 0; y < height; y++)
		{
			int screen_y = static_cast<signed int>(y) + world()->hero()->camera_off_y;
			if (screen_y < 0 || screen_y > iocore->get_rows()) continue;

			if (x == world()->hero()->x && y == world()->hero()->y) iocore->print_at('@', screen_x, screen_y, Colour::WHITE);
			else
			{
				Tile &here = tiles[x + y * width];
				unsigned char r, g, b;
				iocore->parse_colour(here.colour, r, g, b);
				s_rgb here_col;
				if (render_lighting) here_col = light_surface({r,g,b}, lighting[x + y * width]);
				else here_col = { r, g, b };
				if (see_all)
				{
					if (here_col.r < 30 && here_col.g < 30 && here_col.b < 30)
					{
						here_col.r += 30;
						here_col.g += 30;
						here_col.b += 30;
					}
				}
				if (here_col.r >= 30 || here_col.g >= 30 || here_col.b >= 30)
				{
					iocore->print_at(static_cast<Glyph>(here.glyph), screen_x, screen_y, here_col.r, here_col.g, here_col.b);
					explore(x, y);
				}
				else if (here.explored()) iocore->print_at(static_cast<Glyph>(here.glyph), screen_x, screen_y, 30, 30, 30);
			}
		}
	}
}
// Saves this dungeon to disk.
void Dungeon::save()
{
	STACK_TRACE();
	try
	{
		world()->save_db()->exec("CREATE TABLE IF NOT EXISTS dungeon ( level INTEGER PRIMARY KEY UNIQUE NOT NULL, width INTEGER NOT NULL, height INTEGER NOT NULL, tiles BLOB NOT NULL );");
		SQLite::Statement delete_level(*world()->save_db(), "DELETE FROM dungeon WHERE level = ?");
		delete_level.bind(1, level);
		delete_level.exec();
		SQLite::Statement sql(*world()->save_db(), "INSERT INTO dungeon (level,width,height,tiles) VALUES (?,?,?,?)");
		sql.bind(1, level);
		sql.bind(2, width);
		sql.bind(3, height);
		sql.bind(4, (void*)tiles, sizeof(Tile) * width * height);
		sql.exec();
	}
	catch(std::exception &e)
	{
		guru->halt(e.what());
	}
}

// Sets a specified tile, with error checking.
void Dungeon::set_tile(unsigned short x, unsigned short y, Tile &tile)
{
	STACK_TRACE();
	if (x >= width || y >= height) guru->halt("Attempted to set out-of-bounds tile.");
	tiles[x + y * width] = tile;
}

// Retrieves a specified tile pointer.
shared_ptr<Tile> Dungeon::tile(unsigned short x, unsigned short y)
{
	STACK_TRACE();
	if (x >= width || y >= height) guru->halt("Attempted to set out-of-bounds tile.");
	return std::make_shared<Tile>(tiles[x + y * width]);
}

// Checks if this tile touches a different region.
bool Dungeon::touches_two_regions(unsigned short x, unsigned short y)
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
bool Dungeon::viable_doorway(unsigned short x, unsigned short y)
{
	STACK_TRACE();
	if (x < 2 || y < 2 || x >= width - 2 || y >= height - 2) return false;
	if (tiles[x + y * width].name != strx::hash("floor")) return false;	// Only basic floor can become a door.

	if (tiles[x - 1 + y * width].wall() && tiles[x + 1 + y * width].wall())
	{
		if (tiles[x + (y + 1) * width].wall()) return false;
		if (tiles[x + (y - 1) * width].wall()) return false;
		if (!tiles[x - 1 + (y + 1) * width].wall() && !tiles[x + 1 + (y + 1) * width].wall()) return true;
		if (!tiles[x - 1 + (y - 1) * width].wall() && !tiles[x + 1 + (y - 1) * width].wall()) return true;
	}
	else if (tiles[x + (y - 1) * width].wall() && tiles[x + (y + 1) * width].wall())
	{
		if (tiles[x - 1 + y * width].wall()) return false;
		if (tiles[x + 1 + y * width].wall()) return false;
		if (!tiles[x + 1 + (y - 1) * width].wall() && !tiles[x + 1 + (y + 1) * width].wall()) return true;
		if (!tiles[x - 1 + (y - 1) * width].wall() && !tiles[x - 1 + (y + 1) * width].wall()) return true;
	}

	return false;
}

// Checks if this tile is a viable position to build a maze corridor.
bool Dungeon::viable_maze_position(unsigned short x, unsigned short y)
{
	STACK_TRACE();
	if (x < 2 || y < 2 || x >= width - 2 || y >= height - 2) return false;

	unsigned short current_exits = 0;
	if (!tiles[x + y * width].wall()) return false;
	for (int ox = -1; ox <= 1; ox++)
	{
		for (int oy = -1; oy <= 1; oy++)
		{
			if (ox == 0 && oy == 0) continue;
			if (!tiles[(x + ox) + ((y + oy) * width)].wall()) current_exits++;
		}
	}
	if (current_exits <= 1) return true;
	else return false;
}

// Checks if this is a viable position to place a new room.
bool Dungeon::viable_room_position(unsigned short x, unsigned short y, unsigned short w, unsigned short h)
{
	STACK_TRACE();
	for (unsigned short rx = x - 1; rx < x + w + 2; rx++)
		for (unsigned short ry = y - 1; ry < y + h + 2; ry++)
			if (!tiles[rx + ry * width].destroyable_wall()) return false;
	return true;
}
