// static-data.cpp -- A source for static data defined in the JSON data files.
// Copyright (c) 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "actor.h"
#include "dungeon.h"
#include "filex.h"
#include "guru.h"
#include "iocore.h"
#include "static-data.h"
#include "strx.h"

#include "jsoncpp/json/json.h"

#include <unordered_map>


namespace data
{

std::unordered_map<string, shared_ptr<Actor>>	static_mob_data;	// The data containing templates for monsters from mobs.json
std::unordered_map<string, shared_ptr<Tile>>	static_tile_data;	// The data about dungeon tiles from tiles.json
std::unordered_map<unsigned int, string>		tile_names;	// The tile name strings, which are stored as integers on the tiles themselves.

// Retrieves a copy of a specified mob.
shared_ptr<Actor> get_mob(string mob_id)
{
	STACK_TRACE();
	auto found = static_mob_data.find(mob_id);
	if (found == static_mob_data.end()) guru::halt("Could not find mob ID " + mob_id + "!");
	return std::make_shared<Actor>(*found->second);
}

// Retrieves a copy of a specified Tile
Tile get_tile(string tile_id)
{
	STACK_TRACE();
	auto found = static_tile_data.find(tile_id);
	if (found == static_tile_data.end()) guru::halt("Could not retrieve tile ID " + tile_id + "!");
	return *found->second;
}

// Loads the static data from JSON files.
void init()
{
	STACK_TRACE();
	guru::log("Attempting to load static data from JSON files...", GURU_INFO);
	init_tiles_json();
	init_mobs_json();
}

// Loads an Actor's data from JSON.
void init_actor_json(Json::Value jval, string actor_id, shared_ptr<Actor> actor)
{
	STACK_TRACE();
	const string actor_name = jval.get("name", "").asString();
	if (!actor_name.size()) guru::log("No actor name specified for " + actor_id, GURU_WARN);
	else actor->name = actor_name;

	const string actor_colour_unparsed = jval.get("colour", "").asString();
	if (!actor_colour_unparsed.size()) guru::log("No actor colour specified  for " + actor_id, GURU_WARN);
	actor->colour = parse_colour_string(actor_colour_unparsed);

	const string actor_glyph_unparsed = jval.get("glyph", "").asString();
	if (!actor_glyph_unparsed.size()) guru::log("No actor glyph specified for " + actor_id, GURU_WARN);
	actor->glyph = parse_glyph_string(actor_glyph_unparsed);
}

// Load the data from mobs.json
void init_mobs_json()
{
	STACK_TRACE();

	Json::Value json = filex::load_json("mobs");
	const Json::Value::Members jmem = json.getMemberNames();
	for (unsigned int i = 0; i < jmem.size(); i++)
	{
		const string actor_id = jmem.at(i);
		const Json::Value jval = json[actor_id];
		auto new_mob = std::make_shared<Actor>();
		init_actor_json(jval, actor_id, new_mob);
		new_mob->flags |= ACTOR_FLAG_BLOCKER;	// Mobs cannot be walked through or over.
		static_mob_data.insert(std::pair<string, shared_ptr<Actor>>(actor_id, new_mob));
	}
}

// Load the data from tiles.json
void init_tiles_json()
{
	STACK_TRACE();

	const std::unordered_map<string, unsigned int> tile_flag_map = { { "IMPASSIBLE", TILE_FLAG_IMPASSIBLE }, { "OPAQUE", TILE_FLAG_OPAQUE }, { "WALL", TILE_FLAG_WALL }, { "PERMAWALL", TILE_FLAG_PERMAWALL },
		{ "EXPLORED", TILE_FLAG_EXPLORED } };

	Json::Value json = filex::load_json("tiles");
	const Json::Value::Members jmem = json.getMemberNames();
	for (unsigned int i = 0; i < jmem.size(); i++)
	{
		const string tile_id = jmem.at(i);
		const Json::Value jval = json[tile_id];
		auto new_tile = std::make_shared<Tile>();

		const string tile_colour_unparsed = jval.get("colour", "").asString();
		if (!tile_colour_unparsed.size()) guru::log("No tile colour specified in tiles.json for " + tile_id, GURU_WARN);
		new_tile->colour = parse_colour_string(tile_colour_unparsed);

		const string tile_flags_unparsed = jval.get("flags", "").asString();
		new_tile->flags = 0;
		if (tile_flags_unparsed.size())
		{
			const vector<string> tile_flags_vec = strx::string_explode(tile_flags_unparsed, " ");
			for (auto flag : tile_flags_vec)
			{
				auto found = tile_flag_map.find(strx::str_toupper(flag));
				if (found == tile_flag_map.end()) guru::log("Unknown tile flag in tiles.json for " + tile_id + ": " + flag, GURU_WARN);
				else new_tile->flags |= found->second;
			}
		}

		const string tile_glyph_unparsed = jval.get("glyph", "").asString();
		if (!tile_glyph_unparsed.size()) guru::log("No glyph specified in tiles.json for " + tile_id, GURU_WARN);
		new_tile->glyph = parse_glyph_string(tile_glyph_unparsed);

		const string tile_name = jval.get("name", "").asString();
		if (!tile_name.size())
		{
			new_tile->name = 0;
			guru::log("No name specified in tiles.json for " + tile_id, GURU_WARN);
		}
		else
		{
			const unsigned int hashed_name = strx::hash(tile_name);
			if (tile_names.find(hashed_name) == tile_names.end()) tile_names.insert(std::pair<unsigned int, string>(hashed_name, tile_name));
			new_tile->name = hashed_name;
		}

		static_tile_data.insert(std::pair<string, shared_ptr<Tile>>(tile_id, new_tile));
	}
}

// Parses a colour string into a Colour.
Colour parse_colour_string(string colour_string)
{
	STACK_TRACE();
	const std::unordered_map<string, Colour> colour_map = { { "GRAY", Colour::GRAY }, { "AQUA", Colour::AQUA }, { "BLUE", Colour::BLUE }, { "PURPLE", Colour::PURPLE }, { "MAGENTA", Colour::MAGENTA }, { "PINK", Colour::PINK },
		{ "RED", Colour::RED }, { "ORANGE", Colour::ORANGE }, { "YELLOW", Colour::YELLOW }, { "LIME", Colour::LIME }, { "GREEN", Colour::GREEN }, { "TURQUOISE", Colour::TURQ }, { "TURQ", Colour::TURQ }, { "CYAN", Colour::CYAN },
		{ "BLACK", Colour::BLACK }, { "BROWN", Colour::BROWN }, { "GRAY_LIGHT", Colour::GRAY_LIGHT }, { "AQUA_LIGHT", Colour::AQUA_LIGHT }, { "BLUE_LIGHT", Colour::BLUE_LIGHT }, { "PURPLE_LIGHT", Colour::PURPLE_LIGHT },
		{ "MAGENTA_LIGHT", Colour::MAGENTA_LIGHT }, { "PINK_LIGHT", Colour::PINK_LIGHT }, { "RED_LIGHT", Colour::RED_LIGHT }, { "ORANGE_LIGHT", Colour::ORANGE_LIGHT }, { "YELLOW_LIGHT", Colour::YELLOW_LIGHT },
		{ "LIME_LIGHT", Colour::LIME_LIGHT }, { "GREEN_LIGHT", Colour::GREEN_LIGHT }, { "TURQUOISE_LIGHT", Colour::TURQ_LIGHT }, { "TURQ_LIGHT", Colour::TURQ_LIGHT }, { "CYAN_LIGHT", Colour::CYAN_LIGHT },
		{ "BLACK_LIGHT", Colour::BLACK_LIGHT }, { "BROWN_LIGHT", Colour::BROWN_LIGHT }, { "YELLOW_PURE", Colour::YELLOW_PURE }, { "AQUA_BRIGHT", Colour::AQUA_BRIGHT }, { "BLUE_BRIGHT", Colour::BLUE_BRIGHT },
		{ "PURPLE_BRIGHT", Colour::PURPLE_BRIGHT }, { "MAGENTA_BRIGHT", Colour::MAGENTA_BRIGHT }, { "PINK_BRIGHT", Colour::PINK_BRIGHT }, { "RED_BRIGHT", Colour::RED_BRIGHT }, { "ORANGE_BRIGHT", Colour::ORANGE_BRIGHT },
		{ "YELLOW_BRIGHT", Colour::YELLOW_BRIGHT }, { "LIME_BRIGHT", Colour::LIME_BRIGHT }, { "GREEN_BRIGHT", Colour::GREEN_BRIGHT }, { "TURQUOISE_BRIGHT", Colour::TURQ_BRIGHT }, { "TURQ_BRIGHT", Colour::TURQ_BRIGHT },
		{ "CYAN_BRIGHT", Colour::CYAN_BRIGHT }, { "GRAY_DARK", Colour::GRAY_DARK }, { "BROWN_PALE", Colour::BROWN_PALE }, { "WHITE", Colour::WHITE }, { "AQUA_PALE", Colour::AQUA_PALE }, { "BLUE_PALE", Colour::BLUE_PALE },
		{ "PURPLE_PALE", Colour::PURPLE_PALE }, { "MAGENTA_PALE", Colour::MAGENTA_PALE }, { "PINK_PALE", Colour::PINK_PALE }, { "RED_PALE", Colour::RED_PALE }, { "ORANGE_PALE", Colour::ORANGE_PALE },
		{ "YELLOW_PALE", Colour::YELLOW_PALE }, { "LIME_PALE", Colour::LIME_PALE }, { "GREEN_PALE", Colour::GREEN_PALE }, { "TURQUOISE_PALE", Colour::TURQ_PALE }, { "TURQ_PALE", Colour::TURQ_PALE },
		{ "CYAN_PALE", Colour::CYAN_PALE }, { "GRAY_PALE", Colour::GRAY_PALE }, { "BLACK_PALE", Colour::BLACK_PALE }, { "TERM_BLACK", Colour::TERM_BLACK }, { "TERM_BLUE", Colour::TERM_BLUE }, { "TERM_GREEN", Colour::TERM_GREEN },
		{ "TERM_CYAN", Colour::TERM_CYAN }, { "TERM_RED", Colour::TERM_RED }, { "TERM_MAGENTA", Colour::TERM_MAGENTA }, { "TERM_YELLOW", Colour::TERM_YELLOW }, { "TERM_LGRAY", Colour::TERM_LGRAY },
		{ "TERM_LBLUE", Colour::TERM_LBLUE }, { "TERM_LGREEN", Colour::TERM_LGREEN }, { "TERM_LCYAN", Colour::TERM_LCYAN }, { "TERM_RED", Colour::TERM_RED }, { "TERM_LMAGENTA", Colour::TERM_LMAGENTA },
		{ "TERM_LYELLOW", Colour::TERM_YELLOW }, { "TERM_WHITE", Colour::TERM_WHITE }, { "CGA_BLACK", Colour::CGA_BLACK }, { "CGA_BLUE", Colour::CGA_BLUE }, { "CGA_GREEN", Colour::CGA_GREEN }, { "CGA_CYAN", Colour::CGA_CYAN },
		{ "CGA_RED", Colour::CGA_RED }, { "CGA_MAGENTA", Colour::CGA_MAGENTA }, { "CGA_YELLOW", Colour::CGA_YELLOW }, { "CGA_LGRAY", Colour::CGA_LGRAY }, { "CGA_GRAY", Colour::CGA_GRAY }, { "CGA_LBLUE", Colour::CGA_LBLUE },
		{ "CGA_LGREEN", Colour::CGA_LGREEN }, { "CGA_LCYAN", Colour::CGA_LCYAN }, { "CGA_LRED", Colour::CGA_LRED }, { "CGA_LMAGENTA", Colour::CGA_LMAGENTA }, { "CGA_LYELLOW", Colour::CGA_LYELLOW },
		{ "CGA_WHITE", Colour::CGA_WHITE } };

	auto found = colour_map.find(strx::str_toupper(colour_string));
	if (found == colour_map.end())
	{
		guru::log("Could not parse colour code: " + colour_string);
		return Colour::ERROR_COLOUR;
	}
	else return found->second;
}

// Parses a glyph string into a Glyph.
unsigned int parse_glyph_string(string glyph_string)
{
	STACK_TRACE();
	const std::unordered_map<string, Glyph> glyph_map = { { "FACE_BLACK", Glyph::FACE_BLACK }, { "FACE_WHITE", Glyph::FACE_WHITE }, { "HEART", Glyph::HEART }, { "DIAMOND", Glyph::DIAMOND }, { "CLUB", Glyph::CLUB },
		{ "SPADE", Glyph::SPADE }, { "BULLET", Glyph::BULLET }, { "BULLET_INVERT", Glyph::BULLET_INVERT }, { "CIRCLE", Glyph::CIRCLE }, { "CIRCLE_INVERT", Glyph::CIRCLE_INVERT }, { "MALE", Glyph::MALE }, { "FEMALE", Glyph::FEMALE },
		{ "MUSIC", Glyph::MUSIC }, { "MUSIC_DOUBLE", Glyph::MUSIC_DOUBLE }, { "SUN", Glyph::SUN }, { "TRIANGLE_RIGHT", Glyph::TRIANGLE_RIGHT }, { "TRIANGLE_LEFT", Glyph::TRIANGLE_LEFT }, { "ARROW_UD", Glyph::ARROW_UD },
		{ "DOUBLE_EXCLAIM", Glyph::DOUBLE_EXCLAIM }, { "PILCROW", Glyph::PILCROW }, { "SECTION", Glyph::SECTION }, { "GEOM_A", Glyph::GEOM_A }, { "ARROW_UD_B", Glyph::ARROW_UD_B }, { "ARROW_UP", Glyph::ARROW_UP },
		{ "ARROW_DOWN", Glyph::ARROW_DOWN }, { "ARROW_RIGHT", Glyph::ARROW_RIGHT }, { "ARROW_LEFT", Glyph::ARROW_LEFT }, { "BRACKET", Glyph::BRACKET }, { "ARROW_LR", Glyph::ARROW_LR }, { "TRIANGLE_UP", Glyph::TRIANGLE_UP },
		{ "TRIANGLE_DOWN", Glyph::TRIANGLE_DOWN }, { "HOUSE", Glyph::HOUSE }, { "C_CEDILLA_CAPS", Glyph::C_CEDILLA_CAPS }, { "U_DIAERESIS", Glyph::U_DIAERESIS }, { "E_ACUTE", Glyph::E_ACUTE }, { "A_CIRCUMFLEX", Glyph::A_CIRCUMFLEX },
		{ "A_DIAERESIS", Glyph::A_DIAERESIS }, { "A_GRAVE", Glyph::A_GRAVE }, { "A_OVERRING", Glyph::A_OVERRING }, { "C_CEDILLA", Glyph::C_CEDILLA }, { "E_CIRCUMFLEX", Glyph::E_CIRCUMFLEX }, { "E_DIAERESIS", Glyph::E_DIAERESIS },
		{ "E_GRAVE", Glyph::E_GRAVE }, { "I_DIAERESIS", Glyph::I_DIAERESIS }, { "I_CIRCUMFLEX", Glyph::I_CIRCUMFLEX }, { "I_GRAVE", Glyph::I_GRAVE }, { "A_DIAERESIS_CAPS", Glyph::A_DIAERESIS_CAPS },
		{ "A_OVERRING_CAPS", Glyph::A_OVERRING_CAPS }, { "E_ACUTE_CAPS", Glyph::E_ACUTE_CAPS }, { "AE", Glyph::AE }, { "AE_CAPS", Glyph::AE_CAPS }, { "O_CIRCUMFLE", Glyph::O_CIRCUMFLEX }, { "O_DIAERESIS", Glyph::O_DIAERESIS },
		{ "O_GRAVE", Glyph::O_GRAVE }, { "U_CIRCUMFLEX", Glyph::U_CIRCUMFLEX }, { "U_GRAVE", Glyph::U_GRAVE }, { "Y_DIAERESIS", Glyph::Y_DIAERESIS }, { "O_DIAERESIS_CAPS", Glyph::O_DIAERESIS_CAPS },
		{ "U_DIAERESIS_CAPS", Glyph::U_DIAERESIS_CAPS }, { "CENT", Glyph::CENT }, { "POUND", Glyph::POUND }, { "YEN", Glyph::YEN }, { "PESETA", Glyph::PESETA }, { "F_HOOK", Glyph::F_HOOK }, { "A_ACUTE", Glyph::A_ACUTE },
		{ "I_ACUTE", Glyph::I_ACUTE }, { "O_ACUTE", Glyph::O_ACUTE }, { "U_ACUTE", Glyph::U_ACUTE }, { "N_TILDE", Glyph::N_TILDE }, { "N_TILDE_CAPS", Glyph::N_TILDE_CAPS }, { "ORDINAL_F", Glyph::ORDINAL_F },
		{ "ORDINAL_M", Glyph::ORDINAL_M }, { "QUESTION_INVERTED", Glyph::QUESTION_INVERTED }, { "NOT_REVERSE", Glyph::NOT_REVERSE }, { "NOT", Glyph::NOT }, { "HALF", Glyph::HALF }, { "QUARTER", Glyph::QUARTER },
		{ "EXCLAIM_INVERTED", Glyph::EXCLAIM_INVERTED }, { "GUILLEMET_OPEN", Glyph::GUILLEMET_OPEN }, { "GUILLEMET_CLOSE", Glyph::GUILLEMET_CLOSE }, { "SHADE_LIGHT", Glyph::SHADE_LIGHT }, { "SHADE_MEDIUM", Glyph::SHADE_MEDIUM },
		{ "SHADE_HEAVY", Glyph::SHADE_HEAVY }, { "LINE_V", Glyph::LINE_V }, { "LINE_VL", Glyph::LINE_VL }, { "LINE_VLL", Glyph::LINE_VLL }, { "LINE_VVL", Glyph::LINE_VVL }, { "LINE_DDL", Glyph::LINE_DDL },
		{ "LINE_DLL", Glyph::LINE_DLL }, { "LINE_VVLL", Glyph::LINE_VVLL }, { "LINE_VV", Glyph::LINE_VV }, { "LINE_DDLL", Glyph::LINE_DDLL }, { "LINE_UULL", Glyph::LINE_UULL }, { "LINE_UUL", Glyph::LINE_UUL },
		{ "LINE_ULL", Glyph::LINE_ULL }, { "LINE_DL", Glyph::LINE_DL }, { "LINE_UR", Glyph::LINE_UR }, { "LINE_UH", Glyph::LINE_UH }, { "LINE_DH", Glyph::LINE_DH }, { "LINE_VR", Glyph::LINE_VR }, { "LINE_H", Glyph::LINE_H },
		{ "LINE_VH", Glyph::LINE_VH }, { "LINE_VRR", Glyph::LINE_VRR }, { "LINE_VVR", Glyph::LINE_VVR }, { "LINE_UURR", Glyph::LINE_UURR }, { "LINE_DDRR", Glyph::LINE_DDRR }, { "LINE_UUHH", Glyph::LINE_UUHH },
		{ "LINE_DDHH", Glyph::LINE_DDHH }, { "LINE_VVRR", Glyph::LINE_VVRR }, { "LINE_HH", Glyph::LINE_HH }, { "LINE_VVHH", Glyph::LINE_VVHH }, { "LINE_UHH", Glyph::LINE_UHH }, { "LINE_UUH", Glyph::LINE_UUH },
		{ "LINE_DHH", Glyph::LINE_DHH }, { "LINE_DDH", Glyph::LINE_DDH }, { "LINE_UUR", Glyph::LINE_UUR }, { "LINE_URR", Glyph::LINE_URR }, { "LINE_DRR", Glyph::LINE_DRR }, { "LINE_DDR", Glyph::LINE_DDR },
		{ "LINE_VVH", Glyph::LINE_VVH }, { "LINE_VHH", Glyph::LINE_VHH }, { "LINE_UL", Glyph::LINE_UL }, { "LINE_DR", Glyph::LINE_DR }, { "BLOCK_SOLID", Glyph::BLOCK_SOLID }, { "BLOCK_D", Glyph::BLOCK_D },
		{ "BLOCK_L", Glyph::BLOCK_L }, { "BLOCK_R", Glyph::BLOCK_R }, { "BLOCK_U", Glyph::BLOCK_U }, { "ALPHA", Glyph::ALPHA }, { "BETA", Glyph::BETA }, { "GAMMA_CAPS", Glyph::GAMMA_CAPS }, { "PI_CAPS", Glyph::PI_CAPS },
		{ "SIGMA_CAPS", Glyph::SIGMA_CAPS }, { "SIGMA", Glyph::SIGMA }, { "MU", Glyph::MU }, { "TAU", Glyph::TAU }, { "PHI_CAPS", Glyph::PHI_CAPS }, { "THETA_CAPS", Glyph::THETA_CAPS }, { "OMEGA_CAPS", Glyph::OMEGA_CAPS },
		{ "DELTA", Glyph::DELTA }, { "INFINITY", Glyph::INFINITY }, { "PHI", Glyph::PHI }, { "EPSILON", Glyph::EPSILON }, { "INTERSECTION", Glyph::INTERSECTION }, { "TRIPLE_BAR", Glyph::TRIPLE_BAR },
		{ "PLUS_MINUS", Glyph::PLUS_MINUS }, { "GEQ", Glyph::GEQ }, { "LEQ", Glyph::LEQ }, { "INTEGRAL", Glyph::INTEGRAL }, { "INTEGRAL_INVERTED", Glyph::INTEGRAL_INVERTED }, { "DIVISION", Glyph::DIVISION },
		{ "APPROXIMATION", Glyph::APPROXIMATION }, { "DEGREE", Glyph::DEGREE }, { "BULLET_SMALL", Glyph::BULLET_SMALL }, { "INTERPUNCT", Glyph::INTERPUNCT }, { "SQUARE_ROOT", Glyph::SQUARE_ROOT }, { "N_SUPER", Glyph::N_SUPER },
		{ "SQUARE", Glyph::SQUARE }, { "MIDBLOCK", Glyph::MIDBLOCK }, { "HALF_HEART", Glyph::HALF_HEART }, { "COPYRIGHT", Glyph::COPYRIGHT }, { "BLOCKS_7", Glyph::BLOCKS_7 }, { "BLOCKS_11", Glyph::BLOCKS_11 },
		{ "BLOCKS_14", Glyph::BLOCKS_14 }, { "BLOCKS_13", Glyph::BLOCKS_13 }, { "BLOCKS_4", Glyph::BLOCKS_4 }, { "UPSIDE_DOWN_HD", Glyph::UPSIDE_DOWN_HD }, { "BLOCKS_8", Glyph::BLOCKS_8 }, { "BLOCKS_1", Glyph::BLOCKS_1 },
		{ "BLOCKS_2", Glyph::BLOCKS_2 }, { "CORNER_CLIP_DL", Glyph::CORNER_CLIP_DL }, { "CORNER_CLIP_DR", Glyph::CORNER_CLIP_DR }, { "CURVE_DL", Glyph::CURVE_DL }, { "CURVE_UR", Glyph::CURVE_UR }, { "CURVE_UL", Glyph::CURVE_UL },
		{ "CURVE_DR", Glyph::CURVE_DR }, { "FLOPPY_DISK_METAL_HOLE", Glyph::FLOPPY_DISK_METAL_HOLE }, { "RETURN", Glyph::RETURN }, { "TICK", Glyph::TICK }, { "MIDDOT", Glyph::MIDDOT }, { "MIDCOMMA", Glyph::MIDCOMMA },
		{ "SKULL", Glyph::SKULL }, { "ELLIPSIS", Glyph::ELLIPSIS } };

	if (glyph_string.size() == 1) return glyph_string[0];
	auto found = glyph_map.find(strx::str_toupper(glyph_string));
	if (found == glyph_map.end())
	{
		guru::log("Could not parse glyph code: " + glyph_string);
		return '?';
	}
	else return static_cast<unsigned int>(found->second);
}

// Parses a tile name ID into a string.
string tile_name(unsigned int name_id)
{
	auto found = tile_names.find(name_id);
	if (found == tile_names.end())
	{
		guru::log("Could not decode tile name ID " + strx::itos(name_id) + "!", GURU_ERROR);
		return "[unknown]";
	}
	return found->second;
}

}	// namespace data
