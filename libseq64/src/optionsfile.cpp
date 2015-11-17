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
 *  This module declares/defines the base class for managing the <tt>
 *  ~/.seq24rc </tt> or <tt> ~/.config/sequencer64/sequencer64.rc </tt> ("rc")
 *  configuration files.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-16
 * \license       GNU GPLv2 or above
 *
 *  The <tt> ~/.seq24rc </tt> or <tt> ~/.config/sequencer64/sequencer64.rc
 *  </tt> configuration file is fairly simple in layout.  The documentation
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
 */

#include "midibus.hpp"
#include "optionsfile.hpp"
#include "perform.hpp"

namespace seq64
{

/**
 *  Principal constructor.
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
 *  Parse the ~/.seq24rc or ~/.config/sequencer64/sequencer64.rc file.
 *
 *  [midi-control]
 *
 *  Get the number of sequence definitions provided in the [midi-control]
 *  section.  Ranges from 32 on up.  Then read in all of the sequence
 *  lines.  The first 32 apply to the first screen set.  There can also be
 *  a comment line "# mute in group" followed by 32 more lines.  Then
 *  there are addditional comments and single lines for BPM up, BPM down,
 *  Screen Set Up, Screen Set Down, Mod Replace, Mod Snapshot, Mod Queue,
 *  Mod Gmute, Mod Glearn, and Screen Set Play.  These are all forms of
 *  MIDI automation useful to control the playback while not sitting near
 *  the computer.
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
 *  The keyboard control defines the keys that will toggle the stage of
 *  each of up to 32 patterns in a pattern/sequence box.  These keys are
 *  displayed in each box as a reminder.  The first number specifies the
 *  Key number, and the second number specifies the Sequence number.
 *
 *  [keyboard-group]
 *
 *  The keyboard group specifies more automation for the application.
 *  The first number specifies the Key number, and the second number
 *  specifies the Group number.  This section should be better described
 *  in the seq24-doc project on GitHub.
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
 *      -   jack_start_mode:
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
 *  This section covers....
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
 */

bool
optionsfile::parse (perform & p)
{
    std::ifstream file(m_name.c_str(), std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        printf("? error opening [%s] for reading\n", m_name.c_str());
        return false;
    }
    file.seekg(0, std::ios::beg);                           /* seek to start */
    line_after(file, "[midi-control]");                     /* find section  */
    unsigned int sequences = 0;
    sscanf(m_line, "%u", &sequences);

    /*
     * The above value is called "sequences", but what was written was the
     * value of c_midi_controls.  If we make that value non-constant, then
     * we should modify that value, instead of the throw-away "sequences"
     * values.  Note that c_midi_controls is, in a roundabout way, defined
     * as 74.  See the old "dot-seq24rc" file in the contrib directory.
     */

    next_data_line(file);
    for (unsigned int i = 0; i < sequences; ++i)    /* 0 to c_midi_controls-1 */
    {
        int sequence = 0;
        sscanf
        (
            m_line,
            "%d [ %d %d %ld %ld %ld %ld ]"
                " [ %d %d %ld %ld %ld %ld ]"
                " [ %d %d %ld %ld %ld %ld ]",
            &sequence,
            (int *) &p.get_midi_control_toggle(i)->m_active,
                (int *) &p.get_midi_control_toggle(i)->m_inverse_active,
                &p.get_midi_control_toggle(i)->m_status,
                &p.get_midi_control_toggle(i)->m_data,
                &p.get_midi_control_toggle(i)->m_min_value,
                &p.get_midi_control_toggle(i)->m_max_value,
            (int *) &p.get_midi_control_on(i)->m_active,
                (int *) &p.get_midi_control_on(i)->m_inverse_active,
                &p.get_midi_control_on(i)->m_status,
                &p.get_midi_control_on(i)->m_data,
                &p.get_midi_control_on(i)->m_min_value,
                &p.get_midi_control_on(i)->m_max_value,
            (int *) &p.get_midi_control_off(i)->m_active,
                (int *) &p.get_midi_control_off(i)->m_inverse_active,
                &p.get_midi_control_off(i)->m_status,
                &p.get_midi_control_off(i)->m_data,
                &p.get_midi_control_off(i)->m_min_value,
                &p.get_midi_control_off(i)->m_max_value
        );
        next_data_line(file);
    }

    line_after(file, "[mute-group]");               /* Group MIDI control */
    int gtrack = 0;
    sscanf(m_line, "%d", &gtrack);                  /* value thrown away  */
    next_data_line(file);
    int mtx[c_seqs_in_set];
    int j = 0;
    for (int i = 0; i < c_seqs_in_set; i++)
    {
        p.select_group_mute(j);
        sscanf
        (
            m_line,
            "%d [%d %d %d %d %d %d %d %d]"
              " [%d %d %d %d %d %d %d %d]"
              " [%d %d %d %d %d %d %d %d]"
              " [%d %d %d %d %d %d %d %d]",
            &j,
            &mtx[0], &mtx[1], &mtx[2], &mtx[3],             /* 1st */
                &mtx[4], &mtx[5], &mtx[6], &mtx[7],
            &mtx[8], &mtx[9], &mtx[10], &mtx[11],           /* 2nd */
                &mtx[12], &mtx[13], &mtx[14], &mtx[15],
            &mtx[16], &mtx[17], &mtx[18], &mtx[19],         /* 3rd */
                &mtx[20], &mtx[21], &mtx[22], &mtx[23],
            &mtx[24], &mtx[25], &mtx[26], &mtx[27],         /* 4th */
                &mtx[28], &mtx[29], &mtx[30], &mtx[31]
        );
        for (int k = 0; k < c_seqs_in_set; k++)
            p.set_group_mute_state(k, mtx[k]);

        j++;
        next_data_line(file);
    }

    line_after(file, "[midi-clock]");
    long buses = 0;
    sscanf(m_line, "%ld", &buses);
    next_data_line(file);
    for (int i = 0; i < buses; ++i)
    {
        long bus_on, bus;
        sscanf(m_line, "%ld %ld", &bus, &bus_on);
        p.master_bus().set_clock(bus, (clock_e) bus_on);
        next_data_line(file);
    }

    line_after(file, "[keyboard-control]");
    long keys = 0;
    sscanf(m_line, "%ld", &keys);
    next_data_line(file);

    /*
     * Bug involving the optionsfile and perform modules:  At the
     * 4th or 5th line of data in the "rc" file, setting this key event
     * results in the size remaining at 4, so the final size is 31.
     * This bug is present even in seq24 r.0.9.2, and occurs only if the
     * Keyboard options are actually edited.  Also, the size of the
     * reverse container is constant at 32.  Clearing the latter container
     * as well appears to fix both bugs.
     */

    p.get_key_events().clear();
    p.get_key_events_rev().clear();       // \new ca 2015-09-16
    for (int i = 0; i < keys; ++i)
    {
        long key = 0, seq = 0;
        sscanf(m_line, "%ld %ld", &key, &seq);
        p.set_key_event(key, seq);
        next_data_line(file);
    }

    line_after(file, "[keyboard-group]");
    long groups = 0;
    sscanf(m_line, "%ld", &groups);
    next_data_line(file);
    p.get_key_groups().clear();
    p.get_key_groups_rev().clear();       // \new ca 2015-09-16
    for (int i = 0; i < groups; ++i)
    {
        long key = 0, group = 0;
        sscanf(m_line, "%ld %ld", &key, &group);
        p.set_key_group(key, group);
        next_data_line(file);
    }

    keys_perform_transfer ktx;
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

    if (! rc().legacy_format())
    {
        /*
         * New feature for showing sequence number in the GUI.
         */

        next_data_line(file);
        sscanf(m_line, "%d", &show_key);
        ktx.kpt_show_ui_sequence_number = bool(show_key);
    }

    p.keys().set_keys(ktx);                /* copy into perform keys   */

    line_after(file, "[jack-transport]");
    long flag = 0;
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
    rc().jack_start_mode(bool(flag));

    line_after(file, "[midi-input]");
    buses = 0;
    sscanf(m_line, "%ld", &buses);
    next_data_line(file);
    for (int i = 0; i < buses; ++i)
    {
        long bus_on, bus;
        sscanf(m_line, "%ld %ld", &bus, &bus_on);
        p.master_bus().set_input(bus, bool(bus_on));
        next_data_line(file);
    }

    line_after(file, "[midi-clock-mod-ticks]");
    long ticks = 64;
    sscanf(m_line, "%ld", &ticks);
    midibus::set_clock_mod(ticks);

    line_after(file, "[manual-alsa-ports]");
    sscanf(m_line, "%ld", &flag);
    rc().manual_alsa_ports(bool(flag));

    line_after(file, "[last-used-dir]");
    if (strlen(m_line) > 0)
        rc().last_used_dir(m_line); // FIXME: check for valid path

    long method = 0;
    line_after(file, "[interaction-method]");
    sscanf(m_line, "%ld", &method);
    rc().interaction_method(interaction_method_t(method));

    if (! rc().legacy_format())
    {
        next_data_line(file);
        sscanf(m_line, "%ld", &method);
        rc().allow_mod4_mode(method != 0);

        line_after(file, "[lash-session]");
        sscanf(m_line, "%ld", &method);
        rc().lash_support(method != 0);
    }

    /*
     * Done parsing the "rc" configuration file.  Copy the newly-read values
     * into any global variables that are still in use, and then close the
     * file.
     */

    rc().set_globals();
    file.close();
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
    std::ofstream file(m_name.c_str(), std::ios::out | std::ios::trunc);
    perform & ucperf = const_cast<perform &>(p);
    if (! file.is_open())
    {
        printf("? error opening [%s] for writing\n", m_name.c_str());
        return false;
    }

    /*
     * Initial comments and MIDI control section.  The
     * rc_settings::get_globals() call grabs any legacy global variables that
     * are still being used, and copies them to the global rc_settings object,
     * before writing them to the "rc" configuration file.
     */

    rc().get_globals();
    if (rc().legacy_format())
    {
        file <<
           "# Sequencer64 user configuration file (legacy Seq24 0.9.2 format)\n";
    }
    else
    {
        file <<
            "# Sequencer64 0.9.9.4 (and above) rc configuration file\n"
            "# (Also works with Sequencer24)\n"
            ;
    }
    file << "\n"
        "[midi-control]\n\n"
        <<  c_midi_controls << "      # MIDI controls count\n" // constant count
        ;

    char outs[SEQ64_LINE_MAX];
    for (int mcontrol = 0; mcontrol < c_midi_controls; mcontrol++)
    {
        /*
         * \tricky
         *      32 mutes for channel, 32 group mutes, 9 odds-and-ends.
         *      This first output item merely write a <i> comment </i> to
         *      the "rc" file to indicate what the next section describes.
         *      The first section of [midi-control] specifies 74 items.
         *      The first 32 are unlabelled by a comment, and run from 0
         *      to 31.  The next 32 are labelled "mute in group", and run
         *      from 32 to 63.  The next 10 are each labelled, starting
         *      with "bpm up" and ending with "screen set play", and are
         *      each one line long.
         */

        switch (mcontrol)
        {
        case c_seqs_in_set:                 // 32
            file << "# mute in group section:\n";
            break;

        case c_midi_control_bpm_up:         // 64
            file << "# bpm up:\n";
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

        /*
         * case c_midi_controls:  74, the last value, not written.
         */

        default:
            break;
        }
        snprintf
        (
            outs, sizeof(outs),
            "%d [%1d %1d %3ld %3ld %3ld %3ld]"
                " [%1d %1d %3ld %3ld %3ld %3ld]"
                " [%1d %1d %3ld %3ld %3ld %3ld]",
             mcontrol,
             ucperf.get_midi_control_toggle(mcontrol)->m_active,
                 ucperf.get_midi_control_toggle(mcontrol)->m_inverse_active,
                 ucperf.get_midi_control_toggle(mcontrol)->m_status,
                 ucperf.get_midi_control_toggle(mcontrol)->m_data,
                 ucperf.get_midi_control_toggle(mcontrol)->m_min_value,
                 ucperf.get_midi_control_toggle(mcontrol)->m_max_value,
             ucperf.get_midi_control_on(mcontrol)->m_active,
                 ucperf.get_midi_control_on(mcontrol)->m_inverse_active,
                 ucperf.get_midi_control_on(mcontrol)->m_status,
                 ucperf.get_midi_control_on(mcontrol)->m_data,
                 ucperf.get_midi_control_on(mcontrol)->m_min_value,
                 ucperf.get_midi_control_on(mcontrol)->m_max_value,
             ucperf.get_midi_control_off(mcontrol)->m_active,
                 ucperf.get_midi_control_off(mcontrol)->m_inverse_active,
                 ucperf.get_midi_control_off(mcontrol)->m_status,
                 ucperf.get_midi_control_off(mcontrol)->m_data,
                 ucperf.get_midi_control_off(mcontrol)->m_min_value,
                 ucperf.get_midi_control_off(mcontrol)->m_max_value
        );
        file << std::string(outs) << "\n";
    }

    /*
     * Group MIDI control
     */

    file << "\n[mute-group]\n\n";
    int mtx[c_seqs_in_set];
    file <<  c_gmute_tracks << "    # group mute value count\n";
    for (int seqj = 0; seqj < c_seqs_in_set; seqj++)
    {
        ucperf.select_group_mute(seqj);
        for (int seqi = 0; seqi < c_seqs_in_set; seqi++)
        {
            mtx[seqi] = ucperf.get_group_mute_state(seqi);
        }
        snprintf
        (
            outs, sizeof(outs),
            "%d [%1d %1d %1d %1d %1d %1d %1d %1d]"
            " [%1d %1d %1d %1d %1d %1d %1d %1d]"
            " [%1d %1d %1d %1d %1d %1d %1d %1d]"
            " [%1d %1d %1d %1d %1d %1d %1d %1d]",
            seqj,
            mtx[0], mtx[1], mtx[2], mtx[3],
                mtx[4], mtx[5], mtx[6], mtx[7],
            mtx[8], mtx[9], mtx[10], mtx[11],
                mtx[12], mtx[13], mtx[14], mtx[15],
            mtx[16], mtx[17], mtx[18], mtx[19],
                mtx[20], mtx[21], mtx[22], mtx[23],
            mtx[24], mtx[25], mtx[26], mtx[27],
                mtx[28], mtx[29], mtx[30], mtx[31]
        );
        file << std::string(outs) << "\n";
    }

    /*
     * Bus mute/unmute data
     */

    int buses = ucperf.master_bus().get_num_out_buses();
    file << "\n[midi-clock]\n\n";
    file << buses << "    # number of MIDI clocks/busses\n";
    for (int bus = 0; bus < buses; bus++)
    {
        file
            << "# Output buss name: "
            << ucperf.master_bus().get_midi_out_bus_name(bus)
            << "\n"
            ;
        snprintf
        (
            outs, sizeof(outs), "%d %d  # buss number, clock status",
            bus, (char) ucperf.master_bus().get_clock(bus)
        );
        file << outs << "\n";
    }

    /*
     * MIDI clock mod
     */

    file
        << "\n\n[midi-clock-mod-ticks]\n\n"
        << midibus::get_clock_mod() << "\n"
        ;

    /*
     * Bus input data
     */

    buses = ucperf.master_bus().get_num_in_buses();
    file
        << "\n[midi-input]\n\n"
        << buses << "   # number of MIDI busses\n\n"
        ;
    for (int i = 0; i < buses; i++)
    {
        file
            << "# "
            << ucperf.master_bus().get_midi_in_bus_name(i)
            << "\n"
            ;
        snprintf
        (
            outs, sizeof(outs), "%d %d",
            i, (char) ucperf.master_bus().get_input(i)
        );
        file << outs << "\n";
    }

    /*
     * Manual ALSA ports
     */

    file
        << "\n[manual-alsa-ports]\n\n"
        << "# Set to 1 if you want sequencer64 to create its own ALSA ports and\n"
        << "# not connect to other clients\n"
        << "\n"
        << rc().manual_alsa_ports()
        << "   # flag for manual ALSA ports\n"
        ;

    /*
     * Interaction-method
     */

    int x = 0;
    file << "\n[interaction-method]\n\n";
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
        << "\n" << rc().interaction_method() << "\n\n"
        "# Set to 1 to allow Sequencer64 to stay in note-adding mode when\n"
        "# the right-click is released while holding the Mod4 (Super or\n"
        "# Windows) key.\n"
        "\n"
        << (rc().allow_mod4_mode() ? "1" : "0")   // @new 2015-08-28
        << "\n"
        ;

    size_t kevsize = ucperf.get_key_events().size() < size_t(c_seqs_in_set) ?
         ucperf.get_key_events().size() : size_t(c_seqs_in_set)
         ;
    file
        << "\n[keyboard-control]\n\n"
        << kevsize << "     # number of keys\n\n"
        << "# Key #  Sequence #  Key name\n\n"
        ;

    for
    (
        keys_perform::SlotMap::const_iterator i = ucperf.get_key_events().begin();
        i != ucperf.get_key_events().end(); ++i
    )
    {
        snprintf
        (
            outs, sizeof(outs), "%u  %ld   # %s",
            i->first, i->second,
            ucperf.key_name(i->first).c_str()   // gdk_keyval_name(i->first)
        );
        file << std::string(outs) << "\n";
    }

    size_t kegsize = ucperf.get_key_groups().size() < size_t(c_seqs_in_set) ?
         ucperf.get_key_groups().size() : size_t(c_seqs_in_set)
         ;
    file
        << "\n[keyboard-group]\n\n"
        << kegsize << "     # number of key groups\n\n"
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
            outs, sizeof(outs), "%u  %ld   # %s",
            i->first, i->second,
            ucperf.key_name(i->first).c_str()   // gdk_keyval_name(i->first)
        );
        file << std::string(outs) << "\n";
    }

    keys_perform_transfer ktx;
    ucperf.keys().get_keys(ktx);      /* copy perform key to structure    */
    file
        << "# bpm up and bpm down:\n"
        << ktx.kpt_bpm_up << " "
        << ktx.kpt_bpm_dn << "   # "
        << ucperf.key_name(ktx.kpt_bpm_up) << " "
        << ucperf.key_name(ktx.kpt_bpm_dn) << "\n"
        ;
    file
        << "# screen set up, screen set down, play:\n"
        << ktx.kpt_screenset_up << " "
        << ktx.kpt_screenset_dn << " "
        << ktx.kpt_set_playing_screenset << "   # "
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
        << "    # show_ui_sequence_key (1 = true / 0 = false)\n"
        ;

    file
        << ktx.kpt_start << " # "
        << ucperf.key_name(ktx.kpt_start)
        << " start sequencer\n"
       ;

    file
        << ktx.kpt_stop << " # "
        << ucperf.key_name(ktx.kpt_stop)
        << " stop sequencer\n"
        ;

    /**
     *  New boolean to show sequence numbers; ignored in legacy mode.
     */

    if (! rc().legacy_format())
    {
        file
            << ktx.kpt_show_ui_sequence_number << " # "
            << " show sequence numbers (1 = true / 0 = false); "
               " ignored in legacy mode\n"
            ;
    }

    file
        << "\n[jack-transport]\n\n"
        "# jack_transport - Enable sync with JACK Transport.\n\n"
        << rc().with_jack_transport() << "\n\n"
        "# jack_master - Sequencer64 attempts to serve as JACK Master.\n\n"
        << rc().with_jack_master() << "\n\n"
    "# jack_master_cond - Sequencer64 is master if no other master exists.\n\n"
        << rc().with_jack_master_cond()  << "\n\n"
        "# jack_start_mode\n"
        "# 0 = Playback in live mode. Allows muting and unmuting of loops.\n"
        "# 1 = Playback uses the song editor's data.\n\n"
        << rc().jack_start_mode() << "\n"
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
        "[last-used-dir]\n\n"
        "# Last used directory:\n\n"
        << rc().last_used_dir() << "\n\n"
        ;
    file
        << "# End of " << m_name << "\n#\n"
        << "# vim: sw=4 ts=4 wm=4 et ft=sh\n"   /* ft=sh for nice colors */
        ;
    file.close();
    return true;
}

}           // namespace seq64

/*
 * optionsfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

