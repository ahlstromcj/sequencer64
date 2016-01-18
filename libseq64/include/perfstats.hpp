#ifndef SEQ64_PERFSTATS_HPP
#define SEQ64_PERFSTATS_HPP

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
 * \file          perfstats.hpp
 *
 *  This module declares a class for collecting statistics on a performance.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2016-01-17
 * \updates       2016-01-17
 * \license       GNU GPLv2 or above
 *
 *  This class was created to reduce the clutter in the perform output
 *  function.
 *
 */

#include "globals.h"                    /* globals, nullptr, & more         */

#ifndef PLATFORM_WINDOWS
#include <time.h>                       /* struct timespec                  */
#endif

#include "midibyte.hpp"                 /* the midipulse typedef            */

/**
 *  The size of the statics buffers.  Not sure why 100 was chosen.
 */

#define SEQ64_STATS_BUFFER_SIZE     100

namespace seq64
{

class jack_scratchpad;

/**
 *  This class supports gathering perform-object statistics, and also
 *  some accumulation variables of the perform class.
 */

class perfstats
{

private:

    /**
     *  Indicates if statistics gathering is in force.  This member ultimately
     *  comes from the rc_settings member.
     */

    bool m_using_statistics;

    /**
     *  Accumulator for ticks.
     */

    midipulse m_stats_total_tick;

    /**
     *
     */

    long m_stats_loop_index;

    /**
     *
     */

    long m_stats_min;

    /**
     *
     */

    long m_stats_max;

    /**
     *
     */

    long m_stats_avg;

    /**
     *
     */

    long m_stats_last_clock_us;

    /**
     *
     */

    long m_stats_clock_width_us;

    /**
     *
     */

    long m_stats_all[SEQ64_STATS_BUFFER_SIZE];

    /**
     *
     */

    long m_stats_clock[SEQ64_STATS_BUFFER_SIZE];

    /**
     *  Holds the last time for use in further function calls.
     */

#ifdef PLATFORM_WINDOWS
    long m_last;
#else
    struct timespec m_last;
#endif

    /**
     *  Holds the current time for use in further function calls.
     */

#ifdef PLATFORM_WINDOWS
    long m_current;
#else
    struct timespec m_current;
#endif

    /**
     *  Holds the stats_loop_start time for use in further function calls.
     */

#ifdef PLATFORM_WINDOWS
    long m_stats_loop_start;
#else
    struct timespec m_stats_loop_start;
#endif

    /**
     *  Holds the stats_loop_finish time for use in further function calls.
     */

#ifdef PLATFORM_WINDOWS
    long m_stats_loop_finish;
#else
    struct timespec m_stats_loop_finish;
#endif

    /**
     *  Holds the delta time for use in further function calls.
     */

#ifdef PLATFORM_WINDOWS
    long m_delta;
#else
    struct timespec m_delta;
#endif

    /**
     *  Holds the PPQN value for usage.
     */

    int m_ppqn;

public:

    perfstats (bool use_stats, int ppqn);
    ~perfstats ();

    /**
     * \accessor m_using_statistics
     */

    bool in_use () const
    {
        return m_using_statistics;
    }

    void init ();
    void get_last_clock ();
    void get_loop_start ();
    long get_delta_time ();
    void get_total_ticks (const jack_scratchpad & pad);
    long get_elapsed_time ();
    void sleep (long delta_us);
    void show ();
    void final_stats ();

};

}           // namespace seq64

#endif      // SEQ64_PERFSTATS_HPP

/*
 * perfstats.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

