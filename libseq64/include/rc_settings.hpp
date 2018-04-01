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
 * \updates       2018-03-30
 * \license       GNU GPLv2 or above
 *
 *  This collection of variables describes the options of the application,
 *  accessible from the command-line or from the "rc" file.
 *
 * \warning
 *      We're making the "statistics" support a configure-time option.  The
 *      run-time option will be left here, but the actual usage of it will be
 *      disabled unless configured with the --enable-statistics option.
 *
 * \todo
 *      Consolidate the usr and rc settings classes, or at least have a base
 *      class for common elements like "[comments]".
 */

#include <string>

#include "seq64_features.h"             /* SEQ64_USE_ZOOM_POWER_OF_2    */
#include "recent.hpp"                   /* seq64::recent class          */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

class perform;                          /* forward declaration          */

/**
 *  Provides mutually-exclusive codes for the mouse-handling used by the
 *  application.  Moved here from the globals.h module.
 */

enum interaction_method_t
{
    e_seq24_interaction,            /**< Use the normal mouse interactions. */
    e_fruity_interaction,           /**< The "fruity" mouse interactions.   */
    e_number_of_interactions        /**< Keep this last... a size value.    */
};

/**
 *  Provides mutually-exclusive codes for handling the reading of mute-groups
 *  from the "rc" file versus the "MIDI" file.  There's no GUI way to set this
 *  item yet.
 *
 *  e_mute_group_stomp:
 *  This is the legacy (seq24) option, which reads the mute-groups from the
 *  MIDI file, and saves them back to the "rc" file and to the MIDI file.
 *  However, for Sequencer64 MIDI files such as b4uacuse-stress.midi, seq24
 *  never reads the mute-groups in that MIDI file!  In any case, this can be
 *  considered a corruption of the "rc" file.
 *
 *  e_mute_group_preserve:
 *  In this option, the mute groups are only written to the "rc" file if the
 *  MIDI file did not contain non-zero mute groups.  This option prevents the
 *  contamination of the "rc" mute-groups by the MIDI file's mute-groups.
 *  We're going to make this the default option.
 */

enum mute_group_handling_t
{
    e_mute_group_stomp,         /**< Save main group to "rc"/MIDI files.    */
    e_mute_group_preserve,      /**< Write new groups only to MIDI file.    */
    e_mute_group_max            /**< Keep this last... a size value.        */
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
    friend class rtmidi_info;
    friend int parse_command_line_options (perform &, int , char * []);
    friend bool help_check (int, char * []);

private:

    /**
     *  [comments]
     *
     *  Provides a way to embed comments in the "rc" file and not lose
     *  them when the "rc" file is auto-saved.
     */

    std::string m_comments_block;

    /*
     * Much more complete descriptions of these options can be found in the
     * sequencer64.rc file.
     */

    bool m_verbose_option;          /**< [auto-option-save] setting.        */
    bool m_auto_option_save;        /**< [auto-option-save] setting.        */
    bool m_legacy_format;           /**< Write files in legacy format.      */
    bool m_lash_support;            /**< Enable LASH, if compiled in.       */
    bool m_allow_mod4_mode;         /**< Allow Mod4 to hold drawing mode.   */
    bool m_allow_snap_split;        /**< Allow snap-split of a trigger.     */
    bool m_allow_click_edit;        /**< Allow double-click edit pattern.   */
    bool m_show_midi;               /**< Show MIDI events to console.       */
    bool m_priority;                /**< Run at high priority (Linux only). */
    bool m_stats;                   /**< Show some output statistics.       */
    bool m_pass_sysex;              /**< Pass SysEx to outputs, not ready.  */
    bool m_with_jack_transport;     /**< Enable synchrony with JACK.        */
    bool m_with_jack_master;        /**< Serve as a JACK transport Master.  */
    bool m_with_jack_master_cond;   /**< Serve as JACK Master if possible.  */
    bool m_with_jack_midi;          /**< Use JACK MIDI.                     */
    bool m_filter_by_channel;       /**< Record only sequence channel data. */
    bool m_manual_alsa_ports;       /**< [manual-alsa-ports] setting.       */
    bool m_reveal_alsa_ports;       /**< [reveal-alsa-ports] setting.       */
    bool m_print_keys;              /**< Show hot-key in main window slot.  */
    bool m_device_ignore;           /**< From seq24 module, unused!         */
    int m_device_ignore_num;        /**< From seq24 module, unused!         */
    interaction_method_t m_interaction_method;  /**< [interaction-method]   */
    mute_group_handling_t m_mute_group_saving;  /**< Handling of mutes.     */

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
     *  "sequencer64.rc" by default.
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

    /**
     *  Holds the application name, e.g. "sequencer64", "seq64portmidi", or
     *  "seq64".  This is a constant, set to SEQ64_APP_NAME.  Also see the
     *  seq_app_name() function.
     */

    const std::string m_application_name;

    /**
     *  Holds the client name for the application.  This is much like the
     *  application name, but in the future will be a configuration option.
     *  For now it is just the value of the SEQ64_CLIENT_NAME macro.  Also see
     *  the seq_client_name() function.
     */

    std::string m_app_client_name;

    /**
     *  New value to allow the user to violate the MIDI specification and use a
     *  track other than the first track (#0) as the MIDI tempo track.
     */

    int m_tempo_track_number;

    /**
     *  Holds a few MIDI file-names most recently used.  Although this is a
     *  vector, we do not let it grow past SEQ64_RECENT_FILES_MAX.
     *
     *  New feature from Oli Kester's kepler34 project.
     *
     * std::vector<std::string> m_recent_files;
     */

    recent m_recent_files;

public:

    rc_settings ();
    rc_settings (const rc_settings & rhs);
    rc_settings & operator = (const rc_settings & rhs);

    std::string config_filespec () const;
    std::string user_filespec () const;
    void set_defaults ();

    /**
     * \getter m_comments_block
     */

    const std::string & comments_block () const
    {
        return m_comments_block;
    }

    /**
     * \setter m_comments_block
     */

    void clear_comments ()
    {
        m_comments_block.clear();
    }

    /**
     * \setter m_comments_block
     */

    void append_comment_line (const std::string & line)
    {
        m_comments_block += line;
    }

    /**
     * \getter m_verbose_option
     */

    bool verbose_option () const
    {
        return m_verbose_option;
    }

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
     * \getter m_allow_click_edit
     */

    bool allow_click_edit () const
    {
        return m_allow_click_edit;
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
     * \getter m_with_jack_midi
     */

    bool with_jack_midi () const
    {
        return m_with_jack_midi;
    }

    void with_jack_transport (bool flag);
    void with_jack_master (bool flag);
    void with_jack_master_cond (bool flag);

    /**
     * \getter m_with_jack_transport m_with_jack_master, and
     * m_with_jack_master_cond, to save client code some trouble.  Do not
     * confuse these original options with the new "no JACK MIDI" option.
     */

    bool with_jack () const
    {
        return
        (
            m_with_jack_transport || m_with_jack_master || m_with_jack_master_cond
        );
    }

    /**
     * \getter m_filter_by_channel
     */

    bool filter_by_channel () const
    {
        return m_filter_by_channel;
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
     * \getter m_mute_group_saving
     */

    mute_group_handling_t mute_group_saving () const
    {
        return m_mute_group_saving;
    }

    /**
     * \getter m_filename
     */

    const std::string & filename () const
    {
        return m_filename;
    }

    void filename (const std::string & value);

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

    void last_used_dir (const std::string & value);

    /**
     * \setter m_recent_files
     *
     *  First makes sure the filename is not already present, and removes
     *  the back entry from the list, if it is full (SEQ64_RECENT_FILES_MAX)
     *  before adding
     *  it.
     *
     *  Now the full pathname is added.
     *
     * \param fname
     *      Provides the full path to the MIDI file that is to be added to
     *      the recent-files list.
     *
     * \return
     *      Returns true if the file-name was able to be added.
     */

    bool add_recent_file (const std::string & filename)
    {
        return m_recent_files.add(filename);
    }

    /**
     *
     */

    bool append_recent_file (const std::string & filename)
    {
        return m_recent_files.append(filename);
    }

    /**
     *
     */

    bool remove_recent_file (const std::string & filename)
    {
        return m_recent_files.remove(filename);
    }

    /**
     * \getter m_config_directory
     */

    const std::string & config_directory () const
    {
        return m_config_directory;
    }

    std::string home_config_directory () const;
    void set_config_files (const std::string & value);

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

    /**
     * \getter m_application_name
     */

    const std::string application_name () const
    {
        return m_application_name;
    }

    /**
     * \getter m_app_client_name
     */

    const std::string & app_client_name () const
    {
        return m_app_client_name;
    }

    /**
     * \getter m_tempo_track_number
     */

    int tempo_track_number () const
    {
        return m_tempo_track_number;
    }

    std::string recent_file (int index, bool shorten = true) const;

    /**
     * \getter m_recent_files.size()
     */

    int recent_file_count () const
    {
        return m_recent_files.count();
    }

protected:

    /**
     * \setter m_verbose_option
     */

    void verbose_option (bool flag)
    {
        m_verbose_option = flag;
    }

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
     * \setter m_allow_click_edit
     */

    void allow_click_edit (bool flag)
    {
        m_allow_click_edit = flag;
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
     * \setter m_with_jack_midi
     */

    void with_jack_midi (bool flag)
    {
        m_with_jack_midi = flag;
    }

    /**
     * \setter m_filter_by_channel
     */

    void filter_by_channel (bool flag)
    {
        m_filter_by_channel = flag;
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
     * they do some heavier validation.
     */

    void tempo_track_number (int track);
    void device_ignore_num (int value);
    bool interaction_method (interaction_method_t value);
    bool mute_group_saving (mute_group_handling_t mgh);
    void jack_session_uuid (const std::string & value);
    void config_directory (const std::string & value);
    void config_filename (const std::string & value);
    void user_filename (const std::string & value);
    void config_filename_alt (const std::string & value);
    void user_filename_alt (const std::string & value);

};          // class rc_settings

}           // namespace seq64

#endif      // SEQ64_RC_SETTINGS_HPP

/*
 * rc_settings.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

