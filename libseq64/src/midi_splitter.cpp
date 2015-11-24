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
 * \file          midi_splitter.cpp
 *
 *  This module declares/defines the class for splitting a MIDI track based on
 *  channel number.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-24
 * \updates       2015-11-24
 * \license       GNU GPLv2 or above
 *
 */

#include <fstream>

#include "event_list.hpp"               /* seq64::event_list            */
#include "perform.hpp"                  /* seq64::perform               */
#include "midi_splitter.hpp"            /* seq64::midi_splitter         */
#include "sequence.hpp"                 /* seq64::sequence              */

namespace seq64
{

/**
 *  Principal constructor.
 *
 * \param name
 *      Provides the name of the MIDI file to be read or written.
 *
 * \param ppqn
 *      Provides the initial value of the PPQN setting.  It is handled
 *      differently for parsing (reading) versus writing the MIDI file.
 *      -   Reading.
 *          -   If set to SEQ64_USE_DEFAULT_PPQN, the legacy application
 *              behavior is used.  The m_ppqn member is set to the default
 *              PPQN, DEFAULT_PPQN.  The value read from the MIDI
 *              file, ppqn, is then use to scale the running-time of the
 *              sequence relative to DEFAULT_PPQN.
 *          -   Otherwise, m_ppqn is set to the value read from the MIDI file.
 *              No scaling is done.  Since the value gets written, specify
 *              ppqn as 0, an obviously bogus value, to get this behavior.
 *      -   Writing.  This value is written to the MIDI file in the header
 *          chunk of the song.  Note that the caller must query for the
 *          PPQN set during parsing, and pass it to the constructor when
 *          preparing to write the file.  See how it is done in the mainwnd
 *          class.
 *
 * \param oldformat
 *      If true, write out the MIDI file using the old Seq24 format, instead
 *      of the new MIDI-compliant sequencer-specific format, for the
 *      seq24-specific SeqSpec tags defined in the globals module.  This
 *      option is false by default.  Note that this option is only used in
 *      writing; reading can handle either format transparently.
 *
 * \param globalbgs
 *      If true, write any non-default values of the key, scale, and
 *      background sequence to the global "proprietary" section of the MIDI
 *      file, instead of to each sequence.  Note that this option is only used
 *      in writing; reading can handle either format transparently.
 */

midi_splitter::midi_splitter (int ppqn)
 :
    m_ppqn                  (0),
    m_use_default_ppqn      (ppqn == SEQ64_USE_DEFAULT_PPQN),
    m_smf0_channels_count   (0),
    m_smf0_channels         (),         /* array, initialized in parse()    */
    m_smf0_main_sequence    (nullptr),
    m_smf0_seq_number       (-1),
    m_smf0_map              ()          /* map, filled in parse_smf_0()     */
{
    m_ppqn = choose_ppqn(ppqn);
    initialize();
}

/**
 *  A rote destructor.
 */

midi_splitter::~midi_splitter ()
{
    // empty body
}

/**
 *  Resets the SMF 0 support variables in preparation for parsing a new MIDI
 *  file.
 */

void
midi_splitter::initialize ()
{
    m_smf0_channels_count = 0;
    for (int i = 0; i < 16; i++)
        m_smf0_channels[i] = false;
}

/**
 *  Processes a channel number by raising its flag in the m_smf0_channels[]
 *  array.  If it is the first entry for that channel, m_smf0_channels_count
 *  is incremented.  We won't check the channel number, to save time,
 *  until someday we segfault :-D
 */

void
midi_splitter::increment (int channel)
{
    if (! m_smf0_channels[channel])  /* channel not yet logged?      */
    {
        m_smf0_channels[channel] = true;
        ++m_smf0_channels_count;
    }
}

/**
 *  Logs the main sequence (an SMF 0 track) for later usage in splitting the
 *  track.
 */

bool
midi_splitter::log_main_sequence (sequence & seq, int seqnum)
{
    bool result;
    if (is_nullptr(m_smf0_main_sequence))
    {
        m_smf0_main_sequence = &seq;
        m_smf0_seq_number = seqnum;
        infoprint("SMF 0 sequence logged");
        result = true;
    }
    else
    {
        errprint("SMF 0 sequence already logged");
        result = false;
    }
    return result;
}

/**
 *  This function splits an SMF 0, splitting all of the channels in the
 *  sequence out into separate sequences, and adding each to the perform
 *  object.  Lastly, it adds the SMF 0 track as the last track; the user can
 *  then examine it before removing it.  Is this worth the effort?
 *
 * \param p
 *      Provides a reference to the perform object into which sequences/tracks
 *      are to be added.
 *
 * \param screenset
 *      The screen-set offset to be used when loading a sequence (track) from
 *      the file.
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
midi_splitter::split (perform & p, int screenset)
{
    bool result = not_nullptr(m_smf0_main_sequence);
    if (result)
    {
        if (m_smf0_channels_count > 1)
        {
            int seqnum = screenset * c_seqs_in_set;
            for (int channel = 0; channel < 16; channel++, seqnum++)
            {
                if (m_smf0_channels[channel])
                {
                    sequence * s = new sequence(m_ppqn);
                    sequence & seq = *s;                /* references nicer */
                    seq.set_master_midi_bus(&p.master_bus());
                    if (split_channel(*m_smf0_main_sequence, seq, channel))
                        p.add_sequence(s, seqnum);
                }
            }
            p.add_sequence(m_smf0_main_sequence, seqnum);
        }
    }
    else
    {
        errprint("split(): No SMF 0 main sequence logged.");
    }
    return result;
}

/**
 *  This function splits the given sequence into new sequences, one for each
 *  channel found in the SMF 0 track.
 *
 *  It doesn't set the sequence number of the sequence; that is set when the
 *  sequence is added to the perform object.
 */

bool
midi_splitter::split_channel
(
    const sequence & main_seq,
    sequence & seq,
    int channel
)
{
    bool result = false;
    char temp[24];
    snprintf(temp, sizeof temp, "%d: %.13s", channel, main_seq.name().c_str());
    seq.set_name(std::string(temp));
    seq.set_midi_channel(channel);
    seq.set_midi_bus(main_seq.get_midi_bus());
    seq.zero_markers();

    long length_in_ticks = 0;                       // HOW TO OBTAIN?
    const event_list & evl = main_seq.events();
    for (event_list::const_iterator i = evl.begin(); i != evl.end(); i++)
    {
        const event & er = DREF(i);
        if (er.check_channel(channel))
        {
            seq.add_event(er);
        }
    }


    seq.set_length(length_in_ticks);

    return result;
}

}           // namespace seq64

/*
 * midi_splitter.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

