// version.h -- The current version number of the game project.
// Copyright (c) 2017-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

// NOTE TO CONTRIBUTORS!
// Please DO NOT adjust the version numbers here! I already have a versioning system in mind for the official releases of Duskfall, and things would great all mixed up
// and confusing if indivudual modders/developers started changing the version numbers in their pull requests. Please leave these numbers as-is, and I will assign new
// version numbers personally to all code that is added to the master branch.

#define DUSKFALL_VERSION_EPOCH		0
#define DUSKFALL_VERSION_MAJOR		13
#define DUSKFALL_VERSION_MINOR		0
#define DUSKFALL_VERSION_PATCH		4
#define DUSKFALL_EDITION			"A New Beginning"

#define DUSKFALL_MAJOR_VERSION_STRING	(std::to_string(DUSKFALL_VERSION_EPOCH) + "." + std::to_string(DUSKFALL_VERSION_MAJOR) + "." + std::to_string(DUSKFALL_VERSION_MINOR))
#define DUSKFALL_VERSION_STRING			(DUSKFALL_MAJOR_VERSION_STRING + "." + std::to_string(DUSKFALL_VERSION_PATCH))
