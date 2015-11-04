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
 *  This module declares the abstract base class for configuration and
 *  options files.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2015-11-04
 * \license       GNU GPLv2 or above
 *
 */

#include "globals.h"                    /* c_timesig and other flags    */
#include "midi_container.hpp"           /* seq64::midi_container ABC    */
#include "sequence.hpp"                 /* seq64::sequence ABC          */

namespace seq64
{

/**
 *    Fills in the few members of this class.
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
 *  shifts it right 7, and, if there are still set bits, it encodes it
 *  into the buffer in reverse order.
 *
 *  This function "replaces" sequence::add_list_var().
 */

void
midi_container::add_variable (long v)
{
    long buffer = v & 0x7F;                     /* mask off a no-sign byte  */
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
 *  What is the difference between this function and add_list_var()?
 *
 *  This function "replaces" sequence::add_long_list().
 *
 *  This was a <i> global </i> internal function called addLongList().
 *  Let's at least make it a private member now, and hew to the naming
 *  conventions of this class.
 */

void
midi_container::add_long (long x)
{
    put((x & 0xFF000000) >> 24);
    put((x & 0x00FF0000) >> 16);
    put((x & 0x0000FF00) >> 8);
    put((x & 0x000000FF));
}

/**
 *  This function fills the given character list with MIDI data from the
 *  current sequence, preparatory to writing it to a file.
 *
 *  Note that some of the events might not come out in the same order they
 *  were stored in (we see that with program-change events.
 *
 *  This function replaces sequence::fill_container().
 *
 *  Now, for sequence 0, an alternate format for writing the sequencer number
 *  chunk is "FF 00 00".  But that format can only occur in the first track,
 *  and the rest of the tracks then don't need a sequence number, since it is
 *  assume to increment.  This application doesn't bother with that shortcut.
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
    add_variable(0);                                 /* sequence number  */
    put(0xFF);
    put(0x00);
    put(0x02);
    put((tracknumber & 0xFF00) >> 8);
    put(tracknumber & 0x00FF);
    add_variable(0);                                 /* track name       */
    put(0xFF);
    put(0x03);

    const std::string & trackname = m_sequence.name();
    int len =  trackname.length();
    if (len > 0x7F)
        len = 0x7f;

    put(midibyte(len));
    for (int i = 0; i < len; i++)
        put(midibyte(trackname[i]));

    long timestamp = 0;
    long deltatime = 0;
    long prevtimestamp = 0;
    event_list evl = m_sequence.events();
    for (event_list::iterator i = evl.begin(); i != evl.end(); i++)
    {
        event & er = DREF(i);
        const event & e = er;
        timestamp = e.get_timestamp();
        deltatime = timestamp - prevtimestamp;
        prevtimestamp = timestamp;
        add_variable(deltatime);                    /* encode delta_time    */

        unsigned char d0 = e.data(0);               /* encode status & data */
        unsigned char d1 = e.data(1);
        put(e.status() | m_sequence.get_midi_channel());    /* add channel  */
        switch (e.status() & 0xF0)
        {
        case 0x80:
        case 0x90:
        case 0xA0:
        case 0xB0:
        case 0xE0:
            put(d0);
            put(d1);
            break;

        case 0xC0:
        case 0xD0:
            put(d0);
            break;

        default:
            break;
        }
    }

    triggers::List & triggerlist = m_sequence.triggerlist();
    int triggercount = int(triggerlist.size());
    add_variable(0);
    put(0xFF);
    put(0x7F);
    add_variable((triggercount * 3 * 4) + 4);       // ???????
    add_long(c_triggers_new);
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
    add_variable(0);                                /* bus              */
    put(0xFF);
    put(0x7F);
    put(0x05);
    add_long(c_midibus);
    put(m_sequence.get_midi_bus());
    add_variable(0);                                /* timesig          */
    put(0xFF);
    put(0x7F);
    put(0x06);
    add_long(c_timesig);
    put(m_sequence.get_beats_per_bar());
    put(m_sequence.get_beat_width());
    add_variable(0);                                /* channel          */
    put(0xFF);
    put(0x7F);
    put(0x05);
    add_long(c_midich);
    put(m_sequence.get_midi_channel());
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

