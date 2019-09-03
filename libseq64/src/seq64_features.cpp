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
 * \file          seq64_features.cpp
 *
 *  This module adds some functions that reflect the features compiled into
 *  the application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2017-03-12
 * \updates       2019-09-01
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.
 *
 *  Well, now it is empty.  We will wait a bit before removing this module.
 */

#include <sstream>                      /* std::ostringstream               */

#include "seq64_features.h"             /* more build-time configuration    */
#include "app_limits.h"                 /* macros for seq_build_details()   */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

static std::string s_app_name = SEQ64_APP_NAME;
static std::string s_client_name = SEQ64_CLIENT_NAME;
static std::string s_version = SEQ64_VERSION;
static std::string s_apptag = SEQ64_APP_NAME " " SEQ64_VERSION;
static std::string s_versiontext = SEQ64_APP_NAME " " SEQ64_GIT_VERSION " "
    SEQ64_VERSION_DATE_SHORT "\n";

/**
 *
 */

void
set_app_name (const std::string & aname)
{
    s_app_name = aname;
}

/**
 *
 */

void
set_client_name (const std::string & cname)
{
    s_client_name = cname;
}

/**
 *  Returns the name of the application.  We could continue to use the macro
 *  SEQ64_APP_NAME, but we might eventually want to make this name
 *  configurable.  Not too likely, but possible.
 *
 * \return
 *      Returns SEQ64_APP_NAME.
 */

const std::string &
seq_app_name ()
{
    return s_app_name;
}

/**
 *  Returns the name of the client for the application.  We could continue to
 *  use the macro SEQ64_CLIENT_NAME, but we might eventually want to make this
 *  name configurable.  More likely to be a configuration option in the
 *  future.
 *
 * \return
 *      Returns SEQ64_CLIENT_NAME.
 */

const std::string &
seq_client_name ()
{
    return s_client_name;
}

/**
 *  Returns the version of the application.  We could continue to use the macro
 *  SEQ64_VERSION, but ... let's be consistent.  :-D
 *
 * \return
 *      Returns SEQ64_VERSION.
 */

const std::string &
seq_version ()
{
    return s_version;
}

/**
 *  Sets up the "hardwired" version text for Sequencer64.  This value
 *  ultimately comes from the configure.ac script (for now).
 */

const std::string &
seq_version_text ()
{
    return s_versiontext;
}

/**
 *
 */

const std::string &
seq_app_tag ()
{
    return s_apptag;
}

/**
 *  This section of variables provide static information about the options
 *  enabled or disabled during the build.
 */

#ifdef PLATFORM_32_BIT
const static std::string s_bitness = "32-bit";
#else
const static std::string s_bitness = "64-bit";
#endif

/**
 *  Generates a string describing the features of the build.
 *
 * \return
 *      Returns an ordered, human-readable string enumerating the built-in
 *      features of this application.
 */

std::string
seq_build_details ()
{
    std::ostringstream result;
    result
        << "Build features:" << std::endl
        << "  C++ version " << std::to_string(__cplusplus) << std::endl
#ifdef SEQ64_RTMIDI_SUPPORT
        << "  Native JACK/ALSA (rtmidi) on" << std::endl
#endif
#ifdef SEQ64_ALSAMIDI_SUPPORT
        << "  ALSA-only MIDI support on" << std::endl
#endif
#ifdef SEQ64_PORTMIDI_SUPPORT
        << "  PortMIDI support on" << std::endl
#endif
        << "  Event editor on" << std::endl
#ifdef SEQ64_USE_EVENT_MAP
        << "  Event multimap (vs list) on" << std::endl
#endif
        << "  Follow progress bar on" << std::endl
#ifdef SEQ64_EDIT_SEQUENCE_HIGHLIGHT
        << "  Highlight edit pattern on" << std::endl
#endif
#ifdef SEQ64_HIGHLIGHT_EMPTY_SEQS
        << "  Highlight empty patterns on" << std::endl
#endif
#ifdef SEQ64_JACK_SESSION
        << "  JACK session on" << std::endl
#endif
#ifdef SEQ64_JACK_SUPPORT
        << "  JACK support on" << std::endl
#endif
#ifdef SEQ64_LASH_SUPPORT
        << "  LASH support on" << std::endl
#endif
        << "  MIDI vector (vs list) on" << std::endl
        << "  Seq32 chord generator on" << std::endl
        << "  Seq32 LFO window on" << std::endl
        << "  Seq32 menu buttons on" << std::endl
        << "  Seq32 transpose on" << std::endl
        << "  BPM Tap button on" << std::endl
        << "  Solid piano-roll grid on" << std::endl
#ifdef SEQ64_JE_PATTERN_PANEL_SCROLLBARS
        << "  Main window scroll-bars on" << std::endl
#endif
#ifdef SEQ64_SHOW_COLOR_PALETTE
        << "  Optional pattern coloring on" << std::endl
#endif
#ifdef SEQ64_MULTI_MAINWID
        << "  Multiple main windows on" << std::endl
#endif
        << "  Song performance recording on" << std::endl
#ifdef SEQ64_SONG_BOX_SELECT
        << "  Box song selection on" << std::endl
#endif
#ifdef SEQ64_STATISTICS_SUPPORT
        << "  Statistics support on" << std::endl
#endif
#ifdef PLATFORM_WINDOWS
        << "  Windows support on" << std::endl
#endif
        << "  Pause support on" << std::endl
        << "  Save time-sig/tempo on" << std::endl
#ifdef PLATFORM_DEBUG
        << "  Debug code on" << std::endl
#endif
        << "  " << s_bitness << " support enabled" << std::endl
        << std::endl
    << "Options are enabled/disabled via the configure script," << std::endl
    << "libseq64/include/seq64_features.h, or the build-specific" << std::endl
    << "seq64-config.h file in include or in include/qt/portmidi" << std::endl
    ;
    return result.str();
}

}           // namespace seq64

/*
 * seq64_features.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

