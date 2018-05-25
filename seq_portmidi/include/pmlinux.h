#ifndef SEQ64_PMLINUX_H
#define SEQ64_PMLINUX_H

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
 * \file        pmlinux.h
 *
 *      Device ID functions for Linux.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-04-10
 * \license     GNU GPLv2 or above
 *
 *  System-specific definitions.
 */

#ifdef __cplusplus
extern "C"
{
#endif

extern PmDeviceID pm_default_input_device_id;
extern PmDeviceID pm_default_output_device_id;

#ifdef __cplusplus
}               // extern "C"
#endif

#endif          // SEQ64_PMLINUX_H

/*
 * pmlinux.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

