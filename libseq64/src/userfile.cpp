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
 * \file          userfile.cpp
 *
 *  This module declares/defines the base class for managing the user's
 *  <code> ~/.seq24usr </code> or <code> ~/.config/sequencer64/sequencer64.rc
 *  </code> configuration file.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2018-05-28
 * \license       GNU GPLv2 or above
 *
 *  Note that the parse function has some code that is not yet enabled.
 *  Also note that, unlike the "rc" settings, these settings have no
 *  user-interface.  One must use a text editor to modify its settings.
 */

#include <iostream>

#include "globals.h"
#include "file_functions.hpp"           /* seq64::strip_quotes()        */
#include "settings.hpp"                 /* seq64::rc()                  */
#include "userfile.hpp"                 /* seq64::userfile              */
#include "user_instrument.hpp"          /* seq64::user_instrument       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

//////// class perform;          // temporary forward reference

/**
 *  Principal constructor.
 *
 * \param name
 *      Provides the full file path specification to the configuration file.
 */

userfile::userfile (const std::string & name)
  :
    configfile (name)
{
    // Empty body
}

/**
 *  A rote destructor needed for a derived class.
 */

userfile::~userfile ()
{
    // Empty body
}

/**
 *  Provides a purely internal, ad hoc helper function to create numbered
 *  section names for the userfile class.
 *
 * \param label
 *      The base-name of the section.
 *
 * \param value
 *      The numeric value to append to the section name.
 *
 * \return
 *      Returns a string of the form "[basename-1]".
 */

static std::string
make_section_name (const std::string & label, int value)
{
    char temp[16];
    snprintf(temp, sizeof temp, "%d", value);
    std::string result = "[";
    result += label;
    result += "-";
    result += temp;
    result += "]";
    return result;
}

/**
 *  Provides a debug dump of basic information to help debug a surprisingly
 *  intractable problem with all busses having the name and values of the last
 *  buss in the configuration.  Does work only if PLATFORM_DEBUG is defined;
 *  see the user_settings class.
 */

void
userfile::dump_setting_summary ()
{
    usr().dump_summary();
}

/**
 *  Parses a "usr" file, filling in the given perform object.  This function
 *  opens the file as a text file (line-oriented).
 *
 * \param a_perf
 *      The performance object, currently unused.
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
userfile::parse (perform & /* p */)
{
    std::ifstream file(m_name.c_str(), std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        fprintf(stderr, "? error opening [%s]\n", m_name.c_str());
        return false;
    }
    file.seekg(0, std::ios::beg);                       /* seek to start    */

    /*
     * [comments]
     *
     * Header commentary is skipped during parsing.  However, we now try to
     * read an optional comment block.
     */

    if (line_after(file, "[comments]"))                 /* gets first line  */
    {
        usr().clear_comments();
        do
        {
            usr().append_comment_line(m_line);
            usr().append_comment_line("\n");

        } while (next_data_line(file));
    }

    /*
     *  Next, if we're using the manual or hide ALSA port options specified in
     *  the "rc" file, do want to override the ports that the queries of the
     *  ALSA system find.  Otherwise, we might want to reveal the names
     *  obtained by port detection for ALSA, and do not want to override the
     *  names of the ports that are found.
     */

    if (! rc().reveal_alsa_ports())
    {
        /*
         * [user-midi-bus-definitions]
         */

        int buses = 0;
        if (line_after(file, "[user-midi-bus-definitions]"))
            sscanf(m_line, "%d", &buses);               /* atavistic!       */

        /*
         * [user-midi-bus-x]
         */

        for (int bus = 0; bus < buses; ++bus)
        {
            std::string label = make_section_name("user-midi-bus", bus);
            if (! line_after(file, label))
                break;

            if (usr().add_bus(m_line))
            {
                (void) next_data_line(file);
                int instruments = 0;
                sscanf(m_line, "%d", &instruments);     /* no. of channels  */
                for (int j = 0; j < instruments; ++j)
                {
                    int channel, instrument;
                    (void) next_data_line(file);
                    sscanf(m_line, "%d %d", &channel, &instrument);
                    usr().set_bus_instrument(bus, channel, instrument);
                }
            }
            else
            {
                fprintf
                (
                    stderr, "? error adding %s (line = '%s')\n",
                    label.c_str(), m_line
                );
            }
        }
    }

    /*
     * [user-instrument-definitions]
     */

    int instruments = 0;
    if (line_after(file, "[user-instrument-definitions]"))
        sscanf(m_line, "%d", &instruments);

    /*
     * [user-instrument-x]
     */

    for (int i = 0; i < instruments; ++i)
    {
        std::string label = make_section_name("user-instrument", i);
        if (! line_after(file, label))
            break;

        if (usr().add_instrument(m_line))
        {
            char ccname[SEQ64_LINE_MAX];
            int ccs = 0;
            (void) next_data_line(file);
            sscanf(m_line, "%d", &ccs);
            for (int j = 0; j < ccs; ++j)
            {
                int c = 0;
                if (! next_data_line(file))
                    break;

                ccname[0] = 0;                              // clear the buffer
                sscanf(m_line, "%d %[^\n]", &c, ccname);
                if (c >= 0 && c < SEQ64_MIDI_CONTROLLER_MAX)      // 128
                {
                    std::string name(ccname);
                    if (name.empty())
                    {
                        /*
                         * TMI:
                         *
                        fprintf
                        (
                            stderr, "? missing controller name for '%s'\n",
                            label.c_str()
                        );
                         */
                    }
                    else
                        usr().set_instrument_controllers(i, c, name, true);
                }
                else
                {
                    fprintf
                    (
                        stderr, "? illegal controller value %d for '%s'\n",
                        c, label.c_str()
                    );
                }
            }
        }
        else
        {
            fprintf
            (
                stderr, "? error adding %s (line = '%s')\n",
                label.c_str(), m_line
            );
        }
    }

    /*
     * [user-interface-settings]
     *
     * These are new items stored in the user file.  Only variables whose
     * effects we can be completely sure of are read from this section, and
     * used, at this time.  More to come.
     */

    if (! rc().legacy_format())
    {
        int scratch = 0;
        if (line_after(file, "[user-interface-settings]"))
        {
            sscanf(m_line, "%d", &scratch);
            usr().grid_style(scratch);

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().grid_brackets(scratch);

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().mainwnd_rows(scratch);

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().mainwnd_cols(scratch);

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().max_sets(scratch);            /* should ignore this setting */

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().mainwid_border(scratch);

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().mainwid_spacing(scratch);

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().control_height(scratch);

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().zoom(scratch);

            /*
             * This boolean affects the behavior of the scale, key, and
             * background sequence features, but their actual values are
             * stored in the MIDI file, not in the "user" configuration file.
             */

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().global_seq_feature(scratch != 0);

            /*
             * The user-interface font is now selectable at run time.  Old
             * versus new font.
             */

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().use_new_font(scratch != 0);

            (void) next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().allow_two_perfedits(scratch != 0);

            if (next_data_line(file))
            {
                sscanf(m_line, "%d", &scratch);
                usr().perf_h_page_increment(scratch);
            }

            if (next_data_line(file))
            {
                sscanf(m_line, "%d", &scratch);
                usr().perf_v_page_increment(scratch);
            }

            /*
             *  Here, we start checking the lines, on the theory that these
             *  very new (2016-02-14) items might mess up people who already
             *  have older Sequencer64 "user" configuration files.
             */

            if (next_data_line(file))
            {
                sscanf(m_line, "%d", &scratch);             /* now an int   */
                usr().progress_bar_colored(scratch);        /* pick a color */
                if (next_data_line(file))
                {
                    sscanf(m_line, "%d", &scratch);
                    usr().progress_bar_thick(scratch != 0);
                }
                if (next_data_line(file))
                {
                    sscanf(m_line, "%d", &scratch);
                    if (scratch <= 1)                       /* boolean?     */
                    {
                        usr().inverse_colors(scratch != 0);
                        if (next_data_line(file))
                            sscanf(m_line, "%d", &scratch); /* get redraw   */
                    }
                    if (scratch < SEQ64_MINIMUM_REDRAW)
                        scratch = SEQ64_MINIMUM_REDRAW;
                    else if (scratch > SEQ64_MAXIMUM_REDRAW)
                        scratch = SEQ64_MAXIMUM_REDRAW;

                    usr().window_redraw_rate(scratch);
                }
                if (next_data_line(file))
                {
                    sscanf(m_line, "%d", &scratch);
                    if (scratch <= 1)                       /* boolean?     */
                        usr().use_more_icons(scratch != 0);
                }

#if defined SEQ64_MULTI_MAINWID

                if (next_data_line(file))
                {
                    sscanf(m_line, "%d", &scratch);
                    if (scratch > 0 && scratch <= SEQ64_MAINWID_BLOCK_ROWS_MAX)
                        usr().block_rows(scratch);
                }
                if (next_data_line(file))
                {
                    sscanf(m_line, "%d", &scratch);
                    if (scratch > 0 && scratch <= SEQ64_MAINWID_BLOCK_COLS_MAX)
                        usr().block_columns(scratch);
                }
                if (next_data_line(file))
                {
                    sscanf(m_line, "%d", &scratch);
                    usr().block_independent(scratch != 0);
                }

#endif  // SEQ64_MULTI_MAINWID

                if (next_data_line(file))
                {
                    float scale = 1.0f;
                    sscanf(m_line, "%f", &scale);
                    usr().window_scale(scale);
                }
            }
        }
        usr().normalize();    /* calculate derived values */
    }
    else
    {
        /*
         * Legacy values.  Compare to user_settings::set_defaults().
         */

        usr().grid_style(0);
        usr().grid_brackets(1);
        usr().mainwnd_rows(SEQ64_MIN_MAINWND_ROWS);
        usr().mainwnd_cols(SEQ64_DEFAULT_MAINWND_COLUMNS);
        usr().max_sets(SEQ64_DEFAULT_SET_MAX);
        usr().mainwid_border(0);
        usr().mainwid_spacing(2);
        usr().control_height(0);
        usr().zoom(SEQ64_DEFAULT_ZOOM);
        usr().global_seq_feature(false);
        usr().use_new_font(false);
        usr().allow_two_perfedits(false);
        usr().perf_h_page_increment(1);
        usr().perf_v_page_increment(1);
        usr().progress_bar_colored(0);
        usr().progress_bar_thick(false);
        usr().inverse_colors(false);
        usr().window_redraw_rate(c_redraw_ms);
    }

    /*
     * [user-midi-settings]
     */

    if (! rc().legacy_format())
    {
        if (line_after(file, "[user-midi-settings]"))
        {
            int scratch = 0;
            sscanf(m_line, "%d", &scratch);
            usr().midi_ppqn(scratch);

            next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().midi_beats_per_bar(scratch);

            float beatspm;
            next_data_line(file);
            sscanf(m_line, "%f", &beatspm);
            usr().midi_beats_per_minute(midibpm(beatspm));

            next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().midi_beat_width(scratch);

            next_data_line(file);
            sscanf(m_line, "%d", &scratch);
            usr().midi_buss_override(char(scratch));

            if (next_data_line(file))
            {
                sscanf(m_line, "%d", &scratch);
                usr().velocity_override(scratch);
            }
            if (next_data_line(file))
            {
                sscanf(m_line, "%d", &scratch);
                usr().bpm_precision(scratch);
            }
            if (next_data_line(file))
            {
                float inc;
                sscanf(m_line, "%f", &inc);
                usr().bpm_step_increment(midibpm(inc));
            }
            if (next_data_line(file))
            {
                float inc;
                sscanf(m_line, "%f", &inc);
                usr().bpm_page_increment(midibpm(inc));
            }
            if (next_data_line(file))
            {
                sscanf(m_line, "%f", &beatspm);
                usr().midi_bpm_minimum(midibpm(beatspm));
            }
            if (next_data_line(file))
            {
                sscanf(m_line, "%f", &beatspm);
                usr().midi_bpm_maximum(midibpm(beatspm));
            }
        }

        /*
         * -o special options support.
         */

        if (line_after(file, "[user-options]"))
        {
            int scratch = 0;
            sscanf(m_line, "%d", &scratch);
            usr().option_daemonize(scratch != 0);

            char temp[256];
            if (next_data_line(file))
            {
                sscanf(m_line, "%s", temp);
                std::string logfile = std::string(temp);
                if (logfile == "\"\"")
                    logfile.clear();
                else
                {
                    logfile = strip_quotes(logfile);
                    printf("[option_logfile: '%s']\n", logfile.c_str());
                }
                usr().option_logfile(logfile);
            }
        }

        /*
         * Work-arounds for sticky issues
         */

        if (line_after(file, "[user-work-arounds]"))
        {
            int scratch = 0;
            sscanf(m_line, "%d", &scratch);
            usr().work_around_play_image(scratch != 0);
            if (next_data_line(file))
            {
                sscanf(m_line, "%d", &scratch);
                usr().work_around_transpose_image(scratch != 0);
            }
        }

        /*
         * [user-ui-tweaks]
         */

        if (line_after(file, "[user-ui-tweaks]"))
        {
            int scratch = 0;
            sscanf(m_line, "%d", &scratch);
            usr().key_height(scratch);
        }

    }

    /*
     * We have all of the data.  Close the file.
     */

    dump_setting_summary();
    file.close();                       /* End Of File, EOF, done! */
    return true;
}

/**
 *  This function just returns false, as there is no "perform" information
 *  in the user-file yet.
 *
 * \param a_perf
 *      The performance object, currently unused.
 *
 * \return
 *      Returns true if the writing succeeded.
 */

bool
userfile::write (const perform & /* a_perf */ )
{
    std::ofstream file(m_name.c_str(), std::ios::out | std::ios::trunc);
    if (! file.is_open())
    {
        fprintf(stderr, "? error opening [%s] for writing\n", m_name.c_str());
        return false;
    }
    dump_setting_summary();

    /*
     * Header commentary.  Write out comments about the nature of this file.
     */

    if (rc().legacy_format())
    {
        file <<
           "# Sequencer64 user-configuration file (legacy Seq24 0.9.2 format)\n";
    }
    else
        file << "# Sequencer64 0.95.0 (and above) user-configuration file\n";

    file <<
        "#\n"
        "# Created by reading the following file and writing it out via the\n"
        "# Sequencer64 application:\n"
        "#\n"
        "# https://raw.githubusercontent.com/vext01/"
               "seq24/master/seq24usr.example\n"
        "#\n"
        "# This is a sequencer64.usr file. Edit it and place it in the\n"
        "# $HOME/.config/sequencer64 directory. It allows one to provide an\n"
        "# alias (alternate name) to each MIDI bus, MIDI channel, and MIDI\n"
        "# control codes per channel.\n"
        ;

    if (! rc().legacy_format())
    {
        file <<
        "#\n"
        "# The [comments] section lets one document this file.  Lines starting\n"
        "# with '#' and '[' are ignored.  Blank lines are ignored.  To show a\n"
        "# blank line, add a space character to the line.\n"
            ;

        /*
         * [comments]
         */

        file << "\n[comments]\n\n" << usr().comments_block() << "\n";
    }

    file <<
        "# 1. Define your instruments and their control-code names,\n"
        "#    if they have them.\n"
        "# 2. Define a MIDI bus, its name, and what instruments are\n"
        "#    on which channel.\n"
        "#\n"
        "# In the following MIDI buss definitions, channels are counted\n"
        "# from 0 to 15, not 1 to 16.  Instruments not set here are set to\n"
        "# -1 (SEQ64_GM_INSTRUMENT_FLAG) and are GM (General MIDI).\n"
        "# These replacement MIDI buss labels are shown in MIDI Clocks,\n"
        "# MIDI Inputs, and in the Pattern Editor buss drop-down.\n"
        "#\n"
        "# To temporarily disable the entries, set the count values to 0.\n"
        ;

    /*
     * [user-midi-bus-definitions]
     */

    file << "\n"
        << "[user-midi-bus-definitions]\n"
        << "\n"
        << usr().bus_count()
        << "     # number of user-defined MIDI busses\n"
        ;

    if (usr().bus_count() == 0)
        file << "\n";

    /*
     * [user-midi-bus-x]
     */

    for (int buss = 0; buss < usr().bus_count(); ++buss)
    {
        file << "\n[user-midi-bus-" << buss << "]\n\n";
        const user_midi_bus & umb = usr().bus(buss);
        if (umb.is_valid())
        {
            file
                << "# Device name\n"
                << umb.name() << "\n"
                << "\n"
                << umb.channel_count()
                << "      # number of instrument settings\n"
                ;

            for (int channel = 0; channel < umb.channel_count(); ++channel)
            {
                if (umb.instrument(channel) != SEQ64_GM_INSTRUMENT_FLAG)
                {
                    file
                        << channel << " " << umb.instrument(channel)
                        << "    # channel index & instrument number\n"
                        ;
                }
#if defined PLATFORM_DEBUG && defined SHOW_IGNORED_ITEMS
                else
                {
                    fprintf
                    (
                        stderr, "bus %d, channel %d (%d) ignored\n",
                        buss, channel, umb.instrument(channel)
                    );
                }
#endif
            }
        }
        else
            file << "? This buss specification is invalid\n";
    }

    file << "\n"
        "# In the following MIDI instrument definitions, active controller\n"
        "# numbers (i.e. supported by the instrument) are paired with\n"
        "# the (optional) name of the controller supported.\n"
        ;

    /*
     * [user-instrument-definitions]
     */

    file << "\n"
        << "[user-instrument-definitions]\n"
        << "\n"
        << usr().instrument_count()
        << "     # instrument list count\n"
        ;

    if (usr().instrument_count() == 0)
        file << "\n";

    /*
     * [user-instrument-x]
     */

    for (int inst = 0; inst < usr().instrument_count(); ++inst)
    {
        file << "\n[user-instrument-" << inst << "]\n"
            << "\n"
            ;

        const user_instrument & uin = usr().instrument(inst);
        if (uin.is_valid())
        {
            file
                << "# Name of instrument\n"
                << uin.name() << "\n"
                << "\n"
                << uin.controller_count()
                << "    # number of MIDI controller number & name pairs\n"
                ;

            if (uin.controller_count() > 0)
            {
                for (int ctlr = 0; ctlr < uin.controller_max(); ++ctlr)
                {
                    if (uin.controller_active(ctlr))
                    {
                        file << ctlr << " " << uin.controller_name(ctlr) << "\n";

                            /*
                             * TMI
                             *
                             * << "# controller number & name:\n"
                             */
                    }
#if defined PLATFORM_DEBUG && defined SHOW_IGNORED_ITEMS
                    else
                    {
                        fprintf
                        (
                            stderr, "instrument %d, controller %d (%s) ignored\n",
                            inst, ctlr, uin.controller_name(ctlr).c_str()
                        );
                    }
#endif
                }
            }
        }
        else
        {
            file << "? This instrument specification is invalid\n";
        }
    }

    /*
     * [user-interface settings]
     *
     * These are new items stored in the user file.  The settings are obtained
     * from member functions of the user_settings class.  Not all members are
     * saved to the "user" configuration file.
     */

    if (! rc().legacy_format())
    {
        file << "\n"
            "# ======== Sequencer64-Specific Variables Section ========\n"
            "\n"
            "[user-interface-settings]\n"
            "\n"
            "# These settings specify the modifiable configuration\n"
            "# of some of the Sequencer64 user-interface elements.\n"
            ;

        file << "\n"
            "# Specifies the style of the main-window grid of patterns.\n"
            "#\n"
            "# 0 = Normal style, matches the GTK theme, has brackets.\n"
            "# 1 = White grid boxes that have brackets.\n"
            "# 2 = Black grid boxes (no brackets, our favorite).\n"
            "\n"
            << usr().grid_style() << "       # grid_style\n"
            ;

        file << "\n"
            "# Specifies box style of an empty slot in the main-window grid.\n"
            "#\n"
            "# 0  = Draw a one-pixel box outline around the pattern slot.\n"
            "# 1  = Draw brackets on the sides of the pattern slot.\n"
            "# 2 to 30 = Make the brackets thicker and thicker.\n"
            "# -1 = Same as 0, draw a box outline one-pixel thick.\n"
            "# -2 to -30 = Draw a box outline, thicker and thicker.\n"
            "\n"
            << usr().grid_brackets() << "       # grid_brackets\n"
            ;

        file << "\n"
            "# Specifies the number of rows in the main window.\n"
            "# Values of 4 (the default) through 8 (the best alternative value)\n"
            "# are allowed. Same as R in the '-o sets=RxC' option.\n"
            "\n"
            << usr().mainwnd_rows() << "       # mainwnd_rows\n"
            ;

        file << "\n"
            "# Specifies the number of columns in the main window.\n"
            "# At present, values from 8 (the default) to 12 are supported.\n"
            "# are allowed. Same as C in the '-o sets=RxC' option.\n"
            "\n"
            << usr().mainwnd_cols() << "       # mainwnd_cols\n"
            ;

        file << "\n"
            "# Specifies the maximum number of sets, which defaults to 32.\n"
            "# It is currently never necessary to change this value. In fact,\n"
            "# it should be a derived value.\n"
            "\n"
            << usr().max_sets() << "      # max_sets\n"
            ;

        file << "\n"
            "# Specifies the border width in the main window.\n"
            "\n"
            << usr().mainwid_border() << "      # mainwid_border\n"
            ;

        file << "\n"
            "# Specifies the border spacing in the main window.\n"
            "\n"
            << usr().mainwid_spacing() << "      # mainwid_spacing\n"
            ;

        file << "\n"
            "# Specifies a quantity that affects the height of the main window.\n"
            "\n"
            << usr().control_height() << "      # control_height\n"
            ;

        file << "\n"
            "# Specifies the initial zoom for the piano rolls.  Ranges from 1.\n"
            "# to 512 (the legacy maximum was 32), and defaults to 2 unless\n"
            "# changed here.  Note that large PPQN values will require larger\n"
            "# zoom values in order to look good in the sequence editor.\n"
            "# Sequencer64 adapts the zoom to the PPQN, if zoom is set to 0.\n"
            "\n"
            << usr().zoom() << "      # default zoom (0 = auto-adjust to PPQN)\n"
            ;

        /*
         * This boolean affects the behavior of the scale, key, and background
         * sequence features.
         */

        file << "\n"
            "# Specifies if the key, scale, and background sequence are to be\n"
            "# applied to all sequences, or to individual sequences.  The\n"
            "# behavior of Seq24 was to apply them to all sequences.  But\n"
            "# Sequencer64 takes it further by applying it immediately, and\n"
            "# by saving to the end of the MIDI file.  Note that these three\n"
            "# values are stored in the MIDI file, not this configuration file.\n"
            "# Also note that reading MIDI files not created with this feature\n"
            "# will pick up this feature if active, and the file gets saved.\n"
            "# It is contagious.\n"
            ;

        file << "#\n"
            "# 0 = Allow each sequence to have its own key/scale/background.\n"
            "#     Settings are saved with each sequence.\n"
            "# 1 = Apply these settings globally (similar to seq24).\n"
            "#     Settings are saved in the global final section of the file.\n"
            "\n"
            << (usr().global_seq_feature() ? "1" : "0")
            << "      # global_seq_feature\n"
            ;

        /*
         * The usage of the old versus new font is now a run-time option.
         */

        file << "\n"
            "# Specifies if the old, console-style font, or the new anti-\n"
            "# aliased font, is to be used as the font throughout the GUI.\n"
            "# In legacy mode, the old font is the default.\n"
            "#\n"
            "# 0 = Use the old-style font.\n"
            "# 1 = Use the new-style font.\n"
            "\n"
            << (usr().use_new_font() ? "1" : "0")
            << "      # use_new_font\n"
            ;

        file << "\n"
            "# Specifies if the user-interface will support two song editor\n"
            "# windows being shown at the same time.  This makes it easier to\n"
            "# edit songs with a large number of sequences.\n"
            "#\n"
            "# 0 = Allow only one song editor (performance editor).\n"
            "# 1 = Allow two song editors.\n"
            "\n"
            << (usr().allow_two_perfedits() ? "1" : "0")
            << "      # allow_two_perfedits\n"
            ;

        file << "\n"
            "# Specifies the number of 4-measure blocks for horizontal page\n"
            "# scrolling in the song editor.  The old default, 1, is a bit\n"
            "# small.  The new default is 4.  The legal range is 1 to 6, where\n"
            "# 6 is the width of the whole performance piano roll view.\n"
            "\n"
            << usr().perf_h_page_increment()
            << "      # perf_h_page_increment\n"
            ;

        file << "\n"
            "# Specifies the number of 1-track blocks for vertical page\n"
            "# scrolling in the song editor.  The old default, 1, is a bit\n"
            "# small.  The new default is 8.  The legal range is 1 to 18, where\n"
            "# 18 is about the height of the whole performance piano roll view.\n"
            "\n"
            << usr().perf_v_page_increment()
            << "      # perf_v_page_increment\n"
            ;

        file << "\n"
            "# Specifies if the progress bar is colored black, or a different\n"
            "# color.  The following integer color values are supported:\n"
            "# \n"
            "# 0 = black\n"
            "# 1 = dark red\n"
            "# 2 = dark green\n"
            "# 3 = dark orange\n"
            "# 4 = dark blue\n"
            "# 5 = dark magenta\n"
            "# 6 = dark cyan\n"
            "\n"
            << usr().progress_bar_colored() // (usr().progress_bar_colored() ? "1" : "0")
            << "      # progress_bar_colored\n"
            ;

        file << "\n"
            "# Specifies if the progress bar is thicker.  The default is 1\n"
            "# pixel.  The 'thick' value is 2 pixels.  (More than that is not\n"
            "# useful.  Set this value to 1 to enable the feature, 0 to disable\n"
            "# it.\n"
            "\n"
            << (usr().progress_bar_thick() ? "1" : "0")
            << "      # progress_bar_thick\n"
            ;

        file << "\n"
            "# Specifies using an alternate (darker) color palette.  The\n"
            "# default is the normal palette.  Not all items in the user\n"
            "# interface are altered by this setting, and it's not perfect.\n"
            "# Set this value to 1 to enable the feature, 0 to disable it.\n"
            "# Same as the -K or --inverse command-line options.\n"
            "\n"
            << (usr().inverse_colors() ? "1" : "0")
            << "      # inverse_colors\n"
            ;

        file << "\n"
            "# Specifies the window redraw rate for all windows that support\n"
            "# that concept.  The default is 40 ms.  Some windows used 25 ms,\n"
            "# which is faster.\n"
            "\n"
            << usr().window_redraw_rate()
            << "      # window_redraw_rate\n"
            ;

        file << "\n"
            "# Specifies using icons for some of the user-interface buttons\n"
            "# instead of text buttons.  This is purely a preference setting.\n"
            "# If 0, text is used in some buttons (the main window buttons).\n"
            "# Otherwise, icons are used.  One will have to experiment :-).\n"
            "\n"
            << usr().use_more_icons()
            << "      # use_more_icons (currently affects only main window)\n"
            ;

#if defined SEQ64_MULTI_MAINWID

        file << "\n"
            "# Specifies the number of set-window ('wid') rows to show.\n"
            "# The long-standing default is 1, but 2 or 3 may also be set.\n"
            "# Corresponds to R in the '-o wid=RxC,F' option.\n"
            "\n"
            << usr().block_rows()
            << "      # block_rows (number of rows of set blocks/wids)\n"
            ;

        file << "\n"
            "# Specifies the number of set window ('wid') columns to show.\n"
            "# The long-standing default is 1, but 2 may also be set.\n"
            "# Corresponds to C in the '-o wid=RxC,F' option.\n"
            "\n"
            << usr().block_columns()
            << "      # block_columns (number of columns of set blocks/wids)\n"
            ;

        file << "\n"
            "# Specifies if the multiple set windows are 'in sync' or can\n"
            "# be set to arbitrary set numbers independently.\n"
            "# The default is false (0), means that there is a single set\n"
            "# spinner, which controls the set number of the upper-left 'wid',\n"
            "# and the rest of the set numbers follow sequentially.  If true\n"
            "# (1), then each 'wid' can be set to any set-number.\n"
            "# Corresponds to the 'f' (true, false, or 'indep') in the\n"
            "# '-o wid=RxC,F' option.  Here, 1 is the same as 'indep' or false,\n"
            "# and 0 is the same as f = true.  Backwards, so be careful.\n"
            "\n"
            << (usr().block_independent() ? "1" : "0")
            << "      # block_independent (set spinners for each block/wid)\n"
            ;

        file << "\n"
            "# Specifies an enlargement of the main window of Sequencer64.\n"
            "# The normal value is 1.0, which is the legacy sizing.  If this\n"
            "# value is between 1.0 and 3.0, it will increase the size of all\n"
            "# of the main window elements proportionately. Same as the\n"
            "# '-o scale=x.y' option.\n"
            "\n"
            << usr().window_scale()
            << "      # window_scale (scales the main window upwards in size)\n"
            ;

#endif  // SEQ64_MULTI_MAINWID

    }

    /*
     * [user-midi-settings] and [user-options]
     */

    if (! rc().legacy_format())
    {
        file << "\n"
            "[user-midi-settings]\n"
            "\n"
            "# These settings specify MIDI-specific value that might be\n"
            "# better off as variables, rather than constants.\n"
            "\n"
            "# Specifies parts-per-quarter note to use, if the MIDI file.\n"
            "# does not override it.  Default is 192, but we'd like to go\n"
            "# higher than that.\n"
            "\n"
            << usr().midi_ppqn() << "       # midi_ppqn\n"
            ;

        file << "\n"
            "# Specifies the default beats per measure, or beats per bar.\n"
            "# The default value is 4.\n"
            "\n"
            << usr().midi_beats_per_bar()
            << "       # midi_beats_per_measure/bar\n"
            ;

        file << "\n"
            "# Specifies the default beats per minute.  The default value\n"
            "# is 120, and the legal range is 20 to 600. Also see the value of\n"
            "# midi_bpm_minimum and midi_bpm_maximum below.\n"
            "\n"
            << usr().midi_beats_per_minute() << "       # midi_beats_per_minute\n"
            ;

        file << "\n"
            "# Specifies the default beat width. The default value is 4.\n"
            "\n"
            << usr().midi_beat_width() << "       # midi_beat_width\n"
            ;

        file << "\n"
            "# Specifies the buss-number override, the same as the --bus\n"
            "# command-line option. The default value is -1, which means that\n"
            "# there is no buss override.  If a value from 0 to 31 is given,\n"
            "# then that buss value overrides all buss values in all patterns.\n"
            "# Change this value from -1 only to use a single output buss,\n"
            "# either for testing or convenience.  And don't save the MIDI file\n"
            "# afterwards, unless you want to overwrite all the buss values!\n"
            "\n"
            ;

        int bo = int(usr().midi_buss_override());   /* writing char no good */
        if (SEQ64_NO_BUSS_OVERRIDE(bo))
            file << "-1" << "       # midi_buss_override (disabled)\n";
        else
            file << bo   << "       # midi_buss_override (enabled, careful!)\n";

        file << "\n"
            "# Specifies the default velocity override when adding notes in the\n"
            "# sequence/pattern editor.  This value is obtained via the 'Vol'\n"
            "# button, and ranges from 0 (not recommended :-) to 127.  If the\n"
            "# value is -1, then the incoming note velocity is preserved.\n"
            "\n"
            ;

        int vel = usr().velocity_override();
        file << vel      << "       # velocity_override (-1 = 'Free')\n";

        file << "\n"
            "# Specifies the precision of the beats-per-minutes spinner and\n"
            "# MIDI control over the BPM value.  The default is 0, which means\n"
            "# the BPM is an integer.  Other values are 1 and 2 decimal digits\n"
            "# of precision.\n"
            "\n"
            ;

        int precision = usr().bpm_precision();
        file << precision << "       # bpm_precision\n";

        file << "\n"
            "# Specifies the step increment of the beats/minute spinner and\n"
            "# MIDI control over the BPM value.  The default is 1. For a\n"
            "# precision of 1 decimal point, 0.1 is a good value.  For a\n"
            "# precision of 2 decimal points, 0.01 is a good value, but one\n"
            "# might want somethings a little faster, like 0.05.\n"
            "\n"
            ;

        midibpm increment = usr().bpm_step_increment();
        file << increment << "       # bpm_step_increment\n";

        file << "\n"
            "# Specifies the page increment of the beats/minute field. It is\n"
            "# used when the Page-Up/Page-Down keys are pressed while the BPM\n"
            "# field has the keyboard focus.  The default value is 10.\n"
            "\n"
            ;

        increment = usr().bpm_page_increment();
        file << increment << "       # bpm_page_increment\n";

        file << "\n"
            "# Specifies the minimum value of beats/minute in tempo graphing.\n"
            "# By default, the tempo graph ranges from 0.0 to 127.0.\n"
            "# This value can be increased to give a magnified view of tempo.\n"
            "\n"
            ;

        increment = usr().midi_bpm_minimum();
        file << increment << "       # midi_bpm_minimum\n";

        file << "\n"
            "# Specifies the maximum value of beats/minute in tempo graphing.\n"
            "# By default, the tempo graph ranges from 0.0 to 127.0.\n"
            "# This value can be increased to give a magnified view of tempo.\n"
            "\n"
            ;

        increment = usr().midi_bpm_maximum();
        file << increment << "       # midi_bpm_maximum\n";

        /*
         * [user-options]
         */

        file << "\n"
            "[user-options]\n"
            "\n"
            "# These settings specify application-specific values that are\n"
            "# set via the -o or --option switch, which help expand the number\n"
            "# of options the Sequencer64 options can support.\n"
            "\n"
            "# The 'daemonize' option is used in seq64cli to indicate that the\n"
            "# application should be gracefully run as a service.\n"
            "\n"
            ;

        int uscratch = usr().option_daemonize() ? 1 : 0 ;
        file << uscratch << "       # option_daemonize\n";
        file << "\n"
            "# This value specifies an optional log-file that replaces output\n"
            "# to standard output and standard error.  To indicate no log-file,\n"
            "# the string \"\" is used.  Currently, this option works best from\n"
            "# the command line, as in '-o log=filename.log'.  However, the\n"
            "# name here is used only if the bare option '-o log' is specified.\n"
            "\n"
            ;
        std::string logfile = usr().option_logfile();
        if (logfile.empty())
            file << "\"\"\n";
        else
            file << logfile << "\n";

        /*
         * [user-work-arounds]
         */

        file << "\n"
            "[user-work-arounds]\n"
            "\n"
            "# These settings specify application-specific values that work\n"
            "# around issues that we have not been able to truly fix for all\n"
            "# users.\n"
            "\n"
            "# The work_around_play_image option can be set to 0 or 1.  0 is\n"
            "# the normal setting. Set it to 1 if multiple-clicks of the play\n"
            "# button (or the equivalent play/pause/stop actions) cause the\n"
            "# sequencer64 application to crash.\n"
            "\n"
            ;

        uscratch = usr().work_around_play_image() ? 1 : 0 ;
        file
            << uscratch << "       # work_around_play_image\n"
            "\n"
            "# The work_around_transpose_image option is similar, for an issue\n"
            "# some users have setting the transpose button in seqedit.\n"
            "\n"
            ;

        uscratch = usr().work_around_transpose_image() ? 1 : 0 ;
        file << uscratch << "       # work_around_transpose_image\n";

        /*
         * [user-ui-tweaks]
         */

        file << "\n"
            "[user-ui-tweaks]\n"
            "\n"
            "# This first value specifies the height of the keys in the\n"
            "# sequence editor.  Defaults to 12 (pixels).\n"
            "# Currently used only in the Qt GUI.\n"
            "\n"
            ;

        uscratch = usr().key_height();
        file << uscratch << "       # (user_ui_) key_height\n";
    }

    /*
     * End of file.
     */

    file << "\n"
        << "# End of " << m_name
        << "\n#\n"
        << "# vim: sw=4 ts=4 wm=4 et ft=sh\n"   /* ft=sh for nice colors */
        ;
    file.close();
    return true;
}

}           // namespace seq64

/*
 * userfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

