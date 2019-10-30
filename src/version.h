// version.h -- The current version number of the game project.
// Copyright (c) 2017-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

#define DUSKFALL_VERSION_EPOCH		0
#define DUSKFALL_VERSION_RELEASE	0
#define DUSKFALL_VERSION_MAJOR		0
#define DUSKFALL_VERSION_MINOR		2

#define DUSKFALL_MAJOR_VERSION_STRING	(std::to_string(DUSKFALL_VERSION_EPOCH) + "." + std::to_string(DUSKFALL_VERSION_RELEASE) + "." + std::to_string(DUSKFALL_VERSION_MAJOR))
#define DUSKFALL_VERSION_STRING			(DUSKFALL_MAJOR_VERSION_STRING + "." + std::to_string(DUSKFALL_VERSION_MINOR))
