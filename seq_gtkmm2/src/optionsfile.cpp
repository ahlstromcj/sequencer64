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
 *  This module declares/defines the base class for managing the
 *  <tt> ~/.seq24rc </tt> configuration file.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 *  The <tt> ~/.seq24rc </tt> configuration file is fairly simple in
 *  layout.  The documentation for this module is supplemented by the
 *  following GitHub project:
 *
 *      https://github.com/ahlstromcj/seq24-doc.git
 *
 *  That document also relates these file setting to the application's
 *  command-line options.
 *
 *  Note that these options are primarily read/written from/to the perform
 *  object that is passed to the parse() and write() functions.
 *  For an easier-to-digest list of items read and written, see the file
 *  <tt> option-storage.txt </tt> in the <tt> contrib </tt>
 *  directory.
 */

#include "midibus.hpp"
#include "optionsfile.hpp"
#include "perform.hpp"
#include "keys_perform.hpp"

namespace seq64
{

/**
 *  Principal constructor.
 */

optionsfile::optionsfile (const std::string & a_name)
 :
    configfile  (a_name)               // base class constructor
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
 *  Parse the ~/.seq24rc file.
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
optionsfile::parse (perform & a_perf)
{
    std::ifstream file(m_name.c_str(), std::ios::in | std::ios::ate);
    if (! file.is_open())
        return false;

    file.seekg(0, std::ios::beg);                           /* seek to start */
    line_after(file, "[midi-control]");                     /* find section  */
    unsigned int sequences = 0;
    sscanf(m_line, "%u", &sequences);
    next_data_line(file);
    for (unsigned int i = 0; i < sequences; ++i)
    {
        int sequence = 0;
        sscanf
        (
            m_line,
            "%d [ %d %d %ld %ld %ld %ld ]"
            " [ %d %d %ld %ld %ld %ld ]"
            " [ %d %d %ld %ld %ld %ld ]",
            &sequence,
            (int *) &a_perf.get_midi_control_toggle(i)->m_active,
            (int *) &a_perf.get_midi_control_toggle(i)->m_inverse_active,
            &a_perf.get_midi_control_toggle(i)->m_status,
            &a_perf.get_midi_control_toggle(i)->m_data,
            &a_perf.get_midi_control_toggle(i)->m_min_value,
            &a_perf.get_midi_control_toggle(i)->m_max_value,

            (int *) &a_perf.get_midi_control_on(i)->m_active,
            (int *) &a_perf.get_midi_control_on(i)->m_inverse_active,
            &a_perf.get_midi_control_on(i)->m_status,
            &a_perf.get_midi_control_on(i)->m_data,
            &a_perf.get_midi_control_on(i)->m_min_value,
            &a_perf.get_midi_control_on(i)->m_max_value,

            (int *) &a_perf.get_midi_control_off(i)->m_active,
            (int *) &a_perf.get_midi_control_off(i)->m_inverse_active,
            &a_perf.get_midi_control_off(i)->m_status,
            &a_perf.get_midi_control_off(i)->m_data,
            &a_perf.get_midi_control_off(i)->m_min_value,
            &a_perf.get_midi_control_off(i)->m_max_value
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
        a_perf.select_group_mute(j);
        sscanf
        (
            m_line,
            "%d [%d %d %d %d %d %d %d %d]"
            " [%d %d %d %d %d %d %d %d]"
            " [%d %d %d %d %d %d %d %d]"
            " [%d %d %d %d %d %d %d %d]",
            &j,
            &mtx[0], &mtx[1], &mtx[2], &mtx[3],
            &mtx[4], &mtx[5], &mtx[6], &mtx[7],

            &mtx[8], &mtx[9], &mtx[10], &mtx[11],
            &mtx[12], &mtx[13], &mtx[14], &mtx[15],

            &mtx[16], &mtx[17], &mtx[18], &mtx[19],
            &mtx[20], &mtx[21], &mtx[22], &mtx[23],

            &mtx[24], &mtx[25], &mtx[26], &mtx[27],
            &mtx[28], &mtx[29], &mtx[30], &mtx[31]
        );
        for (int k = 0; k < c_seqs_in_set; k++)
            a_perf.set_group_mute_state(k, mtx[k]);

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
        a_perf.master_bus().set_clock(bus, (clock_e) bus_on);
        next_data_line(file);
    }

    line_after(file, "[keyboard-control]");
    long keys = 0;
    sscanf(m_line, "%ld", &keys);
    next_data_line(file);
    a_perf.get_key_events().clear();
    for (int i = 0; i < keys; ++i)
    {
        long key = 0, seq = 0;
        sscanf(m_line, "%ld %ld", &key, &seq);
        a_perf.set_key_event(key, seq);
        next_data_line(file);
    }

    line_after(file, "[keyboard-group]");
    long groups = 0;
    sscanf(m_line, "%ld", &groups);
    next_data_line(file);
    a_perf.get_key_groups().clear();
    for (int i = 0; i < groups; ++i)
    {
        long key = 0, group = 0;
        sscanf(m_line, "%ld %ld", &key, &group);
        a_perf.set_key_group(key, group);
        next_data_line(file);
    }

#ifndef USE_NEW_KEYS_CODE

    sscanf(m_line, "%u %u", &a_perf.m_key_bpm_up, &a_perf.m_key_bpm_dn);
    next_data_line(file);
    sscanf
    (
        m_line, "%u %u %u",
        &a_perf.m_key_screenset_up,
        &a_perf.m_key_screenset_dn,
        &a_perf.m_key_set_playing_screenset
    );
    next_data_line(file);
    sscanf
    (
        m_line, "%u %u %u",
        &a_perf.m_key_group_on,
        &a_perf.m_key_group_off,
        &a_perf.m_key_group_learn
    );
    next_data_line(file);
    sscanf
    (
        m_line, "%u %u %u %u %u",
        &a_perf.m_key_replace,
        &a_perf.m_key_queue,
        &a_perf.m_key_snapshot_1,
        &a_perf.m_key_snapshot_2,
        &a_perf.m_key_keep_queue
    );

    int show_key = 0;
    next_data_line(file);
    sscanf(m_line, "%d", &show_key);
    a_perf.m_show_ui_sequence_key = (bool) show_key;
    next_data_line(file);
    sscanf(m_line, "%u", &a_perf.m_key_start);
    next_data_line(file);
    sscanf(m_line, "%u", &a_perf.m_key_stop);

#else   // USE_NEW_KEYS_CODE

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
    ktx.kpt_show_ui_sequence_key = (bool) show_key;
    next_data_line(file);
    sscanf(m_line, "%u", &ktx.kpt_start);
    next_data_line(file);
    sscanf(m_line, "%u", &ktx.kpt_stop);
    a_perf.keys().set_keys(ktx);                /* copy into perform keys   */

#endif  // USE_NEW_KEYS_CODE

    line_after(file, "[jack-transport]");
    long flag = 0;
    sscanf(m_line, "%ld", &flag);
    global_with_jack_transport = (bool) flag;

    next_data_line(file);
    sscanf(m_line, "%ld", &flag);
    global_with_jack_master = (bool) flag;

    next_data_line(file);
    sscanf(m_line, "%ld", &flag);
    global_with_jack_master_cond = (bool) flag;

    next_data_line(file);
    sscanf(m_line, "%ld", &flag);
    global_jack_start_mode = (bool) flag;

    line_after(file, "[midi-input]");
    buses = 0;
    sscanf(m_line, "%ld", &buses);
    next_data_line(file);
    for (int i = 0; i < buses; ++i)
    {
        long bus_on, bus;
        sscanf(m_line, "%ld %ld", &bus, &bus_on);
        a_perf.master_bus().set_input(bus, (bool) bus_on);
        next_data_line(file);
    }

    line_after(file, "[midi-clock-mod-ticks]");
    long ticks = 64;
    sscanf(m_line, "%ld", &ticks);
    midibus::set_clock_mod(ticks);

    line_after(file, "[manual-alsa-ports]");
    sscanf(m_line, "%ld", &flag);
    global_manual_alsa_ports = (bool) flag;

    line_after(file, "[last-used-dir]");
    if (m_line[0] == '/')
        global_last_used_dir.assign(m_line); // FIXME: need check for valid path

    long method = 0;
    line_after(file, "[interaction-method]");
    sscanf(m_line, "%ld", &method);
    global_interactionmethod = interaction_method_e(method);
    next_data_line(file);                   // @new 2015-08-28
    sscanf(m_line, "%ld", &method);         //
    global_allow_mod4_mode = method != 0;   //

    file.close();
    return true;
}

/**
 *  This options-writing function is just about as complex as the
 *  options-reading function.
 *
 * \param a_perf
 *      Provides a const reference to the main perform object.  However,
 *      we have to cast away the constness, because too many of the
 *      perform getter functions are used in non-const contexts.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
optionsfile::write (const perform & a_perf)
{
    std::ofstream file(m_name.c_str(), std::ios::out | std::ios::trunc);
    char outs[SEQ64_LINE_MAX];
    perform & uca_perf = const_cast<perform &>(a_perf);
    if (! file.is_open())
        return false;

    /*
     * Initial comments and MIDI control section
     */

    file
        << "#\n"
        << "# Sequencer24 0.9.4 (and above) initialization file\n"
        << "#\n"
        << "[midi-control]\n"
        <<  c_midi_controls << "\n"               // constant count
        ;
    for (int i = 0; i < c_midi_controls; i++)
    {
        /* 32 mutes for channel, 32 group mutes */

        switch (i)
        {
        case c_seqs_in_set:
            file << "# mute in group\n";
            break;

        case c_midi_control_bpm_up:
            file << "# bpm up\n";
            break;

        case c_midi_control_bpm_dn:
            file << "# bpm down\n";
            break;

        case c_midi_control_ss_up:
            file << "# screen set up\n";
            break;

        case c_midi_control_ss_dn:
            file << "# screen set down\n";
            break;

        case c_midi_control_mod_replace:
            file << "# mod replace\n";
            break;

        case c_midi_control_mod_snapshot:
            file << "# mod snapshot\n";
            break;

        case c_midi_control_mod_queue:
            file << "# mod queue\n";
            break;

        case c_midi_control_mod_gmute:
            file << "# mod gmute\n";
            break;

        case c_midi_control_mod_glearn:
            file << "# mod glearn\n";
            break;

        case c_midi_control_play_ss:
            file << "# screen set play\n";
            break;

        default:
            break;
        }
        snprintf
        (
            outs, sizeof(outs),
            "%d [%1d %1d %3ld %3ld %3ld %3ld]"
            " [%1d %1d %3ld %3ld %3ld %3ld]"
            " [%1d %1d %3ld %3ld %3ld %3ld]",
             i,
             uca_perf.get_midi_control_toggle(i)->m_active,
             uca_perf.get_midi_control_toggle(i)->m_inverse_active,
             uca_perf.get_midi_control_toggle(i)->m_status,
             uca_perf.get_midi_control_toggle(i)->m_data,
             uca_perf.get_midi_control_toggle(i)->m_min_value,
             uca_perf.get_midi_control_toggle(i)->m_max_value,

             uca_perf.get_midi_control_on(i)->m_active,
             uca_perf.get_midi_control_on(i)->m_inverse_active,
             uca_perf.get_midi_control_on(i)->m_status,
             uca_perf.get_midi_control_on(i)->m_data,
             uca_perf.get_midi_control_on(i)->m_min_value,
             uca_perf.get_midi_control_on(i)->m_max_value,

             uca_perf.get_midi_control_off(i)->m_active,
             uca_perf.get_midi_control_off(i)->m_inverse_active,
             uca_perf.get_midi_control_off(i)->m_status,
             uca_perf.get_midi_control_off(i)->m_data,
             uca_perf.get_midi_control_off(i)->m_min_value,
             uca_perf.get_midi_control_off(i)->m_max_value
        );
        file << std::string(outs) << "\n";
    }

    /*
     * Group MIDI control
     */

    file << "\n[mute-group]\n";
    int mtx[c_seqs_in_set];
    file <<  c_gmute_tracks << "\n";
    for (int j = 0; j < c_seqs_in_set; j++)
    {
        uca_perf.select_group_mute(j);
        for (int i = 0; i < c_seqs_in_set; i++)
        {
            mtx[i] = uca_perf.get_group_mute_state(i);
        }
        snprintf
        (
            outs, sizeof(outs),
            "%d [%1d %1d %1d %1d %1d %1d %1d %1d]"
            " [%1d %1d %1d %1d %1d %1d %1d %1d]"
            " [%1d %1d %1d %1d %1d %1d %1d %1d]"
            " [%1d %1d %1d %1d %1d %1d %1d %1d]",
            j,
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

    int buses = uca_perf.master_bus().get_num_out_buses();
    file << "\n[midi-clock]\n";
    file << buses << "\n";
    for (int i = 0; i < buses; i++)
    {
        file
            << "# "
            << uca_perf.master_bus().get_midi_out_bus_name(i)
            << "\n"
            ;
        snprintf
        (
            outs, sizeof(outs), "%d %d",
            i, (char) uca_perf.master_bus().get_clock(i)
        );
        file << outs << "\n";
    }

    /*
     * MIDI clock mod
     */

    file
        << "\n\n[midi-clock-mod-ticks]\n"
        << midibus::get_clock_mod() << "\n"
        ;

    /*
     * Bus input data
     */

    buses = uca_perf.master_bus().get_num_in_buses();
    file
        << "\n[midi-input]\n"
        << buses << "\n"
        ;
    for (int i = 0; i < buses; i++)
    {
        file
            << "# "
            << uca_perf.master_bus().get_midi_in_bus_name(i)
            << "\n"
            ;
        snprintf
        (
            outs, sizeof(outs), "%d %d",
            i, (char) uca_perf.master_bus().get_input(i)
        );
        file << outs << "\n";
    }

    /*
     * Manual ALSA ports
     */

    file
        << "\n[manual-alsa-ports]\n"
        << "# Set to 1 if you want seq24 to create its own ALSA ports and\n"
        << "# not connect to other clients\n"
        << "\n"
        << global_manual_alsa_ports << "\n"
        ;

    /*
     * Interaction-method
     */

    int x = 0;
    file << "\n[interaction-method]\n";
    while (c_interaction_method_names[x] && c_interaction_method_descs[x])
    {
        file
            << "# " << x
            << " - '" << c_interaction_method_names[x]
            << "' (" << c_interaction_method_descs[x] << ")\n"
            ;
        ++x;
    }
    file << global_interactionmethod << "\n";

    file
        << "\n"
        << "# Set to 1 to allow seq24 to stay in note-adding mode when\n"
        << "# the right-click is released while holding the Mod4 (Super or\n"
        << "# Windows) key.\n"
        << "\n"
        << (global_allow_mod4_mode ? "1" : "0") << "\n";   // @new 2015-08-28

    size_t kevsize = uca_perf.get_key_events().size() < (size_t) c_seqs_in_set ?
         uca_perf.get_key_events().size() :
         (size_t) c_seqs_in_set
         ;
    file
        << "\n[keyboard-control]\n"
        << "# Key #, Sequence #\n"
        << kevsize << "\n"
        ;

    for
    (
        perform::SlotMap::const_iterator i = uca_perf.get_key_events().begin();
        i != uca_perf.get_key_events().end(); ++i
    )
    {
        snprintf
        (
            outs, sizeof(outs), "%u  %ld # %s",
            i->first, i->second, gdk_keyval_name(i->first)
        );
        file << std::string(outs) << "\n";
    }
    size_t kegsize = uca_perf.get_key_groups().size() < size_t(c_seqs_in_set) ?
         uca_perf.get_key_groups().size() :
         (size_t)c_seqs_in_set
         ;
    file
        << "\n[keyboard-group]\n"
        << "# Key #, group # \n"
        << "\n"
        << kegsize << "\n"
        ;

    for
    (
        perform::SlotMap::const_iterator i = uca_perf.get_key_groups().begin();
        i != uca_perf.get_key_groups().end(); ++i
    )
    {
        snprintf
        (
            outs, sizeof(outs), "%u  %ld # %s",
            i->first, i->second, gdk_keyval_name(i->first)
        );
        file << std::string(outs) << "\n";
    }

#ifndef USE_NEW_KEYS_CODE

    file
        << "# bpm up, down\n"
        << uca_perf.m_key_bpm_up << " "
        << uca_perf.m_key_bpm_dn << " # "
        << gdk_keyval_name(uca_perf.m_key_bpm_up) << " "
        << gdk_keyval_name(uca_perf.m_key_bpm_dn) << "\n"
        ;
    file
        << "# screen set up, down, play\n"
        << uca_perf.m_key_screenset_up << " "
        << uca_perf.m_key_screenset_dn << " "
        << uca_perf.m_key_set_playing_screenset << " # "
        << gdk_keyval_name(uca_perf.m_key_screenset_up) << " "
        << gdk_keyval_name(uca_perf.m_key_screenset_dn) << " "
        << gdk_keyval_name(uca_perf.m_key_set_playing_screenset) << "\n"
        ;
    file
        << "# group on, off, learn\n"
        << uca_perf.m_key_group_on << " "
        << uca_perf.m_key_group_off << " "
        << uca_perf.m_key_group_learn << " # "
        << gdk_keyval_name(uca_perf.m_key_group_on) << " "
        << gdk_keyval_name(uca_perf.m_key_group_off) << " "
        << gdk_keyval_name(uca_perf.m_key_group_learn) << "\n"
        ;
    file
        << "# replace, queue, snapshot_1, snapshot 2, keep queue\n"
        << uca_perf.m_key_replace << " "
        << uca_perf.m_key_queue << " "
        << uca_perf.m_key_snapshot_1 << " "
        << uca_perf.m_key_snapshot_2 << " "
        << uca_perf.m_key_keep_queue << " # "
        << gdk_keyval_name(uca_perf.m_key_replace) << " "
        << gdk_keyval_name(uca_perf.m_key_queue) << " "
        << gdk_keyval_name(uca_perf.m_key_snapshot_1) << " "
        << gdk_keyval_name(uca_perf.m_key_snapshot_2) << " "
        << gdk_keyval_name(uca_perf.m_key_keep_queue) << "\n"
        ;
    file
        << uca_perf.m_show_ui_sequence_key
        << " # show_ui_sequence_key (1=true/0=false)\n"
        ;
    file
        << uca_perf.m_key_start << " # "
        << gdk_keyval_name(uca_perf.m_key_start)
        << " start sequencer\n"
       ;
    file
        << uca_perf.m_key_stop << " # "
        << gdk_keyval_name(uca_perf.m_key_stop)
        << " stop sequencer\n"
        ;

#else   // USE_NEW_KEYS_CODE

    keys_perform_transfer ktx;
    uca_perf.keys().get_keys(ktx);      /* copy perform key to structure    */
    file
        << "# bpm up, down\n"
        << ktx.kpt_bpm_up << " "
        << ktx.kpt_bpm_dn << " # "
        << gdk_keyval_name(ktx.kpt_bpm_up) << " "
        << gdk_keyval_name(ktx.kpt_bpm_dn) << "\n"
        ;
    file
        << "# screen set up, down, play\n"
        << ktx.kpt_screenset_up << " "
        << ktx.kpt_screenset_dn << " "
        << ktx.kpt_set_playing_screenset << " # "
        << gdk_keyval_name(ktx.kpt_screenset_up) << " "
        << gdk_keyval_name(ktx.kpt_screenset_dn) << " "
        << gdk_keyval_name(ktx.kpt_set_playing_screenset) << "\n"
        ;
    file
        << "# group on, off, learn\n"
        << ktx.kpt_group_on << " "
        << ktx.kpt_group_off << " "
        << ktx.kpt_group_learn << " # "
        << gdk_keyval_name(ktx.kpt_group_on) << " "
        << gdk_keyval_name(ktx.kpt_group_off) << " "
        << gdk_keyval_name(ktx.kpt_group_learn) << "\n"
        ;
    file
        << "# replace, queue, snapshot_1, snapshot 2, keep queue\n"
        << ktx.kpt_replace << " "
        << ktx.kpt_queue << " "
        << ktx.kpt_snapshot_1 << " "
        << ktx.kpt_snapshot_2 << " "
        << ktx.kpt_keep_queue << " # "
        << gdk_keyval_name(ktx.kpt_replace) << " "
        << gdk_keyval_name(ktx.kpt_queue) << " "
        << gdk_keyval_name(ktx.kpt_snapshot_1) << " "
        << gdk_keyval_name(ktx.kpt_snapshot_2) << " "
        << gdk_keyval_name(ktx.kpt_keep_queue) << "\n"
        ;
    file
        << ktx.kpt_show_ui_sequence_key
        << " # show_ui_sequence_key (1=true/0=false)\n"
        ;
    file
        << ktx.kpt_start << " # "
        << gdk_keyval_name(ktx.kpt_start)
        << " start sequencer\n"
       ;
    file
        << ktx.kpt_stop << " # "
        << gdk_keyval_name(ktx.kpt_stop)
        << " stop sequencer\n"
        ;

#endif  // USE_NEW_KEYS_CODE

    file
        << "\n[jack-transport]\n\n"
        << "# jack_transport - Enable sync with JACK Transport.\n"
        << global_with_jack_transport << "\n\n"
        << "# jack_master - Seq24 will attempt to serve as JACK Master.\n"
        << global_with_jack_master << "\n\n"
        << "# jack_master_cond - Seq24 won't be master if another master exists.\n"
        << global_with_jack_master_cond  << "\n\n"
        << "# jack_start_mode\n"
        << "# 0 = Playback in live mode. Allows muting and unmuting of loops.\n"
        << "# 1 = Playback uses the song editor's data.\n"
        << global_jack_start_mode << "\n\n"
        ;
    file
        << "\n[last-used-dir]\n\n"
        << "# Last used directory.\n"
        << global_last_used_dir << "\n\n"
        ;

    file.close();
    return true;
}

}           // namespace seq64

/*
 * optionsfile.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
