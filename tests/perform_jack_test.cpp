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
 * \file          perform_jack_test.cpp
 *
 *  This module defines a simple JACK test application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 *  We have no plans yet to actually write this application.
 */

#include <sched.h>
#include <stdio.h>

#ifndef PLATFORM_WINDOWS
#include <time.h>
#endif

#include "perform.hpp"
#include "midibus.hpp"
#include "midifile.hpp"
#include "event.hpp"
#include "sequence.hpp"

/*
 * This section provides a main routine for testing purposes.
 */

int main ()
{
    jack_client_t * client;

    /* become a new client of the JACK server */

    if ((client = jack_client_new("transport tester")) == 0)
    {
        fprintf(stderr, "jack server not running?\n");
        return 1;
    }
    jack_on_shutdown(client, jack_shutdown, 0);
    jack_set_sync_callback(client, jack_sync_callback, NULL);
    if (jack_activate(client))
    {
        fprintf(stderr, "cannot activate client");
        return 1;
    }

    bool cond = false; /* true if we want to fail if there is already a master */
    if (jack_set_timebase_callback(client, cond, timebase, NULL) != 0)
    {
        printf("Unable to take over timebase or there is already a master.\n");
        exit(1);
    }

    jack_position_t pos;
    pos.valid = JackPositionBBT;
    pos.bar = 0;
    pos.beat = 0;
    pos.tick = 0;
    pos.beats_per_bar = time_beats_per_bar;
    pos.beat_type = time_beat_type;
    pos.ticks_per_beat = time_ticks_per_beat;
    pos.beats_per_minute = time_beats_per_minute;
    pos.bar_start_tick = 0.0;

    // jack_transport_reposition( client, &pos );

    jack_transport_start(client);

    // void jack_transport_stop (jack_client_t *client);

    int bob;
    scanf("%d", &bob);

    jack_transport_stop(client);
    jack_release_timebase(client);
    jack_client_close(client);
    return 0;
}

/*
 * perform_jack_test.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
