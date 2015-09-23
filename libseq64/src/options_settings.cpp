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
 * \file          options_settings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-22
 * \updates       2015-09-22
 * \license       GNU GPLv2 or above
 *
 */

#include "options_settings.hpp"

/*
 * Most of these variables were declared and used in other modules, but
 * are now consolidated here.
 */

#if EXPOSE_THESE
interaction_method_e global_interactionmethod = e_seq24_interaction;
bool global_allow_mod4_mode = true;
user_midi_bus_definition   global_user_midi_bus_definitions[c_max_busses];
user_instrument_definition global_user_instrument_definitions[c_max_instruments];
#endif

/**
 *  Default constructor.
 */

options_settings::options_settings ()
 :
    m_legacy_format             (false),
    m_lash_support              (false),
    m_show_midi                 (false),
    m_priority                  (false),
    m_stats                     (false),
    m_pass_sysex                (false),
    m_with_jack_transport       (false),
    m_with_jack_master          (false),
    m_with_jack_master_cond     (false),
    m_jack_start_mode           (false),
    m_manual_alsa_ports         (false),
    m_is_pattern_playing        (false),
    m_print_keys                (false),
    m_device_ignore             (false),
    m_device_ignore_num         (0),
    m_filename                  (),
    m_jack_session_uuid         (),
    m_last_used_dir             (),
    m_config_directory          (),
    m_config_filename           (),
    m_user_filename             (),
    m_config_filename_alt       (),
    m_user_filename_alt         ()
{
    // Empty body
}

/**
 *  Copy constructor.
 */

options_settings::options_settings (const options_settings & rhs)
 :
    m_legacy_format             (rhs.m_legacy_format),
    m_lash_support              (rhs.m_lash_support),
    m_show_midi                 (rhs.m_show_midi),
    m_priority                  (rhs.m_priority),
    m_stats                     (rhs.m_stats),
    m_pass_sysex                (rhs.m_pass_sysex),
    m_with_jack_transport       (rhs.m_with_jack_transport),
    m_with_jack_master          (rhs.m_with_jack_master),
    m_with_jack_master_cond     (rhs.m_with_jack_master_cond),
    m_jack_start_mode           (rhs.m_jack_start_mode),
    m_manual_alsa_ports         (rhs.m_manual_alsa_ports),
    m_is_pattern_playing        (rhs.m_is_pattern_playing),
    m_print_keys                (rhs.m_print_keys),
    m_device_ignore             (rhs.m_device_ignore),
    m_device_ignore_num         (rhs.m_device_ignore_num),
    m_filename                  (rhs.m_filename),
    m_jack_session_uuid         (rhs.m_jack_session_uuid),
    m_last_used_dir             (rhs.m_last_used_dir),
    m_config_directory          (rhs.m_config_directory),
    m_config_filename           (rhs.m_config_filename),
    m_user_filename             (rhs.m_user_filename),
    m_config_filename_alt       (rhs.m_config_filename_alt),
    m_user_filename_alt         (rhs.m_user_filename_alt)
{
    // Empty body
}

/**
 *  Principal assignment operator.
 */

options_settings &
options_settings::operator = (const options_settings & rhs)
{
    if (this != &rhs)
    {
        m_legacy_format             = rhs.m_legacy_format;
        m_lash_support              = rhs.m_lash_support;
        m_show_midi                 = rhs.m_show_midi;
        m_priority                  = rhs.m_priority;
        m_stats                     = rhs.m_stats;
        m_pass_sysex                = rhs.m_pass_sysex;
        m_with_jack_transport       = rhs.m_with_jack_transport;
        m_with_jack_master          = rhs.m_with_jack_master;
        m_with_jack_master_cond     = rhs.m_with_jack_master_cond;
        m_jack_start_mode           = rhs.m_jack_start_mode;
        m_manual_alsa_ports         = rhs.m_manual_alsa_ports;
        m_is_pattern_playing        = rhs.m_is_pattern_playing;
        m_print_keys                = rhs.m_print_keys;
        m_device_ignore             = rhs.m_device_ignore;
        m_device_ignore_num         = rhs.m_device_ignore_num;
        m_filename                  = rhs.m_filename;
        m_jack_session_uuid         = rhs.m_jack_session_uuid;
        m_last_used_dir             = rhs.m_last_used_dir;
        m_config_directory          = rhs.m_config_directory;
        m_config_filename           = rhs.m_config_filename;
        m_user_filename             = rhs.m_user_filename;
        m_config_filename_alt       = rhs.m_config_filename_alt;
        m_user_filename_alt         = rhs.m_user_filename_alt;
    }
    return *this;
}

/**
 *  Sets the default values.
 */

void
options_settings::set_defaults ()
{
    m_legacy_format             = false;
    m_lash_support              = false;
    m_show_midi                 = false;
    m_priority                  = false;
    m_stats                     = false;
    m_pass_sysex                = false;
    m_with_jack_transport       = false;
    m_with_jack_master          = false;
    m_with_jack_master_cond     = false;
    m_jack_start_mode           = true;
    m_manual_alsa_ports         = false;
    m_is_pattern_playing        = false;
    m_print_keys                = false;
    m_device_ignore             = false;
    m_device_ignore_num         = 0;
    m_filename.clear();
    m_jack_session_uuid.clear();
    m_last_used_dir             = "/"
    m_config_directory          = ".config/sequencer64";
    m_config_filename           = "sequencer64rc";
    m_user_filename             = "sequencer64user";
    m_config_filename_alt       = ".seq24rc";
    m_user_filename_alt         = ".seq24usr";
}

/*
 * options_settings.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
