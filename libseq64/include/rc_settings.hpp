#ifndef SEQ64_RC_SETTINGS_HPP
#define SEQ64_RC_SETTINGS_HPP

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
 * \file          rc_settings.hpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-22
 * \updates       2016-07-30
 * \license       GNU GPLv2 or above
 *
 *  This collection of variables describes the options of the application,
 *  accessible from the command-line or from the "rc" file.
 *
 * \warning
 *      We're making the "statistics" support a configure-time option.  The
 *      run-time option will be left here, but the actual usage of it will be
 *      disabled unless configured with the --enable-statistics option.
 */

#include <string>

namespace seq64
{

/**
 *  Provides codes for the mouse-handling used by the application.  Moved
 *  here from the globals.h module.
 */

enum interaction_method_t
{
    e_seq24_interaction,
    e_fruity_interaction,
    e_number_of_interactions    // keep this one last... a "size" value
};

/**
 *  This class contains the options formerly named "global_xxxxxx".
 */

class rc_settings
{

private:

    bool m_auto_option_save;
    bool m_legacy_format;
    bool m_lash_support;
    bool m_allow_mod4_mode;
    bool m_show_midi;
    bool m_priority;
    bool m_stats;
    bool m_pass_sysex;
    bool m_with_jack_transport;
    bool m_with_jack_master;
    bool m_with_jack_master_cond;
    bool m_jack_start_mode;
    bool m_manual_alsa_ports;
    bool m_reveal_alsa_ports;
    bool m_print_keys;
    bool m_device_ignore;                       /* seq24 module, unused!    */
    int m_device_ignore_num;                    /* seq24 module, unused!    */
    interaction_method_t m_interaction_method;

    /**
     *  Provides the name of current MIDI file.
     */

    std::string m_filename;

    std::string m_jack_session_uuid;
    std::string m_last_used_dir;
    std::string m_config_directory;
    std::string m_config_filename;
    std::string m_user_filename;
    std::string m_config_filename_alt;
    std::string m_user_filename_alt;

public:

    rc_settings ();
    rc_settings (const rc_settings & rhs);
    rc_settings & operator = (const rc_settings & rhs);

    std::string config_filespec () const;
    std::string user_filespec () const;
    void set_defaults ();

    /**
     * \accessor m_auto_option_save
     */

    bool auto_option_save () const
    {
        return m_auto_option_save;
    }

    void auto_option_save (bool flag)
    {
        m_auto_option_save = flag;
    }

    /**
     * \accessor m_legacy_format
     */

    bool legacy_format () const
    {
        return m_legacy_format;
    }

    void legacy_format (bool flag)
    {
        m_legacy_format = flag;
    }

    /**
     * \accessor m_lash_support
     */

    bool lash_support () const
    {
        return m_lash_support;
    }

    void lash_support (bool flag)
    {
        m_lash_support = flag;
    }

    /**
     * \accessor m_allow_mod4_mode
     */

    bool allow_mod4_mode () const
    {
        return m_allow_mod4_mode;
    }

    void allow_mod4_mode (bool flag)
    {
        m_allow_mod4_mode = flag;
    }

    /**
     * \accessor m_show_midi
     */

    bool show_midi () const
    {
        return m_show_midi;
    }

    void show_midi (bool flag)
    {
        m_show_midi = flag;
    }

    /**
     * \accessor m_priority
     */

    bool priority () const
    {
        return m_priority;
    }

    void priority (bool flag)
    {
        m_priority = flag;
    }

    /**
     * \accessor m_stats
     */

    bool stats () const
    {
        return m_stats;
    }

    void stats (bool flag)
    {
        m_stats = flag;
    }

    /**
     * \accessor m_pass_sysex
     */

    bool pass_sysex () const
    {
        return m_pass_sysex;
    }

    void pass_sysex (bool flag)
    {
        m_pass_sysex = flag;
    }

    /**
     * \accessor m_with_jack_transport
     */

    bool with_jack_transport () const
    {
        return m_with_jack_transport;
    }

    void with_jack_transport (bool flag)
    {
        m_with_jack_transport = flag;
    }

    /**
     * \accessor m_with_jack_master
     */

    bool with_jack_master () const
    {
        return m_with_jack_master;
    }

    void with_jack_master (bool flag)
    {
        m_with_jack_master = flag;
    }

    /**
     * \accessor m_with_jack_master_cond
     */

    bool with_jack_master_cond () const
    {
        return m_with_jack_master_cond;
    }

    void with_jack_master_cond (bool flag)
    {
        m_with_jack_master_cond = flag;
    }

    /**
     * \accessor m_with_jack_transport m_with_jack_master, and
     * m_with_jack_master_cond, to save client code some trouble.
     */

    bool with_jack () const
    {
        return
        (
            m_with_jack_transport || m_with_jack_master || m_with_jack_master_cond
        );
    }

    /**
     * \accessor m_jack_start_mode,
     */

    bool jack_start_mode () const
    {
        return m_jack_start_mode;
    }

    void jack_start_mode (bool flag)
    {
        m_jack_start_mode = flag;
    }

    /**
     * \accessor m_manual_alsa_ports
     */

    bool manual_alsa_ports () const
    {
        return m_manual_alsa_ports;
    }

    void manual_alsa_ports (bool flag)
    {
        m_manual_alsa_ports = flag;
    }

    /**
     * \accessor m_reveal_alsa_ports
     */

    bool reveal_alsa_ports () const
    {
        return m_reveal_alsa_ports;
    }

    void reveal_alsa_ports (bool flag)
    {
        m_reveal_alsa_ports = flag;
    }

    /**
     * \accessor m_print_keys
     */

    bool print_keys () const
    {
        return m_print_keys;
    }

    void print_keys (bool flag)
    {
        m_print_keys = flag;
    }

    /**
     * \accessor m_device_ignore
     */

    bool device_ignore () const
    {
        return m_device_ignore;
    }

    void device_ignore (bool flag)
    {
        m_device_ignore = flag;
    }

    /**
     * \getter m_device_ignore_num
     */

    int device_ignore_num () const
    {
        return m_device_ignore_num;
    }

    /**
     * \getter m_interaction_method
     */

    interaction_method_t interaction_method () const
    {
        return m_interaction_method;
    }

    /**
     * \getter m_filename
     */

    const std::string & filename () const
    {
        return m_filename;
    }

    /**
     * \getter m_jack_session_uuid
     */

    const std::string & jack_session_uuid () const
    {
        return m_jack_session_uuid;
    }

    /**
     * \getter m_last_used_dir
     */

    const std::string & last_used_dir () const
    {
        return m_last_used_dir;
    }

    /**
     * \getter m_config_directory
     */

    const std::string & config_directory () const
    {
        return m_config_directory;
    }

    /**
     * \getter m_config_filename
     */

    const std::string & config_filename () const
    {
        return m_config_filename;
    }

    /**
     * \getter m_user_filename
     */

    const std::string & user_filename () const
    {
        return m_user_filename;
    }

    /**
     * \getter m_config_filename_alt;
     */

    const std::string & config_filename_alt () const
    {
        return m_config_filename_alt;
    }

    /**
     * \getter m_user_filename_alt
     */

    const std::string & user_filename_alt () const
    {
        return m_user_filename_alt;
    }

    /*
     * The setters for non-bool values, defined in the cpp file because
     * they do some validation.
     */

    void device_ignore_num (int value);
    void interaction_method (interaction_method_t value);
    void filename (const std::string & value);
    void jack_session_uuid (const std::string & value);
    void last_used_dir (const std::string & value);
    void config_directory (const std::string & value);
    void set_config_files (const std::string & value);
    void config_filename (const std::string & value);
    void user_filename (const std::string & value);
    void config_filename_alt (const std::string & value);
    void user_filename_alt (const std::string & value);

private:

    std::string home_config_directory () const;

};

}           // namespace seq64

#endif      // SEQ64_RC_SETTINGS_HPP

/*
 * rc_settings.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

