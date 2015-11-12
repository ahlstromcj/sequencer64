#ifndef SEQ64_MIDI_CONTAINER_HPP
#define SEQ64_MIDI_CONTAINER_HPP

/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          midi_container.hpp
 *
 *  This module declares the abstract base class for the management of some
 *  MIDI events, using the sequence class.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2015-11-11
 * \license       GNU GPLv2 or above
 *
 */

#include <cstddef>                      /* std::size_t  */

namespace seq64
{

class sequence;

/**
 *  Provides tags used by the midifile class to control the reading and
 *  writing of the extra "proprietary" information stored in a Seq24 MIDI
 *  file.
 *
 *  As an extension, we will eventually grab the last key, scale, and
 *  background sequence value selected in a sequence and write them to the
 *  proprietary header, where they can be read in and applied to all
 *  sequences, when the seqedit object is created.  These values would not be
 *  stored in the legacy format.
 *
 *  Something like this could be done in the "user" configuration file, but
 *  then the key and scale would apply to all songs.  We don't want that.
 *
 *  We could also add snap and note-length to the per-song defaults, but
 *  the "user" configuration file seems like a better place to store these
 *  preferences.
 */

const unsigned long c_midibus =         0x24240001;
const unsigned long c_midich =          0x24240002;
const unsigned long c_midiclocks =      0x24240003;
const unsigned long c_triggers =        0x24240004;
const unsigned long c_notes =           0x24240005;
const unsigned long c_timesig =         0x24240006;
const unsigned long c_bpmtag =          0x24240007;
const unsigned long c_triggers_new =    0x24240008;
const unsigned long c_mutegroups =      0x24240009;
const unsigned long c_midictrl =        0x24240010;
const unsigned long c_musickey =        0x24240011; /* key                  */
const unsigned long c_musicscale =      0x24240012; /* scale                */
const unsigned long c_backsequence =    0x24240013; /* background sequence  */

/**
 *  Provides a fairly common type definition for a byte value.
 */

typedef unsigned char midibyte;

/**
 *    This class is the abstract base class for a container of MIDI track
 *    information.
 */

class midi_container
{

private:

    /**
     *  Provide a hook into a sequence so that we can exchange data with a
     *  sequence object.
     */

    sequence & m_sequence;

    /**
     *  Provides the position in the container when making a series of get()
     *  calls on the container.
     */

    mutable unsigned int m_position_for_get;

public:

    midi_container (sequence & seq);

    /**
     *  A rote constructor needed for a base class.
     */

    virtual ~midi_container()
    {
        // empty body
    }

    void fill (int tracknumber);

    /**
     *  Returns the size of the container, in midibytes.
     */

    virtual std::size_t size () const
    {
        return 0;
    }

    /**
     *  Instead of checking for the size of the container when "emptying" it
     *  [see the midifile::write() function], use this function, which is
     *  overridden to match the type of container being used.
     */

    virtual bool done () const
    {
        return true;
    }

    /**
     *  Provides a way to add a MIDI byte into the container.
     *  The original seq24 container used an std::list and a push_front
     *  operation.
     */

    virtual void put (midibyte b) = 0;

    /**
     *  Provide a way to get the next byte from the container.
     *  It also increments m_position_for_get.
     */

    virtual midibyte get () = 0;

protected:

    unsigned int position_reset () const
    {
        m_position_for_get = 0;
        return m_position_for_get;
    }

    /**
     *  Returns the current position.  Before the return, the position counter
     *  is incremented to the next position.
     */

    unsigned int position () const
    {
        return m_position_for_get;
    }

    void position_increment () const
    {
        ++m_position_for_get;
    }

private:

    void add_variable (long v);
    void add_long (long x);

};

}           // namespace seq64

#endif      // SEQ64_MIDI_CONTAINER_HPP

/*
 * midi_container.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

