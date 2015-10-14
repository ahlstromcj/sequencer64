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
 * \file          globals.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-08-07
 * \updates       2015-10-14
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures will
 *  eventually completely replace.
 *
 *  The second part of this file defines some functions needed by other
 *  modules, such as MIDI timing calculations.  These items are in the seq64
 *  namespace.
 *
 * \note
 *      The MIDI time signature is specified by "FF 58 nn dd cc bb" where:
 *
 *      -   <i> nn </i> is the numerator, and counts the number of beats in a
 *          measure (bar).
 *      -   <i> dd </i> is the denominator, and specifies the unit of the beat
 *          (e.g. 4 or 8), and is specified as 2 to the <i> dd </i> power.
 *      -   <i> cc </i> is the MIDI ticks per metronome click.  The standard
 *          MIDI clock ticks 24 times per quarter note, so a value of 6 would
 *          mean the metronome clicks every 1/8th note.
 *      -   <i> bb </i> is the number of 32nd notes per MIDI quarter note.
 *          For example, a value of 16 means that the music plays two quarter
 *          notes for each quarter note metered out by the MIDI clock, so that
 *          the music plays at double speed.
 */

#include "globals.h"

/**
 *  Provide the eventual replacement for all of the other "global_xxx"
 *  variables.
 */

rc_settings g_rc_settings;

/**
 *  Provide the eventual replacement for all of the other settings in the
 *  "user" configuration file, plus some of the "constants" in the globals
 *  module.
 */

user_settings g_user_settings;

/**
 *  Provides a way to override the buss number for smallish MIDI files.
 *  It replaces the buss-number read from the file.  This option is turned on
 *  by the --bus option, and is merely a convenience feature for the
 *  quick previewing of a tune.  (It's called "developer laziness".)
 *
 *  If -1, this feature is disabled.
 *
 *  Current not part of the global "rc" settings structure.
 */

char global_buss_override = char(-1);

/**
 * Most of these variables were declared and used in other modules, but
 * are now consolidated here.
 */

bool global_legacy_format = false;
bool global_lash_support = false;
bool global_manual_alsa_ports = false;
bool global_showmidi = false;
bool global_priority = false;
bool global_device_ignore = false;
int global_device_ignore_num = 0;
bool global_stats = false;
bool global_pass_sysex = false;
std::string global_filename = "";
std::string global_last_used_dir = "/";
std::string global_config_directory = ".config/sequencer64";
std::string global_config_filename = "sequencer64.rc";
std::string global_user_filename = "sequencer64.usr";
std::string global_config_filename_alt = ".seq24rc";
std::string global_user_filename_alt = ".seq24usr";
bool global_print_keys = false;
bool global_is_pattern_playing = false;
bool global_with_jack_transport = false;
bool global_with_jack_master = false;
bool global_with_jack_master_cond = false;
bool global_jack_start_mode = true;
std::string global_jack_session_uuid = "";
interaction_method_t global_interactionmethod = e_seq24_interaction;
bool global_allow_mod4_mode = true;
user_midi_bus_t global_user_midi_bus_definitions[c_max_busses];
user_instrument_t global_user_instrument_definitions[c_max_instruments];

namespace seq64
{

}       // namespace seq64

/*
 * globals.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

