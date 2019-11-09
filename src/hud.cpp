// hud.cpp -- The overlay HUD, displaying important information about the game.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "defender.h"
#include "hero.h"
#include "hud.h"
#include "iocore.h"
#include "prefs.h"
#include "strx.h"
#include "world.h"

#include <cmath>


namespace hud
{

// Renders the HUD with the player's essential core stats.
void render()
{
	STACK_TRACE();
	if (prefs::tileset == "ascii")
	{
		iocore::print("HP: " + strx::itos(world::hero()->defender->hp) + "/" + strx::itos(world::hero()->defender->hp_max), 1, 1, Colour::CGA_WHITE);
		return;
	}
	const unsigned int health_perc = round((static_cast<float>(world::hero()->defender->hp) / static_cast<float>(world::hero()->defender->hp_max)) * 100.0f);
	for (unsigned int i = 0; i < 4; i++)
	{
		if (health_perc >= 25 * i && health_perc > 0)
		{
			Sprite heart_sprite = Sprite::HEART_RED_4;
			unsigned int heart_perc = round((static_cast<float>(health_perc) - (i * 25)) * 4);
			if (heart_perc > 75) heart_sprite = static_cast<Sprite>(static_cast<unsigned int>(heart_sprite) - 6);
			else if (heart_perc > 50) heart_sprite = static_cast<Sprite>(static_cast<unsigned int>(heart_sprite) - 4);
			else if (heart_perc > 25) heart_sprite = static_cast<Sprite>(static_cast<unsigned int>(heart_sprite) - 2);
			iocore::sprite_print(heart_sprite, (i * 2) + 1, 1, SPRITE_FLAG_ANIMATED);
		}
		else iocore::sprite_print(Sprite::HEART_CONTAINER, (i * 2) + 1, 1);
	}
}

}
