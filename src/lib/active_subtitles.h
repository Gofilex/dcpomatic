/*
    Copyright (C) 2017 Carl Hetherington <cth@carlh.net>

    This file is part of DCP-o-matic.

    DCP-o-matic is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DCP-o-matic is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DCP-o-matic.  If not, see <http://www.gnu.org/licenses/>.

*/

/** @file  src/lib/active_subtitles.h
 *  @brief ActiveSubtitles class.
 */

#include "dcpomatic_time.h"
#include "player_subtitles.h"
#include <boost/noncopyable.hpp>
#include <list>
#include <map>

class Piece;

/** @class ActiveSubtitles
 *  @brief A class to maintain information on active subtitles for Player.
 */
class ActiveSubtitles : public boost::noncopyable
{
public:
	std::list<PlayerSubtitles> get_burnt (DCPTimePeriod period, bool always_burn_subtitles) const;
	void clear_before (DCPTime time);
	void clear ();
	void add_from (boost::weak_ptr<Piece> piece, PlayerSubtitles ps, DCPTime from);
	std::pair<PlayerSubtitles, DCPTime> add_to (boost::weak_ptr<Piece> piece, DCPTime to);
	bool have (boost::weak_ptr<Piece> piece) const;

private:
	class Period
	{
	public:
		Period () {}

		Period (PlayerSubtitles s, DCPTime f)
			: subs (s)
			, from (f)
		{}

		PlayerSubtitles subs;
		DCPTime from;
		boost::optional<DCPTime> to;
	};

	typedef std::map<boost::weak_ptr<Piece>, std::list<Period> > Map;

	Map _data;
};
