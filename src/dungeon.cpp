// dungeon.cpp -- The Dungeon class, which handles all the core functionality of a dungeon level, including generation, rendering, saving and loading.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "dungeon.h"
#include "guru.h"
#include "hero.h"
#include "iocore.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"


Dungeon::Dungeon(unsigned short new_level, unsigned short new_width, unsigned short new_height) : level(new_level), width(new_width), height(new_height)
{
	STACK_TRACE();
	if (!new_width || !new_height) return;
	tiles = new Tile[width * height]();
}

Dungeon::~Dungeon()
{
	STACK_TRACE();
	delete[] tiles;
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
			const void* blob = query.getColumn("tiles").getBlob();
			memcpy(tiles, blob, sizeof(Tile) * width * height);
		}
	}
	catch(std::exception &e)
	{
		guru->halt(e.what());
	}
}

// Renders the dungeon on the screen.
void Dungeon::render()
{
	STACK_TRACE();
	iocore->cls();
	for (unsigned int x = 0; x < width; x++)
		for (unsigned int y = 0; y < height; y++)
			render_tile(x, y);
}

// Renders a specific tile.
void Dungeon::render_tile(unsigned short x, unsigned short y)
{
	STACK_TRACE();
	if (x == world()->hero()->x && y == world()->hero()->y)
	{
		iocore->print_at('@', x, y, Colour::WHITE);
		return;
	}
	Tile &here = tiles[x + y * width];
	iocore->print_at(static_cast<Glyph>(here.glyph), x, y, here.colour);
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
