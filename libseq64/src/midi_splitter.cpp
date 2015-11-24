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

#include "perform.hpp"                  /* must precede midi_splitter.hpp !  */
#include "midi_splitter.hpp"            /* seq64::midi_splitter              */
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
midi_splitter::log (sequence & seq, int seqnum)
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
 *  This function parses an SMF 0 binary MIDI file as if it were an SMF 1
 *  file, then, if more than one MIDI channel was encountered in the sequence,
 *  splits all of the channels in the sequence out separate sequences, and
 *  deletes the original sequence.
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
            for (int channel = 0; channel < 16; channel++)
            {
                if (m_smf0_channels[channel])
                {
                    sequence * s = new sequence(m_ppqn);
                    sequence & seq = *s;                /* references nicer */
                    seq.set_master_midi_bus(&p.master_bus());
                    seq.set_midi_channel(channel);

                    /*
                     * Need a reference to the original sequence
                     *
                    split_channel(*m_smf0_main_sequence, seq, channel);
                    p.add_sequence(&seq, seqnum + (screenset * c_seqs_in_set));
                     *
                     */
                }
            }

            /**
             * Lastly, add the SMF 0 track as the last track; the user can
             * then examine it before removing it.  Is this worth the extra
             * members?
             */

            p.add_sequence
            (
                m_smf0_main_sequence,
                m_smf0_seq_number + (screenset * c_seqs_in_set)
            );
        }
    }
    else
    {
        errprint("No SMF 0 main sequence logged.");
    }
    return result;
}

/**
 *  This function splits the given sequence into new sequences, one for each
 *  channel found in the SMF 0 track.
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

    return result;
}

}           // namespace seq64

/*
 * midi_splitter.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

