#ifndef SEQ64_MASTERMIDIBUS_AM_HPP
#define SEQ64_MASTERMIDIBUS_AM_HPP

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
 * \file          mastermidibus_am.hpp
 *
 *  This module declares/defines the Master MIDI Bus.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-30
 * \updates       2017-03-21
 * \license       GNU GPLv2 or above
 *
 *  The mastermidibus module is the Linux version of the mastermidibus module.
 *  There's almost enough commonality to be worth creating a base class
 *  for both classes, and it might be nice to put the mastermidibus
 *  classes into their own modules.  This module is the latter.
 */

#include <vector>                       /* for channel-filtered recording   */

#include "mastermidibase.hpp"           /* seq64::mastermidibase ABC    */

#if SEQ64_HAVE_LIBASOUND                /* covers this whole module         */

#include <alsa/asoundlib.h>
#include <alsa/seq_midi_event.h>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class event;
    class midibus;
    class sequence;

/**
 *  The class that "supervises" all of the midibus objects.
 */

class mastermidibus : public mastermidibase
{

private:

    /**
     *  The ALSA sequencer client handle.
     */

    snd_seq_t * m_alsa_seq;

    /**
     *  The number of descriptors for polling.
     */

    int m_num_poll_descriptors;

    /**
     *  Points to the list of descriptors for polling.
     */

    struct pollfd * m_poll_descriptors;

public:

    mastermidibus
    (
        int ppqn    = SEQ64_USE_DEFAULT_PPQN,
        midibpm bpm = SEQ64_DEFAULT_BPM         /* c_beats_per_minute */
    );
    virtual ~mastermidibus ();

private:

    virtual bool api_get_midi_event (event * in);
    virtual int api_poll_for_midi ();
    virtual void api_init (int ppqn, midibpm bpm);
    virtual void api_set_ppqn (int ppqn);
    virtual void api_set_beats_per_minute (midibpm bpm);
    virtual void api_flush ();
    virtual void api_start ();
    virtual void api_stop ();
    virtual void api_continue_from (midipulse tick);
    virtual void api_port_start (int client, int port);

    /*
     * Not implemented:
     *
     *  api_init_clock()
     *  api_clock()
     *  api_port_exit (int client, int port)
     */

};          // class mastermidibus (ALSA version)

#endif      // SEQ64_HAVE_LIBASOUND

}           // namespace seq64

#endif      // SEQ64_MASTERMIDIBUS_AM_HPP

/*
 * mastermidibus_am.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

