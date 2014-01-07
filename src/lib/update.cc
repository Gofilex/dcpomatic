/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

#include <string>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <curl/curl.h>
#include <libcxml/cxml.h>
#include "update.h"
#include "version.h"
#include "ui_signaller.h"

#define BUFFER_SIZE 1024

using std::cout;
using std::min;
using std::string;
using std::stringstream;
using boost::lexical_cast;

UpdateChecker* UpdateChecker::_instance = 0;

static size_t
write_callback_wrapper (void* data, size_t size, size_t nmemb, void* user)
{
	return reinterpret_cast<UpdateChecker*>(user)->write_callback (data, size, nmemb);
}

UpdateChecker::UpdateChecker ()
	: _buffer (new char[BUFFER_SIZE])
	, _offset (0)
	, _curl (0)
	, _state (NOT_RUN)
	, _emits (0)
{
	curl_global_init (CURL_GLOBAL_ALL);
	_curl = curl_easy_init ();

	curl_easy_setopt (_curl, CURLOPT_URL, "http://dcpomatic.com/update");
	curl_easy_setopt (_curl, CURLOPT_WRITEFUNCTION, write_callback_wrapper);
	curl_easy_setopt (_curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt (_curl, CURLOPT_TIMEOUT, 20);
	
	string const agent = "dcpomatic/" + string (dcpomatic_version);
	curl_easy_setopt (_curl, CURLOPT_USERAGENT, agent.c_str ());

	_thread = new boost::thread (boost::bind (&UpdateChecker::thread, this));
}

UpdateChecker::~UpdateChecker ()
{
	/* We are not cleaning up our thread, but hey well */
	
	curl_easy_cleanup (_curl);
	curl_global_cleanup ();
	delete[] _buffer;
}

void
UpdateChecker::run ()
{
	boost::mutex::scoped_lock lm (_process_mutex);
	_condition.notify_one ();
}

void
UpdateChecker::thread ()
{
	while (1) {
		boost::mutex::scoped_lock lock (_process_mutex);
		_condition.wait (lock);
		lock.unlock ();
		
		try {
			_offset = 0;
			
			int r = curl_easy_perform (_curl);
			if (r != CURLE_OK) {
				set_state (FAILED);
				return;
			}
			
			_buffer[_offset] = '\0';
			stringstream s;
			s << _buffer;
			cxml::Document doc ("Update");
			doc.read_stream (s);
			
			{
				boost::mutex::scoped_lock lm (_data_mutex);
				_stable = doc.string_child ("Stable");
				_test = doc.string_child ("Test");
			}
			
			string current = string (dcpomatic_version);
			bool current_pre = false;
			if (boost::algorithm::ends_with (current, "pre")) {
				current = current.substr (0, current.length() - 3);
				current_pre = true;
			}
			
			float current_float = lexical_cast<float> (current);
			if (current_pre) {
				current_float -= 0.005;
			}
			
			if (current_float < lexical_cast<float> (_stable)) {
				set_state (YES);
			} else {
				set_state (NO);
			}
		} catch (...) {
			set_state (FAILED);
		}
	}
}
	
size_t
UpdateChecker::write_callback (void* data, size_t size, size_t nmemb)
{
	size_t const t = min (size * nmemb, size_t (BUFFER_SIZE - _offset - 1));
	memcpy (_buffer + _offset, data, t);
	_offset += t;
	return t;
}

void
UpdateChecker::set_state (State s)
{
	{
		boost::mutex::scoped_lock lm (_data_mutex);
		_state = s;
		_emits++;
	}

	ui_signaller->emit (boost::bind (boost::ref (StateChanged)));
}

UpdateChecker *
UpdateChecker::instance ()
{
	if (!_instance) {
		_instance = new UpdateChecker ();
	}

	return _instance;
}

	
