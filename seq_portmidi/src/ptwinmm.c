/*
 *  This file is part of sequencer64, adapted from the PortMIDI project.
 *
 *  sequencer64 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  sequencer64 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file ptwinmm.c
 *
 *      Portable timer implementation for Win32.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-04-11
 * \license     GNU GPLv2 or above
 */

#include <windows.h>
#include <time.h>

#include "porttime.h"

/**
 *
 */

TIMECAPS caps;

/**
 *
 */

static long time_offset = 0;
static int time_started_flag = FALSE;
static long time_resolution;
static MMRESULT timer_id;
static PtCallback *time_callback;

/**
 *
 */

void CALLBACK
winmm_time_callback
(
    UINT uID, UINT uMsg, DWORD_PTR dwUser,
    DWORD_PTR dw1, DWORD_PTR dw2
)
{
    (*time_callback)(Pt_Time(), (void *) dwUser);
}

/**
 *
 */

PMEXPORT PtError
Pt_Start (int resolution, PtCallback * callback, void * userData)
{
    if (time_started_flag)
        return ptAlreadyStarted;

    timeBeginPeriod(resolution);
    time_resolution = resolution;
    time_offset = timeGetTime();
    time_started_flag = TRUE;
    time_callback = callback;
    if (callback)
    {
        timer_id = timeSetEvent
        (
            resolution, 1, winmm_time_callback,
            (DWORD_PTR) userData, TIME_PERIODIC | TIME_CALLBACK_FUNCTION
        );
        if (! timer_id)
            return ptHostError;
    }
    return ptNoError;
}

/**
 *
 */

PMEXPORT PtError
Pt_Stop ()
{
    if (! time_started_flag)
        return ptAlreadyStopped;

    if (time_callback && timer_id)
    {
        timeKillEvent(timer_id);
        time_callback = NULL;
        timer_id = 0;
    }
    time_started_flag = FALSE;
    timeEndPeriod(time_resolution);
    return ptNoError;
}

/**
 *
 */

PMEXPORT int
Pt_Started ()
{
    return time_started_flag;
}

/**
 *
 */

PMEXPORT PtTimestamp
Pt_Time ()
{
    return timeGetTime() - time_offset;
}

/**
 *
 */

PMEXPORT void
Pt_Sleep (int32_t duration)
{
    Sleep(duration);
}

/*
 * ptwinmm.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

