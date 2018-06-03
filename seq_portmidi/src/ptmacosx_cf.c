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
 * \file ptmacosx_cf.c
 *
 *      Portable timer implementation for Mac OS X.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2018-05-14
 * \updates     2018-05-14
 * \license     GNU GPLv2 or above
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <CoreFoundation/CoreFoundation.h>

#import <mach/mach.h>
#import <mach/mach_error.h>
#import <mach/mach_time.h>
#import <mach/clock.h>

#include "porttime.h"

#define THREAD_IMPORTANCE 30
#define LONG_TIME 1000000000.0

static int time_started_flag = FALSE;
static CFAbsoluteTime startTime = 0.0;
static CFRunLoopRef timerRunLoop;

typedef struct
{
    int resolution;
    PtCallback *callback;
    void *userData;
} PtThreadParams;


void Pt_CFTimerCallback(CFRunLoopTimerRef timer, void *info)
{
    PtThreadParams *params = (PtThreadParams*)info;
    (*params->callback)(Pt_Time(), params->userData);
}

static void* Pt_Thread(void *p)
{
    CFTimeInterval timerInterval;
    CFRunLoopTimerContext timerContext;
    CFRunLoopTimerRef timer;
    PtThreadParams *params = (PtThreadParams*)p;
    //CFTimeInterval timeout;

    /* raise the thread's priority */
    kern_return_t error;
    thread_extended_policy_data_t extendedPolicy;
    thread_precedence_policy_data_t precedencePolicy;

    extendedPolicy.timeshare = 0;
    error = thread_policy_set(mach_thread_self(), THREAD_EXTENDED_POLICY,
                              (thread_policy_t)&extendedPolicy,
                              THREAD_EXTENDED_POLICY_COUNT);
    if (error != KERN_SUCCESS)
    {
        mach_error("Couldn't set thread timeshare policy", error);
    }

    precedencePolicy.importance = THREAD_IMPORTANCE;
    error = thread_policy_set(mach_thread_self(), THREAD_PRECEDENCE_POLICY,
                              (thread_policy_t)&precedencePolicy,
                              THREAD_PRECEDENCE_POLICY_COUNT);
    if (error != KERN_SUCCESS)
    {
        mach_error("Couldn't set thread precedence policy", error);
    }

    /* set up the timer context */
    timerContext.version = 0;
    timerContext.info = params;
    timerContext.retain = NULL;
    timerContext.release = NULL;
    timerContext.copyDescription = NULL;

    /* create a new timer */
    timerInterval = (double)params->resolution / 1000.0;
    timer = CFRunLoopTimerCreate(NULL, startTime + timerInterval, timerInterval,
                                 0, 0, Pt_CFTimerCallback, &timerContext);

    timerRunLoop = CFRunLoopGetCurrent();
    CFRunLoopAddTimer(timerRunLoop, timer, CFSTR("PtTimeMode"));

    /* run until we're told to stop by Pt_Stop() */
    CFRunLoopRunInMode(CFSTR("PtTimeMode"), LONG_TIME, false);

    CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), timer, CFSTR("PtTimeMode"));
    CFRelease(timer);
    free(params);

    return NULL;
}

PtError Pt_Start(int resolution, PtCallback *callback, void *userData)
{
    PtThreadParams *params = (PtThreadParams*)malloc(sizeof(PtThreadParams));
    pthread_t pthread_id;

    printf("Pt_Start() called\n");

    // /* make sure we're not already playing */
    if (time_started_flag) return ptAlreadyStarted;
    startTime = CFAbsoluteTimeGetCurrent();

    if (callback)
    {

        params->resolution = resolution;
        params->callback = callback;
        params->userData = userData;

        pthread_create(&pthread_id, NULL, Pt_Thread, params);
    }

    time_started_flag = TRUE;
    return ptNoError;
}


PtError Pt_Stop()
{
    printf("Pt_Stop called\n");

    CFRunLoopStop(timerRunLoop);
    time_started_flag = FALSE;
    return ptNoError;
}


int Pt_Started()
{
    return time_started_flag;
}


PtTimestamp Pt_Time()
{
    CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
    return (PtTimestamp)((now - startTime) * 1000.0);
}


void Pt_Sleep(int32_t duration)
{
    usleep(duration * 1000);
}

/*
 * ptmacosx_cf.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

