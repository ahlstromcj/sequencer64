#ifndef SEQ64_MIDI_LIST_HPP
#define SEQ64_MIDI_LIST_HPP

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
 * \file          midi_list.hpp
 *
 *  This module declares/defines the concrete class for a container of MIDI
 *  data.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2015-10-10
 * \license       GNU GPLv2 or above
 *
 */

#include <list>                         /* std::list<>                  */

#include "midi_container.hpp"           /* seq64::midi_container ABC    */

namespace seq64
{

/**
 *    This class is the abstract base class for optionsfile and userfile.
 */

class midi_list : public midi_container
{

private:

    /**
     *  Provides the type of this container.  This type is basically the same
     *  as the container used in the midifile module, and almost identical to
     *  the CharList type defined in the sequence module.
     */

    typedef std::list<midibyte> CharList;

    /**
     *  The container itself.
     */

    CharList m_char_list;

public:

    midi_list ();

    /**
     *  A rote constructor needed for a base class.
     */

    virtual ~midi_list()
    {
        // empty body
    }

    /**
     *  Returns the size of the container, in midibytes.
     */

    virtual std::size_t size () const
    {
        return m_char_list.size();
    }

    /**
     *  Provides a way to add a MIDI byte into the list.
     *  The original seq24 list used an std::list and a push_front
     *  operation.
     */

    virtual void put (midibyte b)
    {
        m_char_list.push_front(b);
    }

    /**
     *  Provide a way to get the next byte from the container.
     *  In this implement, m_position_for_get is not used.  The elements of
     *  the container are popped of backward!
     */

    virtual midibyte get ()
    {
        midibyte result = m_char_list.back();
        m_char_list.pop_back();
        return result;
    }

};

}           // namespace seq64

#endif      // SEQ64_MIDI_LIST_HPP

/*
 * midi_list.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
