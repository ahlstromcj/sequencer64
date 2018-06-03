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
 * \file        pmwin.c
 *
 *      PortMidi os-dependent code for Windows.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-05-05
 * \license     GNU GPLv2 or above

 *  This file needs to implement:
 *
 *      -   pm_init(), which calls various routines to register the available
 *          MIDI devices.
 *      -   Pm_GetDefaultInputDeviceID().
 *      -   Pm_GetDefaultOutputDeviceID().
 *
 *  The latter two exist only if SEQ64_PORTMIDI_DEFAULT_DEVICE_ID is defined;
 *  we have our own way of setting device configuration.
 *
 *  This file must be separate from the main portmidi.c file because it is
 *  system dependent, and it is separate from, say, pmwinmm.c, because it might
 *  need to register devices for WinMM, DirectX, and others.
 */

#include <stdlib.h>

#include "easy_macros.h"
#include "portmidi.h"
#include "pmutil.h"
#include "pminternal.h"
#include "pmwinmm.h"

#if defined PLATFORM_DEBUG
#include <stdio.h>
#endif

#include <windows.h>

/*
 * #include <tchar.h>
 */

/**
 *  This macro is part of Microsoft's tchar.h, but we want to use it only as
 *  a marker for now.
 */

#define _T(x)           ((char *)(x))

/**
 *  The maximum length of the name of a device.
 */

#define PATTERN_MAX     256

/**
 *  pm_exit() is called when the program exits.  It calls pm_term() to make
 *  sure PortMidi is properly closed.  If PLATFORM_DEBUG is on, we prompt for
 *  input to avoid losing error messages.  Without this prompting, client
 *  console application cannot see one of its errors before closing.
 */

static void
pm_exit (void)
{
    pm_term();

#if defined PLATFORM_DEBUG_XXX
    char line[PM_STRING_MAX];
    printf("Type Enter to exit...\n");
    fgets(line, PM_STRING_MAX, stdin);
#endif

}

/*
 *  pm_init() provides the windows-dependent initialization.  It also sets the
 *  atexit() callback to pm_exit().
 */

void
pm_init (void)
{
    atexit(pm_exit);
    pm_winmm_init();

    /*
     * Initialize other APIs (DirectX?) here.  DOn't we need to set
     * pm_initialized = TRUE here?  And call find_default_device()?
     */
}

/**
 *  Calls pm_winmm_term() to end the PortMidi session.
 */

void
pm_term (void)
{
    pm_winmm_term();
}

#ifdef SEQ64_PORTMIDI_USE_JAVA_PREFS

/**
 *  Gets the default MIDI device by querying the Windows Registry.
 *
 * \param is_input
 *      Set to true if this is an input device.
 *
 *  \param key
 *      A pointer to the name of the devices.  It will be altered as a
 *      side-effect in this function.
 */

static PmDeviceID
pm_get_default_device_id (int is_input, char * key)
{
    HKEY hkey;
    BYTE pattern[PATTERN_MAX];
    ULONG pattern_max = PATTERN_MAX;
    DWORD dwType;
    PmDeviceID id = pmNoDevice;
    int i;
    int j;

    /*
     * Find the first input or device; this is the default.
     */

    Pm_Initialize();                    /* make sure the descriptors exist! */
    for (i = 0; i < pm_descriptor_index; ++i)
    {
        if (pm_descriptors[i].pub.input == is_input)
        {
            id = i;
            break;
        }
    }

    /*
     * Look in the Windows Registry for a default device name pattern.
     *
     *  We want to get rid of this freakin' dependency on Java!  Also, we
     *  do not need "Prefs", we have our own configuration file.  The Linux
     *  version does not use "JavaSoft" or "Prefs".
     */

    if
    (
        RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software"), 0, KEY_READ, &hkey)
            != ERROR_SUCCESS
    )
    {
        return id;
    }
    if (RegOpenKeyEx(hkey, _T("JavaSoft"), 0, KEY_READ, &hkey) != ERROR_SUCCESS)
    {
        return id;
    }
    if (RegOpenKeyEx(hkey, _T("Prefs"), 0, KEY_READ, &hkey) != ERROR_SUCCESS)
    {
        return id;
    }
    if
    (
        RegOpenKeyEx(hkey, _T("/Port/Midi"), 0, KEY_READ, &hkey)
            != ERROR_SUCCESS
    )
    {
        return id;
    }
    if
    (
        RegQueryValueEx(hkey, key, NULL, &dwType, pattern, &pattern_max)
            != ERROR_SUCCESS
    )
    {
        return id;
    }

    i = j = 0;          /* decode pattern: upper case encoded with "/" prefix */
    while (pattern[i])
    {
        if (pattern[i] == '/' && pattern[i + 1])
        {
            pattern[j++] = toupper(pattern[++i]);
        }
        else
        {
            pattern[j++] = tolower(pattern[i]);
        }
        ++i;
    }
    pattern[j] = 0;                         /* end of string */

    /*
     * Now pattern is the string from the Registry; search for match.
     * We may need to use typedefs to properly switch between Linux and
     * Windows.
     */

    i = pm_find_default_device((char *) pattern, is_input);
    if (i != pmNoDevice)
        id = i;

    return id;
}

#endif  // SEQ64_PORTMIDI_USE_JAVA_PREFS

#ifdef SEQ64_PORTMIDI_DEFAULT_DEVICE_ID

/**
 * \tricky
 *      The string is meant to be compared to a UTF-16 string from the
 *      Registry.  Do not use _T() here.
 */

PmDeviceID
Pm_GetDefaultInputDeviceID ()
{
    return pm_get_default_device_id
    (
         TRUE, "/P/M_/R/E/C/O/M/M/E/N/D/E/D_/I/N/P/U/T_/D/E/V/I/C/E"
    );
}

/**
 * \tricky
 *      The string is meant to be compared to a UTF-16 string from the
 *      Registry.  Do not use _T() here.
 */

PmDeviceID
Pm_GetDefaultOutputDeviceID ()
{
    return pm_get_default_device_id
    (
         FALSE, "/P/M_/R/E/C/O/M/M/E/N/D/E/D_/O/U/T/P/U/T_/D/E/V/I/C/E"
    );
}

#endif  // SEQ64_PORTMIDI_DEFAULT_DEVICE_ID

/**
 *  A simple wrapper for malloc().
 *
 * \param s
 *      Provides the desired size of the allocation.
 *
 * \return
 *      Returns a void pointer to the allocated buffer.
 */

void *
pm_alloc (size_t s)
{
    return malloc(s);
}

/**
 *  The inverse of pm_alloc(), a wrapper for the free(3) function.
 *
 * \param ptr
 *      Provides the pointer to be freed, if it is not null.
 */

void
pm_free (void * ptr)
{
    if (not_nullptr(ptr))
        free(ptr);
}

/*
 * pmwin.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

