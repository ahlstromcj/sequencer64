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
 * \file          perfstats.cpp
 *
 *  This module defines a class for collecting statistics on a performance.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2016-01-17
 * \updates       2016-01-18
 * \license       GNU GPLv2 or above
 *
 *  This class was created to reduce the clutter in the perform output
 *  function.
 */

#include <stdio.h>

#ifndef PLATFORM_WINDOWS
#include <time.h>                       /* struct timespec                  */
#endif

#include "jack_assistant.hpp"           /* optional seq64::jack_assistant   */
#include "calculations.hpp"
#include "perfstats.hpp"

namespace seq64
{

perfstats::perfstats (bool use_stats, int ppqn)
 :
    m_using_statistics      (use_stats),
    m_stats_total_tick      (0),
    m_stats_loop_index      (0),
    m_stats_min             (0x7FFFFFFF),
    m_stats_max             (0),
    m_stats_avg             (0),
    m_stats_last_clock_us   (0),
    m_stats_clock_width_us  (0),
    m_stats_all             (),             // array
    m_stats_clock           (),             // array
#ifdef PLATFORM_WINDOWS
    m_last                  (0),            // long
    m_current               (0),
    m_stats_loop_start      (0),
    m_stats_loop_finish     (0),
    m_delta                 (0),            // member necessary???
#else
    m_last                  (),             // struct timespec
    m_current               (),
    m_stats_loop_start      (),
    m_stats_loop_finish     (),
    m_delta                 (),             // member necessary???
#endif
    m_ppqn                  (choose_ppqn(ppqn))
{
#ifndef PLATFORM_WINDOWS
    m_last.tv_sec               = m_last.tv_nsec              = 0;
    m_current.tv_sec            = m_current.tv_nsec           = 0;
    m_stats_loop_start.tv_sec   = m_stats_loop_start.tv_nsec  = 0;
    m_stats_loop_finish.tv_sec  = m_stats_loop_finish.tv_nsec = 0;
    m_delta.tv_sec              = m_delta.tv_nsec             = 0;
#endif
}

/**
 *  The destructor ...
 */

perfstats::~perfstats ()
{
    // Empty body
}

/**
 *  Initializes the statistics buffers.
 */

void
perfstats::init ()
{
    if (m_using_statistics)
    {
        m_stats_total_tick = 0;
        m_stats_loop_index = 0;
        m_stats_min = 0x7FFFFFFF;
        m_stats_max = 0;
        m_stats_avg = 0;
        m_stats_last_clock_us = 0;
        m_stats_clock_width_us = 0;
        for (int i = 0; i < SEQ64_STATS_BUFFER_SIZE; ++i)
        {
            m_stats_all[i] = 0;
            m_stats_clock[i] = 0;
        }
    }
}

/**
 *  Gets the last-clock (microseconds) time.  Note that the Windows
 *  implementation is broken, as there is no timeGetTime() function defined in
 *  this source-code base.
 */

void
perfstats::get_last_clock ()
{
    if (m_using_statistics)
    {
#ifdef PLATFORM_WINDOWS
        long m_last = timeGetTime();              // get start time position
        m_stats_last_clock_us = m_last * 1000;
#else
        struct timespec m_last;
        clock_gettime(CLOCK_REALTIME, &m_last);   // get start time position
        m_stats_last_clock_us = (m_last.tv_sec * 1000000) +
            (m_last.tv_nsec / 1000);
#endif
    }
}

/**
 *  Gets/sets the loop-start time.
 */

void
perfstats::get_loop_start ()
{
    if (m_using_statistics)
    {
#ifdef PLATFORM_WINDOWS
       m_ stats_loop_start = timeGetTime();
#else
        clock_gettime(CLOCK_REALTIME, &m_stats_loop_start);
#endif
    }
}

/**
 *  Gets the delta time.  This function is about more than just statistics; it
 *  holds values needed for normal performance calculations.  Again, note that
 *  the Windows version is broken by the lack of the timeGetTime() function.
 */

long
perfstats::get_delta_time ()
{
#ifdef PLATFORM_WINDOWS
    long m_current = timeGetTime();
    long m_delta = m_current - m_last;          // or just local?
    long delta_us = m_delta * 1000;
#else
    struct timespec m_current;
    struct timespec m_delta;                    // or just local?
    clock_gettime(CLOCK_REALTIME, &m_current);
    m_delta.tv_sec  = m_current.tv_sec  - m_last.tv_sec;
    m_delta.tv_nsec = m_current.tv_nsec - m_last.tv_nsec;
    long delta_us = (m_delta.tv_sec * 1000000) + (m_delta.tv_nsec / 1000);
#endif
    return delta_us;
}

/**
 * Calculates the total ticks for.....
 */

void
perfstats::get_total_ticks (const jack_scratchpad & pad)
{
    if (m_using_statistics)
    {
        int ct = clock_ticks_from_ppqn(m_ppqn);
        while (m_stats_total_tick <= pad.js_total_tick)
        {
            /*
             * Uses inline function for c_ppqn / 24.  Checks to see
             * if there was a tick.  What's up with the constants
             * 100 and 300?
             */

//          int ct = clock_ticks_from_ppqn(m_ppqn);
            if ((m_stats_total_tick % ct) == 0)
            {

#ifndef PLATFORM_WINDOWS
                long current_us = (m_current.tv_sec * 1000000) +
                    (m_current.tv_nsec / 1000);
#else
                long current_us = m_current * 1000;
#endif
                m_stats_clock_width_us = current_us - m_stats_last_clock_us;
                m_stats_last_clock_us = current_us;

                int index = m_stats_clock_width_us / 300;
                if (index >= SEQ64_STATS_BUFFER_SIZE)
                    index = SEQ64_STATS_BUFFER_SIZE - 1;

                ++m_stats_clock[index];
            }
            ++m_stats_total_tick;
        }
    }
}

/**
 *  Calculates the elapsed time.
 *
 *  Does m_delta really need to be a member, as opposed to a local variable?
 */

long
perfstats::get_elapsed_time ()
{
    m_last = m_current;

#ifdef PLATFORM_WINDOWS
    m_current = timeGetTime();
    m_delta = m_current - m_last;
    long elapsed_us = m_delta * 1000;
#else
    clock_gettime(CLOCK_REALTIME, &m_current);
    m_delta.tv_sec  = m_current.tv_sec  - m_last.tv_sec;
    m_delta.tv_nsec = m_current.tv_nsec - m_last.tv_nsec;
    long elapsed_us = (m_delta.tv_sec * 1000000) + (m_delta.tv_nsec / 1000);
#endif

    return elapsed_us;
}

/**
 *  Implements the sleep functionality of perform.  Not really a statistical
 *  function, but we can hide the Windows functionality here.
 */

void
perfstats::sleep (long delta_us)
{
#ifdef PLATFORM_WINDOWS                    // nanosleep() is actually Linux
    if (delta_us > 0)
    {
        m_delta = delta_us / 1000;
        m_Sleep(delta);                     // local or member???
    }
#else
    if (delta_us > 0)
    {
        m_delta.tv_sec = delta_us / 1000000;
        m_delta.tv_nsec = (delta_us % 1000000) * 1000;
        nanosleep(&m_delta, NULL);
    }
#endif
    else
    {
        if (m_using_statistics)
        {
            errprint("Underrun");
        }
    }
}

/**
 * Calculate and show
 */

void
perfstats::show ()
{
    if (m_using_statistics)
    {
#ifndef PLATFORM_WINDOWS

        /*
         * Note that the subtractions here are not correct; the finish
         * nanoseconds can be less than the start nanoseconds.  LATER.
         */

        clock_gettime(CLOCK_REALTIME, &m_stats_loop_finish);
        m_delta.tv_sec =
            m_stats_loop_finish.tv_sec - m_stats_loop_start.tv_sec;

        m_delta.tv_nsec =
            m_stats_loop_finish.tv_nsec - m_stats_loop_start.tv_nsec;

        long delta_us = (m_delta.tv_sec*1000000) + (m_delta.tv_nsec/1000);
#else
        m_stats_loop_finish = timeGetTime();
        m_delta = m_stats_loop_finish - m_stats_loop_start;
        long delta_us = m_delta * 1000;
#endif

        int index = delta_us / SEQ64_STATS_BUFFER_SIZE;         // why the 100?
        if (index >= SEQ64_STATS_BUFFER_SIZE)
            index = SEQ64_STATS_BUFFER_SIZE - 1;

        m_stats_all[index]++;
        if (delta_us > m_stats_max)
            m_stats_max = delta_us;

        if (delta_us < m_stats_min)
            m_stats_min = delta_us;

        m_stats_avg += delta_us;
        m_stats_loop_index++;
        if (m_stats_loop_index > 200)         // what is 200?  nice time to stop?
        {
            m_stats_loop_index = 0;
            m_stats_avg /= 200;
            printf
            (
                "stats_avg[%ld]us stats_min[%ld]us stats_max[%ld]us\n",
                m_stats_avg, m_stats_min, m_stats_max
            );
            m_stats_min = 0x7FFFFFFF;
            m_stats_max = 0;
            m_stats_avg = 0;
        }
    }
}

void
perfstats::final_stats ()
{
    if (m_using_statistics)
    {
        printf("\n\n-- trigger width --\n");
        for (int i = 0; i < SEQ64_STATS_BUFFER_SIZE; i++)
        {
            printf("[%3d][%8ld]\n", i * SEQ64_STATS_BUFFER_SIZE, m_stats_all[i]);
        }
        /*
        printf("\n\n-- clock width --\n");
        int bpm  = m_master_bus.get_beats_per_minute();
        printf
        (
            "optimal: [%d us]\n", int(clock_tick_duration_bogus(bpm, m_ppqn))
        );
         */
        for (int i = 0; i < SEQ64_STATS_BUFFER_SIZE; i++)
        {
            printf("[%3d][%8ld]\n", i * 300, m_stats_clock[i]);
        }
    }
}

#ifdef XXOOZY

/**
 *  Performance output function.  This function is called by the free function
 *  output_thread_func().  Here's how it works:
 *
 *      -   It runs while m_outputing is true.  This is an overall flag which
 *          is true throughout the lifetime of the perform object.
 */

void
perfstats::output_func ()
{
    while (m_outputing)
    {
        m_statistics.init();

        m_statistics.get_last_clock();          // stats_last_clock_us, last

        while (m_running)
        {
            /**
             * -# Get delta time (current - last).
             * -# Get delta ticks from time.
             * -# Add to current_ticks.
             * -# Compute prebuffer ticks.
             * -# Play from current tick to prebuffer.
             */

            m_statistics.get_loop_start();      // stats_loop_start

            /*
             * Get the delta time.
             */

            long delta_us = m_statistics.get_delta_time();

#ifndef PLATFORM_WINDOWS
//          clock_gettime(CLOCK_REALTIME, &current);
//          delta.tv_sec  = current.tv_sec  - last.tv_sec;
//          delta.tv_nsec = current.tv_nsec - last.tv_nsec;
//          long delta_us = (delta.tv_sec * 1000000) + (delta.tv_nsec / 1000);
#else
//          current = timeGetTime();
//          delta = current - last;
//          long delta_us = delta * 1000;
#endif

//          int bpm  = m_master_bus.get_beats_per_minute();
//          double delta_tick = delta_time_us_to_ticks(delta_us, bpm, ppqn);
//
//              play(long(pad.js_current_tick));                // play!
//              m_master_bus.clock(long(pad.js_clock_tick));    // MIDI clock

                m_statistics.get_total_tick(pad);

            /**
             *  Figure out how much time we need to sleep, and do it.
             */

            long elapsed_us = m_statistics.get_elapsed_time();

            /*
             * Now we want to trigger every c_thread_trigger_width_ms,
             * and it took us delta_us to play().
             */

            delta_us = (c_thread_trigger_width_ms * 1000) - elapsed_us;

            /*
             * Check MIDI clock adjustment.  Note that we replaced
             * "60000000.0f / m_ppqn / bpm" with a call to a function.
             * We also removed the "f" specification from the constants.
             */

            double dct = double_ticks_from_ppqn(m_ppqn);
            double next_total_tick = pad.js_total_tick + dct;
            double next_clock_delta = next_total_tick - pad.js_total_tick - 1;
            double next_clock_delta_us =
                next_clock_delta * pulse_length_us(bpm, m_ppqn);

            if (next_clock_delta_us < (c_thread_trigger_width_ms * 1000.0 * 2.0))
                delta_us = long(next_clock_delta_us);

            m_statistics.sleep(delta_us);

            m_statistics.show();
            if (pad.js_jack_stopped)
                inner_stop();
        }

        m_statistics.final_stats();

        m_tick = 0;
        m_master_bus.flush();
        m_master_bus.stop();
    }
    pthread_exit(0);
}

#endif      // XXOOZY

}           // namespace seq64

/*
 * perfstats.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

