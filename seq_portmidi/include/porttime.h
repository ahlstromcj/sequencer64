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
 * \file        porttime.h
 *
 *      A portable interface to millisecond timer.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-05-25
 * \license     GNU GPLv2 or above
 *
 * change log for porttime:
 *
 *      10-Jun-03 Mark Nelson & RBD
 *      Boost priority of timer thread in ptlinux.c implementation.
 */

#include "platform_macros.h"
#include "pminternal.h"                 /* int32_t  */

/* Should there be a way to choose the source of time here? */

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef PMEXPORT

#ifdef _WINDLL
#define PMEXPORT __declspec(dllexport)

#else

#define PMEXPORT
#endif

#endif

/**
 *
 */

typedef enum
{
    ptNoError = 0,         /**< Success.                                    */
    ptHostError = -10000,  /**< A system-specific error occurred.           */
    ptAlreadyStarted,      /**< Can't start timer, it is already started.   */
    ptAlreadyStopped,      /**< Can't stop timer, it is already stopped.    */
    ptInsufficientMemory   /**< Memory could not be allocated.              */

} PtError;

/**
 *
 */

typedef int32_t PtTimestamp;

/**
 *
 */

typedef void (PtCallback) (PtTimestamp timestamp, void * userData);

/*
 *  Pt_Start() starts a real-time service.
 *
 * \param resolution
 *      is the timer resolution in ms. The time will advance every
 *  resolution ms.
 *
 * \param callback
 *      Is a function pointer to be called every resolution ms.
 *
 * \param userData
 *      Is passed to callback as a parameter.
 *
 * \return
 *      Upon success, returns ptNoError. See PtError for other values.
 */

PMEXPORT PtError Pt_Start
(
    int resolution, PtCallback * callback, void * userData
);

/**
 *  Pt_Stop() stops the timer.
 *
 *  return value:
 *  Upon success, returns ptNoError. See PtError for other values.
 */

PMEXPORT PtError Pt_Stop ();

/**
 *  Pt_Started() returns true iff the timer is running.
 */

PMEXPORT int Pt_Started ();

/**
 *  Pt_Time() returns the current time in ms.
 */

PMEXPORT PtTimestamp Pt_Time ();

/**
 *  Pt_Sleep() pauses, allowing other threads to run.
 *
 * \param duration
 *      The length of the pause in milliseconds. The true duration of the
 *      pause may be rounded to the nearest or next clock tick as determined
 *      by resolution in Pt_Start().
 */

PMEXPORT void Pt_Sleep (int32_t duration);

/*
 *  New functions to support setting the tempo and PPQN, as well as
 *  converting PortMidi time to MIDI pulses (ticks).
 */

PMEXPORT void Pt_Set_Midi_Timing (double bpm, int ppqn);
PMEXPORT long Pt_Time_To_Pulses (int tsms);
PMEXPORT void Pt_Set_Midi_Timing (double bpm, int ppqn);
PMEXPORT void Pt_Set_Bpm (double bpm);
PMEXPORT void Pt_Set_Ppqn (int ppqn);
PMEXPORT double Pt_Get_Bpm (void);
PMEXPORT int Pt_Get_Tempo_Microseconds (void);
PMEXPORT int Pt_Get_Ppqn (void);

#ifdef __cplusplus
}       // extern "C"
#endif

/*
 * porttime.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

