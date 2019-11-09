// actor.cpp -- The Actor class, encompassing all items, NPCs, and other dynamic things in the game world.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "actor.h"
#include "ai-basic.h"
#include "attacker.h"
#include "defender.h"
#include "graveyard.h"
#include "guru.h"
#include "inventory.h"
#include "iocore.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"


Actor::Actor(unsigned long long new_id) : ai(nullptr), flags(0), id(new_id), inventory(nullptr), x(0), y(0) { }

Actor::~Actor() { }

// Adds AI to this Actor.
void Actor::add_ai(string type, unsigned long long new_id)
{
	STACK_TRACE();
	if (ai && ai->id != new_id) graveyard::destroy_ai(ai->id);
	if (type == "BASIC") ai = std::make_shared<BasicAI>(this, new_id);
	else
	{
		guru::nonfatal("Invalid AI type specified: " + type, GURU_ERROR);
		return;
	}
	ai_type = type;
}

// Clears a flag on this Actor.
void Actor::clear_flag(unsigned int flag)
{
	if ((flags & flag) == flag) flags ^= flag;
}

// Gets the name of this Actor, with 'the' at the start if it doesn't have a proper noun name.
string Actor::get_name(bool first_letter_caps) const
{
	if (has_proper_noun()) return name;
	else if (first_letter_caps) return "The " + name;
	else return "the " + name;
}

// Does this Actor have lower-priority rendering (i.e. other Actors go on top)?
bool Actor::has_low_priority_rendering() const
{
	return is_item();
}

// Does this Actor have a proper noun for a name?
bool Actor::has_proper_noun() const
{
	STACK_TRACE();
	return (flags & ACTOR_FLAG_PROPER_NOUN) == ACTOR_FLAG_PROPER_NOUN;
}

// Does this Actor have an animated sprite?
bool Actor::is_animated() const
{
	return (flags & ACTOR_FLAG_ANIMATED) == ACTOR_FLAG_ANIMATED;
}

// Does this Actor block movement into its tile?
bool Actor::is_blocker() const
{
	return (flags & ACTOR_FLAG_BLOCKER) == ACTOR_FLAG_BLOCKER;
}

// Is this Actor some sort of door?
bool Actor::is_door() const
{
	return (flags & ACTOR_FLAG_DOOR) == ACTOR_FLAG_DOOR;
}

// Is this Actor unable to be normally seen?
bool Actor::is_invisible() const
{
	return (flags & ACTOR_FLAG_INVISIBLE) == ACTOR_FLAG_INVISIBLE;
}

// Is this Actor an item that can be picked up?
bool Actor::is_item() const
{
	return (flags & ACTOR_FLAG_ITEM) == ACTOR_FLAG_ITEM;
}

// Does this Actor block line-of-sight?
bool Actor::is_los_blocker() const
{
	return (flags & ACTOR_FLAG_BLOCKS_LOS) == ACTOR_FLAG_BLOCKS_LOS;
}

// Is this Actor an NPC or monster?
bool Actor::is_monster() const
{
	return (flags & ACTOR_FLAG_MONSTER) == ACTOR_FLAG_MONSTER;
}

// Loads this Actor's data from disk.
void Actor::load(unsigned long long owner_id)
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world::save_db(), "SELECT * FROM actors WHERE id = ?");
		query.bind(1, static_cast<signed long long>(id));
		if (query.executeStep())
		{
			name = query.getColumn("name").getString();
			sprite = query.getColumn("sprite").getString();
			flags = query.getColumn("flags").getUInt();
			x = query.getColumn("x").getUInt();
			y = query.getColumn("y").getUInt();
			if (!query.isColumnNull("inventory"))
			{
				unsigned long long inventory_id = query.getColumn("inventory").getInt64();
				inventory = std::make_shared<Inventory>(inventory_id);
				inventory->load();
			}
			if (!query.isColumnNull("attacker"))
			{
				unsigned long long attacker_id = query.getColumn("attacker").getInt64();
				attacker = std::make_shared<Attacker>(attacker_id);
				attacker->load();
			}
			if (!query.isColumnNull("defender"))
			{
				unsigned long long defender_id = query.getColumn("defender").getInt64();
				defender = std::make_shared<Defender>(defender_id);
				defender->load();
			}
			if (!query.isColumnNull("ai"))
			{
				unsigned long long ai_id = query.getColumn("ai").getInt64();
				SQLite::Statement ai_query(*world::save_db(), "SELECT type FROM ai WHERE id = ?");
				ai_query.bind(1, static_cast<signed long long>(ai_id));
				if (ai_query.executeStep())
				{
					string type = ai_query.getColumn("type").getString();
					add_ai(type, ai_id);
					ai->load();
				}
				else guru::halt("Could not load AI data from save file!");
			}
		}
		else guru::halt("Could not load Actor " + strx::uitos(owner_id));
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Saves this Actor's data to disk.
void Actor::save(unsigned long long owner_id)
{
	STACK_TRACE();
	try
	{
		SQLite::Statement delete_statement(*world::save_db(), "DELETE FROM actors WHERE id = ?");
		delete_statement.bind(1, static_cast<signed long long>(id));
		delete_statement.exec();
		SQLite::Statement query(*world::save_db(), "INSERT INTO actors (id, owner, name, sprite, flags, x, y, inventory, attacker, defender, ai) VALUES (?,?,?,?,?,?,?,?,?,?,?)");
		query.bind(1, static_cast<long long>(id));
		query.bind(2, static_cast<long long>(owner_id));
		query.bind(3, name);
		query.bind(4, sprite);
		query.bind(5, flags);
		query.bind(6, x);
		query.bind(7, y);
		if (inventory) query.bind(8, static_cast<signed long long>(inventory->id)); else query.bind(8);
		if (attacker) query.bind(9, static_cast<signed long long>(attacker->id)); else query.bind(9);
		if (defender) query.bind(10, static_cast<signed long long>(defender->id)); else query.bind(10);
		if (ai && id != 1) query.bind(11, static_cast<signed long long>(ai->id)); else query.bind(11);
		query.exec();
	}
	catch(std::exception &e)
	{
		guru::halt(e.what());
	}

	if (inventory) inventory->save();
	if (attacker) attacker->save();
	if (defender) defender->save();
	if (ai && id != 1) ai->save();
}

// Sets a flag on this Actor.
void Actor::set_flag(unsigned int flag)
{
	flags |= flag;
}
