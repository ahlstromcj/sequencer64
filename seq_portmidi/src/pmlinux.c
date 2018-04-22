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
 * \file        pmlinux.c
 *
 *      PortMidi os-dependent code for Linux.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-04-20
 * \license     GNU GPLv2 or above
 *
 *  This file only needs to implement pm_init(), which calls various routines
 *  to register the available midi devices. This file must be separate from
 *  the main portmidi.c file because it is system dependent, and it is
 *  separate from, pmlinuxalsa.c, because it might need to register non-ALSA
 *  devices as well.
 *
 * Note:
 *
 *      If you add non-ALSA support, you need to fix alsa_poll() in
 *      pmlinuxalsa.c, which assumes all input devices are ALSA.
 */

#include <stdlib.h>

#include "seq64-config.h"
#include "finddefault.h"
#include "pmutil.h"
#include "pminternal.h"

#ifdef SEQ64_HAVE_LIBASOUND
#include "pmlinuxalsa.h"
#endif

PmDeviceID pm_default_input_device_id = -1;
PmDeviceID pm_default_output_device_id = -1;

/**
 * Note:
 *
 *  It is not an error for ALSA to fail to initialize.  It may be a design
 *  error that the client cannot query what subsystems are working properly
 *  other than by looking at the list of available devices.
 */

void
pm_init ()
{
#ifdef SEQ64_HAVE_LIBASOUND
	pm_linuxalsa_init();
#endif

    /*
     * This is set when we return to Pm_Initialize, but we need it
     * now in order to (successfully) call Pm_CountDevices().  Ugh.
     * At least we get to assume UTF-8 here, rather than Window's UTF-16.
     */

    pm_initialized = TRUE;
    pm_default_input_device_id = find_default_device
    (
        "/PortMidi/PM_RECOMMENDED_INPUT_DEVICE", TRUE,
        pm_default_input_device_id
    );

    pm_default_output_device_id = find_default_device
    (
        "/PortMidi/PM_RECOMMENDED_OUTPUT_DEVICE", FALSE,
        pm_default_output_device_id
    );
}

/**
 *
 */

void
pm_term (void)
{
#ifdef SEQ64_HAVE_LIBASOUND
    pm_linuxalsa_term();
#endif
}

#ifdef SEQ64_PORTMIDI_DEFAULT_DEVICE_ID

/**
 *
 */

PmDeviceID
Pm_GetDefaultInputDeviceID ()
{
    Pm_Initialize();
    return pm_default_input_device_id;
}

/**
 *
 */

PmDeviceID
Pm_GetDefaultOutputDeviceID ()
{
    Pm_Initialize();
    return pm_default_output_device_id;
}

#endif  // ifdef SEQ64_PORTMIDI_DEFAULT_DEVICE_ID

/**
 *
 */

void *
pm_alloc (size_t s)
{
    return malloc(s);
}

/**
 *
 */

void
pm_free (void * ptr)
{
    free(ptr);
}

/*
 * pmlinux.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

