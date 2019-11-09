// static-data.cpp -- A source for static data defined in the JSON data files.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "actor.h"
#include "attacker.h"
#include "defender.h"
#include "dungeon.h"
#include "filex.h"
#include "guru.h"
#include "inventory.h"
#include "iocore.h"
#include "static-data.h"
#include "strx.h"
#include "world.h"

#include "jsoncpp/json/json.h"

#include <algorithm>
#include <unordered_map>

enum class ActorType : unsigned int { MONSTER, ITEM, TILE_FEATURE };


namespace data
{

std::unordered_map<string, shared_ptr<Actor>>	static_item_data;	// The data containing templates for items from items.json
std::unordered_map<string, shared_ptr<Actor>>	static_mob_data;	// The data containing templates for monsters from mobs.json
std::unordered_map<string, shared_ptr<Tile>>	static_tile_data;	// The data about dungeon tiles from tiles.json
std::unordered_map<string, shared_ptr<Actor>>	static_tile_feature_data;	// The data containing templates for tile features from tile features.json


// Internal code used by get_item(), get_mob() and get_tile_feature().
shared_ptr<Actor> get_actor(std::unordered_map<string, shared_ptr<Actor>> &map, string id, string type)
{
	STACK_TRACE();
	auto found = map.find(id);
	if (found == map.end()) guru::halt("Could not find " + type + " ID " + id + "!");
	shared_ptr<Actor> result = std::make_shared<Actor>(*found->second);
	result->id = world::unique_id();
	if (result->attacker)
	{
		result->attacker = std::make_shared<Attacker>(*result->attacker);
		result->attacker->id = world::unique_id();
	}
	if (result->defender)
	{
		result->defender = std::make_shared<Defender>(*result->defender);
		result->defender->id = world::unique_id();
	}
	if (result->inventory)
	{
		result->inventory = std::make_shared<Inventory>(*result->inventory);
		result->inventory->id = world::unique_id();
	}
	return result;
}

// Retrieves a copy of the specified item.
shared_ptr<Actor> get_item(string item_id)
{
	STACK_TRACE();
	return get_actor(static_item_data, item_id, "item");
}

// Retrieves a copy of a specified mob.
shared_ptr<Actor> get_mob(string mob_id)
{
	STACK_TRACE();
	return get_actor(static_mob_data, mob_id, "mob");
}

// Retrieves a copy of a specified Tile
Tile get_tile(string tile_id)
{
	STACK_TRACE();
	auto found = static_tile_data.find(tile_id);
	if (found == static_tile_data.end()) guru::halt("Could not retrieve tile ID " + tile_id + "!");
	return *found->second;
}

// Retrieves a copy of a specified tile feature.
shared_ptr<Actor> get_tile_feature(string feature_id)
{
	STACK_TRACE();
	return get_actor(static_tile_feature_data, feature_id, "tile feature");
}

// Loads the static data from JSON files.
void init()
{
	STACK_TRACE();
	guru::log("Attempting to load static data from JSON files...", GURU_INFO);
	init_tiles_json();
	init_actors_json("items", ActorType::ITEM, &static_item_data);
	init_actors_json("mobs", ActorType::MONSTER, &static_mob_data);
	init_actors_json("tile features", ActorType::TILE_FEATURE, &static_tile_feature_data);
}

// Loads an Actor's data from JSON.
void init_actors_json(string filename, ActorType type, std::unordered_map<string, shared_ptr<Actor>> *the_map)
{
	STACK_TRACE();
	const std::unordered_map<string, unsigned int> actor_flag_map = { { "BLOCKER", ACTOR_FLAG_BLOCKER }, { "BLOCKS_LOS", ACTOR_FLAG_BLOCKS_LOS }, { "MONSTER", ACTOR_FLAG_MONSTER }, { "ITEM", ACTOR_FLAG_ITEM },
			{ "INVISIBLE", ACTOR_FLAG_INVISIBLE }, { "DOOR", ACTOR_FLAG_DOOR }, { "ANIMATED", ACTOR_FLAG_ANIMATED } };

	Json::Value json = filex::load_json("json/" + filename);
	const Json::Value::Members jmem = json.getMemberNames();
	for (unsigned int i = 0; i < jmem.size(); i++)
	{
		const string actor_id = jmem.at(i);
		const Json::Value jval = json[actor_id];
		const Json::Value::Members jmem_actor = jval.getMemberNames();
		auto actor = std::make_shared<Actor>(0);

		const string actor_name = jval.get("name", "").asString();
		if (!actor_name.size()) guru::log("No actor name specified for " + actor_id, GURU_WARN);
		else actor->name = actor_name;

		const string actor_tile = jval.get("tile", "").asString();
		if (!actor_tile.size()) guru::log("No tile specified for " + actor_id, GURU_WARN);
		else actor->sprite = actor_tile;

		const string actor_flags_unparsed = jval.get("flags", "").asString();
		actor->flags = 0;
		bool not_monster = false, not_blocker = false, not_item = false;	// override flags
		if (actor_flags_unparsed.size())
		{
			const vector<string> actor_flags_vec = strx::string_explode(actor_flags_unparsed, " ");
			for (auto flag : actor_flags_vec)
			{
				auto found = actor_flag_map.find(strx::str_toupper(flag));
				if (found == actor_flag_map.end())
				{
					if (flag == "!MONSTER") not_monster = true;
					else if (flag == "!BLOCKER") not_blocker = true;
					else if (flag == "!ITEM") not_item = true;
					else guru::log("Unknown actor flag for " + actor_id + ": " + flag, GURU_WARN);
				}
				else actor->flags |= found->second;
			}
		}

		auto attacker_pos = std::find(jmem_actor.begin(), jmem_actor.end(), "attacker");
		if (attacker_pos != jmem_actor.end())
		{
			Json::Value attacker_jval = jval["attacker"];
			actor->attacker = std::make_shared<Attacker>(0);
			actor->attacker->power = attacker_jval.get("power", 0).asUInt();
		}

		auto defender_pos = std::find(jmem_actor.begin(), jmem_actor.end(), "defender");
		if (defender_pos != jmem_actor.end())
		{
			Json::Value defender_jval = jval["defender"];
			actor->defender = std::make_shared<Defender>(0);
			actor->defender->armour = defender_jval.get("armour", 0).asUInt();
			actor->defender->hp_max = defender_jval.get("hp", 0).asUInt();
			actor->defender->hp = actor->defender->hp_max;
		}

		if (type == ActorType::MONSTER)
		{
			if (!not_monster) actor->flags |= ACTOR_FLAG_MONSTER;	// Mobs are counted as monsters unless specified otherwise.
			if (!not_blocker) actor->flags |= ACTOR_FLAG_BLOCKER;	// Same with the blocker flag.
		}
		else if (type == ActorType::ITEM && !not_item) actor->flags |= ACTOR_FLAG_ITEM;	// Items are marked as items unless specified otherwise.

		the_map->insert(std::pair<string, shared_ptr<Actor>>(actor_id, actor));
	}
}

// Load the data from tiles.json
void init_tiles_json()
{
	STACK_TRACE();

	const std::unordered_map<string, unsigned int> tile_flag_map = { { "IMPASSIBLE", TILE_FLAG_IMPASSIBLE }, { "OPAQUE", TILE_FLAG_OPAQUE }, { "WALL", TILE_FLAG_WALL }, { "PERMAWALL", TILE_FLAG_PERMAWALL },
		{ "EXPLORED", TILE_FLAG_EXPLORED }, { "FLOOR", TILE_FLAG_FLOOR } };

	Json::Value json = filex::load_json("json/tiles");
	const Json::Value::Members jmem = json.getMemberNames();
	for (unsigned int i = 0; i < jmem.size(); i++)
	{
		const string tile_id = jmem.at(i);
		const Json::Value jval = json[tile_id];
		auto new_tile = std::make_shared<Tile>();

		const string tile_flags_unparsed = jval.get("flags", "").asString();
		new_tile->flags = 0;
		if (tile_flags_unparsed.size())
		{
			const vector<string> tile_flags_vec = strx::string_explode(tile_flags_unparsed, " ");
			for (auto flag : tile_flags_vec)
			{
				auto found = tile_flag_map.find(strx::str_toupper(flag));
				if (found == tile_flag_map.end()) guru::log("Unknown tile flag in tiles.json for " + tile_id + ": " + flag, GURU_ERROR);
				else new_tile->flags |= found->second;
			}
		}

		const string tile_name = jval.get("name", "").asString();
		if (!tile_name.size()) guru::log("No name specified in tiles.json for " + tile_id, GURU_ERROR);
		else new_tile->name = tile_name;

		const string tile_sprite = jval.get("tile", "").asString();
		if (!tile_sprite.size()) guru::log("No tile sprite specified in tiles.json for " + tile_id, GURU_ERROR);
		else new_tile->set_sprite(tile_sprite);

		static_tile_data.insert(std::pair<string, shared_ptr<Tile>>(tile_id, new_tile));
	}
}

}	// namespace data
