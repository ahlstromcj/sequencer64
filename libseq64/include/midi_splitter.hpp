#ifndef SEQ64_MIDI_SPLITTER_HPP
#define SEQ64_MIDI_SPLITTER_HPP

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
 * \file          midi_splitter.hpp
 *
 *  This module declares/defines the base class for MIDI files.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-24
 * \updates       2015-11-24
 * \license       GNU GPLv2 or above
 *
 *  Sequencer64 can also split an SMF 0 file into multiple tracks, effectively
 *  converting it to SMF 1.  This class holds all the information needed to do
 *  that.
 */

#include <string>
#include <map>

#include "globals.h"                    /* SEQ64_USE_DEFAULT_PPQN   */

namespace seq64
{

class perform;                          /* forward reference        */
class sequence;                         /* forward reference        */

/**
 *  This class handles the parsing and writing of MIDI files.  In addition to
 *  the standard MIDI tracks, it also handles some "private" or "proprietary"
 *  tracks specific to Seq24.  It does not, however, handle SYSEX events.
 */

class midi_splitter
{

private:

    /**
     *  Provides SMF 0 support for breaking a multi-channel SMF 0 file into a
     *  number of sequences
     */

    typedef std::map<int, sequence *> ChannelMap;

private:

    /**
     *  Provides the current value of the PPQN, which used to be constant
     *  and is now only the macro DEFAULT_PPQN.
     */

    int m_ppqn;

    /**
     *  Indicates that the default PPQN is in force.
     */

    bool m_use_default_ppqn;

    /**
     *  Provides support for SMF 0, indicates how many channels were found in
     *  the file in a single sequence.  SMF 1 file parsing will only warn
     *  about more than one channel found in a given sequence.
     */

    int m_smf0_channels_count;

    /**
     *  Provides support for SMF 0, holds a bool value that indicates the
     *  occurrence of a given channel.  Obviously, we don't have to worry
     *  about multiple MIDI busses.
     */

    bool m_smf0_channels[16];

    /**
     *  Provides support for SMF 0, points to the initial SMF 0 sequence, from
     *  which the single-channel sequences in the ChannelMap will be created.
     */

    sequence * m_smf0_main_sequence;

    /**
     *  Provides support for SMF 0, holds the prospective sequence number of
     *  the main (SMF 0) sequence.  We want to be able to add that sequence
     *  last, for easier and cleaner removal of that sequence by the user.
     */

    int m_smf0_seq_number;

    /**
     *  Provides support for SMF 0, holds a map of sequences keyed by channel
     *  number.
     */

    ChannelMap m_smf0_map;

public:

    midi_splitter (int ppqn = SEQ64_USE_DEFAULT_PPQN);
    ~midi_splitter ();

    bool log (sequence & seq, int seqnum);
    void initialize ();
    void increment (int channel);
    bool split (perform & p, int screenset);

    /**
     * \getter m_ppqn
     *      Provides a way to get the actual value of PPQN used in processing
     *      the sequences when parse() was called.  The PPQN will be either
     *      the global ppqn (legacy behavior) or the value read from the
     *      file, depending on the ppqn parameter passed to the midi_splitter
     *      constructor.
     */

    int ppqn () const
    {
        return m_ppqn;
    }

    /**
     * \getter m_smf0_channels_count
     */

    int count () const
    {
        return m_smf0_channels_count;
    }

private:

    bool split_channel
    (
        const sequence & main_seq,
        sequence & seq,
        int channel
    );

};          // class midi_splitter

}           // namespace seq64

#endif      // SEQ64_MIDI_SPLITTER_HPP

/*
 * midi_splitter.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

