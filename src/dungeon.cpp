// dungeon.cpp -- The Dungeon class, which handles all the core functionality of a dungeon level, including generation, rendering, saving and loading.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "dungeon.h"
#include "guru.h"
#include "iocore.h"

#include "SQLiteCpp/SQLiteCpp.h"

SQLite::Database*	save_db;	// TEMPORARY CODE: REPLACE THIS when a proper player class is created!

// TEMPORARY CODE
#include <fstream>
// END OF TEMPORARY CODE


Dungeon::Dungeon(unsigned short new_width, unsigned short new_height) : width(new_width), height(new_height)
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
		SQLite::Statement query(*save_db, "SELECT width,height,tiles FROM dungeon WHERE level = 0");
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
	{
		for (unsigned int y = 0; y < height; y++)
		{
			Tile &here = tiles[x + y * width];
			iocore->print_at(static_cast<Glyph>(here.glyph), x, y, here.colour);
		}
	}
	iocore->flip();
}

// Saves this dungeon to disk.
void Dungeon::save()
{
	STACK_TRACE();
	try
	{
		SQLite::Transaction transaction(*save_db);
		save_db->exec("CREATE TABLE IF NOT EXISTS dungeon ( level INTEGER PRIMARY KEY UNIQUE NOT NULL, width INTEGER NOT NULL, height INTEGER NOT NULL, tiles BLOB NOT NULL );");
		save_db->exec("DELETE FROM dungeon WHERE level = 0");
		SQLite::Statement sql_tiles(*save_db, "INSERT INTO dungeon (level,width,height,tiles) VALUES (0,?,?,?)");
		sql_tiles.bind(1, width);
		sql_tiles.bind(2, height);
		sql_tiles.bind(3, (void*)tiles, sizeof(Tile) * width * height);
		sql_tiles.exec();
		transaction.commit();
	}
	catch(std::exception &e)
	{
		guru->halt(e.what());
	}

	std::ofstream tag_file("userdata/save/0/tag.dat");
	tag_file << "TEST\n01:02:03\nZ Novice\n1\n20\n0\n1\n";
	tag_file.close();
}

// Sets a specified tile, with error checking.
void Dungeon::set_tile(unsigned short x, unsigned short y, Tile &tile)
{
	STACK_TRACE();
	if (x >= width || y >= height) guru->halt("Attempted to set out-of-bounds tile.");
	tiles[x + y * width] = tile;
}
