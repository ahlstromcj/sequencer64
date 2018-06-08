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
 * \file        porttime.c
 *
 *      Portable API for millisecond timer.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-05-25
 * \license     GNU GPLv2 or above
 *
 *  There is no machine-independent implementation code to put here.
 */

#include "porttime.h"

/**
 *
 */

static double s_pm_beats_per_minute = 125;
static int s_pm_tempo_microseconds = 480000;
static int s_pm_ppqn = 480;

/**
 *  New functions to support setting the tempo and PPQN, as well as
 *  converting PortMidi time to MIDI pulses (ticks).  Note that the setting
 *  of PPQN here is roughly similar to seq64::choose_ppqn() in the settings
 *  module.
 */

void
Pt_Set_Midi_Timing (double bpm, int ppqn)
{
    Pt_Set_Bpm(bpm);
    Pt_Set_Ppqn(ppqn);
}

/**
 *
 */

void
Pt_Set_Bpm (double bpm)
{
    if (bpm > 0.0)
    {
        s_pm_beats_per_minute = bpm;
        s_pm_tempo_microseconds = (int) (60000000.0 / bpm);
    }
}

/**
 *
 */

void
Pt_Set_Ppqn (int ppqn)
{
    s_pm_ppqn = ppqn > 0 ? ppqn : 192 ;
}

/**
 *  Convert the milliseconds timestamp to pulses (ticks).
 */

long
Pt_Time_To_Pulses (int tsms)
{
    return (long) (tsms * s_pm_beats_per_minute / 60000);
}

/**
 *
 */

double
Pt_Get_Bpm (void)
{
    return s_pm_beats_per_minute;
}

/**
 *
 */

int
Pt_Get_Tempo_Microseconds (void)
{
    return s_pm_tempo_microseconds;
}

/**
 *
 */

int
Pt_Get_Ppqn (void)
{
    return s_pm_ppqn;
}

/*
 * porttime.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

