// wiki.h -- In-game wiki for game documentation and in-game help.
// Copyright (c) 2017, 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"


namespace wiki
{

string	get_wiki_data(string key);		// Attempts to retrieve wiki data; returns a blank string if the requested page does not exist.
void	init();							// Loads the wiki data from JSON files.
string	link(string page);				// Processes a wiki link.
void	process_key(unsigned int key);	// Processes input in the wiki window.
void	process_wiki_buffer();			// Processes the wiki buffer after an update or screen resize.
void	render();						// Redraws in the in-game wiki.
void	reset_buffer_pos();				// Resets the wiki buffer position.
void	wiki(string page);				// Displays a specific wiki window.

}	// namespace  wiki
