// wiki.h -- In-game wiki for game documentation and in-game help.
// Copyright (c) 2017, 2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"
#include <unordered_map>


class Wiki
{
public:
			Wiki();
	void	wiki(string page);				// Displays a specific wiki window.

private:
	string	get_wiki_data(string key);		// Attempts to retrieve wiki data; returns a blank string if the requested page does not exist.
	string	link(string page);				// Processes a wiki link.
	void	process_key(unsigned int key);	// Processes input in the wiki window.
	void	process_wiki_buffer();			// Processes the wiki buffer after an update or screen resize.
	void	render();						// Redraws in the in-game wiki.
	void	reset_buffer_pos();				// Resets the wiki buffer position.

	unsigned int	buffer_pos;			// The position of the console buffer.
	vector<std::pair<unsigned short, unsigned short>>	link_coords;	// Coordinates of the wiki links on the page.
	vector<bool>	link_good;			// Is this a valid link?
	unsigned int	link_selected;		// The current active wiki link.
	vector<string>	link_str;			// Strings for the links.
	std::unordered_map<string, string>	wiki_data;	// The actual wiki entries, read from wiki.json
	vector<string>	wiki_history;		// Previous wiki pages viewed.
	vector<string>	wiki_prc;			// The nicely processed wiki buffer, ready for rendering.
	vector<string>	wiki_raw;			// The raw, unprocessed wiki buffer.
};


extern shared_ptr<Wiki>	wiki;	// The in-game wiki object.
