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
 * \updates       2016-05-17
 * \license       GNU GPLv2 or above
 *
 */

#include <fstream>

#include "event_list.hpp"               /* seq64::event_list            */
#include "perform.hpp"                  /* seq64::perform               */
#include "midi_splitter.hpp"            /* seq64::midi_splitter         */
#include "sequence.hpp"                 /* seq64::sequence              */
#include "settings.hpp"                 /* seq64::choose_ppqn()         */

namespace seq64
{

/**
 *  Principal constructor.
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
 */

midi_splitter::midi_splitter (int ppqn)
 :
    m_ppqn                  (0),
    m_use_default_ppqn      (ppqn == SEQ64_USE_DEFAULT_PPQN),
    m_smf0_channels_count   (0),
    m_smf0_channels         (),         /* array, initialized in parse()    */
    m_smf0_main_sequence    (nullptr),
    m_smf0_seq_number       (-1)
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
    for (int i = 0; i < SEQ64_MIDI_CHANNEL_MAX; ++i)
        m_smf0_channels[i] = false;
}

/**
 *  Processes a channel number by raising its flag in the m_smf0_channels[]
 *  array.  If it is the first entry for that channel, m_smf0_channels_count
 *  is incremented.  We won't check the channel number, to save time,
 *  until someday we segfault :-D
 *
 * \param channel
 *      The MIDI channel number.  The caller is responsible to make sure it
 *      ranges from 0 to 15.
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
 *
 * /param seq
 *      The main sequence to be logged.
 *
 * /param seqnum
 *      The sequence number of the main sequence.
 *
 * /return
 *      Returns true if the main sequence's address was logged, and false if
 *      it was already logged.
 */

bool
midi_splitter::log_main_sequence (sequence & seq, int seqnum)
{
    bool result;
    if (is_nullptr(m_smf0_main_sequence))
    {
        m_smf0_main_sequence = &seq;
        m_smf0_seq_number = seqnum;
        infoprint("SMF 0 main sequence logged");
        result = true;
    }
    else
    {
        errprint("SMF 0 main sequence already logged");
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
 *  There is a little oddity, in that, if the SMF 0 track has events for only
 *  one channel, this code will still create a new sequence, as well as the
 *  main sequence.  Not sure if this is worth extra code to just change the
 *  channels on the main sequence and put it into the correct track for the
 *  one channel it contains.  In fact, we just want to keep it in patter slot
 *  number 16, to keep it out of the way.
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
 *      Returns true if the parsing succeeded.  Returns false if no SMF 0 main
 *      sequence was logged.
 */

bool
midi_splitter::split (perform & p, int screenset)
{
    bool result = not_nullptr(m_smf0_main_sequence);
    if (result)
    {
        if (m_smf0_channels_count > 0)
        {
            int seqnum = screenset * c_seqs_in_set;
            for (int chan = 0; chan < SEQ64_MIDI_CHANNEL_MAX; ++chan, ++seqnum)
            {
                if (m_smf0_channels[chan])
                {
                    sequence * s = new sequence(m_ppqn);

                    /*
                     * The master MIDI buss must be set before the split,
                     * otherwise the null pointer causes a segfault.
                     */

                    s->set_master_midi_bus(&p.master_bus());
                    if (split_channel(*m_smf0_main_sequence, s, chan))
                    {
                        p.add_sequence(s, seqnum);
#ifdef SEQ64_USE_DEBUG_OUTPUT
                        s->show_events();
#endif
                    }
                    else
                        delete s;   /* empty sequence, not even meta events */
                }
            }
            m_smf0_main_sequence->set_midi_channel(EVENT_NULL_CHANNEL);
            p.add_sequence(m_smf0_main_sequence, seqnum);
        }
    }
    return result;
}

/**
 *  This function splits the given sequence into new sequences, one for each
 *  channel found in the SMF 0 track.
 *
 *  Note that the events that are read from the MIDI file have delta times.
 *  Sequencer64 converts these delta times to cumulative times.    We
 *  need to preserve that here.  Conversion back to delta times is needed only
 *  when saving the sequences to a file.  This is done in
 *  midi_container::fill().
 *
 *  We have to accumulate the delta times in order to be able to set the
 *  length of the sequence in pulses.
 *
 *  Luckily, we don't have to worry about copying triggers, since the imported
 *  SMF 0 track won't have any Seq24/Sequencer24 triggers.
 *
 *  It doesn't set the sequence number of the sequence; that is set when the
 *  sequence is added to the perform object.
 *
 * \param main_seq
 *      This parameter is the whole SMF 0 track that was read from the MIDI
 *      file.  It contains all of the channel data that needs to be split into
 *      separate sequences.
 *
 * \param s
 *      Provides the new sequence that needs to have its settings made, and
 *      all of the selected channel events added to it.
 *
 * \param channel
 *      Provides the MIDI channel number (re 0) that marks the channel data
 *      the needs to be extracted and added to the new sequence.
 *
 * \return
 *      Returns true if at least one event got added.   If none were added,
 *      the caller should delete the sequence object represented by parameter
 *      \a s.
 */

bool
midi_splitter::split_channel
(
    const sequence & main_seq,
    sequence * s,
    int channel
)
{
    bool result = false;
    char tmp[24];
    if (main_seq.name().empty())
    {
        snprintf(tmp, sizeof tmp, "Track %d", channel+1);
    }
    else
    {
        snprintf
        (
            tmp, sizeof tmp, "%d: %.13s", channel+1, main_seq.name().c_str()
        );
    }

    s->set_name(std::string(tmp));
    s->set_midi_channel(channel);
    s->set_midi_bus(main_seq.get_midi_bus());
    s->zero_markers();

    midipulse length_in_ticks = 0;      /* an accumulator of delta times    */
    const event_list & evl = main_seq.events();
    for (event_list::const_iterator i = evl.begin(); i != evl.end(); ++i)
    {
        const event & er = DREF(i);
        if (er.check_channel(channel))
        {
            length_in_ticks = er.get_timestamp();
            if (s->add_event(er))
                result = true;          /* an event got added               */
        }
    }

    /*
     * No triggers to add.  Whew!  And setting the length is now a no-brainer.
     */

    s->set_length(length_in_ticks);
    return result;
}

}           // namespace seq64

/*
 * midi_splitter.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

