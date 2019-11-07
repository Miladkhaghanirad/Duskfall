// menu.cpp -- Menu class definition, for a fairly generic scrollable menu of items.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "iocore.h"
#include "menu.h"
#include "strx.h"
#include "world.h"

//#include "sdl2/SDL_keycode.h"

#define MENU_SIDEBOX_WIDTH	20


// Adds an item to this Menu.
void Menu::add_item(string txt, Colour col, string sidebox)
{
	STACK_TRACE();
	items.push_back(txt);
	item_x.push_back(0);
	colour.push_back(col);
	item_sidebox.push_back(sidebox);
	if (sidebox.size())
	{
		const unsigned int height = strx::ansi_vector_split(sidebox, MENU_SIDEBOX_WIDTH).size();
		if (height > sidebox_height) sidebox_height = height;
	}
}

// Repositions the menu.
void Menu::reposition()
{
	STACK_TRACE();
	if (!items.size()) return;
	const int midrow = iocore::midrow(), midcol = iocore::midcol();
	unsigned int widest = 0;
	for (auto item : items)
	{
		const unsigned int len = strx::ansi_strlen(item);
		if (len > widest) widest = len;
	}
	if (strx::ansi_strlen(tag_bl) + strx::ansi_strlen(tag_br) > widest) widest = strx::ansi_strlen(tag_bl) + strx::ansi_strlen(tag_br);
	if (title.size() > widest) widest = title.size();
	x_size = widest + 4;
	y_size = items.size() + 4;
	if (y_size > 24) y_size = 24;
	x_pos = midcol - (x_size / 2);
	y_pos = midrow - (y_size / 2);
	if (offset_text) x_pos += ((MENU_SIDEBOX_WIDTH + 4) / 2);

	for (unsigned int i = 0; i < item_x.size(); i++)
	{
		item_x.at(i) = midcol - (strx::ansi_strlen(items.at(i)) / 2);
		if (offset_text) item_x.at(i) += ((MENU_SIDEBOX_WIDTH + 4) / 2);
	}

	title_x = midcol - (title.size() / 2);
	if (offset_text) title_x += ((MENU_SIDEBOX_WIDTH + 4) / 2);
	bl_x = x_pos + 1;
	br_x = x_pos + x_size - strx::ansi_strlen(tag_br) - 1;
}

// Renders the menu, returns the chosen menu item (or -1 if none chosen)
int Menu::render()
{
	STACK_TRACE();
	if (!items.size()) return -1;
	redraw_background(false);

	reposition();
	while(true)
	{
		iocore::box(x_pos, y_pos, x_size, y_size, UI_COLOUR_BOX);
		if (title.size())
		{
			iocore::print_at(Glyph::LINE_VL, title_x - 1, y_pos, UI_COLOUR_BOX);
			iocore::print_at(Glyph::LINE_VR, title_x + title.size(), y_pos, UI_COLOUR_BOX);
			iocore::print(title, title_x, y_pos, Colour::CGA_LCYAN, PRINT_FLAG_ALT_FONT);
		}
		if (tag_bl.size()) iocore::print(tag_bl, bl_x, y_pos + y_size - 1, Colour::CGA_WHITE);
		if (tag_br.size()) iocore::print(tag_br, br_x, y_pos + y_size - 1, Colour::CGA_WHITE);
		const unsigned int start = offset;
		unsigned int end = items.size();
		if (end - offset > 20) end = 20 + offset;
		for (unsigned int i = start; i < end; i++)
		{
			if (selected == i) iocore::rect(item_x.at(i), y_pos + 2 + i - offset, strx::ansi_strlen(items.at(i)), 1, static_cast<Colour>(colour.at(i)));
			iocore::print(items.at(i), item_x.at(i), y_pos + 2 + i - offset, (selected == i ? Colour::BLACK_LIGHT : static_cast<Colour>(colour.at(i))), PRINT_FLAG_ALPHA);
		}
		if (offset > 0) iocore::print_at(Glyph::ARROW_UP, x_pos + x_size - 1, y_pos + 1, Colour::CGA_LGREEN);
		if (end < items.size()) iocore::print_at(Glyph::ARROW_DOWN, x_pos + x_size - 1, y_pos + y_size - 2, Colour::CGA_LGREEN);
		if (offset_text)
		{
			iocore::box(x_pos - MENU_SIDEBOX_WIDTH - 4, y_pos, MENU_SIDEBOX_WIDTH + 4, sidebox_height + 4, UI_COLOUR_BOX);
			if (item_sidebox.at(selected).size())
			{
				vector<string> lines = strx::ansi_vector_split(item_sidebox.at(selected), MENU_SIDEBOX_WIDTH);
				for (unsigned int i = 0; i < lines.size(); i++)
					iocore::ansi_print(lines.at(i), x_pos - MENU_SIDEBOX_WIDTH - 2, y_pos + 2 + i);
			}
		}
		iocore::flip();

		const unsigned int key = iocore::wait_for_key();
		if (key == RESIZE_KEY)
		{
			redraw_background(false);
			reposition();
		}
		else if ((iocore::is_up(key) || key == MOUSEWHEEL_UP_KEY) && selected > 0)
		{
			selected--;
			while (selected > 0 && (!items.at(selected).size() || colour.at(selected) == Colour::CGA_GRAY)) selected--;
		}
		else if ((iocore::is_down(key) || key == MOUSEWHEEL_DOWN_KEY) && selected < items.size() - 1)
		{
			selected++;
			while (selected < items.size() - 1 && (!items.at(selected).size() || colour.at(selected) == Colour::CGA_GRAY)) selected++;
		}
		else if (iocore::is_left(key) && allow_left) { redraw_background(false); return -2; }
		else if (iocore::is_right(key) && allow_right) { redraw_background(false); return -3; }
		else if (iocore::is_select(key)) { redraw_background(redraw_on_exit); return selected; }
		else if (iocore::is_cancel(key) || key == RMB_KEY) { redraw_background(redraw_on_exit); return -1; }
		else if (key == LMB_KEY)
		{
			for (unsigned int i = start; i < end; i++)
			{
				if (iocore::did_mouse_click(item_x.at(i), y_pos + 2 + i - offset, strx::ansi_strlen(items.at(i))))
				{
					selected = i;
					redraw_background(redraw_on_exit);
					return selected;
				}
			}
			if (allow_left && iocore::did_mouse_click(bl_x, y_pos + y_size - 1, tag_bl.size(), 1)) { redraw_background(false); return -2; }
			else if (allow_right && iocore::did_mouse_click(br_x, y_pos + y_size - 1, tag_br.size(), 1)) { redraw_background(false); return -3; }
			else if (offset > 0 && iocore::did_mouse_click(x_pos + x_size - 1, y_pos + 1, 1, 1))
			{
				offset--;
				if (selected >= end - 1) selected--;
			}
			else if (end < items.size() && iocore::did_mouse_click(x_pos + x_size - 1, y_pos + y_size - 2, 1, 1))
			{
				offset++;
				if (offset > selected) selected++;
			}
		}

		if (selected > offset + 19) offset++;
		else if (selected < offset) offset--;
	}
	return 0;
}

// Redraws whatever's behind the menu.
void Menu::redraw_background(bool flip) const
{
	STACK_TRACE();
	world::full_redraw();
	if (flip) iocore::flip();
}

// Sets a title for this menu.
void Menu::set_title(string new_title)
{
	title = new_title;
}

// Sets one or both of the bottom tags.
void Menu::set_tags(string bl, string br)
{
	STACK_TRACE();
	if (bl.size()) tag_bl = bl;
	if (br.size()) tag_br = br;
}

// Allow left and/or right keys as input.
void Menu::allow_left_right(unsigned int flags)
{
	if ((flags & ALR_FLAG_LEFT) == ALR_FLAG_LEFT) allow_left = true;
	if ((flags & ALR_FLAG_RIGHT) == ALR_FLAG_RIGHT) allow_right = true;
}

// Sets the currently-selected item.
void Menu::set_selected(unsigned int pos)
{
	selected = pos;
	while (selected > offset + 19) offset++;
	while (selected < offset) offset--;
}
