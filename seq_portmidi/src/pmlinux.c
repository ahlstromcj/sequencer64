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
 * \file pmlinux.c
 *
 * PortMidi os-dependent code
 *
 * This file only needs to implement pm_init(), which calls various
 * routines to register the available midi devices. This file must
 * be separate from the main portmidi.c file because it is system
 * dependent, and it is separate from, pmlinuxalsa.c, because it
 * might need to register non-alsa devices as well.
 *
 * NOTE: if you add non-ALSA support, you need to fix :alsa_poll()
 * in pmlinuxalsa.c, which assumes all input devices are ALSA.
 */

#include "stdlib.h"
#include "finddefault.h"
#include "pmutil.h"
#include "pminternal.h"

#ifdef PMALSA
#include "pmlinuxalsa.h"
#endif

#ifdef PMNULL
#include "pmlinuxnull.h"
#endif

PmDeviceID pm_default_input_device_id = -1;
PmDeviceID pm_default_output_device_id = -1;

/**
 * Note: it is not an error for PMALSA to fail to initialize.
 * It may be a design error that the client cannot query what subsystems
 * are working properly other than by looking at the list of available
 * devices.
 */

void
pm_init ()
{
#ifdef PMALSA
	pm_linuxalsa_init();
#endif

#ifdef PMNULL
    pm_linuxnull_init();
#endif

    /*
     * this is set when we return to Pm_Initialize, but we need it
     * now in order to (successfully) call Pm_CountDevices()
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
#ifdef PMALSA
    pm_linuxalsa_term();
#endif
}

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

