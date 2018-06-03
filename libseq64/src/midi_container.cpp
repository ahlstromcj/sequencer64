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
 * \updates       2018-05-31
 * \license       GNU GPLv2 or above
 *
 *  This class is important when writing the MIDI and sequencer data out to a
 *  MIDI file.  The data handled here are specific to a single
 *  sequence/pattern/track.
 */

#include "globals.h"                    /* c_timesig and other flags        */
#include "calculations.hpp"             /* log2_time_sig_value(), etc.      */
#include "midi_container.hpp"           /* seq64::midi_container ABC        */
#include "perform.hpp"                  /* seq64::perform master class      */
#include "sequence.hpp"                 /* seq64::sequence                  */
#include "settings.hpp"                 /* seq64::rc() and choose_ppqn()    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

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
 *  Adds an event to the container.  It handles regular MIDI events separately
 *  from "extended" (our term) MIDI events (SysEx and Meta events).
 *
 *  For normal MIDI events, if the sequence's MIDI channel is
 *  EVENT_NULL_CHANNEL == 0xFF, then it is the copy of an SMF 0 sequence that
 *  the midi_splitter created.  We want to be able to save it along with the
 *  other tracks, but won't be able to read it back if all the channels are
 *  bad.  So we just use the channel from the event.
 *
 *  SysEx and Meta events are detected and passed to the new add_ex_event()
 *  function for proper dumping.
 *
 * \param e
 *      Provides the event to be added to the container.
 *
 * \param deltatime
 *      Provides the time-location of the event.
 */

void
midi_container::add_event (const event & e, midipulse deltatime)
{
    if (e.is_ex_data())
    {
        add_ex_event(e, deltatime);
    }
    else
    {
        midibyte d0 = e.data(0);                    /* encode status & data */
        midibyte d1 = e.data(1);
        midibyte channel = m_sequence.get_midi_channel();
        midibyte st = e.get_status();
        add_variable(deltatime);                    /* encode delta_time    */
        if (channel == EVENT_NULL_CHANNEL)
            put(st | e.get_channel());              /* channel from event   */
        else
            put(st | channel);                      /* the sequence channel */

        switch (st & EVENT_CLEAR_CHAN_MASK)                     /* 0xF0 */
        {
        case EVENT_NOTE_OFF:                                    /* 0x80 */
        case EVENT_NOTE_ON:                                     /* 0x90 */
        case EVENT_AFTERTOUCH:                                  /* 0xA0 */
        case EVENT_CONTROL_CHANGE:                              /* 0xB0 */
        case EVENT_PITCH_WHEEL:                                 /* 0xE0 */
            put(d0);
            put(d1);
            break;

        case EVENT_PROGRAM_CHANGE:                              /* 0xC0 */
        case EVENT_CHANNEL_PRESSURE:                            /* 0xD0 */
            put(d0);
            break;

        default:
            break;
        }
    }
}

/**
 *  Adds the bytes of a SysEx or Meta MIDI event.
 *
 * \param e
 *      Provides the MIDI event to add.  The caller must ensure that this is
 *      either SysEx or Meta event, using the event::is_ex_data() function.
 *
 * \param deltatime
 *      Provides the time of the event, which is encoded into the event.
 */

void
midi_container::add_ex_event (const event & e, midipulse deltatime)
{
    add_variable(deltatime);                    /* encode delta_time        */
    put(e.get_status());                        /* indicates SysEx/Meta     */
    if (e.is_meta())
        put(e.get_channel());                   /* indicates meta type      */

    int count = e.get_sysex_size();             /* applies for meta, too    */
    put(count);
    for (int i = 0; i < count; ++i)
        put(e.get_sysex()[i]);
}

/**
 *  Fills in the sequence number.  Writes 0xFF 0x00 0x02 ss ss, where ss ss is
 *  the variable-length value for the sequence number.  This function is used
 *  in the new midifile::write_song() function, which should be ready to go by
 *  the time you're reading this.  Compare this function to the beginning of
 *  midi_container::fill().
 *
 * \warning
 *      This is an optional event, which must occur only at the start of a
 *      track, before any non-zero delta-time.  For Format 2 MIDI files, this
 *      is used to identify each track. If omitted, the sequences are numbered
 *      sequentially in the order the tracks appear.  For Format 1 files, this
 *      event should occur on the first track only.  So, are we writing a
 *      hybrid format?
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

#ifdef USE_FILL_TIME_SIG_AND_TEMPO

/**
 *  Combines the two functions fill_tempo() and fill_time_signature().  This
 *  function is called only for track 0.  And it only puts out the events if
 *  the track does not contain tempo or time-signature events; in that case,
 *  it needs to grab the global values from the performance object and put
 *  them out.
 *
 * \param p
 *      The performance object that holds the time signature and tempo values.
 *
 * \param has_time_sig
 *      Indicates whether or not the current track (usually track 0) has a
 *      time signature event.  If so, then we do not need to fill in the
 *      global time signature value.
 *
 * \param has_tempo
 *      Indicates whether or not the current track (track 0) has a tempo
 *      event.  If so, then we do not need to fill in the global tempo value.
 */

void
midi_container::fill_time_sig_and_tempo
(
    const perform & p,
    bool has_time_sig,
    bool has_tempo
)
{
    if (! has_tempo)
        fill_tempo(p);

    if (! has_time_sig)
        fill_time_sig(p);
}


/**
 *  Fill in the time-signature information.  This function is used only for
 *  the first track, and only if no such event is in the track data.
 *
 *  We now make sure that the proper values are part of the perform object for
 *  usage in this particular track.  For export, we cannot guarantee that the
 *  first (0th) track/sequence is exportable.
 *
 * \param p
 *      Provides the performance object from which we get some global MIDI
 *      parameters.
 */

void
midi_container::fill_time_sig (const perform & p)
{
    int beatwidth = p.get_beat_width();
    int bpb = p.get_beats_per_bar();;
    int cpm = p.clocks_per_metronome();
    int get32pq = p.get_32nds_per_quarter();
    int bw = log2_time_sig_value(beatwidth);
    add_variable(0);                    /* delta time                   */
    put(0xFF);                          /* EVENT_MIDI_META              */
    put(0x58);                          /* EVENT_MIDI_TIME_SIGNATURE    */
    put(0x04);                          /* data length                  */
    put(bpb);
    put(bw);
    put(cpm);
    put(get32pq);
}

/**
 *  Fill in the tempo information.  This function is used only for the first
 *  track, and only if no such event is int the track data.
 *
 *  We now make sure that the proper values are part of the perform object for
 *  usage in this particular track.  For export, we cannot guarantee that the
 *  first (0th) track/sequence is exportable.
 *
 * \change ca 2017-08-15
 *      Fixed issue #103, was writing tempo bytes in the wrong order here.
 *      Accidentally committed along with fruity changes, sigh, so go back a
 *      couple of commits to see the changes.
 *
 * \param p
 *      Provides the performance object from which we get some global MIDI
 *      parameters.
 */

void
midi_container::fill_tempo (const perform & p)
{
    midibyte t[4];                              /* hold tempo bytes */
    int usperqn = p.us_per_quarter_note();
    tempo_us_to_bytes(t, usperqn);
    add_variable(0);                            /* delta time       */
    put(0xFF);                                  /* meta event       */
    put(0x51);                                  /* tempo event      */
    put(0x03);                                  /* data length      */
    put(t[0]);                                  /* NOT 2, 1, 0!     */
    put(t[1]);
    put(t[2]);
}

#endif  // USE_FILL_TIME_SIG_AND_TEMPO

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
        add_variable(0);                            /* no delta time    */
        put(0xFF);
        put(0x7F);
        put(0x05);                                  /* long + midibyte  */
        add_long(c_transpose);
        put(transpose);                             /* a boolean byte   */
        if (m_sequence.color() != SEQ64_COLOR_NONE)
        {
            add_variable(0);                            /* key selection dt */
            put(0xFF);
            put(0x7F);
            put(0x05);                                  /* long + colorbyte */
            add_long(c_seq_color);
            put(colorbyte(m_sequence.color()));
        }

#endif  // SEQ64_STAZED_TRANSPOSE

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
    midipulse trig_offset = trig.offset() % len;
    midipulse start_offset = trig.tick_start() % len;
    midipulse timestamp_adjust = trig.tick_start() + trig_offset - start_offset;
    int note_is_used[c_midi_notes];
    for (int i = 0; i < c_midi_notes; ++i)
        note_is_used[i] = 0;                        /* initialize to off */

    /*
     * This calculation needs investigation.  The number of times the pattern
     * is played is given by how many pattern lengths fit in the trigger
     * length.   But the commented calculation adds to the value of 1 already
     * assigned.  And what about triggers that are somehow of 0 size?  Let's
     * try a different calculation, currently the same.
     *
     * int times_played = 1;
     * times_played += (trig.tick_end() - trig.tick_start()) / len;
     */

    int times_played = 1 + (trig.length() - 1) / len;
    if (trig_offset > start_offset)                 /* offset len too far   */
        timestamp_adjust -= len;

    for (int p = 0; p <= times_played; ++p)
    {
        midipulse delta_time = 0;
        event_list::Events::iterator i;
        for (i = m_sequence.events().begin(); i != m_sequence.events().end(); ++i)
        {
            const event & e = DREF(i);
            midipulse timestamp = e.get_timestamp() + timestamp_adjust;
            if (timestamp >= trig.tick_start())     /* at/after trigger     */
            {
                /*
                 * Save the note; eliminate Note Off if Note On is unused.
                 */

                midibyte note = e.get_note();
                if (e.is_note_on())
                {
                    if (timestamp <= trig.tick_end())
                        note_is_used[note]++;       /* count the note       */
                    else
                        continue;                   /* skip                 */
                }
                else if (e.is_note_off())
                {
                    if (note_is_used[note] > 0)
                    {
                        /*
                         * We have a Note On, and if past the end of trigger,
                         * use the trigger end.
                         */

                        note_is_used[note]--;       /* turn off the note    */
                        if (timestamp > trig.tick_end())
                            timestamp = trig.tick_end();
                    }
                    else
                        continue;                   /* if no Note On, skip  */
                }
            }
            else
                continue;                           /* before trigger, skip */

            /*
             * If the event is past the trigger end, for non-notes, skip.
             */

            if (timestamp >= trig.tick_end())       /* event past trigger   */
            {
                if (! e.is_note_on() && ! e.is_note_off())
                    continue;                       /* drop the event       */
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
    add_variable(0);                            /* no delta time            */
    put(0xFF);                                  /* indicates a meta event   */
    put(0x7F);                                  /* sequencer-specific       */
    add_variable((num_triggers * 3 * 4) + 4);   /* 3 long values + tag      */
    add_long(c_triggers_new);                   /* Seq24 tag for triggers   */

    /*
     * Using all the trigger values seems to be the same as these values, but
     * we're basically zeroing the start and offset values to make "one big
     * trigger" for the whole pattern.
     *
     * add_long(trig.tick_start());
     * add_long(trig.tick_end());
     * add_long(trig.offset());
     */

    add_long(0);                            // the start tick
    add_long(trig.tick_end());
    add_long(0);                            // offset is done in event
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
 *  We have noticed differences in saving files in sets=4x8 versus sets=8x8,
 *  and pre-sorting the event list gets rid of some of the differences, except
 *  for the last, multi-line SeqSpec.  Some event-reordering still seems to
 *  occur, though.
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
 * Meta and SysEx Events:
 *
 *      These events can now be detected and added to the list of bytes to
 *      dump.  However, historically Seq24 has forced Time Signature and Set
 *      Tempo events to be written to the container, and has ignored these
 *      events (after the first occurrence).  So we need to figure out what to
 *      do here yet; we need to distinguish between forcing these events and
 *      them being part of the edit.
 *
 * \threadunsafe
 *      The sequence object bound to this container needs to provide the
 *      locking mechanism when calling this function.
 *
 * \param track
 *      Provides the track number, re 0.  This number is masked into the track
 *      information.
 *
 * \param p
 *      The performance object that will hold some of the parameters needed
 *      when filling the MIDI container.
 *
 * \param doseqspec
 *      If true (the default), writes out the SeqSpec information.  If false,
 *      we want to write out a regular MIDI track without this information; it
 *      writes a smaller file.
 */

void
midi_container::fill (int track, const perform & p, bool doseqspec)
{
    event_list evl = m_sequence.events();           /* used below */
    evl.sort();
    if (doseqspec)
        fill_seq_number(track);

    fill_seq_name(m_sequence.name());

    /**
     * To allow other sequencers to read Seq24/Sequencer64 files, we should
     * provide the Time Signature and Tempo meta events, in the 0th (first)
     * track (sequence).  These events must precede any "real" MIDI events.
     * They are not included if the legacy-format option is in force.
     * We also need to skip this if tempo track support is in force.
     */

    if (track == 0 && ! rc().legacy_format())
    {
#ifdef USE_FILE_TIME_SIG_AND_TEMPO
        fill_time_sig_and_tempo(p, evl.has_time_signature(), evl.has_tempo());
#endif
    }

    midipulse timestamp = 0;
    midipulse deltatime = 0;
    midipulse prevtimestamp = 0;
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

    if (doseqspec)
    {
        /*
         * Here, we add SeqSpec entries (specific to seq24) for triggers
         * (c_triggers_new), the MIDI buss (c_midibus), time signature
         * (c_timesig), and MIDI channel (c_midich).   Should we restrict this
         * to only track 0?  No; seq24 saves these events with each sequence.
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
            /*
             * Similar to the code in song_fill_seq_trigger().
             */

            add_long(ti->tick_start());
            add_long(ti->tick_end());
            add_long(ti->offset());
        }
        fill_proprietary ();
    }

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

