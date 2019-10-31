// dungeon.cpp -- The Dungeon class, which handles all the core functionality of a dungeon level, including generation, rendering, saving and loading.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "dungeon.h"
#include "guru.h"
#include "hero.h"
#include "iocore.h"
#include "mathx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"

#include <cmath>
#include <set>

// Used by the shadowcasting system below.
static signed char shadowcast_multipliers[4][8] = {
	{ 1, 0, 0, -1, -1, 0, 0, 1 },
	{ 0, 1, -1, 0, 0, -1, 1, 0 },
	{ 0, 1, 1, 0, 0, -1, -1, 0 },
	{ 1, 0, 0, 1, -1, 0, 0, -1 }
};
std::set<std::pair<unsigned short, unsigned short>> dynamic_light_temp;


Dungeon::Dungeon(unsigned short new_level, unsigned short new_width, unsigned short new_height) : level(new_level), width(new_width), height(new_height)
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

// Shadowcasting engine, thanks to RogueBasin's C++ implementation of Björn Bergström's recursive shadowcasting FOV algorithm.
void Dungeon::cast_light(unsigned int x, unsigned int y, unsigned int radius, unsigned int row, float start_slope, float end_slope, unsigned int xx, unsigned int xy, unsigned int yx, unsigned int yy, bool always_visible)
{
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
	float divisor = pow(distance, falloff) - distance + 1;
	colour.r = static_cast<unsigned char>(round(static_cast<float>(colour.r) / divisor));
	colour.g = static_cast<unsigned char>(round(static_cast<float>(colour.g) / divisor));
	colour.b = static_cast<unsigned char>(round(static_cast<float>(colour.b) / divisor));
	return colour;
}

// Generates a new dungeon level.
void Dungeon::generate()
{
	STACK_TRACE();
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

	for (unsigned int i = 0; i < 100; i++)
	{
		const unsigned short rx = mathx::rnd(width) - 1;
		const unsigned short ry = mathx::rnd(height) - 1;
		tiles[rx + ry * width] = indestructible_wall;
	}
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

		float distance = mathx::grid_dist(x, y, xy.first, xy.second);
		light_tile(xy.first, xy.second, diminish_light(colour, distance, 1.05f));
	}
	dynamic_light_temp.clear();
}

// Clears the lighting array and recalculates all light sources.
void Dungeon::recalc_lighting()
{
	STACK_TRACE();
	memset(lighting, 0, sizeof(struct s_rgb) * width * height);
	recalc_light_source(world()->hero()->x, world()->hero()->y, { 255, 0, 0 }, 30, true);
	recalc_light_source(5, 5, { 0, 255, 0 }, 30, false);
}

// Renders the dungeon on the screen.
void Dungeon::render()
{
	STACK_TRACE();
	iocore->cls();
	for (unsigned int x = 0; x < width; x++)
	{
		for (unsigned int y = 0; y < height; y++)
		{
			if (x == world()->hero()->x && y == world()->hero()->y) iocore->print_at('@', x, y, Colour::WHITE);
			else
			{
				Tile &here = tiles[x + y * width];
				s_rgb here_col = lighting[x + y * width];
				iocore->print_at(static_cast<Glyph>(here.glyph), x, y, here_col.r, here_col.g, here_col.b);
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
Tile* Dungeon::tile(unsigned short x, unsigned short y)
{
	STACK_TRACE();
	if (x >= width || y >= height) guru->halt("Attempted to set out-of-bounds tile.");
	return &tiles[x + y * width];
}
