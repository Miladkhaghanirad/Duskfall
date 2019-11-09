// attacker.cpp -- The Attacker class, which allows Actors to (attempt to) deal damage to other Actors with an attached Defender.
// Copyright (c) 2019 Raine "Gravecat" Simmons. All rights reserved.

#include "actor.h"
#include "attacker.h"
#include "defender.h"
#include "dungeon.h"
#include "guru.h"
#include "hero.h"
#include "mathx.h"
#include "message.h"
#include "strx.h"
#include "world.h"

#include "SQLiteCpp/SQLiteCpp.h"

#define COMBAT_SOUNDS_MAX_RANGE		20	// The number of tiles away that the player can hear combat happening.
#define COMBAT_SOUNDS_DISTANT_RANGE	15	// The range at which combat sounds are considered 'distant'.
#define COMBAT_SOUNDS_NEARBY_RANGE	5	// The range at which combat sounds are considered 'nearby'.


// Attacks another Actor!
void Attacker::attack(shared_ptr<Actor> owner, shared_ptr<Actor> target)
{
	STACK_TRACE();
	if (!target->defender)
	{
		guru::nonfatal("Attempting to attack a target with no defined Defender!", GURU_ERROR);
		return;
	}
	const bool attacker_is_hero = this->is_hero(), defender_is_hero = target->defender->is_hero();
	const bool hero_is_involved = (attacker_is_hero || defender_is_hero);

	// Check if we have NPC-on-NPC aggression happening.
	if (!hero_is_involved)
	{
		const bool attacker_in_sight = world::dungeon()->los_check(owner->x, owner->y);
		const bool defender_in_sight = world::dungeon()->los_check(target->x, target->y);
		if (attacker_in_sight) message::msg(owner->get_name(true) + " attacks " + (defender_in_sight ? target->get_name(false) : "something") + "!", MC::INFO);
		else
		{
			if (defender_in_sight) message::msg("Something attacks " + target->get_name(false) + "!", MC::INFO);
			else
			{
				// Neither attacker nor defender in sight. We'll see if we can hear this combat.
				const unsigned int combat_range = mathx::grid_dist(world::hero()->x, world::hero()->y, owner->x, owner->y);
				if (combat_range <= COMBAT_SOUNDS_MAX_RANGE)
				{
					string distance_str = "";
					if (combat_range <= COMBAT_SOUNDS_NEARBY_RANGE) distance_str = " nearby";
					else if (combat_range >= COMBAT_SOUNDS_DISTANT_RANGE) distance_str = " distant";
					message::msg("You hear the sounds of" + distance_str + " combat!", MC::INFO);
				}
			}
		}
	}
	else
	{
		// The player is attacking or being attacked!
		string attack_str;
		MC colour = MC::INFO;
		bool can_see_combatant;
		if (attacker_is_hero) can_see_combatant = world::dungeon()->los_check(target->x, target->y);
		else can_see_combatant = world::dungeon()->los_check(owner->x, owner->y);
		if (attacker_is_hero)
		{
			colour = MC::GOOD;
			attack_str = "You attack " + (can_see_combatant ? target->get_name(false) : "something");
		}
		else
		{
			colour = MC::BAD;
			attack_str = (can_see_combatant ? target->get_name(true) : "Something") + " attacks you";
		}

		int damage = this->power - target->defender->armour;
		if (damage < 1)
		{
			if (attacker_is_hero) colour = MC::WARN;
			else colour = MC::INFO;
			attack_str += "... but it has no efffect!";
		}
		else
		{
			attack_str += " for " + strx::itos(damage) + " damage";
			if (damage >= target->defender->hp) attack_str += ", fatally wounding " + string(defender_is_hero ? "you!" : "it!");
		}
		message::msg(attack_str, colour);
		target->defender->take_damage(damage, target);
	}
}

// Loads this Attacker from disk.
void Attacker::load()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement query(*world::save_db(), "SELECT * FROM attackers WHERE id = ?");
		query.bind(1, static_cast<signed long long>(id));
		if (query.executeStep())
		{
			power = query.getColumn("power").getUInt();
			if (!query.isColumnNull("flags")) flags = query.getColumn("flags").getUInt();
		}
		else guru::halt("Could not load Attacker data.");
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}

// Saves this Attacker to disk.
void Attacker::save()
{
	STACK_TRACE();
	try
	{
		SQLite::Statement delete_attacker(*world::save_db(), "DELETE FROM attackers WHERE id = ?");
		delete_attacker.bind(1, static_cast<signed long long>(id));
		delete_attacker.exec();
		SQLite::Statement attacker(*world::save_db(), "INSERT INTO attackers (id, power, flags) VALUES (?,?,?)");
		attacker.bind(1, static_cast<signed long long>(id));
		attacker.bind(2, power);
		if (flags) attacker.bind(3, flags); else attacker.bind(3);
		attacker.exec();
	}
	catch (std::exception &e)
	{
		guru::halt(e.what());
	}
}
