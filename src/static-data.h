// static-data.h -- A source for static data defined in the JSON data files.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"
#include <unordered_map>

class Actor;	// defined in actor.h
class Tile;		// defined in dungeon.h
enum class ActorType : unsigned int;	// defined in static-data.cpp
enum class Colour : unsigned char;		// defined in iocore.h
namespace Json { class Value; }			// defined in json.cpp/json/json.h


namespace data
{

shared_ptr<Actor>	get_actor(std::unordered_map<string, shared_ptr<Actor>> &map, string id, string type);	// Internal code used by get_item(), get_mob() and get_tile_feature().
shared_ptr<Actor>	get_item(string item_id);	// Retrieves a copy of the specified item.
shared_ptr<Actor>	get_mob(string mob_id);		// Retrieves a copy of a specified mob.
Tile				get_tile(string tile_id);	// Retrieves a copy of a specified Tile.
shared_ptr<Actor>	get_tile_feature(string feature_id);	// Retrieves a copy of a specified tile feature.
void				init();	// Loads the static data from JSON files.
void				init_actors_json(string filename, ActorType type, std::unordered_map<string, shared_ptr<Actor>> *the_map);	// Loads an Actor's data from JSON.
void				init_items_json();	// Load the data from items.json
void				init_mobs_json();	// Load the data from mobs.json
void				init_tiles_json();	// Load the data from tiles.json

}	// namespace data
