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
 * \file          midi_container.cpp
 *
 *  This module declares a class for holding and managing MIDI data.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2016-06-26
 * \license       GNU GPLv2 or above
 *
 *  This class is important when writing the MIDI and sequencer data out to a
 *  MIDI file.
 */

#include "globals.h"                    /* c_timesig and other flags        */
#include "calculations.hpp"             /* log2_time_sig_value(), etc.      */
#include "midi_container.hpp"           /* seq64::midi_container ABC        */
#include "sequence.hpp"                 /* seq64::sequence ABC              */
#include "settings.hpp"                 /* seq64::rc() and choose_ppqn()    */

namespace seq64
{

/**
 *  Fills in the few members of this class.
 *
 * \param seq
 *      Provides a reference to the sequence/track for which this container
 *      holds MIDI data.
 */

midi_container::midi_container (sequence & seq)
 :
    m_sequence          (seq),
    m_position_for_get  (0)
{
    // Empty body
}

/**
 *  This function masks off the lower 8 bits of the long parameter, then
 *  shifts it right 7, and, if there are still set bits, it encodes it into
 *  the buffer in reverse order.  This function "replaces"
 *  sequence::add_list_var().
 *
 * \param v
 *      The data value to be added to the current event in the MIDI container.
 */

void
midi_container::add_variable (midipulse v)
{
    midipulse buffer = v & 0x7F;                /* mask off a no-sign byte  */
    while (v >>= 7)                             /* shift right 7 bits, test */
    {
        buffer <<= 8;                           /* move LSB bits to MSB     */
        buffer |= ((v & 0x7F) | 0x80);          /* add LSB and set bit 7    */
    }
    for (;;)
    {
        put(midibyte(buffer) & 0xFF);           /* add the LSB              */
        if (buffer & 0x80)                      /* if bit 7 set             */
            buffer >>= 8;                       /* get next MSB             */
        else
            break;
    }
}

/**
 *  Adds a long value (a MIDI pulse/tick value) to the container.
 *
 *  What is the difference between this function and add_list_var()?
 *  This function "replaces" sequence::add_long_list().
 *  This was a <i> global </i> internal function called addLongList().
 *  Let's at least make it a private member now, and hew to the naming
 *  conventions of this class.
 *
 * \param x
 *      Provides the timestamp (pulse value) to be added to the container.
 */

void
midi_container::add_long (midipulse x)
{
    put((x & 0xFF000000) >> 24);
    put((x & 0x00FF0000) >> 16);
    put((x & 0x0000FF00) >> 8);
    put((x & 0x000000FF));
}

/**
 *  This function fills the given track (sequence) with MIDI data from the
 *  current sequence, preparatory to writing it to a file.  Note that some of
 *  the events might not come out in the same order they were stored in (we
 *  see that with program-change events).  This function replaces
 *  sequence::fill_container().
 *
 *  Now, for sequence 0, an alternate format for writing the sequencer number
 *  chunk is "FF 00 00".  But that format can only occur in the first track,
 *  and the rest of the tracks then don't need a sequence number, since it is
 *  assume to increment.  This application doesn't use with that shortcut.
 *
 * Triggers:
 *
 *      Triggers are added by first calling add_variable(0), which is needed
 *      because why?
 *
 *      Then 0xFF 0x7F is written, followed by the length value, which is the
 *      number of triggers at 3 long integers per trigger, plus the 4-byte
 *      code for triggers, c_triggers_new = 0x24240008.
 *
 * \threadunsafe
 *      The sequence object bound to this container needs to provide the
 *      locking mechanism when calling this function.
 *
 * \param tracknumber
 *      Provides the track number.  This number is masked into the track
 *      information.
 */

void
midi_container::fill (int tracknumber)
{
    add_variable(0);                                /* sequence number  */
    put(0xFF);
    put(0x00);                                      /* 0 delta time     */
    put(0x02);
    put((tracknumber & 0xFF00) >> 8);
    put(tracknumber & 0x00FF);
    add_variable(0);                                /* track name       */
    put(0xFF);
    put(0x03);

    const std::string & trackname = m_sequence.name();
    int len = trackname.length();
    if (len > 0x7F)
        len = 0x7f;

    put(midibyte(len));
    for (int i = 0; i < len; ++i)
        put(midibyte(trackname[i]));

#ifdef SEQ64_HANDLE_TIMESIG_AND_TEMPO           /* defined in sequence.hpp */

    /**
     * To allow other sequencers to read Seq24/Sequencer64 files, we should
     * provide the Time Signature and Tempo meta events, in the 0th (first)
     * track (sequence).  These events must precede any "real" MIDI events.
     * They are not included if the legacy-format option is in force.
     */

    if (tracknumber == 0 && ! rc().legacy_format())
    {
        int bw = log2_time_sig_value(m_sequence.get_beat_width());
        midibyte t[3];                              /* hold tempo bytes */
        tempo_to_bytes(t, m_sequence.us_per_quarter_note());

        add_variable(0);                            /* delta time       */
        put(0xFF);                                  /* meta event       */
        put(0x58);                                  /* time sig event   */
        put(0x04);
        put(m_sequence.get_beats_per_bar());
        put(bw);
        put(m_sequence.clocks_per_metronome());
        put(m_sequence.get_32nds_per_quarter());

        add_variable(0);                            /* delta time       */
        put(0xFF);                                  /* meta event       */
        put(0x51);                                  /* tempo event      */
        put(0x03);
        put(t[2]);
        put(t[1]);
        put(t[0]);
    }

#endif  // SEQ64_HANDLE_TIMESIG_AND_TEMPO

    midipulse timestamp = 0;
    midipulse deltatime = 0;
    midipulse prevtimestamp = 0;
    event_list evl = m_sequence.events();
    for (event_list::iterator i = evl.begin(); i != evl.end(); ++i)
    {
        event & er = DREF(i);
        const event & e = er;
        timestamp = e.get_timestamp();
        deltatime = timestamp - prevtimestamp;
        if (deltatime < 0)                          /* midipulse == long    */
        {
            errprint("midi_container::fill(): Bad delta-time, aborting");
            break;
        }
        prevtimestamp = timestamp;
        add_variable(deltatime);                    /* encode delta_time    */

        midibyte d0 = e.data(0);                    /* encode status & data */
        midibyte d1 = e.data(1);

        /*
         * If the sequence's MIDI channel is EVENT_NULL_CHANNEL == 0xFF, then
         * it is the copy of an SMF 0 sequence that the midi_splitter created.
         * We want to be able to save it along with the other tracks, but
         * won't be able to read it back if all the channels are bad.  So we
         * just use the channel from the event.
         */

        midibyte channel = m_sequence.get_midi_channel();
        if (channel == EVENT_NULL_CHANNEL)
            put(e.get_status() | e.get_channel());  /* channel from event   */
        else
            put(e.get_status() | channel);          /* the sequence channel */

        switch (e.get_status() & EVENT_CLEAR_CHAN_MASK)         /* 0xF0     */
        {
        case EVENT_NOTE_OFF:                                    /* 0x80:    */
        case EVENT_NOTE_ON:                                     /* 0x90:    */
        case EVENT_AFTERTOUCH:                                  /* 0xA0:    */
        case EVENT_CONTROL_CHANGE:                              /* 0xB0:    */
        case EVENT_PITCH_WHEEL:                                 /* 0xE0:    */
            put(d0);
            put(d1);
            break;

        case EVENT_PROGRAM_CHANGE:                              /* 0xC0:    */
        case EVENT_CHANNEL_PRESSURE:                            /* 0xD0:    */
            put(d0);
            break;

        default:
            break;
        }
    }
    
    /*
     * Here, we add SeqSpec entries (specific to seq24) for triggers
     * (c_triggers_new), the MIDI buss (c_midibus), time signature
     * (c_timesig), and MIDI channel (c_midich).   Should we restrict this to
     * only track 0?  Probably not; seq24 saves these events with each
     * sequence.
     */

    triggers::List & triggerlist = m_sequence.triggerlist();
    int triggercount = int(triggerlist.size());
    add_variable(0);
    put(0xFF);
    put(0x7F);
    add_variable((triggercount * 3 * 4) + 4);       /* 3 long ints plus...  */
    add_long(c_triggers_new);                       /* ...the triggers code */
#ifdef SEQ64_USE_DEBUG_OUTPUT
    int counter = 0;
#endif
    for
    (
        triggers::List::iterator ti = triggerlist.begin();
        ti != triggerlist.end(); ++ti
    )
    {
        add_long(ti->tick_start());
        add_long(ti->tick_end());
        add_long(ti->offset());
#ifdef SEQ64_USE_DEBUG_OUTPUT
        printf
        (
            "Trk %2d: trigger %2d: start = %ld, end = %ld, offset = %ld\n",
            tracknumber, counter, ti->tick_start(), ti->tick_end(), ti->offset()
        );
        counter++;
#endif
    }
    add_variable(0);                                /* bus delta time   */
    put(0xFF);
    put(0x7F);
    put(0x05);
    add_long(c_midibus);
    put(m_sequence.get_midi_bus());

    add_variable(0);                                /* timesig delta t  */
    put(0xFF);
    put(0x7F);
    put(0x06);
    add_long(c_timesig);
    put(m_sequence.get_beats_per_bar());
    put(m_sequence.get_beat_width());

    add_variable(0);                                /* channel delta t  */
    put(0xFF);
    put(0x7F);
    put(0x05);
    add_long(c_midich);
    put(m_sequence.get_midi_channel());
    if (! usr().global_seq_feature() && ! rc().legacy_format())
    {
        /**
         * New feature: save more sequence-specific values, if not legacy
         * format and not saved globally.  We use a single byte for the key
         * and scale, and a long for the background sequence.  We save these
         * values only if they are different from the defaults; in most cases
         * they will have been left alone by the user.  We save per-sequence
         * values here only if the global-background-sequence feature is not
         * in force.
         */

        if (m_sequence.musical_key() != SEQ64_KEY_OF_C)
        {
            add_variable(0);                        /* key selection dt */
            put(0xFF);
            put(0x7F);
            put(0x05);                              /* long + midibyte  */
            add_long(c_musickey);
            put(m_sequence.musical_key());
        }
        if (m_sequence.musical_scale() != int(c_scale_off))
        {
            add_variable(0);                        /* scale selection  */
            put(0xFF);
            put(0x7F);
            put(0x05);                              /* long + midibyte  */
            add_long(c_musicscale);
            put(m_sequence.musical_scale());
        }
        if (SEQ64_IS_VALID_SEQUENCE(m_sequence.background_sequence()))
        {
            add_variable(0);                        /* b'ground seq.    */
            put(0xFF);
            put(0x7F);
            put(0x08);                              /* two long values  */
            add_long(c_backsequence);
            add_long(m_sequence.background_sequence()); /* put_long()?  */
        }
#ifdef SEQ64_STAZED_TRANSPOSE
        if (m_sequence.get_transposable() != 0)
        {
            add_variable(0);                        /* transposition    */
            put(0xFF);
            put(0x7F);
            put(0x05);                              /* long + midibyte  */
            add_long(c_transpose);
            put(m_sequence.get_transposable());
        }
#endif
    }

    /*
     * Last, but certainly not least, write the end-of-track meta-event.
     */

    deltatime = m_sequence.get_length() - prevtimestamp; /* meta track end */
    add_variable(deltatime);
    put(0xFF);
    put(0x2F);
    put(0x00);
}

}           // namespace seq64

/*
 * midi_container.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

