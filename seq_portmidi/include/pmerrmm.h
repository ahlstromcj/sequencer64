#ifndef SEQ64_PMERRMM_H
#define SEQ64_PMERRMM_H

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
 * \file        pmerrmm.h
 *
 *      Provides system-specific error-messages for Windows.
 *
 * \library     sequencer64 application
 * \author      Chris Ahlstrom
 * \date        2018-04-21
 * \updates     2018-04-24
 * \license     GNU GPLv2 or above
 *
 */

#include <windows.h>

#include "pminternal.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern const char * midi_io_get_dev_caps_error
(
    const char * devicename,
    const char * functionname,
    MMRESULT errcode
);

#ifdef __cplusplus
}           // extern "C"
#endif

#endif      // SEQ64_PMERRMM_H

/*
 * pmerrmm.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

