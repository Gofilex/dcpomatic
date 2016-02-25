/*
    Copyright (C) 2015-2016 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "timeline_view.h"

class TimelineReelsView : public TimelineView
{
public:
	TimelineReelsView (Timeline& tl, int y);

	dcpomatic::Rect<int> bbox () const;
	void set_y (int y);

private:
	void do_paint (wxGraphicsContext* gc, std::list<dcpomatic::Rect<int> > overlaps);

	int _y;
};
