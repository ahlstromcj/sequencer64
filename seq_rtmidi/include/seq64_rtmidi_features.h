#ifndef SEQ64_RTMIDI_FEATURES_H
#define SEQ64_RTMIDI_FEATURES_H

/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          seq64_rtmidi_features.h
 *
 *    This module defines configure and build-time
 *    options available for Sequencer64's RtMidi-derived implementation.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2016-11-19
 * \updates       2016-12-03
 * \license       GNU GPLv2 or above
 *
 *  For now, this header file enables only the JACK interface.  That is our
 *  main interest, adding native JACK support to Sequencer64.
 */

#include "platform_macros.h"

#ifdef PLATFORM_WINDOWS
#include "configwin32.h"
#else
#include "seq64-config.h"
#endif

/**
 *  Macros to enable the implementations that are supported under Linux.
 */

#define SEQ64_RTMIDI_PENDING

#ifdef PLATFORM_LINUX
#define SEQ64_BUILD_UNIX_JACK
#define SEQ64_BUILD_LINUX_ALSA          /* an alternative for Linux         */
#define SEQ64_BUILD_RTMIDI_DUMMY        /* an alternative for Linux, etc.   */
#endif

#ifdef PLATFORM_WINDOWS
#define SEQ64_BUILD_WINDOWS_MM
#define SEQ64_BUILD_RTMIDI_DUMMY        /* an alternative for Windows, etc. */
#endif

#ifdef PLATFORM_MACOSX
#define SEQ64_BUILD_MACOSX_CORE
#define SEQ64_BUILD_UNIX_JACK
#define SEQ64_BUILD_RTMIDI_DUMMY        /* an alternative for OSX, etc.     */
#endif

#endif      // SEQ64_RTMIDI_FEATURES_H

/*
 * seq64_rtmidi_features.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

