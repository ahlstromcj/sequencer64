#ifndef SEQ64_FINDDEFAULT_H
#define SEQ64_FINDDEFAULT_H

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
 * \file        finddefault.h
 *
 *      Provides a function to find the default MIDI device.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; new from Chris Ahlstrom
 * \date        2018-04-08
 * \updates     2018-04-08
 * \license     GNU GPLv2 or above
 *
 *  This file is included by files that implement library internals.
 *  However, Sequencer64 doesn't use it, since it has its own configuration
 *  files, located in ~/.config/sequencer64/ or in
 *  C:/Users/username/AppData/Local/.
 */

#include "portmidi.h"                   /* PmDeviceID   */

#ifdef __cplusplus
extern "C"
{
#endif

extern PmDeviceID find_default_device (char * path, int input, PmDeviceID);

#ifdef __cplusplus
}               // extern "C"
#endif

#endif          // SEQ64_FINDDEFAULT_H

/*
 * finddefault.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

