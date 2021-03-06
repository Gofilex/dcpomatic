/*
    Copyright (C) 2013-2017 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_SUBTITLE_DECODER_H
#define DCPOMATIC_SUBTITLE_DECODER_H

#include "decoder.h"
#include "rect.h"
#include "types.h"
#include "content_subtitle.h"
#include "decoder_part.h"
#include <dcp/subtitle_string.h>
#include <boost/signals2.hpp>

namespace sub {
	class Subtitle;
}

class Image;

class SubtitleDecoder : public DecoderPart
{
public:
	SubtitleDecoder (
		Decoder* parent,
		boost::shared_ptr<const SubtitleContent>,
		boost::shared_ptr<Log> log,
		ContentTime first
		);

	ContentTime position () const {
		return _position;
	}

	void emit_image_start (ContentTime from, boost::shared_ptr<Image> image, dcpomatic::Rect<double> rect);
	void emit_text_start (ContentTime from, std::list<dcp::SubtitleString> s);
	void emit_text_start (ContentTime from, sub::Subtitle const & subtitle);
	void emit_text (ContentTimePeriod period, std::list<dcp::SubtitleString> s);
	void emit_text (ContentTimePeriod period, sub::Subtitle const & subtitle);
	void emit_stop (ContentTime to);

	void seek ();

	boost::shared_ptr<const SubtitleContent> content () const {
		return _content;
	}

	boost::signals2::signal<void (ContentImageSubtitle)> ImageStart;
	boost::signals2::signal<void (ContentTextSubtitle)> TextStart;
	boost::signals2::signal<void (ContentTime)> Stop;

private:
	boost::shared_ptr<const SubtitleContent> _content;
	ContentTime _position;
};

#endif
