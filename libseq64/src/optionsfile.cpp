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
 * \file          optionsfile.cpp
 *
 *  This module declares/defines the base class for managing the <code>
 *  ~/.seq24rc </code> legacy configuration file or the new <code>
 *  ~/.config/sequencer64/sequencer64.rc </code> ("rc") configuration file.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-11-03
 * \license       GNU GPLv2 or above
 *
 *  The <code> ~/.seq24rc </code> or <code> ~/.config/sequencer64/sequencer64.rc
 *  </code> configuration file is fairly simple in layout.  The documentation
 *  for this module is supplemented by the following GitHub projects:
 *
 *      -   https://github.com/ahlstromcj/seq24-doc.git (legacy support)
 *      -   https://github.com/ahlstromcj/sequencer64-doc.git
 *
 *  Those documents also relate these file settings to the application's
 *  command-line options.
 *
 *  Note that these options are primarily read/written from/to the perform
 *  object that is passed to the parse() and write() functions.
 *
 *  Also note that the parse() and write() functions process sections in a
 *  different order!  The reason this does not mess things up is that the
 *  line_after() function always rescans from the beginning of the file.  As
 *  long as each section's sub-values are read and written in the same order,
 *  there will be no problem.
 *
 * Fixups:
 *
 *      As of version 0.9.11, a "Pause" key is added.  One must fix up the
 *      sequencer64.rc file.  First, run Sequencer64.  Then open File /
 *      Options, and go to the Keyboard tab.  Fix the Start, Stop, and Pause
 *      fields as desired.  The recommended character for Pause is the period
 *      (".").
 *
 *      Or better yet, add a Pause line to the sequencer.rc file after the
 *      "stop sequencer" line:
 *
 *      46   # period pause sequencer
 *
 *  User jean-emmanuel added a new MIDI control for setting the screen-set
 *  directly by number.
 */

#include <string.h>                     /* memset()                         */

#include "file_functions.hpp"           /* strip_quotes() function          */
#include "gdk_basic_keys.h"             /* SEQ64_equal, SEQ64_minus         */
#include "midibus.hpp"
#include "optionsfile.hpp"
#include "perform.hpp"
#include "settings.hpp"                 /* seq64::rc()                      */

/**
 *  Provides names for the mouse-handling used by the application.
 */

static const char * const c_interaction_method_names[3] =
{
    "seq24",
    "fruity",
    NULL
};

/**
 *  Provides descriptions for the mouse-handling used by the application.
 */

static const char * const c_interaction_method_descs[3] =
{
    "original seq24 method",
    "similar to a certain fruity sequencer we like",
    NULL
};

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Principal constructor.
 *
 * \param name
 *      Provides the name of the options file; this is usually a full path
 *      file-specification.
 */

optionsfile::optionsfile (const std::string & name)
 :
    configfile  (name)               // base class constructor
{
    // Empty body
}

/**
 *  A rote destructor.
 */

optionsfile::~optionsfile ()
{
    // Empty body
}

/**
 *  Helper function for error-handling.  It assembles a message and then
 *  passes it to set_error_message().
 *
 * \param sectionname
 *      Provides the name of the section for reporting the error.
 *
 * \param additional
 *      Additional context information to help in finding the error.
 *
 * \return
 *      Always returns false.
 */

bool
optionsfile::make_error_message
(
    const std::string & sectionname,
    const std::string & additional
)
{
    std::string msg = "BAD OR MISSING DATA in [";
    msg += sectionname;
    msg += "]: ";
    if (! additional.empty())
        msg += additional;

    errprint(msg.c_str());
    set_error_message(msg);
    return false;
}

/**
 *  Parse the ~/.seq24rc or ~/.config/sequencer64/sequencer64.rc file.
 *
 *  [midi-control]
 *
 *  Get the number of sequence definitions provided in the [midi-control]
 *  section.  Ranges from 32 on up.  Then read in all of the sequence
 *  lines.  The first 32 apply to the first screen set.  There can also be
 *  a comment line "# mute in group" followed by 32 more lines.  Then
 *  there are additional comments and single lines for BPM up, BPM down,
 *  Screen Set Up, Screen Set Down, Mod Replace, Mod Snapshot, Mod Queue,
 *  Mod Gmute, Mod Glearn, and Screen Set Play.  These are all forms of
 *  MIDI automation useful to control the playback while not sitting near
 *  the computer.
 *
 *  [midi-control-file]
 *
 *  If this section is present, the [midi-control] section is ignored, even
 *  if present, in favor of reading the MIDI control information from a
 *  separate file.  This allows the user to switch between different setups
 *  without having to mess with editing the "rc" file much.
 *
 *  Then next data line after this section tag should be a filename.  If there
 *  is none, or if it is set to "", then the [midi-control] section is used,
 *  if present.  If neither are present, this is a fatal error.
 *
 *  [mute-group]
 *
 *  The mute-group starts with a line that indicates up to 32 mute-groups
 *  are defined. A common value is 1024, which means there are 32 groups
 *  times 32 keys.  But this value is currently thrown away.  This value
 *  is followed by 32 lines of data, each contained 4 sets of 8 settings.
 *  See the seq24-doc project on GitHub for a much more detailed
 *  description of this section.
 *
 *  [midi-clock]
 *
 *  The MIDI-clock section defines the clocking value for up to 16 output
 *  busses.  The first number, 16, indicates how many busses are
 *  specified.  Generally, these busses are shown to the user with names
 *  such as "[1] seq24 1".
 *
 *  [keyboard-control]
 *
 *  The keyboard control defines the keys that will toggle the state of
 *  each of up to 32 patterns in a pattern/sequence box.  These keys are
 *  displayed in each box as a reminder.  The first number specifies the
 *  Key number, and the second number specifies the Sequence number.
 *
 *  [keyboard-group]
 *
 *  The keyboard group specifies more automation for the application.
 *  The first number specifies the Key number, and the second number
 *  specifies the Group number.  This section should be better described
 *  in the sequencer64-doc project on GitHub.
 *
 *  [extended-keys]
 *
 *  Additional keys (not yet represented in the Options dialog) to support
 *  additional keys for tempo-tapping, Seq32's new transport and connection
 *  functionality, and maybe a little more.
 *
 *  [New-keys]
 *
 *  Conditional support for reading Seq32 "rc" files.
 *
 *  [jack-transport]
 *
 *  This section covers various JACK settings, one setting per line.  In
 *  order, the following numbers are specfied:
 *
 *      -   jack_transport - Enable sync with JACK Transport.
 *      -   jack_master - Seq24 will attempt to serve as JACK Master.
 *      -   jack_master_cond - Seq24 will fail to be Master if there is
 *          already a Master set.
 *      -   song_start_mode:
 *          -   0 = Playback will be in Live mode.  Use this to allow
 *              muting and unmuting of loops.
 *          -   1 = Playback will use the Song Editor's data.
 *
 *  [midi-input]
 *
 *  This section covers the MIDI input busses, and has a format similar to
 *  "[midi-clock]".  Generally, these busses are shown to the user with
 *  names such as "[1] seq24 1", and currently there is only one input
 *  buss.  The first field is the port number, and the second number
 *  indicates whether it is disabled (0), or enabled (1).
 *
 *  [midi-clock-mod-ticks]
 *
 *  This section covers....  One common value is 64.
 *
 *  [manual-alsa-ports]
 *
 *  Set to 1 if you want seq24 to create its own ALSA ports and not
 *  connect to other clients.
 *
 *  [last-used-dir]
 *
 *  This section simply holds the last path-name that was used to read or
 *  write a MIDI file.  We still need to add a check for a valid path, and
 *  currently the path must start with a "/", so it is not suitable for
 *  Windows.
 *
 *  [interaction-method]
 *
 *  This section specified the kind of mouse interaction.
 *
 *  -   0 = 'seq24' (original Seq24 method).
 *  -   1 = 'fruity' (similar to a certain fruity sequencer we like).
 *
 *  The second data line is set to "1" if Mod4 can be used to keep seq24
 *  in note-adding mode even after the right-click is released, and "0"
 *  otherwise.
 *
 * \param p
 *      Provides the performance object to which all of these options apply.
 *
 * \return
 *      Returns true if the file was able to be opened for reading.
 *      Currently, there is no indication if the parsing actually succeeded.
 */

bool
optionsfile::parse (perform & p)
{
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        printf("? error opening [%s] for reading\n", name().c_str());
        return false;
    }
    file.seekg(0, std::ios::beg);                           /* seek to start */

    /*
     * [comments]
     *
     * Header commentary is skipped during parsing.  However, we now try to
     * read an optional comment block.
     */

    if (line_after(file, "[comments]"))                 /* gets first line  */
    {
        rc().clear_comments();
        do
        {
            rc().append_comment_line(m_line);
            rc().append_comment_line("\n");

        } while (next_data_line(file));
    }
    bool ok = true;                                     /* start hopefully! */
    if (line_after(file, "[midi-control-file]"))
    {
        std::string fullpath;
        std::string line = m_line;
        std::string filename = strip_comments(line);
        filename = strip_quotes(filename);
        ok = ! filename.empty();
        if (ok)
        {
            rc().midi_control_filename(filename);       /* base file-name   */
            fullpath = rc().midi_control_filespec();    /* full path-spec   */
            printf("[Reading rc MIDI control file %s]\n", fullpath.c_str());
            ok = parse_midi_control_section(fullpath, p);
            if (! ok)
            {
                std::string info = "cannot parse file '";
                info += fullpath;
                info += "'";
                return make_error_message("midi-control-file", info);
            }
        }
        rc().use_midi_control_file(ok);                 /* did it work?     */
        rc().midi_control_filename(ok ? filename : ""); /* base file-name   */
    }
    else
        rc().use_midi_control_file(false);

   if (! rc().use_midi_control_file())
   {
        /*
         * This call causes parsing to skip all of the header material.
         * Please note that the line_after() function always starts from the
         * beginning of the file every time.  A lot a rescanning!  But it goes
         * fast these days.
         */

        ok = parse_midi_control_section(name(), p);
    }

    /*
     * [mute-group] plus some additional data about how to save them.  After
     * we parse the mute group, we need to see if there is another value for
     * the mute_group_handling_t enumeration.  One little issue... the
     * parse_mute_group_section() function actually re-opens the file itself,
     * and once it exits, it's as if the section never existed.  So we also
     * have to pase the new mute-group handling feature there as well.
     */

    if (ok)
        ok = parse_mute_group_section(p);

    if (ok)
        ok = line_after(file, "[midi-clock]");

    long buses = 0;
    if (ok)
    {
        sscanf(m_line, "%ld", &buses);
        ok = next_data_line(file) && buses > 0 && buses <= SEQ64_DEFAULT_BUSS_MAX;
    }
    if (ok)
    {
        /**
         * One thing about MIDI clock values.  If a device (e.g. my Korg
         * nanoKEY2) is present in a system when Sequencer64 is exited, it
         * will be saved in the [midi-clock] list.  When unplugged, it will be
         * read here at startup, but won't be shown.  The next exit will find
         * it removed from this list.
         *
         * Also, we want to pre-allocate the number of clock entries needed,
         * and then use the buss number to populate the list of clocks, in the
         * odd event that the user changed the bus-order of the entries.
         */

        p.preallocate_clocks(buses);
        for (int i = 0; i < buses; ++i)
        {
            int bus_on;
            int bus;
            sscanf(m_line, "%d %d", &bus, &bus_on);

            /*
             *  The first call ignores the bus number that was read.  The
             *  second call indirectly accesses the mastermidibus, which does
             *  not exist yet.
             *
             *      p.add_clock(static_cast<clock_e>(bus_on));
             *      p.set_clock_bus(bus, static_cast<clock_e>(bus_on));
             */

            p.set_clock(bus, static_cast<clock_e>(bus_on));
            ok = next_data_line(file);
            if (! ok)
            {
                if (i < (buses-1))
                    return make_error_message("midi-clock data line missing");
            }
        }
    }
    else
    {
        /*
         *  If this is zero, we need to fake it to have 1 buss with a 0 clock,
         *  rather than make the poor user figure out how to fix it.
         *
         *      return make_error_message("midi-clock");
         *
         *  And let's use the new e_clock_disabled code instead of
         *  e_clock_off.  LATER.
         */

        p.add_clock(e_clock_off);
    }

    /*
     *  We used to crap out when this section had 0 entries.  But for working
     *  with the new Qt5 implmentation, it is worthwhile to continue.  Also,
     *  we note that Kepler34 has this section commented out.
     */

    line_after(file, "[keyboard-control]");
    long keys = 0;
    sscanf(m_line, "%ld", &keys);
    ok = keys >= 0 && keys <= c_max_keys;
    if (ok && keys > 0)
        ok = next_data_line(file);

    if (ok)
    {
        if (keys == 0)
        {
            warnprint("[keyboard-control] keys = 0!");
        }
    }
    else
    {
        (void) make_error_message("keyboard-control");   // allowed to continue
    }

    /*
     * Bug involving the optionsfile and perform modules:  At the 4th or 5th
     * line of data in the "rc" file, setting this key event results in the
     * size remaining at 4, so the final size is 31.  This bug is present even
     * in seq24 r.0.9.2, and occurs only if the Keyboard options are actually
     * edited.  Also, the size of the reverse container is constant at 32.
     * Clearing the latter container as well appears to fix both bugs.
     */

    p.get_key_events().clear();
    p.get_key_events_rev().clear();
    for (int i = 0; i < keys; ++i)
    {
        long key = 0, seq = 0;
        sscanf(m_line, "%ld %ld", &key, &seq);
        p.set_key_event(key, seq);
        ok = next_data_line(file);
        if (! ok && i < (keys - 1))
            return make_error_message("keyboard-control data line");
    }

    /*
     *  Keys for Group Learn.
     *
     *  We used to crap out when this section had 0 entries.  But for working
     *  with the new Qt5 implmentation, it is worthwhile to continue.  Also,
     *  we note that Kepler34 has this section commented out.
     */

    line_after(file, "[keyboard-group]");
    long groups = 0;
    sscanf(m_line, "%ld", &groups);
    ok = groups >= 0 && groups <= c_max_keys;
    if (ok && groups > 0)
        ok = next_data_line(file);

    if (ok)
    {
        if (groups == 0)
        {
            warnprint("[keyboard-group] groups = 0!");
        }
    }
    else
    {
        (void) make_error_message("keyboard-group");     // allowed to continue
    }

    p.get_key_groups().clear();
    p.get_key_groups_rev().clear();
    for (int i = 0; i < groups; ++i)
    {
        long key = 0, group = 0;
        sscanf(m_line, "%ld %ld", &key, &group);
        p.set_key_group(key, group);
        ok = next_data_line(file);
        if (! ok && i < (groups - 1))
            return make_error_message("keyboard-group data line");
    }

    keys_perform_transfer ktx;
    memset(&ktx, 0, sizeof ktx);
    sscanf(m_line, "%u %u", &ktx.kpt_bpm_up, &ktx.kpt_bpm_dn);
    next_data_line(file);
    sscanf
    (
        m_line, "%u %u %u",
        &ktx.kpt_screenset_up,
        &ktx.kpt_screenset_dn,
        &ktx.kpt_set_playing_screenset
    );
    next_data_line(file);
    sscanf
    (
        m_line, "%u %u %u",
        &ktx.kpt_group_on,
        &ktx.kpt_group_off,
        &ktx.kpt_group_learn
    );
    next_data_line(file);
    sscanf
    (
        m_line, "%u %u %u %u %u",
        &ktx.kpt_replace,
        &ktx.kpt_queue,
        &ktx.kpt_snapshot_1,
        &ktx.kpt_snapshot_2,
        &ktx.kpt_keep_queue
    );

    int show_key = 0;
    next_data_line(file);
    sscanf(m_line, "%d", &show_key);
    ktx.kpt_show_ui_sequence_key = bool(show_key);
    next_data_line(file);
    sscanf(m_line, "%u", &ktx.kpt_start);
    next_data_line(file);
    sscanf(m_line, "%u", &ktx.kpt_stop);

    if (rc().legacy_format())               /* init "non-legacy" fields */
    {
        ktx.kpt_show_ui_sequence_number = false;
        ktx.kpt_pattern_edit = 0;
        ktx.kpt_pattern_shift = 0;
        ktx.kpt_event_edit = 0;
        ktx.kpt_pause = 0;
    }
    else
    {
        /*
         * We have removed the individual key fixups in this module, since
         * keyval_normalize() is called afterward to make sure all key values
         * are legitimate.  However, we do have an issue with all of these
         * conditional compile macros.  We will ultimately read and write them
         * all, even if the application is built to not actually use some of
         * them.
         */

        next_data_line(file);
        sscanf(m_line, "%u", &ktx.kpt_pause);
        if (ktx.kpt_pause <= 1)             /* no pause key value present   */
        {
            ktx.kpt_show_ui_sequence_number = bool(ktx.kpt_pause);
            ktx.kpt_pause = 0;              /* make key_normalize() fix it  */
        }
        else
        {
            /*
             * New feature for showing sequence numbers in the mainwnd GUI.
             */

            next_data_line(file);
            sscanf(m_line, "%d", &show_key);
            ktx.kpt_show_ui_sequence_number = bool(show_key);
        }

        /*
         * Might need to be fixed up for existing config files.  Will fix when
         * we see what the problem would be.  Right now, they both come up as
         * an apostrophe when the config file exists.  Actually, the integer
         * value for each is zero!  So, if it comes up zero, we force them the
         * SEQ64_equal and SEQ64_minus.  This might still screw up
         * configurations that have devoted those keys to other purposes.
         */

        next_data_line(file);
        sscanf(m_line, "%u", &ktx.kpt_pattern_edit);

        next_data_line(file);
        sscanf(m_line, "%u", &ktx.kpt_event_edit);

        if (next_data_line(file))
            sscanf(m_line, "%u", &ktx.kpt_pattern_shift);   /* variset support */
        else
            ktx.kpt_pattern_shift = SEQ64_slash;            /* variset support */

        if (line_after(file, "[New-keys]"))
        {
            sscanf(m_line, "%u", &ktx.kpt_song_mode);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_menu_mode);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_follow_transport);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_toggle_jack);
            next_data_line(file);
        }
        else if (line_after(file, "[extended-keys]"))
        {
            sscanf(m_line, "%u", &ktx.kpt_song_mode);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_toggle_jack);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_menu_mode);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_follow_transport);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_fast_forward);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_rewind);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_pointer_position);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_tap_bpm);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_toggle_mutes);
            next_data_line(file);
#ifdef SEQ64_SONG_RECORDING
            sscanf(m_line, "%u", &ktx.kpt_song_record);
            next_data_line(file);
            sscanf(m_line, "%u", &ktx.kpt_oneshot_queue);
            next_data_line(file);
#endif
        }
        else
        {
            warnprint("WARNING:  no [extended-keys] section");
        }
    }

    keyval_normalize(ktx);                  /* fix any missing values   */
    p.keys().set_keys(ktx);                 /* copy into perform keys   */

    long flag = 0;
    if (line_after(file, "[jack-transport]"))
    {
        sscanf(m_line, "%ld", &flag);
        rc().with_jack_transport(bool(flag));

        next_data_line(file);
        sscanf(m_line, "%ld", &flag);
        rc().with_jack_master(bool(flag));

        next_data_line(file);
        sscanf(m_line, "%ld", &flag);
        rc().with_jack_master_cond(bool(flag));

        next_data_line(file);
        sscanf(m_line, "%ld", &flag);
        p.song_start_mode(bool(flag));

        if (next_data_line(file))
        {
            sscanf(m_line, "%ld", &flag);
            rc().with_jack_midi(bool(flag));
        }
    }

    /*
     *  We are taking a slightly different approach to this section.  When
     *  Sequencer64 exits, it saves all of the inputs it has.  If an input is
     *  removed from the system (e.g. unplugging a MIDI controller), then
     *  there will be too many entries in this section.  The user might remove
     *  one, and forget to update the buss count.  So we basically ignore the
     *  buss count.  But we also have to read the new channel-filter boolean
     *  if not in legacy format. If an error occurs, we abort... the user must
     *  fix the "rc" file.
     */

    if (line_after(file, "[midi-input]"))
    {
        int buses = 0;
        int count = sscanf(m_line, "%d", &buses);
        if (count > 0 && buses > 0)
        {
            int b = 0;
            while (next_data_line(file))
            {
                long bus_on, bus;
                count = sscanf(m_line, "%ld %ld", &bus, &bus_on);
                if (count == 2)
                {
                    p.add_input(bool(bus_on));
                    ++b;
                }
                else if (count == 1)
                {
                    bool flag = bool(bus);
                    rc().filter_by_channel(flag);
                    p.filter_by_channel(flag);              /* important! */
                    infoprintf("[Filter-by-channel %s]\n", flag ? "on" : "off");
                }
            }
            if (b < buses)
                return make_error_message("midi-input", "too few buses");
        }
    }
    else
        return make_error_message("midi-input");

#ifdef USE_THIS_CODE

    /*
     * This is not right; it is already handled above, irregardless of legacy
     * status, and the next section is [manual-alsa-ports], which is handled
     * further on.  The handling here is out of order, but configfile ::
     * line_after() starts from the beginning every time.
     */

    if (! rc().legacy_format())
    {
        if (next_data_line(file))                       /* new 2016-08-20 */
        {
            sscanf(m_line, "%ld", &flag);
            rc().filter_by_channel(bool(flag));
        }
    }

#endif  // USE_THIS_CODE

    if (line_after(file, "[midi-clock-mod-ticks]"))
    {
        long ticks = 64;
        sscanf(m_line, "%ld", &ticks);
        midibus::set_clock_mod(ticks);
    }
    if (line_after(file, "[midi-meta-events]"))
    {
        int track = 0;
        sscanf(m_line, "%d", &track);
        rc().tempo_track_number(track);
        p.set_tempo_track_number(track);    /* MIDI file can override this  */
    }
    if (line_after(file, "[manual-alsa-ports]"))
    {
        sscanf(m_line, "%ld", &flag);
        rc().manual_alsa_ports(bool(flag));
    }
    if (line_after(file, "[reveal-alsa-ports]"))
    {
        /*
         * If this flag is already raised, it was raised on the command line,
         * and we don't want to change it.  An ugly special case.
         */

        sscanf(m_line, "%ld", &flag);
        if (! rc().reveal_alsa_ports())
            rc().reveal_alsa_ports(bool(flag));
    }

    if (line_after(file, "[last-used-dir]"))
    {
        if (strlen(m_line) > 0)
            rc().last_used_dir(m_line); // FIXME: check for valid path
    }

    if (line_after(file, "[recent-files]"))
    {
        int count;
        sscanf(m_line, "%d", &count);
        for (int i = 0; i < count; ++i)
        {
            if (next_data_line(file))
            {
                if (strlen(m_line) > 0)
                {
                    if (! rc().append_recent_file(std::string(m_line)))
                        break;
                }
            }
            else
                break;
        }
    }

    if (line_after(file, "[playlist]"))
    {
        int flag;
        sscanf(m_line, "%d", &flag);
        rc().playlist_active(bool(flag));
        if (flag != 0)
        {
            if (next_data_line(file))
            {
                size_t len = strlen(m_line);
                if (len > 0)
                {
                    if (strcmp(m_line, "\"\"") == 0)
                    {
                        rc().playlist_active(false);
                        rc().playlist_filename("");
                    }
                    else
                        rc().playlist_filename(m_line);
                }
                else
                    rc().playlist_active(false);
            }
        }
    }

    long method = 0;
    if (line_after(file, "[interaction-method]"))
        sscanf(m_line, "%ld", &method);

    /*
     * This now returns true if the value was correct, we should check it.
     */

    if (! rc().interaction_method(interaction_method_t(method)))
        return make_error_message("interaction-method", "illegal value");

    if (! rc().legacy_format())
    {
        if (next_data_line(file))                   /* a new option */
        {
            sscanf(m_line, "%ld", &method);
            rc().allow_mod4_mode(method != 0);
        }
        if (next_data_line(file))                   /* a new option */
        {
            sscanf(m_line, "%ld", &method);
            rc().allow_snap_split(method != 0);
        }
        if (next_data_line(file))                   /* a new option */
        {
            sscanf(m_line, "%ld", &method);
            rc().allow_click_edit(method != 0);
        }
        line_after(file, "[lash-session]");
        sscanf(m_line, "%ld", &method);
        rc().lash_support(method != 0);

        method = 1;         /* preserve legacy seq24 option if not present */
        line_after(file, "[auto-option-save]");
        sscanf(m_line, "%ld", &method);
        rc().auto_option_save(method != 0);
    }
    file.close();           /* done parsing the "rc" configuration file */
    return true;
}

/**
 *  Parses the [mute-group] section.  This function is used both in the
 *  original reading of the "rc" file, and for reloading the original
 *  mute-group data from the "rc".
 *
 *  We used to throw the mute-group count value away, since it was always
 *  1024, but it is useful if no mute groups have been created.  So, if it
 *  reads 0 (instead of 1024), we will assume there are no mute-group
 *  settings.  We also have to be sure to go to the next data line even if the
 *  strip-empty-mutes option is on.
 *
 * \param p
 *      Provides a reference to the main perform object.  However,
 *      we have to cast away the constness, because too many of the
 *      perform getter functions are used in non-const contexts.
 *
 * \return
 *      Returns true if the file was able to be opened for reading, and the
 *      desired data successfully extracted.
 */

bool
optionsfile::parse_mute_group_section (perform & p)
{
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        printf("? error opening [%s] for reading\n", name().c_str());
        return false;
    }
    file.seekg(0, std::ios::beg);                           /* seek to start */

    line_after(file, "[mute-group]");               /* Group MIDI control   */
    int gtrack = 0;
    sscanf(m_line, "%d", &gtrack);
    bool result = next_data_line(file);
    if (result)
    {
        result = gtrack == 0 || gtrack == (c_max_sets * c_max_keys); /* 1024 */
    }
    if (! result)
        (void) make_error_message("mute-group");    /* abort the parsing!   */

    if (result && gtrack > 0)
    {
        /*
         * This loop is a bit odd.  We set groupmute, read it, increment it,
         * and then read it again.  We could just use the i variable, I think.
         * Note that this layout is STILL dependent on c_seqs_in_set = 32.
         * However, though we keep this layout, the boundaries for a
         * non-default value of seqs-in-set may be used internally.
         */

        int gm[c_seqs_in_set];
        int groupmute = 0;
        for (int g = 0; g < c_max_groups; ++g)
        {
            sscanf
            (
                m_line,
                "%d [%d %d %d %d %d %d %d %d]"
                  " [%d %d %d %d %d %d %d %d]"
                  " [%d %d %d %d %d %d %d %d]"
                  " [%d %d %d %d %d %d %d %d]",
                &groupmute,
                &gm[0],  &gm[1],  &gm[2],  &gm[3],
                &gm[4],  &gm[5],  &gm[6],  &gm[7],
                &gm[8],  &gm[9],  &gm[10], &gm[11],
                &gm[12], &gm[13], &gm[14], &gm[15],
                &gm[16], &gm[17], &gm[18], &gm[19],
                &gm[20], &gm[21], &gm[22], &gm[23],
                &gm[24], &gm[25], &gm[26], &gm[27],
                &gm[28], &gm[29], &gm[30], &gm[31]
            );
            if (groupmute < 0 || groupmute >= c_max_groups)
            {
                return make_error_message("group-mute number out of range");
            }
            else
            {
                /*
                 * What's up with this call?  Because we're not in learn-mode
                 * at this time, it only sets perform::m_mute_group_selected.
                 * It might be better to combine the two function calls here,
                 * rather than have a side-effect on the private member
                 * perform::m_mute_group_selected.
                 *
                 * p.select_group_mute(g);
                 * for (int k = 0; k < c_max_groups; ++k)   // c_seqs_in_set!!!
                 *      p.set_group_mute_state(k, gm[k] != 0);
                 */

                p.load_mute_group(g, gm);
            }

            result = next_data_line(file);
            if (! result && g < (c_max_groups - 1))
                return make_error_message("mute-group data line");
            else
                result = true;
        }
        if (result)
        {
            bool present = ! at_section_start();    /* ok if not present    */
            if (present)
            {
                int v = 0;
                sscanf(m_line, "%d", &v);
                result = rc().mute_group_saving((mute_group_handling_t) v);
                if (! result)
                    return make_error_message("mute-group", "handling value bad");
            }
        }
    }
    return true;
}

/**
 *  Parses the [midi-control] section.  This function is used both in the
 *  original reading of the "rc" file, and for reloading the original
 *  midi-control data from the "rc".
 *
 *  We used to throw the midi-control count value away, since it was always
 *  1024, but it is useful if no mute groups have been created.  So, if it
 *  reads 0 (instead of 1024), we will assume there are no midi-control
 *  settings.  We also have to be sure to go to the next data line even if the
 *  strip-empty-mutes option is on.
 *
 * \param p
 *      Provides a reference to the main perform object.  However,
 *      we have to cast away the constness, because too many of the
 *      perform getter functions are used in non-const contexts.
 *
 * \return
 *      Returns true if the file was able to be opened for reading, and the
 *      desired data successfully extracted.
 */

bool
optionsfile::parse_midi_control_section (const std::string & fname, perform & p)
{
    std::ifstream file(fname, std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        printf("? error opening [%s] for reading\n", name().c_str());
        return false;
    }
    file.seekg(0, std::ios::beg);                           /* seek to start */

    /*
     * This call causes parsing to skip all of the header material.  Please note
     * that the line_after() function always starts from the beginning of the
     * file every time.  A lot a rescanning!  But it goes fast these days.
     */

    unsigned sequences = 0;                                 /* seq & ctrl #s */
    line_after(file, "[midi-control]");
    sscanf(m_line, "%u", &sequences);

    /*
     * The above value is called "sequences", but what was written was the
     * value of c_midi_controls.  If we make that value non-constant, then
     * we should modify that value, instead of the throw-away "sequences"
     * values.  Note that c_midi_controls is, in a roundabout way, defined
     * as 74.  See the old "dot-seq24rc" file in the contrib directory.
     */

    bool ok = false;
    if (rc().legacy_format())                   /* init "non-legacy" fields */
        g_midi_control_limit = c_midi_controls; /* use the original value   */

    if (int(sequences) > g_midi_control_limit)
    {
        return make_error_message("midi-control", "too many control entries");
    }
    else if (sequences > 0)
    {
        ok = next_data_line(file);
        if (! ok)
            return make_error_message("midi-control", "no data");
        else
            ok = true;

        for (unsigned i = 0; i < sequences; ++i)    /* 0 to c_midi_controls-1 */
        {
            int sequence = 0;
            int a[6], b[6], c[6];
            sscanf
            (
                m_line,
                "%d [ %d %d %d %d %d %d ]"
                  " [ %d %d %d %d %d %d ]"
                  " [ %d %d %d %d %d %d ]",
                &sequence,
                &a[0], &a[1], &a[2], &a[3], &a[4], &a[5],
                &b[0], &b[1], &b[2], &b[3], &b[4], &b[5],
                &c[0], &c[1], &c[2], &c[3], &c[4], &c[5]
            );
            p.midi_control_toggle(i).set(a);
            p.midi_control_on(i).set(b);
            p.midi_control_off(i).set(c);
            ok = next_data_line(file);
            if (! ok && i < (sequences - 1))
                return make_error_message("midi-control", "not enough data");
            else
                ok = true;
        }
    }
    else
    {
        warnprint("[midi-controls] specifies a count of 0, so skipped");
    }
    return true;
}

/**
 *  This options-writing function is just about as complex as the
 *  options-reading function.
 *
 * \param p
 *      Provides a const reference to the main perform object.  However,
 *      we have to cast away the constness, because too many of the
 *      perform getter functions are used in non-const contexts.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
optionsfile::write (const perform & p)
{
    std::ofstream file(name(), std::ios::out | std::ios::trunc);
    perform & ucperf = const_cast<perform &>(p);
    bool ok = file.is_open();
    if (! ok)
    {
        printf("? error opening [%s] for writing\n", name().c_str());
        return false;
    }

    /*
     * Initial comments and MIDI control section.  No more "global_xxx", yay!
     */

    if (rc().legacy_format())
    {
        file <<
           "# Sequencer64 user configuration file (legacy Seq24 0.9.2 format)\n";
    }
    else
    {
        /*
         * Top banner
         */

        file
            << "# Sequencer64 0.95.1 (and above) rc configuration file\n"
            << "#\n"
            << "# " << name() << "\n"
            << "# Written on " << current_date_time() << "\n"
            << "#\n"
            <<
            "# This file holds the main configuration options for Sequencer64.\n"
            "# It follows the format of the legacy seq24 'rc' configuration\n"
            "# file, but adds some new options, such as LASH, Mod4 interaction\n"
            "# support, an auto-save-on-exit option, and more.  Also provided\n"
            "# is a legacy mode.\n"
            ;

        /*
         * [comments]
         */

        file << "#\n"
            "# The [comments] section can document this file.  Lines starting\n"
            "# with '#' and '[' are ignored.  Blank lines are ignored.  Show a\n"
            "# blank line by adding a space character to the line.\n"
            ;

        file << "\n[comments]\n\n" << rc().comments_block(); // << "\n" ;
    }
    if (rc().use_midi_control_file())
    {
        std::string fspec = rc().midi_control_filespec();
        std::ofstream ctlfile(fspec, std::ios::out | std::ios::trunc);
        ctlfile
            << "# Sequencer64 0.96.1 (and above) midi-control "
                   "configuration file\n"
            << "#\n"
            << "# " << fspec << "\n"
            << "# Written on " << current_date_time() << "\n"
            << "#\n"
            <<
            "# This file holds the MIDI control configuration for Sequencer64.\n"
            "# It follows the format of the 'rc' configuration file, but is\n"
            "# stored separately for convenience.  It is always stored in the\n"
            "# main configuration directory.  To use this file, replace the\n"
            "# [midi-control] section and its contents with a "
                "[midi-control-file]\n"
            "# tag, and simply add the basename (e.g. nanomap.rc) on a\n"
            "# separate line.\n"
            ;
        ok = write_midi_control(p, ctlfile);
        if (ok)
        {
            std::string fname = rc().midi_control_filename();   /* base */
            ctlfile
                << "\n\n# End of " << fspec << "\n#\n"
                << "# vim: sw=4 ts=4 wm=4 et ft=sh\n"   /* ft=sh, nice colors */
                ;

            /*
             * [midi-control-file]
             */

            file
                << "\n[midi-control-file]\n\n" << fname
                << "    # (" << fspec << ")\n"
                ;
        }
        else
        {
            errprintf("Failed to write '%s'\n", fspec.c_str());
        }
    }
    else
    {
        ok = write_midi_control(p, file);
    }

    /*
     * Group MIDI control
     *
     *  We want to save mute-groups only under certain circumstances:
     *
     *      -   They don't exist in sequencer64.rc yet.
     *      -   There are no MIDI mute-group settings to save.
     *      -   The user said to save them.
     *      -   The configuration said to save them if ...

    bool mutegroup_write_zeroes = false;
    bool mutegroup_write_mutes = true;
    bool midimutegroup = p.midi_mute_group_present();
    if ()
    {
        // TODO
    }
     */

    char outs[SEQ64_LINE_MAX];
    file << "\n[mute-group]\n\n";

    /*
     * We might as well save the empty mutes in the "rc" configuration file,
     * even if we don't save empty mutes to the MIDI file.  This is less
     * confusing to the user, especially if issues with the mute groups occur,
     * and is not a lot of space to waste, it's just one file.
     *
     * We've replaced c_gmute_tracks with c_max_sequence, since they're the
     * same concept and same number (1024).
     */

    file <<
        "# All mute-group values are saved in this 'rc' file, even if they\n"
        "# all are zero; but if all are zero, they will be stripped out from\n"
        "# the MIDI file by the new strip-empty-mutes functionality (a build\n"
        "# option).  This is less confusing to the user, who expects that\n"
        "# section to be intact.\n"
        "\n"
        << c_max_sequence << "       # group mute count\n"
        ;

    int gm[c_max_groups];
    for (int group = 0; group < c_max_groups; ++group)
    {
        p.save_mute_group(group, gm);       /* saves mute-group state to gm */
        snprintf
        (
            outs, sizeof outs,
            "%d [%d %d %d %d %d %d %d %d]"
            " [%d %d %d %d %d %d %d %d]"
            " [%d %d %d %d %d %d %d %d]"
            " [%d %d %d %d %d %d %d %d]",
            group,
            gm[0],  gm[1],  gm[2],  gm[3],  gm[4],  gm[5],  gm[6],  gm[7],
            gm[8],  gm[9],  gm[10], gm[11], gm[12], gm[13], gm[14], gm[15],
            gm[16], gm[17], gm[18], gm[19], gm[20], gm[21], gm[22], gm[23],
            gm[24], gm[25], gm[26], gm[27], gm[28], gm[29], gm[30], gm[31]
        );
        file << std::string(outs) << "\n";
    }

    if (! rc().legacy_format())
    {
        mute_group_handling_t mgh = rc().mute_group_saving();
        int v = int(mgh);
        file
            << "\n"
               "# Handling of mute-groups.  If set to 0, a legacy value, then\n"
               "# any mute-groups read from the MIDI file (whether modified or\n"
               "# not) are saved to the 'rc' file as well.  If set to 1, the\n"
               "# 'rc' mute-groups are overwritten only if they were not read\n"
               "# from the MIDI file.\n"
               "\n"
            << v
            ;
        if (mgh == e_mute_group_stomp)
            file << "     # save mute-groups to both the MIDI and 'rc' file\n";
        else if (mgh == e_mute_group_preserve)
            file << "     # preserve 'rc' mute-groups from MIDI mute groups\n";
    }

    /*
     * Bus mute/unmute data.  At this point, we can use the master_bus()
     * accessor, even if a pointer dereference, because it was created at
     * application start-up, and here we are at application close-down.
     */

    int buses = ucperf.master_bus().get_num_out_buses();
    file
        << "\n"
           "[midi-clock]\n\n"
           "# The first line indicates the number of MIDI busses defined.\n"
           "# Each buss line contains the buss (re 0) and the clock status of\n"
           "# that buss.  0 = MIDI Clock is off; 1 = MIDI Clock on, and Song\n"
           "# Position and MIDI Continue will be sent, if needed; 2 = MIDI\n"
           "# Clock Modulo, where MIDI clocking will not begin until the song\n"
           "# position reaches the start modulo value [midi-clock-mod-ticks].\n"
           "# A value of -1 indicates that the output port is totally\n"
           "# disabled.  One can set this value manually for devices that are\n"
           "# present, but not available, perhaps because another application\n"
           "# has exclusive access to the device (e.g. on Windows).\n"
           "\n"
        ;

    file << buses << "    # number of MIDI clocks/busses\n\n";
    for (int bus = 0; bus < buses; ++bus)
    {
        file
            << "# Output buss name: "
            << ucperf.master_bus().get_midi_out_bus_name(bus)
            << "\n"
            ;

        /*
         * We were getting this from the master bus, but we now let perform
         * get the clocks from the master bus, and we get them from perform.
         */

        int bus_on = static_cast<int>(ucperf.get_clock(bussbyte(bus)));
        snprintf
        (
            outs, sizeof outs, "%d %d    # buss number, clock status",
            bus, bus_on
        );
        file << outs << "\n";
    }

    /*
     * MIDI clock modulo value
     */

    file
        << "\n[midi-clock-mod-ticks]\n\n"
           "# The Song Position (in 16th notes) at which clocking will begin\n"
           "# if the buss is set to MIDI Clock mod setting.\n"
           "\n"
        << midibus::get_clock_mod() << "\n"
        ;

    /*
     * New section for MIDI meta events.
     */

    file
        << "\n[midi-meta-events]\n\n"
           "# This section defines some features of MIDI meta-event handling.\n"
           "# Normally, tempo events are supposed to occur in the first track\n"
           "# (pattern 0).  But one can move this track elsewhere to accomodate\n"
           "# one's existing body of tunes.  If affects where tempo events are\n"
           "# recorded.  The default value is 0, the maximum is 1023.\n"
           "# A pattern must exist at this number for it to work.\n"
           "\n"
        << rc().tempo_track_number() << "    # tempo_track_number\n"
        ;

    /*
     * Bus input data
     */

    buses = ucperf.master_bus().get_num_in_buses();
    file
        << "\n[midi-input]\n\n"
        << buses << "   # number of input MIDI busses\n\n"
           "# The first number is the port number, and the second number\n"
           "# indicates whether it is disabled (0), or enabled (1).\n"
           "\n"
        ;

    for (int i = 0; i < buses; ++i)
    {
        file
            << "# Input buss name: "
            << ucperf.master_bus().get_midi_in_bus_name(i)
            << "\n"
            ;
        snprintf
        (
            outs, sizeof outs, "%d %d  # buss number, input status",
            i, static_cast<int>(ucperf.get_input(i))
        );
        file << outs << "\n";
    }

    /*
     * Filter by channel, new option as of 2016-08-20
     */

    file
        << "\n"
        << "# If set to 1, this option allows the master MIDI bus to record\n"
           "# (filter) incoming MIDI data by channel, allocating each incoming\n"
           "# MIDI event to the sequence that is set to that channel.\n"
           "# This is an option adopted from the Seq32 project at GitHub.\n"
           "\n"
        << (rc().filter_by_channel() ? "1" : "0")
        << "   # flag to record incoming data by channel\n"
        ;

    /*
     * Manual ALSA ports
     */

    file
        << "\n[manual-alsa-ports]\n\n"
           "# Set to 1 to have sequencer64 create its own ALSA ports and not\n"
           "# connect to other clients.  Use 1 to expose all 16 MIDI ports to\n"
           "# JACK (e.g. via a2jmidid).  Use 0 to access the ALSA MIDI ports\n"
           "# already running on one's computer, or to use the autoconnect\n"
           "# feature (Sequencer64 connects to existing JACK ports on startup.\n"
           "\n"
        << (rc().manual_alsa_ports() ? "1" : "0")
        << "   # flag for manual ALSA ports\n"
        ;

    /*
     * Reveal ALSA ports
     */

    file
        << "\n[reveal-alsa-ports]\n\n"
           "# Set to 1 to have sequencer64 ignore any system port names\n"
           "# declared in the 'user' configuration file.  Use this option if\n"
           "# you want to be able to see the port names as detected by ALSA.\n"
           "\n"
        << (rc().reveal_alsa_ports() ? "1" : "0")
        << "   # flag for reveal ALSA ports\n"
        ;

    /*
     * Interaction-method
     */

    int x = 0;
    file
        << "\n[interaction-method]\n\n"
        << "# Sets the mouse handling style for drawing and editing a pattern\n"
        << "# This feature is current NOT supported in the Qt version of\n"
        << "# Sequencer64 (qpseq64).\n\n"
        ;

    while (c_interaction_method_names[x] && c_interaction_method_descs[x])
    {
        file
            << "# " << x
            << " - '" << c_interaction_method_names[x]
            << "' (" << c_interaction_method_descs[x] << ")\n"
            ;
        ++x;
    }
    file
        << "\n" << rc().interaction_method() << "   # interaction_method\n\n"
        ;

    file
        << "# Set to 1 to allow Sequencer64 to stay in note-adding mode when\n"
           "# the right-click is released while holding the Mod4 (Super or\n"
           "# Windows) key.\n"
           "\n"
        << (rc().allow_mod4_mode() ? "1" : "0")     // @new 2015-08-28
        << "   # allow_mod4_mode\n\n"
        ;

    file
        << "# Set to 1 to allow Sequencer64 to split performance editor\n"
           "# triggers at the closest snap position, instead of splitting the\n"
           "# trigger exactly in its middle.  Remember that the split is\n"
           "# activated by a middle click.\n"
           "\n"
        << (rc().allow_snap_split() ? "1" : "0")    // @new 2016-08-17
        << "   # allow_snap_split\n\n"
        ;

    file
        << "# Set to 1 to allow a double-click on a slot to bring it up in\n"
           "# the pattern editor.  This is the default.  Set it to 0 if\n"
           "# it interferes with muting/unmuting a pattern.\n"
           "\n"
        << (rc().allow_click_edit() ? "1" : "0")    // @new 2016-10-30
        << "   # allow_click_edit\n"
        ;

    size_t kevsize = ucperf.get_key_events().size() < size_t(c_max_keys) ?
         ucperf.get_key_events().size() : size_t(c_max_keys)
         ;

    file
        << "\n[keyboard-control]\n\n"
        << "# Defines the keys that toggle the state of each of up to 32\n"
        << "# patterns in the pattern/sequence window.  These keys are normally\n"
        << "# shown in each box.  The first number below specifies the key\n"
        << "# code, and the second number specifies the pattern number.\n\n"
        << kevsize << "     # number of keys\n\n"
        << "# Key-No.  Sequence-No.  Key-Name\n\n"
        ;

    for
    (
        keys_perform::SlotMap::const_iterator i = ucperf.get_key_events().begin();
        i != ucperf.get_key_events().end(); ++i
    )
    {
        std::string keyname = ucperf.key_name(i->first);
        snprintf
        (
            outs, sizeof outs, "%u %d   # %s",
            i->first, i->second, keyname.c_str()
        );
        file << std::string(outs) << "\n";
    }

    size_t kegsize = ucperf.get_key_groups().size() < size_t(c_max_keys) ?
         ucperf.get_key_groups().size() : size_t(c_max_keys)
         ;
    file
        << "\n[keyboard-group]\n\n"
        << "# This section actually defines the mute-group keys for the group\n"
        << "# learn function.  Pressing the 'L' button and then pressing one\n"
        << "# of the keys in this list will cause the current set of armed\n"
        << "# patterns to be memorized and associated with that key.\n\n"
        << kegsize << "     # number of group-learn keys (key groups)\n\n"
        << "# Key #  group # Key name\n\n"
        ;

    for
    (
        keys_perform::SlotMap::const_iterator i = ucperf.get_key_groups().begin();
        i != ucperf.get_key_groups().end(); ++i
    )
    {
        snprintf
        (
            outs, sizeof outs, "%u  %d   # %s", i->first, i->second,
            ucperf.key_name(i->first).c_str()
        );
        file << std::string(outs) << "\n";
    }

    keys_perform_transfer ktx;
    ucperf.keys().get_keys(ktx);      /* copy perform key to structure    */
    file
        << "\n"
        << "# bpm up and bpm down:\n"
        << ktx.kpt_bpm_up << " "
        << ktx.kpt_bpm_dn << "          # "
        << ucperf.key_name(ktx.kpt_bpm_up) << " "
        << ucperf.key_name(ktx.kpt_bpm_dn) << "\n"
        ;
    file
        << "# screen set up, screen set down, play:\n"
        << ktx.kpt_screenset_up << " "
        << ktx.kpt_screenset_dn << " "
        << ktx.kpt_set_playing_screenset << "    # "
        << ucperf.key_name(ktx.kpt_screenset_up) << " "
        << ucperf.key_name(ktx.kpt_screenset_dn) << " "
        << ucperf.key_name(ktx.kpt_set_playing_screenset) << "\n"
        ;
    file
        << "# group on, group off, group learn:\n"
        << ktx.kpt_group_on << " "
        << ktx.kpt_group_off << " "
        << ktx.kpt_group_learn << "   # "
        << ucperf.key_name(ktx.kpt_group_on) << " "
        << ucperf.key_name(ktx.kpt_group_off) << " "
        << ucperf.key_name(ktx.kpt_group_learn) << "\n"
        ;
    file
        << "# replace, queue, snapshot_1, snapshot 2, keep queue:\n"
        << ktx.kpt_replace << " "
        << ktx.kpt_queue << " "
        << ktx.kpt_snapshot_1 << " "
        << ktx.kpt_snapshot_2 << " "
        << ktx.kpt_keep_queue << "   # "
        << ucperf.key_name(ktx.kpt_replace) << " "
        << ucperf.key_name(ktx.kpt_queue) << " "
        << ucperf.key_name(ktx.kpt_snapshot_1) << " "
        << ucperf.key_name(ktx.kpt_snapshot_2) << " "
        << ucperf.key_name(ktx.kpt_keep_queue) << "\n"
        ;

    file
        << (ktx.kpt_show_ui_sequence_key ? 1 : 0)
        << "     # show_ui_sequence_key and seq measures (1 = true / 0 = false)\n"
        ;

    file
        << ktx.kpt_start << "    # "
        << ucperf.key_name(ktx.kpt_start)
        << " start sequencer\n"
       ;

    file
        << ktx.kpt_stop << "    # "
        << ucperf.key_name(ktx.kpt_stop)
        << " stop sequencer\n"
        ;

    /**
     *  New boolean to show sequence numbers; ignored in legacy mode.
     */

    if (! rc().legacy_format())
    {
        file
            << ktx.kpt_pause << "    # "
            << ucperf.key_name(ktx.kpt_pause)
            << " pause sequencer\n"
            ;

        file
            << ktx.kpt_show_ui_sequence_number << "     #"
            << " show sequence numbers (1 = true / 0 = false);"
               " ignored in legacy mode\n"
            ;

        file
            << ktx.kpt_pattern_edit << "    # "
            << ucperf.key_name(ktx.kpt_pattern_edit)
            << " is the shortcut key to bring up the pattern editor\n"
            ;

        file
            << ktx.kpt_event_edit << "    # "
            << ucperf.key_name(ktx.kpt_event_edit)
            << " is the shortcut key to bring up the event editor\n"
            ;

        file
            << ktx.kpt_pattern_shift << "    # "
            << ucperf.key_name(ktx.kpt_pattern_shift)
            << " shifts the hot-key so that it toggles pattern + 32\n"
            ;

        /*
         * This section writes all of the new additional keystrokes created by
         * seq32 (stazed) and sequencer64.  Eventually we will provide a
         * use-interface options page for them.  Note that the Pause key is
         * handled elsewhere; it was a much earlier option for Sequencer64.
         */

        file
            << "\n[extended-keys]\n\n"
            << "# The user interface for this section is Options / Ext Keys.\n\n"
            << ktx.kpt_song_mode << "    # "
            << ucperf.key_name(ktx.kpt_song_mode)
            << " handles the Song/Live mode\n"
            << ktx.kpt_toggle_jack << "    # "
            << ucperf.key_name(ktx.kpt_toggle_jack)
            << " handles the JACK mode\n"
            << ktx.kpt_menu_mode << "    # "
            << ucperf.key_name(ktx.kpt_menu_mode)
            << " handles the menu mode\n"
            << ktx.kpt_follow_transport << "    # "
            << ucperf.key_name(ktx.kpt_follow_transport)
            << " handles the following of JACK transport\n"
            << ktx.kpt_fast_forward << "    # "
            << ucperf.key_name(ktx.kpt_fast_forward)
            << " handles the Fast-Forward function\n"
            << ktx.kpt_rewind << "    # "
            << ucperf.key_name(ktx.kpt_rewind)
            << " handles Rewind function\n"
            << ktx.kpt_pointer_position << "    # "
            << ucperf.key_name(ktx.kpt_pointer_position)
            << " handles song pointer-position function\n"
            << ktx.kpt_tap_bpm << "    # "
            << ucperf.key_name(ktx.kpt_tap_bpm)
            << " emulates clicking the Tap (BPM) button\n"
            << ktx.kpt_toggle_mutes << "    # "
            << ucperf.key_name(ktx.kpt_toggle_mutes)
            << " handles the toggling-all-pattern-mutes function\n"
#ifdef SEQ64_SONG_RECORDING
            << ktx.kpt_song_record << "    # "
            << ucperf.key_name(ktx.kpt_song_record)
            << " toggles the song-record function\n"
            << ktx.kpt_oneshot_queue << "    # "
            << ucperf.key_name(ktx.kpt_oneshot_queue)
            << " toggles the one-shot queue function\n"
#endif
            ;
    }

    file
        << "\n[jack-transport]\n\n"
        "# jack_transport - Enable slave synchronization with JACK Transport.\n"
        "# Also contains the new flag to use JACK MIDI.\n\n"
        << rc().with_jack_transport() << "   # with_jack_transport\n\n"
        "# jack_master - Sequencer64 attempts to serve as JACK Master.\n"
        "# Also must enable jack_transport (the user interface forces this,\n"
        "# and also disables jack_master_cond).\n\n"
        << rc().with_jack_master() << "   # with_jack_master\n\n"
        "# jack_master_cond - Sequencer64 is JACK master if no other JACK\n"
        "# master exists. Also must enable jack_transport (the user interface\n"
        "# forces this, and disables jack_master).\n\n"
        << rc().with_jack_master_cond()  << "   # with_jack_master_cond\n\n"
        "# song_start_mode (applies mainly if JACK is enabled).\n\n"
        "# 0 = Playback in live mode. Allows muting and unmuting of loops.\n"
        "#     from the main (patterns) window.  Disables both manual and\n"
        "#     automatic muting and unmuting from the performance window.\n"
        "# 1 = Playback uses the song (performance) editor's data and mute\n"
        "#     controls, regardless of which window was used to start the\n"
        "#     playback.\n\n"
        << p.song_start_mode() << "   # song_start_mode\n\n"
        "# jack_midi - Enable JACK MIDI, which is a separate option from\n"
        "# JACK Transport.\n\n"
        << rc().with_jack_midi()  << "   # with_jack_midi\n"
        ;

    /*
     * New for sequencer64:  provide configurable LASH session management.
     * Ignored in legacy mode, for now.
     */

    if (! rc().legacy_format())
    {
        file << "\n"
            "[lash-session]\n\n"
            "# Set the following value to 0 to disable LASH session management.\n"
            "# Set the following value to 1 to enable LASH session management.\n"
            "# This value will have no effect if LASH support is not built into\n"
            "# the application.  Use --help option to see if LASH is part of\n"
            "# the options list.\n"
            "\n"
            << (rc().lash_support() ? "1" : "0")
            << "     # LASH session management support flag\n"
            ;
    }

    file << "\n"
        "[auto-option-save]\n\n"
        "# Set the following value to 0 to disable the automatic saving of the\n"
        "# current configuration to the 'rc' and 'user' files.  Set it to 1 to\n"
        "# follow legacy seq24 behavior of saving the configuration at exit.\n"
        "# Note that, if auto-save is set, many of the command-line settings,\n"
        "# such as the JACK/ALSA settings, are then saved to the configuration,\n"
        "# which can confuse one at first.  Also note that one currently needs\n"
        "# this option set to 1 to save the configuration, as there is not a\n"
        "# user-interface control for it at present.\n"
        "\n"
        << (rc().auto_option_save() ? "1" : "0")
        << "     # auto-save-options-on-exit support flag\n"
        ;


    file << "\n"
        "[last-used-dir]\n\n"
        "# Last-used and currently-active directory:\n\n"
        << rc().last_used_dir() << "\n"
        ;

    /*
     *  New feature from Kepler34.
     */

    int count = rc().recent_file_count();
    file << "\n"
        "[recent-files]\n\n"
        "# Holds a list of the last few recently-loaded MIDI files.\n\n"
        << count << "\n\n"
        ;

    if (count > 0)
    {
        for (int i = 0; i < count; ++i)
            file << rc().recent_file(i, false) << "\n";

        file << "\n";
    }

    file
        << "[playlist]\n\n"
        "# Provides a configured play-list and a flag to activate it.\n\n"
        << (rc().playlist_active() ? "1" : "0")
        << "     # playlist_active, 1 = active, 0 = do not use it\n"
        ;

    file << "\n"
        "# Provides the name of a play-list.  If there is none, use '\"\"'.\n"
        "# Or set the flag above to 0.\n\n"
        ;

    std::string plname = rc().playlist_filename();
    if (plname.empty())
        plname = "\"\"";

    file << plname << "\n\n";

    file
        << "# End of " << name() << "\n#\n"
        << "# vim: sw=4 ts=4 wm=4 et ft=sh\n"   /* ft=sh for nice colors */
        ;

    file.close();
    return true;
}

/**
 *  Writes the [midi-control] section to the given file stream.
 *
 * \param p
 *      Provides a const reference to the main perform object.
 *
 * \param file
 *      Provides the output file stream to write to.
 *
 * \param separatefile
 *      If true, the [midi-control] section is being written to a separate
 *      file.  The default value is false.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
optionsfile::write_midi_control
(
    const perform & p,
    std::ofstream & file
)
{
    bool result = false;
    perform & ucperf = const_cast<perform &>(p);
    file
        << "\n[midi-control]\n\n"
        "# The leftmost number on each line here is the pattern number, from\n"
        "# 0 to 31; or it is the group number, from 32 to 63, for up to 32 \n"
        "# groups; or it is an automation control number, from 64 to 95.\n"
        "# This internal MIDI control number is followed by three groups of\n"
        "# bracketed numbers, each providing three different type of control:\n"
        "#\n"
        "#    Normal:           [toggle]    [on]      [off]\n"
        "#    Playback:         [pause]     [start]   [stop]\n"
        "#    Playlist:         [by-value]  [next]    [previous] (if active)\n"
        "#\n"
        "# In each group, there are six numbers:\n"
        "#\n"
        "#    [on/off invert status d0 d1min d1max]\n"
        "#\n"
        "# 'on/off' enables/disables (1/0) the MIDI control for the pattern.\n"
        "# 'invert' (1/0) causes the opposite if data is outside the range.\n"
        "# 'status' is by MIDI event to match (channel is NOT ignored).\n"
        "# 'd0' is the first data value.  Example: if status is 144 (Note On),\n"
        "# then d0 represents Note 0.\n"
        "# 'd1min'/'d1max' are the range of second values that should match.\n"
        "# Example:  For a Note On for note 0, 0 and 127 indicate that any\n"
        "# Note On velocity will cause the MIDI control to take effect.\n"
        "\n"
        "#     ------------------ on/off (indicate is the section is enabled)\n"
        "#    | ----------------- inverse\n"
        "#    | |  -------------- MIDI status (event) byte (e.g. note on)\n"
        "#    | | |  ------------ data 1 (e.g. note number)\n"
        "#    | | | |  ---------- data 2 min\n"
        "#    | | | | |  -------- data 2 max\n"
        "#    | | | | | |\n"
        "#    v v v v v v\n"
        "#   [0 0 0 0 0 0]   [0 0 0 0 0 0]   [0 0 0 0 0 0]\n"
        "#    Toggle          On              Off\n"
        "\n"
        <<  g_midi_control_limit << "      # MIDI controls count (74/84/96)\n"
        "\n"
        << "# Pattern-group section:\n"
        ;

    char outs[SEQ64_LINE_MAX];
    for (int mcontrol = 0; mcontrol < g_midi_control_limit; ++mcontrol)
    {
        /*
         * \tricky
         *      32 mutes for channel, 32 group mutes, 10 odds-and-ends, and 10
         *      extended values.  This first output item merely write a <i>
         *      comment </i> to the "rc" file to indicate what the next
         *      section describes.  The first section of [midi-control]
         *      specifies 74 items.  The first 32 are unlabelled by a comment,
         *      and run from 0 to 31.  The next 32 are labelled "mute in
         *      group", and run from 32 to 63.  The next 10 are each labelled,
         *      starting with "bpm up" and ending with "screen set play", and
         *      are each one line long.  Then we've added 10 more, for
         *      playback, record (of performance), solo, thru, and 6 reserved
         *      for expansion.  Finally, if play-list support is enabled,
         *      there are another 12 more controls to handle.
         */

        switch (mcontrol)
        {
        case c_max_groups:                  // 32
            file << "\n# Mute-in group section:\n";
            break;

        case c_midi_control_bpm_up:         // 64
            file << "\n# Automation group\n\n" "# bpm up:\n" ;
            break;

        case c_midi_control_bpm_dn:         // 65
            file << "# bpm down:\n";
            break;

        case c_midi_control_ss_up:          // 66
            file << "# screen set up:\n";
            break;

        case c_midi_control_ss_dn:          // 67
            file << "# screen set down:\n";
            break;

        case c_midi_control_mod_replace:    // 68
            file << "# mod replace:\n";
            break;

        case c_midi_control_mod_snapshot:   // 69
            file << "# mod snapshot:\n";
            break;

        case c_midi_control_mod_queue:      // 70
            file << "# mod queue:\n";
            break;

        case c_midi_control_mod_gmute:      // 71
            file << "# mod gmute:\n";
            break;

        case c_midi_control_mod_glearn:     // 72
            file << "# mod glearn:\n";
            break;

        case c_midi_control_play_ss:        // 73
            file << "# screen set play:\n";
            break;

        case c_midi_control_playback:       // 74 (new values!)
            file << "\n# Extended MIDI controls:\n\n"
                "# start playback (pause, start, stop):\n"
                ;
            break;

        case c_midi_control_song_record:    // 75
            file << "# performance record:\n";
            break;

        case c_midi_control_solo:           // 76
            file << "# solo (toggle, on, off):\n";
            break;

        case c_midi_control_thru:           // 77
            file << "# MIDI THRU (toggle, on, off):\n";
            break;

        case c_midi_control_bpm_page_up:    // 78
            file << "# bpm page up:\n";
            break;

        case c_midi_control_bpm_page_dn:    // 79
            file << "# bpm page down:\n";
            break;

        case c_midi_control_ss_set:         // 80, pull #85
            file << "# screen set by number:\n";
            break;

        case c_midi_control_record:         // 81
            file << "# MIDI RECORD (toggle, on, off):\n";
            break;

        case c_midi_control_quan_record:    // 82
            file << "# MIDI Quantized RECORD (toggle, on, off):\n";
            break;

        case c_midi_control_reset_seq:      // 83
            file << "# reserved for expansion:\n";      // still true???
            break;

        case c_midi_control_reserved_1:     // 84
            file << "# Reserved for expansion 1\n";
            break;

        case c_midi_control_FF:
            file << "# MIDI Control for fast-forward\n";
            break;

        case c_midi_control_rewind:
            file << "# MIDI Control for rewind\n";
            break;

        case c_midi_control_top:
            file << "# MIDI Control for top...\n";
            break;

        case c_midi_control_playlist:
            file << "# MIDI Control to select playlist "
                "(value, next, previous)\n"
                ;
            break;

        case c_midi_control_playlist_song:
            file << "# MIDI Control to select song in current playlist "
                "(value, next, previous)\n"
                ;
            break;

        case c_midi_control_reserved_7:
            file << "# Reserved for expansion 7\n";
            break;

        case c_midi_control_reserved_8:
            file << "# Reserved for expansion 8\n";
            break;

        case c_midi_control_reserved_9:
            file << "# Reserved for expansion 9\n";
            break;

        case c_midi_control_reserved_10:
            file << "# Reserved for expansion 10\n";
            break;

        case c_midi_control_reserved_11:
            file << "# Reserved for expansion 11\n";
            break;

        case c_midi_control_reserved_12:
            file << "# Reserved for expansion 12\n";
            break;

        /*
         * case c_midi_controls_extended:
         *     file << "# Reserved for expansion 9\n";
         *     break;
         */

        /*
         * case g_midi_control_limit:  74/8496, last value, not written.
         */

        default:
            break;
        }
        const midi_control & toggle = ucperf.midi_control_toggle(mcontrol);
        const midi_control & off = ucperf.midi_control_off(mcontrol);
        const midi_control & on = ucperf.midi_control_on(mcontrol);
        snprintf
        (
            outs, sizeof outs,
            "%d [%1d %1d %3d %3d %3d %3d]"
              " [%1d %1d %3d %3d %3d %3d]"
              " [%1d %1d %3d %3d %3d %3d]",
             mcontrol,
             toggle.active(), toggle.inverse_active(), toggle.status(),
                 toggle.data(), toggle.min_value(), toggle.max_value(),
             on.active(), on.inverse_active(), on.status(),
                 on.data(), on.min_value(), on.max_value(),
             off.active(), off.inverse_active(), off.status(),
                 off.data(), off.min_value(), off.max_value()
        );
        file << std::string(outs) << "\n";
        result = file.good();
        if (! result)
            break;
    }
    return result;
}

}           // namespace seq64

/*
 * optionsfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

