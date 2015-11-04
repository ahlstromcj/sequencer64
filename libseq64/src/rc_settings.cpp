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
 * \file          rc_settings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-22
 * \updates       2015-11-04
 * \license       GNU GPLv2 or above
 *
 *  Note that this module also sets the legacy global variables, so that
 *  they can be used by modules that have not yet been cleaned up.
 */

#include <stdlib.h>                     /* getenv()                     */
#include "globals.h"                    /* to support legacy variables  */

#ifdef PLATFORM_UNIX
#include <sys/types.h>                  /* for stat() and mkdir()       */
#include <sys/stat.h>
#endif

#include "rc_settings.hpp"

/**
 *  Select the HOME or HOMEPATH environment variables depending on whether
 *  building for Windows or not.  Also select the appropriate directory
 *  separator for SLASH.
 */

#ifdef PLATFORM_WINDOWS
#define HOME "HOMEPATH"
#define SLASH "\\"
#else
#define HOME "HOME"
#define SLASH "/"
#endif

namespace seq64
{

/**
 *  Default constructor.
 */

rc_settings::rc_settings ()
 :
    m_legacy_format             (false),
    m_lash_support              (false),
    m_allow_mod4_mode           (false),
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
    m_interaction_method        (e_seq24_interaction),
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

rc_settings::rc_settings (const rc_settings & rhs)
 :
    m_legacy_format             (rhs.m_legacy_format),
    m_lash_support              (rhs.m_lash_support),
    m_allow_mod4_mode           (rhs.m_allow_mod4_mode),
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
    m_interaction_method        (rhs.m_interaction_method),
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

rc_settings &
rc_settings::operator = (const rc_settings & rhs)
{
    if (this != &rhs)
    {
        m_legacy_format             = rhs.m_legacy_format;
        m_lash_support              = rhs.m_lash_support;
        m_allow_mod4_mode           = rhs.m_allow_mod4_mode;
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
        m_interaction_method        = rhs.m_interaction_method;
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
rc_settings::set_defaults ()
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
    m_device_ignore_num         = e_seq24_interaction;
    m_filename.clear();
    m_jack_session_uuid.clear();
    m_last_used_dir             = "/";
    m_config_directory          = ".config/sequencer64";
    m_config_filename           = "sequencer64.rc";
    m_user_filename             = "sequencer64.usr";
    m_config_filename_alt       = ".seq24rc";
    m_user_filename_alt         = ".seq24usr";
}

/**
 *  Copies the current values of the member variables into their
 *  corresponding global variables.
 */

void
rc_settings::set_globals ()
{
    global_legacy_format             = m_legacy_format;
    global_lash_support              = m_lash_support;
    global_showmidi                  = m_show_midi;
    global_priority                  = m_priority;
    global_stats                     = m_stats;
    global_pass_sysex                = m_pass_sysex;
    global_with_jack_transport       = m_with_jack_transport;
    global_with_jack_master          = m_with_jack_master;
    global_with_jack_master_cond     = m_with_jack_master_cond;
    global_jack_start_mode           = m_jack_start_mode;
    global_manual_alsa_ports         = m_manual_alsa_ports;
    global_print_keys                = m_print_keys;
    global_device_ignore             = m_device_ignore;
    global_device_ignore_num         = m_device_ignore_num;
    global_device_ignore_num         = m_device_ignore_num;
    global_filename                  = m_filename;
    global_jack_session_uuid         = m_jack_session_uuid;
    global_last_used_dir             = m_last_used_dir;
    global_config_directory          = m_config_directory;
    global_config_filename           = m_config_filename;
    global_user_filename             = m_user_filename;
    global_config_filename_alt       = m_config_filename_alt;
    global_user_filename_alt         = m_user_filename_alt;
}

/**
 *  Copies the current values of the global variables into their
 *  corresponding member variables.
 */

void
rc_settings::get_globals ()
{
    m_legacy_format             = global_legacy_format;
    m_lash_support              = global_lash_support;
    m_show_midi                 = global_showmidi;
    m_priority                  = global_priority;
    m_stats                     = global_stats;
    m_pass_sysex                = global_pass_sysex;
    m_with_jack_transport       = global_with_jack_transport;
    m_with_jack_master          = global_with_jack_master;
    m_with_jack_master_cond     = global_with_jack_master_cond;
    m_jack_start_mode           = global_jack_start_mode;
    m_manual_alsa_ports         = global_manual_alsa_ports;
    m_print_keys                = global_print_keys;
    m_device_ignore             = global_device_ignore;
    m_device_ignore_num         = global_device_ignore_num;
    m_device_ignore_num         = global_device_ignore_num;
    m_filename                  = global_filename;
    m_jack_session_uuid         = global_jack_session_uuid;
    m_last_used_dir             = global_last_used_dir;
    m_config_directory          = global_config_directory;
    m_config_filename           = global_config_filename;
    m_user_filename             = global_user_filename;
    m_config_filename_alt       = global_config_filename_alt;
    m_user_filename_alt         = global_user_filename_alt;
}

/**
 *  An internal function to ensure that the ~/.config/sequencer64
 *  directory exists.  This function is actually a little more general
 *  than that, but it is not sufficiently general, in general.
 *
 * \param pathname
 *      Provides the name of the path to create.  The parent directory of
 *      the final directory must already exist.
 *
 * \return
 *      Returns true if the path-name exists.
 */

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

bool
rc_settings::make_directory (const std::string & pathname) const
{
    bool result = ! pathname.empty();
    if (result)
    {
        static struct stat st =
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 /* and more for Linux! */
        };
        if (stat(pathname.c_str(), &st) == -1)
        {
            int rcode = mkdir(pathname.c_str(), 0700);
            result = rcode == 0;
        }
    }
    return result;
}

/**
 *  Provides the directory for the configuration file, and also creates the
 *  directory if necessary.
 *
 *  If the legacy format is in force, then the home directory for the
 *  configuration is (in Linux) "/home/username", and the configuration file
 *  is ".seq24rc".
 *
 *  If the new format is in force, then the home directory is (in Linux)
 *  "/home/username/.config/sequencer64", and the configuration file is
 *  "sequencer64.rc".
 *
 * \return
 *      Returns the selection home configuration directory.  If it does not
 *      exist orcould not be created, then an empty string is returned.
 */

std::string
rc_settings::home_config_directory () const
{
    std::string result;
    char * env = getenv(HOME);
    if (env != NULL)
    {
        std::string home(getenv(HOME));
        result = home + SLASH;                      /* e.g. /home/username/  */
        if (! g_rc_settings.legacy_format())
        {
            result += config_directory();           /* new, longer directory */
            bool ok = make_directory(result);
            if (! ok)
            {
                printf("? error creating [%s]\n", result.c_str());
                result.clear();
            }
        }
    }
    else
        printf("? error calling getenv(\"%s\")\n", HOME);

    return result;
}

/**
 *  Constructs the full path and file specification for the "rc" file
 *  based on whether or not the legacy Seq24 filenames are being used.
 */

std::string
rc_settings::config_filespec () const
{
    std::string result = home_config_directory();
    if (! result.empty())
    {
        result += SLASH;
        if (g_rc_settings.legacy_format())
            result += config_filename_alt();
        else
            result += config_filename();
    }
    return result;
}

/**
 *  Constructs the full path and file specification for the "user" file
 *  based on whether or not the legacy Seq24 filenames are being used.
 */

std::string
rc_settings::user_filespec () const
{
    std::string result = home_config_directory();
    if (! result.empty())
    {
        result += SLASH;
        if (g_rc_settings.legacy_format())
            result += user_filename_alt();
        else
            result += user_filename();
    }
    return result;
}

/**
 * \setter m_device_ignore_num
 *      However, please note that this value, while set in the options
 *      processing of the main module, does not appear to be used anywhere
 *      in the code in seq24, Sequencer24, and this application.
 */

void
rc_settings::device_ignore_num (int value)
{
    if (value >= 0)
        m_device_ignore_num = value;
}

/**
 * \setter m_interaction_method
 */

void
rc_settings::interaction_method (interaction_method_t value)
{
    switch (value)
    {
    case e_seq24_interaction:
    case e_fruity_interaction:

        m_interaction_method = value;
        break;

    default:
        errprint("illegal interaction-method value");
        break;
    }
}

/**
 * \setter m_filename
 */

void
rc_settings::filename (const std::string & value)
{
    if (! value.empty())
        m_filename = value;
}

/**
 * \setter m_jack_session_uuid
 */

void
rc_settings::jack_session_uuid (const std::string & value)
{
    if (! value.empty())
        m_jack_session_uuid = value;
}

/**
 * \setter m_last_used_dir
 */

void
rc_settings::last_used_dir (const std::string & value)
{
    if (! value.empty())
        m_last_used_dir = value;
}

/**
 * \setter m_config_directory
 */

void
rc_settings::config_directory (const std::string & value)
{
    if (! value.empty())
        m_config_directory = value;
}

/**
 * \setter m_config_filename
 */

void
rc_settings::config_filename (const std::string & value)
{
    if (! value.empty())
        m_config_filename = value;
}

/**
 * \setter m_user_filename
 */

void
rc_settings::user_filename (const std::string & value)
{
    if (! value.empty())
        m_user_filename = value;
}

/**
 * \setter m_config_filename_alt;
 */

void
rc_settings::config_filename_alt (const std::string & value)
{
    if (! value.empty())
        m_config_filename_alt = value;
}

/**
 * \setter m_user_filename_alt
 */

void
rc_settings::user_filename_alt (const std::string & value)
{
    if (! value.empty())
        m_user_filename_alt = value;
}

}           // namespace seq64

/*
 * rc_settings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

