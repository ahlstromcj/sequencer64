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
 * \updates       2016-08-17
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
    e_seq24_interaction,            /**< Use the normal mouse interactions. */
    e_fruity_interaction,           /**< The "fruity" mouse interactions.   */
    e_number_of_interactions        /**< Keep this last... a size value.    */
};

/**
 *  This class contains the options formerly named "global_xxxxxx".  It gives
 *  us a whole lot more encapsulation and control over how the options of the
 *  "rc" file (optionsfile) are set and used.  Note that this class does not
 *  support the hot-keys options; those are handled in the keys_perform class.
 */

class rc_settings
{
    friend class optionsfile;
    friend class options;
    friend class mainwnd;
    friend int parse_command_line_options (int argc, char * argv []);
    friend bool help_check (int argc, char * argv []);

private:

    /*
     * Much more complete descriptions of these options can be found in the
     * sequencer64.rc file.
     */

    bool m_auto_option_save;        /**< [auto-option-save] setting.        */
    bool m_legacy_format;           /**< Write files in legacy format.      */
    bool m_lash_support;            /**< Enable LASH, if compiled in.       */
    bool m_allow_mod4_mode;         /**< Allow Mod4 to hold drawing mode.   */
    bool m_allow_snap_split;        /**< Allow snap-split of a trigger.     */
    bool m_show_midi;               /**< Show MIDI events to console.       */
    bool m_priority;                /**< Run at high priority (Linux only). */
    bool m_stats;                   /**< Show some output statistics.       */
    bool m_pass_sysex;              /**< Pass SysEx to outputs, not ready.  */
    bool m_with_jack_transport;     /**< Enable synchrony with JACK.        */
    bool m_with_jack_master;        /**< Serve as a JACK transport Master.  */
    bool m_with_jack_master_cond;   /**< Serve as JACK Master if possible.  */
    bool m_song_start_mode;         /**< True is song mode, false is live.  */
    bool m_manual_alsa_ports;       /**< [manual-alsa-ports] setting.       */
    bool m_reveal_alsa_ports;       /**< [reveal-alsa-ports] setting.       */
    bool m_print_keys;              /**< Show hot-key in main window slot.  */
    bool m_device_ignore;           /**< From seq24 module, unused!         */
    int m_device_ignore_num;        /**< From seq24 module, unused!         */
    interaction_method_t m_interaction_method;  /**< [interaction-method]   */

    /**
     *  Provides the name of current MIDI file.
     */

    std::string m_filename;

    /**
     *  Holds the JACK UUID value that makes this JACK connection unique.
     */

    std::string m_jack_session_uuid;

    /**
     *  Holds the directory from which the last MIDI file was opened (or
     *  saved).
     */

    std::string m_last_used_dir;

    /**
     *  Holds the current "rc" and "user" configuration directory.  This value
     *  is "~/.config/sequencer64" by default.
     */

    std::string m_config_directory;

    /**
     *  Holds the current "rc" configuration filename.  This value is
     *  "sequencer64.rc" by default.
     */

    std::string m_config_filename;

    /**
     *  Holds the current "user" configuration filename.  This value is
     *  "sequencer64.usr" by default.
     */

    std::string m_user_filename;

    /**
     *  Holds the legacy "rc" filename, ".seq24rc".
     */

    std::string m_config_filename_alt;

    /**
     *  Holds the legacy "user" filename, ".seq24usr".
     */

    std::string m_user_filename_alt;

public:

    rc_settings ();
    rc_settings (const rc_settings & rhs);
    rc_settings & operator = (const rc_settings & rhs);

    std::string config_filespec () const;
    std::string user_filespec () const;
    void set_defaults ();

    /**
     * \getter m_auto_option_save
     */

    bool auto_option_save () const
    {
        return m_auto_option_save;
    }

    /**
     * \getter m_legacy_format
     */

    bool legacy_format () const
    {
        return m_legacy_format;
    }

    /**
     * \getter m_lash_support
     */

    bool lash_support () const
    {
        return m_lash_support;
    }

    /**
     * \getter m_allow_mod4_mode
     */

    bool allow_mod4_mode () const
    {
        return m_allow_mod4_mode;
    }

    /**
     * \getter m_allow_snap_split
     */

    bool allow_snap_split () const
    {
        return m_allow_snap_split;
    }

    /**
     * \getter m_show_midi
     */

    bool show_midi () const
    {
        return m_show_midi;
    }

    /**
     * \getter m_priority
     */

    bool priority () const
    {
        return m_priority;
    }

    /**
     * \getter m_stats
     */

    bool stats () const
    {
        return m_stats;
    }

    /**
     * \getter m_pass_sysex
     */

    bool pass_sysex () const
    {
        return m_pass_sysex;
    }

    /**
     * \getter m_with_jack_transport
     */

    bool with_jack_transport () const
    {
        return m_with_jack_transport;
    }

    /**
     * \getter m_with_jack_master
     */

    bool with_jack_master () const
    {
        return m_with_jack_master;
    }

    /**
     * \getter m_with_jack_master_cond
     */

    bool with_jack_master_cond () const
    {
        return m_with_jack_master_cond;
    }

    /**
     * \getter m_with_jack_transport m_with_jack_master, and
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
     * \getter m_song_start_mode,
     */

    bool song_start_mode () const
    {
        return m_song_start_mode;
    }

    /**
     * \getter m_manual_alsa_ports
     */

    bool manual_alsa_ports () const
    {
        return m_manual_alsa_ports;
    }

    /**
     * \getter m_reveal_alsa_ports
     */

    bool reveal_alsa_ports () const
    {
        return m_reveal_alsa_ports;
    }

    /**
     * \getter m_print_keys
     */

    bool print_keys () const
    {
        return m_print_keys;
    }

    /**
     * \getter m_device_ignore
     */

    bool device_ignore () const
    {
        return m_device_ignore;
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

protected:

    /**
     * \setter m_auto_option_save
     */

    void auto_option_save (bool flag)
    {
        m_auto_option_save = flag;
    }

    /**
     * \setter m_legacy_format
     */

    void legacy_format (bool flag)
    {
        m_legacy_format = flag;
    }

    /**
     * \setter m_lash_support
     */

    void lash_support (bool flag)
    {
        m_lash_support = flag;
    }

    /**
     * \setter m_allow_mod4_mode
     */

    void allow_mod4_mode (bool flag)
    {
        m_allow_mod4_mode = flag;
    }

    /**
     * \setter m_allow_snap_split
     */

    void allow_snap_split (bool flag)
    {
        m_allow_snap_split = flag;
    }

    /**
     * \setter m_show_midi
     */

    void show_midi (bool flag)
    {
        m_show_midi = flag;
    }

    /**
     * \setter m_priority
     */

    void priority (bool flag)
    {
        m_priority = flag;
    }

    /**
     * \setter m_stats
     */

    void stats (bool flag)
    {
        m_stats = flag;
    }

    /**
     * \setter m_pass_sysex
     */

    void pass_sysex (bool flag)
    {
        m_pass_sysex = flag;
    }

    /**
     * \setter m_with_jack_transport
     */

    void with_jack_transport (bool flag)
    {
        m_with_jack_transport = flag;
    }

    /**
     * \setter m_with_jack_master
     */

    void with_jack_master (bool flag)
    {
        m_with_jack_master = flag;
    }

    /**
     * \setter m_with_jack_master_cond
     */

    void with_jack_master_cond (bool flag)
    {
        m_with_jack_master_cond = flag;
    }

    /**
     * \setter m_song_start_mode,
     */

    void song_start_mode (bool flag)
    {
        m_song_start_mode = flag;
    }

    /**
     * \setter m_manual_alsa_ports
     */

    void manual_alsa_ports (bool flag)
    {
        m_manual_alsa_ports = flag;
    }

    /**
     * \setter m_reveal_alsa_ports
     */

    void reveal_alsa_ports (bool flag)
    {
        m_reveal_alsa_ports = flag;
    }

    /**
     * \setter m_print_keys
     */

    void print_keys (bool flag)
    {
        m_print_keys = flag;
    }

    /**
     * \setter m_device_ignore
     */

    void device_ignore (bool flag)
    {
        m_device_ignore = flag;
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

