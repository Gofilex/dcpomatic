/*
    Copyright (C) 2013-2016 Carl Hetherington <cth@carlh.net>

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

/** @file  src/lib/content.cc
 *  @brief Content class.
 */

#include "content.h"
#include "util.h"
#include "content_factory.h"
#include "video_content.h"
#include "audio_content.h"
#include "subtitle_content.h"
#include "exceptions.h"
#include "film.h"
#include "job.h"
#include "compose.hpp"
#include <dcp/locale_convert.h>
#include <dcp/raw_convert.h>
#include <libcxml/cxml.h>
#include <libxml++/libxml++.h>
#include <boost/thread/mutex.hpp>
#include <iostream>

#include "i18n.h"

using std::string;
using std::list;
using std::cout;
using std::vector;
using std::max;
using std::pair;
using boost::shared_ptr;
using boost::optional;
using dcp::raw_convert;
using dcp::locale_convert;

int const ContentProperty::PATH = 400;
int const ContentProperty::POSITION = 401;
int const ContentProperty::LENGTH = 402;
int const ContentProperty::TRIM_START = 403;
int const ContentProperty::TRIM_END = 404;
int const ContentProperty::VIDEO_FRAME_RATE = 405;

Content::Content (shared_ptr<const Film> film)
	: _film (film)
	, _position (0)
	, _trim_start (0)
	, _trim_end (0)
	, _change_signals_frequent (false)
{

}

Content::Content (shared_ptr<const Film> film, DCPTime p)
	: _film (film)
	, _position (p)
	, _trim_start (0)
	, _trim_end (0)
	, _change_signals_frequent (false)
{

}

Content::Content (shared_ptr<const Film> film, boost::filesystem::path p)
	: _film (film)
	, _position (0)
	, _trim_start (0)
	, _trim_end (0)
	, _change_signals_frequent (false)
{
	_paths.push_back (p);
}

Content::Content (shared_ptr<const Film> film, cxml::ConstNodePtr node)
	: _film (film)
	, _change_signals_frequent (false)
{
	list<cxml::NodePtr> path_children = node->node_children ("Path");
	for (list<cxml::NodePtr>::const_iterator i = path_children.begin(); i != path_children.end(); ++i) {
		_paths.push_back ((*i)->content ());
	}
	_digest = node->optional_string_child ("Digest").get_value_or ("X");
	_position = DCPTime (node->number_child<DCPTime::Type> ("Position"));
	_trim_start = ContentTime (node->number_child<ContentTime::Type> ("TrimStart"));
	_trim_end = ContentTime (node->number_child<ContentTime::Type> ("TrimEnd"));
	_video_frame_rate = node->optional_number_child<double> ("VideoFrameRate");
}

Content::Content (shared_ptr<const Film> film, vector<shared_ptr<Content> > c)
	: _film (film)
	, _position (c.front()->position ())
	, _trim_start (c.front()->trim_start ())
	, _trim_end (c.back()->trim_end ())
	, _video_frame_rate (c.front()->video_frame_rate())
	, _change_signals_frequent (false)
{
	for (size_t i = 0; i < c.size(); ++i) {
		if (i > 0 && c[i]->trim_start() > ContentTime ()) {
			throw JoinError (_("Only the first piece of content to be joined can have a start trim."));
		}

		if (i < (c.size() - 1) && c[i]->trim_end () > ContentTime ()) {
			throw JoinError (_("Only the last piece of content to be joined can have an end trim."));
		}

		if (
			(_video_frame_rate && !c[i]->_video_frame_rate) ||
			(!_video_frame_rate && c[i]->_video_frame_rate)
			) {
			throw JoinError (_("Content to be joined must have the same video frame rate"));
		}

		if (_video_frame_rate && fabs (_video_frame_rate.get() - c[i]->_video_frame_rate.get()) > VIDEO_FRAME_RATE_EPSILON) {
			throw JoinError (_("Content to be joined must have the same video frame rate"));
		}

		for (size_t j = 0; j < c[i]->number_of_paths(); ++j) {
			_paths.push_back (c[i]->path (j));
		}
	}
}

void
Content::as_xml (xmlpp::Node* node, bool with_paths) const
{
	boost::mutex::scoped_lock lm (_mutex);

	if (with_paths) {
		for (vector<boost::filesystem::path>::const_iterator i = _paths.begin(); i != _paths.end(); ++i) {
			node->add_child("Path")->add_child_text (i->string ());
		}
	}
	node->add_child("Digest")->add_child_text (_digest);
	node->add_child("Position")->add_child_text (raw_convert<string> (_position.get ()));
	node->add_child("TrimStart")->add_child_text (raw_convert<string> (_trim_start.get ()));
	node->add_child("TrimEnd")->add_child_text (raw_convert<string> (_trim_end.get ()));
	if (_video_frame_rate) {
		node->add_child("VideoFrameRate")->add_child_text (raw_convert<string> (_video_frame_rate.get()));
	}
}

void
Content::examine (shared_ptr<Job> job)
{
	if (job) {
		job->sub (_("Computing digest"));
	}

	boost::mutex::scoped_lock lm (_mutex);
	vector<boost::filesystem::path> p = _paths;
	lm.unlock ();

	/* Some content files are very big, so we use a poor man's
	   digest here: a digest of the first and last 1e6 bytes with the
	   size of the first file tacked on the end as a string.
	*/
	string const d = digest_head_tail (p, 1000000) + raw_convert<string> (boost::filesystem::file_size (p.front ()));

	lm.lock ();
	_digest = d;
}

void
Content::signal_changed (int p)
{
	try {
		emit (boost::bind (boost::ref (Changed), shared_from_this (), p, _change_signals_frequent));
	} catch (boost::bad_weak_ptr) {
		/* This must be during construction; never mind */
	}
}

void
Content::set_position (DCPTime p)
{
	/* video content can modify its position */
	if (video) {
		video->modify_position (p);
	}

	{
		boost::mutex::scoped_lock lm (_mutex);
		if (p == _position) {
			return;
		}

		_position = p;
	}

	signal_changed (ContentProperty::POSITION);
}

void
Content::set_trim_start (ContentTime t)
{
	/* video content can modify its start trim */
	if (video) {
		video->modify_trim_start (t);
	}

	{
		boost::mutex::scoped_lock lm (_mutex);
		_trim_start = t;
	}

	signal_changed (ContentProperty::TRIM_START);
}

void
Content::set_trim_end (ContentTime t)
{
	{
		boost::mutex::scoped_lock lm (_mutex);
		_trim_end = t;
	}

	signal_changed (ContentProperty::TRIM_END);
}


shared_ptr<Content>
Content::clone () const
{
	shared_ptr<const Film> film = _film.lock ();
	if (!film) {
		return shared_ptr<Content> ();
	}

	/* This is a bit naughty, but I can't think of a compelling reason not to do it ... */
	xmlpp::Document doc;
	xmlpp::Node* node = doc.create_root_node ("Content");
	as_xml (node, true);

	/* notes is unused here (we assume) */
	list<string> notes;
	return content_factory (film, cxml::NodePtr (new cxml::Node (node)), Film::current_state_version, notes);
}

string
Content::technical_summary () const
{
	return String::compose ("%1 %2 %3", path_summary(), digest(), position().seconds());
}

DCPTime
Content::length_after_trim () const
{
	return max (DCPTime (), full_length() - DCPTime (trim_start() + trim_end(), film()->active_frame_rate_change (position ())));
}

/** @return string which changes when something about this content changes which affects
 *  the appearance of its video.
 */
string
Content::identifier () const
{
	char buffer[256];
	snprintf (
		buffer, sizeof(buffer), "%s_%" PRId64 "_%" PRId64 "_%" PRId64,
		Content::digest().c_str(), position().get(), trim_start().get(), trim_end().get()
		);
	return buffer;
}

bool
Content::paths_valid () const
{
	for (vector<boost::filesystem::path>::const_iterator i = _paths.begin(); i != _paths.end(); ++i) {
		if (!boost::filesystem::exists (*i)) {
			return false;
		}
	}

	return true;
}

void
Content::set_path (boost::filesystem::path path)
{
	_paths.clear ();
	_paths.push_back (path);
	signal_changed (ContentProperty::PATH);
}

void
Content::set_paths (vector<boost::filesystem::path> paths)
{
	_paths = paths;
	signal_changed (ContentProperty::PATH);
}

string
Content::path_summary () const
{
	/* XXX: should handle multiple paths more gracefully */

	DCPOMATIC_ASSERT (number_of_paths ());

	string s = path(0).filename().string ();
	if (number_of_paths() > 1) {
		s += " ...";
	}

	return s;
}

/** @return a list of properties that might be interesting to the user */
list<UserProperty>
Content::user_properties () const
{
	list<UserProperty> p;
	add_properties (p);
	return p;
}

shared_ptr<const Film>
Content::film () const
{
	shared_ptr<const Film> film = _film.lock ();
	DCPOMATIC_ASSERT (film);
	return film;
}

/** @return DCP times of points within this content where a reel split could occur */
list<DCPTime>
Content::reel_split_points () const
{
	list<DCPTime> t;
	/* This is only called for video content and such content has its position forced
	   to start on a frame boundary.
	*/
	t.push_back (position());
	return t;
}

void
Content::set_video_frame_rate (double r)
{
	{
		boost::mutex::scoped_lock lm (_mutex);
		_video_frame_rate = r;
	}

	signal_changed (ContentProperty::VIDEO_FRAME_RATE);
}

void
Content::unset_video_frame_rate ()
{
	{
		boost::mutex::scoped_lock lm (_mutex);
		_video_frame_rate = optional<double>();
	}

	signal_changed (ContentProperty::VIDEO_FRAME_RATE);
}

double
Content::active_video_frame_rate () const
{
	{
		boost::mutex::scoped_lock lm (_mutex);
		if (_video_frame_rate) {
			return _video_frame_rate.get ();
		}
	}

	/* No frame rate specified, so assume this content has been
	   prepared for any concurrent video content or perhaps
	   just the DCP rate.
	*/
	shared_ptr<const Film> film = _film.lock ();
	DCPOMATIC_ASSERT (film);
	return film->active_frame_rate_change(position()).source;
}

void
Content::add_properties (list<UserProperty>& p) const
{
	p.push_back (UserProperty (UserProperty::GENERAL, _("Filename"), path(0).string ()));

	if (_video_frame_rate) {
		if (video) {
			p.push_back (
				UserProperty (
					UserProperty::VIDEO,
					_("Frame rate"),
					locale_convert<string> (_video_frame_rate.get(), 5),
					_("frames per second")
					)
				);
		} else {
			p.push_back (
				UserProperty (
					UserProperty::GENERAL,
					_("Prepared for video frame rate"),
					locale_convert<string> (_video_frame_rate.get(), 5),
					_("frames per second")
					)
				);
		}
	}
}

/** Take settings from the given content if it is of the correct type */
void
Content::take_settings_from (shared_ptr<const Content> c)
{
	if (video && c->video) {
		video->take_settings_from (c->video);
	}
	if (audio && c->audio) {
		audio->take_settings_from (c->audio);
	}
	if (subtitle && c->subtitle) {
		subtitle->take_settings_from (c->subtitle);
	}
}
