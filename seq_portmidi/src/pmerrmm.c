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
 * \file        pmerrmm.c
 *
 *      System specific error-messages for the Windows MM API.
 *
 * \library     sequencer64 application
 * \author      Chris Ahlstrom
 * \date        2018-04-21
 * \updates     2018-04-23
 * \license     GNU GPLv2 or above
 */

/*
 *  This version level means "Windows 2000 and higher".
 */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>

#include "easy_macros.h"
#include "pmerrmm.h"

/*
 *  This printf() stuff is really important for debugging client app w/host
 *  errors.  Probably want to do something else besides read/write from/to
 *  console for portability, however.
 */

/**
 *  Error messages for:
 *
 *      -   midiInGetDevCaps(deviceid, lpincaps, szincaps).
 *      -   midiOutGetDevCaps(deviceid, lpincaps, szincaps).
 */

const char *
midi_io_get_dev_caps_error
(
    const char * devicename,
    const char * functionname,
    MMRESULT errcode
)
{
    static char s_error_storage[PM_STRING_MAX];
    const char * result = "Unknown";
    switch (errcode)
    {
    case MMSYSERR_NOERROR:

        result = "None";
        break;

    case MMSYSERR_BADDEVICEID:

        result = "The specified device identifier is out of range";
        break;

    case MMSYSERR_INVALPARAM:

        result = "The specified pointer or structure is invalid";
        break;

    case MMSYSERR_NODRIVER:

        result = "The driver is not installed";
        break;

    case MMSYSERR_NOMEM:

        result = "The system is unable to allocate or lock memory";
        break;
    }
    (void) snprintf
    (
        s_error_storage, sizeof s_error_storage,
        "%s() error for device '%s': '%s'\n",
        functionname, devicename, result
    );
    return &s_error_storage[0];
}

/*
 * pmerrmm.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

