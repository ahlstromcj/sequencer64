#ifndef SEQ64_PMWINMM_H
#define SEQ64_PMWINMM_H

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
 * \file        pmwinmm.h
 *
 *      Windows multi-media API setup and teardown functions.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-04-10
 * \license     GNU GPLv2 or above
 *
 *  Provides system-specific definitions for Windows.
 */

#ifdef __cplusplus
extern "C"
{
#endif

extern void pm_winmm_init (void);
extern void pm_winmm_term (void);

#ifdef __cplusplus
}           // extern "C"
#endif

#endif      // SEQ64_PMWINMM_H

/*
 * pmwinmm.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

