// static-data.h -- A source for static data defined in the JSON data files.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

class Actor;	// defined in actor.h
class Tile;		// defined in dungeon.h
enum class ActorType : unsigned int;	// defined in static-data.cpp
enum class Colour : unsigned char;		// defined in iocore.h
namespace Json { class Value; }			// defined in json.cpp/json/json.h


namespace data
{

shared_ptr<Actor>	get_mob(string mob_id);		// Retrieves a copy of a specified mob.
Tile				get_tile(string tile_id);	// Retrieves a copy of a specified Tile.
void				init();	// Loads the static data from JSON files.
void				init_actor_json(Json::Value jval, string actor_id, shared_ptr<Actor> actor, ActorType type);	// Loads an Actor's data from JSON.
void				init_mobs_json();	// Load the data from mobs.json
void				init_tiles_json();	// Load the data from tiles.json
Colour				parse_colour_string(string colour_string);	// Parses a colour string into a Colour.
unsigned int		parse_glyph_string(string glyph_string);	// Parses a glyph string into a Glyph.
string				tile_name(unsigned int name_id);			// Parses a tile name ID into a string.

}	// namespace data
