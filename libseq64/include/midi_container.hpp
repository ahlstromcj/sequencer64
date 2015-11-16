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
 * \updates       2015-11-15
 * \license       GNU GPLv2 or above
 *
 */

#include <cstddef>                      /* std::size_t  */

/**
 *  This macro is used for detecting SeqSpec data that Sequencer64 does not
 *  handle.  If this word is found, then we simply extract the expected number
 *  of characters specified by that construct, and skip them when parsing a
 *  MIDI file.
 */

#define SEQ64_PROPTAG_HIGHWORD          0x24240000

/**
 *  An easier, shorter test for the SEQ64_PROPTAG_HIGHWORD part of a long
 *  value, that clients can use.
 */

#define SEQ64_IS_PROPTAG(p) \
    (((p) & SEQ64_PROPTAG_HIGHWORD) == SEQ64_PROPTAG_HIGHWORD)

/**
 *  The maximum sequence number, in macro form.
 */

/**
 *  Inidcates that no sequence value has been assigned yet.  See the value
 *  seqedit::m_initial_sequence, which was originally set to -1 directly.
 *  However, we have issues saving a negative number, so we will use the
 *  "proprietary" track's bogus sequence number, which double the 1024
 *  sequences we can support.
 */

#define SEQ64_NULL_SEQUENCE             0x0800          /* 2048 */

/**
 *  A convenient macro function to test against SEQ64_NULL_SEQUENCE.
 *  This macro allows SEQ64_NULL_SEQUENCE as a legal value to use.
 */

#define SEQ64_IS_LEGAL_SEQUENCE(s)      ((s) <= SEQ64_NULL_SEQUENCE)

/**
 *  A convenient macro function to test against SEQ64_NULL_SEQUENCE.
 *  This macro does not all SEQ64_NULL_SEQUENCE as a valid value to use.
 */

#define SEQ64_IS_VALID_SEQUENCE(s)      ((s) < SEQ64_NULL_SEQUENCE)

namespace seq64
{

class sequence;

/**
 *  Provides tags used by the midifile class to control the reading and
 *  writing of the extra "proprietary" information stored in a Seq24 MIDI
 *  file.  Some of the information is stored with each track (and in the
 *  midi_container-derived classes), and some is stored in the proprietary
 *  header.
 *
 *  Track (sequencer-specific) data:
 *
 *      -   c_midibus
 *      -   c_midich
 *      -   c_timesig
 *      -   c_triggers
 *      -   c_triggers_new
 *      -   c_musickey
 *      -   c_musicscale
 *
 * Footer ("proprietary") data:
 *
 *      -   c_midictrl
 *      -   c_midiclocks
 *      -   c_notes
 *      -   c_bpmtag (beats per minute)
 *      -   c_mutegroups
 *      -   c_backsequence
 *
 *  Also see the PDF file in the following project for more information about
 *  the "proprietary" data:
 *
 *      https://github.com/ahlstromcj/sequencer64-doc.git
 *
 *  Note that the track data is read from the MIDI file, but not written
 *  directly to the MIDI file.  Instead, it is stored in the MIDI container as
 *  sequences are edited to used these "sequencer-specific" features.
 *  Also note that c_triggers has been replaced by c_triggers_new as the code
 *  that marks the triggers stored with a sequence.
 *
 *  As an extension, we will eventually grab the last key, scale, and
 *  background sequence value selected in a sequence and write them as track
 *  data, where they can be read in and applied to a specific sequence, when
 *  the seqedit object is created.  These values would not be stored in the
 *  legacy format.
 *
 *  Something like this could be done in the "user" configuration file, but
 *  then the key and scale would apply to all songs.  We don't want that.
 *
 *  We could also add snap and note-length to the per-song defaults, but
 *  the "user" configuration file seems like a better place to store these
 *  preferences.
 */

const unsigned long c_midibus =         0x24240001; /* track buss number    */
const unsigned long c_midich =          0x24240002; /* track channel number */
const unsigned long c_midiclocks =      0x24240003; /* track clocking       */
const unsigned long c_triggers =        0x24240004; /* see c_triggers_new   */
const unsigned long c_notes =           0x24240005; /* song ??? data        */
const unsigned long c_timesig =         0x24240006; /* track time signature */
const unsigned long c_bpmtag =          0x24240007; /* song beats/minute    */
const unsigned long c_triggers_new =    0x24240008; /* track trigger data   */
const unsigned long c_mutegroups =      0x24240009; /* song mute group data */
const unsigned long c_midictrl =        0x24240010; /* song MIDI control    */
const unsigned long c_musickey =        0x24240011; /* track key            */
const unsigned long c_musicscale =      0x24240012; /* track scale          */
const unsigned long c_backsequence =    0x24240013; /* track b'ground seq   */

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
     *  Provides a way to add a MIDI byte into the container.  The original
     *  seq24 container used an std::list and a push_front operation.
     */

    virtual void put (midibyte b) = 0;

    /**
     *  Provide a way to get the next byte from the container.  It also
     *  increments m_position_for_get.
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

