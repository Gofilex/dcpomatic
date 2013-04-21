/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_AUDIO_SINK_H
#define DCPOMATIC_AUDIO_SINK_H

class AudioSink
{
public:
	/** Call with some audio data */
	virtual void process_audio (boost::shared_ptr<AudioBuffers>) = 0;
};

class TimedAudioSink
{
public:
        /** Call with some audio data */
        virtual void process_audio (boost::shared_ptr<AudioBuffers>, double t) = 0;
};

#endif
