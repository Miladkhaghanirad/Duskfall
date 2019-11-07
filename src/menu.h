// menu.h -- Menu class definition, for a fairly generic scrollable menu of items.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"

enum class Colour : unsigned char;	// defined in iocore.h

#define ALR_FLAG_LEFT	(1 << 0)
#define ALR_FLAG_RIGHT	(1 << 1)


class Menu
{
public:
	void	add_item(string txt, Colour col = static_cast<Colour>(0x5F), string sidebox = "");	// Adds an item to this Menu.
	int		render();	// Renders the menu, returns the chosen menu item (or -1 if none chosen)
	void	set_title(string new_title);	// Sets a title for this menu.
	void	set_tags(string bl = "", string br = "");	// Sets one or both of the bottom tags.
	void	allow_left_right(unsigned int flags = 0);	// Allow left and/or right keys as input.
	void	set_selected(unsigned int pos);	// Sets the currently-selected item.
	void	no_redraw_on_exit() { redraw_on_exit = false; }	// Disables redraw on exit.
	void	set_centered_text(bool choice) { centered_text = choice; }	// Do we want the text center-aligned?
	void	set_sidebox(bool choice) { offset_text = choice; }	// Do we want an offset sidebox?

private:
	void	redraw_background(bool flip) const;	// Redraws whatever's behind the menu.
	void	reposition();	// Repositions the menu.

	vector<string>	items;	// The menu item text.
	vector<int>		item_x;	// The menu item positions.
	vector<Colour>	colour;	// The colours of menu items.
	vector<string>	item_sidebox;		// Optionally, sidebox text for the selected menu item.
	int	x_pos = 0, y_pos = 0, x_size = 0, y_size = 0, title_x = 0, bl_x = 0, br_x = 0;	// Screen coordinates.
	unsigned int	selected = 0;		// The selected menu item.
	unsigned int	offset = 0;			// The menu scroll.
	unsigned short	sidebox_height = 0;	// The height of the optional sidebox.
	string			title;				// The menu's title, if any.
	string			tag_bl, tag_br;		// Tag text at the bottom left and bottom right.
	bool			allow_left = false, allow_right = false;	// Whether to allow left-right keys as input.
	bool			redraw_on_exit = true;	// Redraws the background when exiting the menu.
	bool			centered_text = true;	// Do we want the text center-aligned?
	bool			offset_text = false;	// Do we want an offset sidebox?
};
