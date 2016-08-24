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
 * \updates       2016-08-24
 * \license       GNU GPLv2 or above
 *
 *  This class is important when writing the MIDI and sequencer data out to a
 *  MIDI file.
 */

#include "globals.h"                    /* c_timesig and other flags        */
#include "calculations.hpp"             /* log2_time_sig_value(), etc.      */
#include "midi_container.hpp"           /* seq64::midi_container ABC        */
#include "perform.hpp"                  /* seq64::perform master class      */
#include "sequence.hpp"                 /* seq64::sequence                  */
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
 *  Adds a short value (two bytes) to the container.
 *
 * \param x
 *      Provides the timestamp (pulse value) to be added to the container.
 */

void
midi_container::add_short (midishort x)
{
    put((x & 0x0000FF00) >> 8);
    put((x & 0x000000FF));
}

/**
 *  Adds an event to the container.  If the sequence's MIDI channel is
 *  EVENT_NULL_CHANNEL == 0xFF, then it is the copy of an SMF 0 sequence that
 *  the midi_splitter created.  We want to be able to save it along with the
 *  other tracks, but won't be able to read it back if all the channels are
 *  bad.  So we just use the channel from the event.
 */

void
midi_container::add_event (const event & e, midipulse deltatime)
{
    midibyte d0 = e.data(0);                    /* encode status & data */
    midibyte d1 = e.data(1);
    add_variable(deltatime);                    /* encode delta_time    */

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

/**
 *  Fills in the sequence number.  Writes 0xFF 0x00 0x02, and then the number.
 *  This function is used in the new midifile::write_song() function, which
 *  should be ready to go by the time you're reading this.
 *
 *  Compare this function to the beginning of midi_container::fill().
 *
 * \param seq
 *      The sequence/track number to write.
 */

void
midi_container::fill_seq_number (int seq)
{
    add_variable(0);                                /* delta time N/A   */
    put(0xFF);                                      /* meta marker      */
    put(0x00);                                      /* seq-num marker   */
    put(0x02);                                      /* length of event  */
    add_short(midishort(seq));
}

/**
 *  Fills in the sequence name.  Writes 0xFF 0x03, and then the track name.
 *  This function is used in the new midifile::write_song() function, which
 *  should be ready to go by the time you're reading this.
 *
 *  Compare this function to the beginning of midi_container::fill().
 *
 * \param name
 *      The sequence/track name to set.  We could get this item from
 *      m_sequence, but the parameter allows the flexibility to change the
 *      name.
 */

void
midi_container::fill_seq_name (const std::string & name)
{
    add_variable(0);                                /* delta time N/A   */
    put(0xFF);                                      /* meta marker      */
    put(0x03);                                      /* track name mark  */

    int len = name.length();
    if (len > SEQ64_MAX_DATA_VALUE)                 /* 0x7F, 127        */
        len = SEQ64_MAX_DATA_VALUE;

    put(midibyte(len));                             /* length of name   */
    for (int i = 0; i < len; ++i)
        put(midibyte(name[i]));
}

/*
 * Last, but certainly not least, write the end-of-track meta-event.
 *
 * \param deltatime
 *      The MIDI delta time to write before the meta-event itself.
 */

void
midi_container::fill_meta_track_end (midipulse deltatime)
{
    add_variable(deltatime);
    put(0xFF);
    put(0x2F);
    put(0x00);
}

/**
 *  Fill in the time-signature and tempo information.  This function is used
 *  only for the first track,  The sizes of these meta events are defined as
 *  SEQ64_TIME_TEMPO_SIZE.  However, we do not have to add that value in, as
 *  it is already counted in the intrinsic size of the container.
 *
 * \param p
 *      Provides the performance object from which we get some global MIDI
 *      parameters.
 */

void
midi_container::fill_time_sig_and_tempo (const perform & p)
{
    if (! rc().legacy_format())
    {
        int beatwidth = p.get_beat_width();
//      if (beatwidth == 0)
//          beatwidth = m_sequence.get_beat_width();

        int usperqn = p.us_per_quarter_note();
//      if (usperqn == 0)
//          usperqn = m_sequence.us_per_quarter_note();

        int bpb = p.get_beats_per_bar();;
//      if (bpb == 0)
//          bpb = m_sequence.get_beats_per_bar();;

        int cpm = p.clocks_per_metronome();
//      if (cpm == 0)
//          cpm = m_sequence.clocks_per_metronome();

        int get32pq = p.get_32nds_per_quarter();
//      if (get32pq == 0)
//          get32pq = m_sequence.get_32nds_per_quarter();

        int bw = log2_time_sig_value(beatwidth);
        midibyte t[4];                              /* hold tempo bytes */
        tempo_us_to_bytes(t, usperqn);

        add_variable(0);                            /* delta time       */
        put(0xFF);                                  /* meta event       */
        put(0x58);                                  /* time sig event   */
        put(0x04);
        put(bpb);
        put(bw);
        put(cpm);
        put(get32pq);

        add_variable(0);                            /* delta time       */
        put(0xFF);                                  /* meta event       */
        put(0x51);                                  /* tempo event      */
        put(0x03);
        put(t[2]);
        put(t[1]);
        put(t[0]);
    }
}

/**
 *  Fills in the Sequencer64-specific information for the current sequence:
 *  The MIDI buss number, the time-signature, and the MIDI channel.  Then, if
 *  we're not using the legacy output format, we add the "events" for the
 *  musical key, musical scale, and the background sequence for the current
 *  sequence. Finally, if tranpose support has been compiled into the program,
 *  we add that information as well.
 */

void
midi_container::fill_proprietary ()
{
    add_variable(0);                                /* bus delta time   */
    put(0xFF);                                      /* meta marker      */
    put(0x7F);                                      /* SeqSpec marker   */
    put(0x05);                                      /* event length     */
    add_long(c_midibus);                            /* Seq24 SeqSpec ID */
    put(m_sequence.get_midi_bus());                 /* MIDI buss number */

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
    if (! rc().legacy_format())
    {
        if (! usr().global_seq_feature())
        {
            /**
             * New feature: save more sequence-specific values, if not legacy
             * format and not saved globally.  We use a single byte for the
             * key and scale, and a long for the background sequence.  We save
             * these values only if they are different from the defaults; in
             * most cases they will have been left alone by the user.  We save
             * per-sequence values here only if the global-background-sequence
             * feature is not in force.
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
        }

#ifdef SEQ64_STAZED_TRANSPOSE

        /**
         *  For the new "transposable" flag (tagged by the value c_transpose)
         *  we really only care about saving the value of "false", because
         *  otherwise we can assume the value is true for the given sequence,
         *  and save space by not saving it... generally only drum patterns
         *  will not be transposable.
         *
         *  However, for now, write it anyway for consistency with Seq32.
         */

        bool transpose = m_sequence.get_transposable();
#ifdef USE_TRANSPOSE_CHECK_HERE
        if (! transpose)                                /* save only false  */
        {
#endif
            add_variable(0);                            /* transposition    */
            put(0xFF);
            put(0x7F);
            put(0x05);                                  /* long + midibyte  */
            add_long(c_transpose);
            put(transpose);                             /* a boolean byte   */
#ifdef USE_TRANSPOSE_CHECK_HERE
        }
#endif

#endif

    }
}

/**
 *  Fills in sequence events based on the trigger and events in the sequence
 *  associated with this midi_container.
 *
 * \param trig
 *      The current trigger to be processed.
 *
 * \param prev_timestamp
 *      The time-stamp of the previous event.
 *
 * \return
 *      The next time-stamp value is returned.
 */

midipulse
midi_container::song_fill_seq_event
(
   const trigger & trig,
   midipulse prev_timestamp
)
{
    midipulse len = m_sequence.get_length();
    midipulse trigger_offset = trig.offset() % len;
    midipulse start_offset = trig.tick_start() % len;
    midipulse timestamp_adjust = trig.tick_start() - start_offset + trigger_offset;
    int times_played = 1;
    int note_is_used[c_midi_notes];
    for (int i = 0; i < c_midi_notes; ++i)
        note_is_used[i] = 0;                        /* initialize to off */

    times_played += (trig.tick_end() - trig.tick_start()) / len;
    if ((trigger_offset - start_offset) > 0)    /* total offset is len too far */
        timestamp_adjust -= len;

    for (int p = 0; p <= times_played; ++p)
    {
        midipulse timestamp = 0, delta_time = 0;         /* events */
        event_list::Events::iterator i;
        for (i = m_sequence.events().begin(); i != m_sequence.events().end(); ++i)
        {
            const event & e = DREF(i);
            timestamp = e.get_timestamp();
            timestamp += timestamp_adjust;
            if (timestamp >= trig.tick_start()) /* event is after trigger start */
            {
                /*
                 * Save the note; eliminate Note Off if Note On is unused.
                 */

                midibyte note = e.get_note();
                if (e.is_note_on())
                {
                    if (timestamp > trig.tick_end())
                        continue;                   /* skip                 */
                    else
                        note_is_used[note]++;       /* count the note       */
                }
                if (e.is_note_off())
                {
                    if (note_is_used[note] <= 0)    /* if no Note On, skip  */
                    {
                        continue;
                    }
                    else                            /* we have a Note On    */
                    {
                        /*
                         * If past the end of trigger, use trigger end
                         */

                        if (timestamp >= trig.tick_end())
                        {
                            note_is_used[note]--;           // turn off note
                            timestamp = trig.tick_end();
                        }
                        else                                // not past end, use it
                            note_is_used[note]--;
                    }
                }
            }
            else
                continue;   // event is before the trigger start - skip

            /*
             * If the event is past the trigger end, for non-notes, skip.
             * What about Aftertouch events?
             */

            if (timestamp >= trig.tick_end())
            {
                if (! e.is_note_on() && ! e.is_note_off())
                    continue;           // these were already taken care of...
            }

            delta_time = timestamp - prev_timestamp;
            prev_timestamp = timestamp;
            add_event(e, delta_time);
        }
        timestamp_adjust += len;        // any side-effects on sequence length?
    }
    return prev_timestamp;
}

/**
 *  Fills in the trigger for the whole sequence.  For a song-performance,
 *  there will be only one trigger, covering the beginning to the end of the
 *  fully unlooped track.
 *
 * \param trig
 *      The current trigger to be processed.
 *
 * \param length
 *      Provides the total length of the sequence.
 *
 * \param prev_timestamp
 *      The time-stamp of the previous event, which is actually the first event.
 */

void
midi_container::song_fill_seq_trigger
(
    const trigger & trig,
    midipulse length,
    midipulse prev_timestamp
)
{
    const int num_triggers = 1;                 /* only one trigger here    */
    add_variable(0);
    put(0xFF);
    put(0x7F);
    add_variable((num_triggers * 3 * 4) + 4);
    add_long(c_triggers_new);

    /*
     * Using all the trigger values seems to be the same as these values:
     *
     *  add_long(0);                            // the start tick
     *  add_long(trig.tick_end());
     *  add_long(0);                            // offset is done in event
     */

    add_long(trig.tick_start());
    add_long(trig.tick_end());
    add_long(trig.offset());
    fill_proprietary();

    midipulse delta_time = length - prev_timestamp;
    fill_meta_track_end(delta_time);
}

/**
 *  This function fills the given track (sequence) with MIDI data from the
 *  current sequence, preparatory to writing it to a file.  Note that some of
 *  the events might not come out in the same order they were stored in (we
 *  see that with program-change events).  This function replaces
 *  sequence::fill_list().
 *
 *  Now, for sequence 0, an alternate format for writing the sequencer number
 *  chunk is "FF 00 00".  But that format can only occur in the first track,
 *  and the rest of the tracks then don't need a sequence number, since it is
 *  assumed to increment.  This application doesn't use that shortcut.
 *
 * Stazed:
 *
 *      The "stazed" (seq32) code implements a function like this one
 *      using a function sequence::fill_proprietary_list() that we
 *      don't need for our implementation... it is part of our
 *      midi_container::fill() function.
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
midi_container::fill (int tracknumber, const perform & p)
{
    fill_seq_number(tracknumber);
    fill_seq_name(m_sequence.name());

    /**
     * To allow other sequencers to read Seq24/Sequencer64 files, we should
     * provide the Time Signature and Tempo meta events, in the 0th (first)
     * track (sequence).  These events must precede any "real" MIDI events.
     * They are not included if the legacy-format option is in force.
     */

    if (tracknumber == 0)
        fill_time_sig_and_tempo(p);

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
        add_event(e, deltatime);
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
    for
    (
        triggers::List::iterator ti = triggerlist.begin();
        ti != triggerlist.end(); ++ti
    )
    {
        add_long(ti->tick_start());
        add_long(ti->tick_end());
        add_long(ti->offset());
    }
    fill_proprietary ();

    /*
     * Last, but certainly not least, write the end-of-track meta-event.
     */

    deltatime = m_sequence.get_length() - prevtimestamp; /* meta track end */
    fill_meta_track_end(deltatime);
}

}           // namespace seq64

/*
 * midi_container.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

