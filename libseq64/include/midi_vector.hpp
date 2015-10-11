#ifndef SEQ64_MIDI_VECTOR_HPP
#define SEQ64_MIDI_VECTOR_HPP

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
 * \file          midi_vector.hpp
 *
 *  This module declares/defines the concrete class for a container of MIDI
 *  data.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-10-11
 * \updates       2015-10-11
 * \license       GNU GPLv2 or above
 *
 *  This implementation attempts to avoid the reversals that can occur using
 *  the list implementation.
 */

#include <vector>                       /* std::vector<>                */

#include "midi_container.hpp"           /* seq64::midi_container ABC    */

namespace seq64
{

/**
 *    This class is the std::vector implementation of the midi_container.
 */

class midi_vector : public midi_container
{

private:

    /**
     *  Provides the type of this container.
     */

    typedef std::vector<midibyte> CharVector;

    /**
     *  The container itself.
     */

    CharVector m_char_vector;

public:

    midi_vector (sequence & seq);

    /**
     *  A rote constructor needed for a base class.
     */

    virtual ~midi_vector()
    {
        // empty body
    }

    /**
     *  Returns the size of the container, in midibytes.
     */

    virtual std::size_t size () const
    {
        return m_char_vector.size();
    }

    /**
     *  For iterating through the data in the MIDI vector, we are done when
     *  we've gotten the last element of the container.
     */

    virtual bool done () const
    {
        return position() >= size();
    }

    /**
     *  Provides a way to add a MIDI byte into the list.
     *  The original seq24 list used an std::list and a push_front
     *  operation.
     */

    virtual void put (midibyte b)
    {
        m_char_vector.push_back(b);
    }

    /**
     *  Provide a way to get the next byte from the container.
     *  In this implement, m_position_for_get is not used.  The elements of
     *  the container are popped of backward!
     */

    virtual midibyte get ()
    {
        midibyte result = m_char_vector[position()];
        position_increment();
        return result;
    }

};

}           // namespace seq64

#endif      // SEQ64_MIDI_VECTOR_HPP

/*
 * midi_vector.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
