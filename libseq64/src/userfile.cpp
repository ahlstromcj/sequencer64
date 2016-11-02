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
 * \updates       2016-11-02
 * \license       GNU GPLv2 or above
 *
 *  Note that the parse function has some code that is not yet enabled.
 */

#include <iostream>

#include "globals.h"
#include "settings.hpp"                 /* seq64::rc()                  */
#include "userfile.hpp"                 /* seq64::userfile              */
#include "user_instrument.hpp"          /* seq64::user_instrument       */

namespace seq64
{

class perform;          // temporary forward reference

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
    char temp[8];
    snprintf(temp, sizeof(temp), "%d", value);
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
userfile::parse (perform & /* a_perf */)
{
    std::ifstream file(m_name.c_str(), std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        fprintf(stderr, "? error opening [%s]\n", m_name.c_str());
        return false;
    }
    file.seekg(0, std::ios::beg);                       /* seek to start */

    /*
     * Header commentary is skipped during parsing.
     *
     * \change ca 2016-04-04
     *      Next, if we're using the manual or auto ALSA port options
     *      specified in the RC file, we do want to override the ports that
     *      the queries of the ALSA system find.  Otherwise, we might want to
     *      reveal the names obtained by port detection for ALSA, and do not
     *      want to override the names of the ports that are found.
     */

    if (! rc().reveal_alsa_ports())
    {
        /*
         * [user-midi-bus-definitions]
         */

        line_after(file, "[user-midi-bus-definitions]");    /* find the tag  */
        int buses = 0;
        sscanf(m_line, "%d", &buses);                       /* atavistic!    */

        /*
         * [user-midi-bus-x]
         */

        for (int bus = 0; bus < buses; ++bus)
        {
            std::string label = make_section_name("user-midi-bus", bus);
            line_after(file, label);
            if (usr().add_bus(m_line))
            {
                (void) next_data_line(file);
                int instruments = 0;
                int instrument;
                int channel;
                sscanf(m_line, "%d", &instruments);
                for (int j = 0; j < instruments; ++j)
                {
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

    line_after(file, "[user-instrument-definitions]");
    int instruments = 0;
    sscanf(m_line, "%d", &instruments);

    /*
     * [user-instrument-x]
     */

    for (int i = 0; i < instruments; ++i)
    {
        std::string label = make_section_name("user-instrument", i);
        line_after(file, label);
        if (usr().add_instrument(m_line))
        {
            char ccname[SEQ64_LINE_MAX];
            int ccs = 0;
            (void) next_data_line(file);
            sscanf(m_line, "%d", &ccs);
            for (int j = 0; j < ccs; ++j)
            {
                int c = 0;
                (void) next_data_line(file);
                ccname[0] = 0;                              // clear the buffer
                sscanf(m_line, "%d %[^\n]", &c, ccname);
                if (c >= 0 && c < SEQ64_MIDI_CONTROLLER_MAX)      // 128
                {
                    usr().set_instrument_controllers
                    (
                        i, c, std::string(ccname), true
                    );
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
        line_after(file, "[user-interface-settings]");
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
        usr().max_sets(scratch);

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
         * This boolean affects the behavior of the scale, key, and background
         * sequence features, but their actual values are stored in the MIDI
         * file, not in the "user" configuration file.
         */

        (void) next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().global_seq_feature(scratch != 0);

        /*
         * The user-interface font is now selectable at run time.  Old versus
         * new font.
         */

        (void) next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().use_new_font(scratch != 0);

        (void) next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().allow_two_perfedits(scratch != 0);

        (void) next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().perf_h_page_increment(scratch);

        (void) next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().perf_v_page_increment(scratch);

        /*
         *  Here, we start checking the lines, on the theory that these
         *  very new (2016-02-14) items might mess up people who already have
         *  older Sequencer64 "user" configuration files.
         */

        if (next_data_line(file))
        {
            sscanf(m_line, "%d", &scratch);                 /* now an int   */
            usr().progress_bar_colored(scratch);            /* pick a color */
            if (next_data_line(file))
            {
                sscanf(m_line, "%d", &scratch);
                usr().progress_bar_thick(scratch != 0);
            }
            if (next_data_line(file))
            {
                sscanf(m_line, "%d", &scratch);
                if (scratch <= 1)                           /* boolean?     */
                {
                    usr().inverse_colors(scratch != 0);
                    if (next_data_line(file))
                        sscanf(m_line, "%d", &scratch);     /* get redraw   */
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
                if (scratch <= 1)                           /* boolean?     */
                    usr().use_more_icons(scratch != 0);
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
        usr().mainwnd_rows(4);
        usr().mainwnd_cols(8);
        usr().max_sets(32);
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
        line_after(file, "[user-midi-settings]");
        int scratch = 0;
        sscanf(m_line, "%d", &scratch);
        usr().midi_ppqn(scratch);

        next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().midi_beats_per_bar(scratch);

        next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().midi_beats_per_minute(scratch);

        next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().midi_beat_width(scratch);

        next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().midi_buss_override(char(scratch));
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
           "# Sequencer64 user configuration file (legacy Seq24 0.9.2 format)\n";
    }
    else
        file << "# Sequencer64 0.9.20 (and above) user configuration file\n";

    file << "#\n"
        "# Created by reading the following file and writing it out via the\n"
        "# Sequencer64 application:\n"
        "#\n"
        "# https://raw.githubusercontent.com/vext01/"
               "seq24/master/seq24usr.example\n"
        ;

    file << "#\n"
        "#  This is an example for a sequencer64.usr file. Edit and place in\n"
        "#  your home directory. It allows you to give an alias to each\n"
        "#  MIDI bus, MIDI channel, and MIDI control codes per channel.\n"
        ;

    file << "#\n"
        "#  1. Define your instruments and their control-code names,\n"
        "#     if they have them.\n"
        "#  2. Define a MIDI bus, its name, and what instruments are\n"
        "#     on which channel.\n"
    ;

    file << "#\n"
        "# In the following MIDI buss definitions, channels are counted\n"
        "# from 0 to 15, not 1 to 16.  Instruments unspecified are set to\n"
        "# -1 (SEQ64_GM_INSTRUMENT_FLAG) and are GM (General MIDI).\n"
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
                << "# Device name for this buss:\n"
                << "\n"
                << umb.name() << "\n"
                << "\n"
                << "# Number of channels:\n"
                << "\n"
                << umb.channel_count() << "\n"
                << "\n"
                << "# channel and instrument (or program) number\n"
                << "\n"
                ;

            for (int channel = 0; channel < umb.channel_max(); ++channel)
            {
                if (umb.instrument(channel) != SEQ64_GM_INSTRUMENT_FLAG)
                    file << channel << " " << umb.instrument(channel) << "\n";
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

        file << "\n# End of buss definition " << buss << "\n";
    }

    file <<
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
                << "# Name of instrument:\n"
                << "\n"
                << uin.name() << "\n"
                << "\n"
                << "# Number of MIDI controller values:\n"
                << "\n"
                << uin.controller_count() << "\n"
                << "\n"
                << "# controller number and (optional) name:\n"
                << "\n"
                ;

            for (int ctlr = 0; ctlr < uin.controller_max(); ++ctlr)
            {
                if (uin.controller_active(ctlr))
                    file << ctlr << " " << uin.controller_name(ctlr) << "\n";
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
        else
        {
            file << "? This instrument specification is invalid\n";
        }
        file << "\n# End of instrument/controllers definition "
            << inst << "\n\n"
            ;
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
        file <<
            "#   ======== Sequencer64-Specific Variables Section ========\n"
            "\n"
            "[user-interface-settings]\n"
            "\n"
            "# These settings specify the soon-to-be-modifiable configuration\n"
            "# of some of the Sequencer64 user-interface elements.\n"
            ;

        file << "\n"
            "# Specifies the style of the main-window grid of patterns.\n"
            "#\n"
            "# 0 = Normal style, matches the GTK theme, has brackets.\n"
            "# 1 = White grid boxes that have brackets.\n"
            "# 2 = Black grid boxes (no brackets).\n"
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
            "# At present, only a value of 4 is supportable.\n"
            "# In the future, we hope to support an alternate value of 8.\n"
            "\n"
            << usr().mainwnd_rows() << "       # mainwnd_rows\n"
            ;

        file << "\n"
            "# Specifies the number of columns in the main window.\n"
            "# At present, only a value of 8 is supportable.\n"
            "\n"
            << usr().mainwnd_cols() << "       # mainwnd_cols\n"
            ;

        file << "\n"
            "# Specifies the maximum number of sets, which defaults to 1024.\n"
            "# It is currently never necessary to change this value.\n"
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
            "# Specifies some quantity, it is not known what it means.\n"
            "\n"
            << usr().control_height() << "      # control_height\n"
            ;

        file << "\n"
            "# Specifies the initial zoom for the piano rolls.  Ranges from 1.\n"
            "# to 512 (the legacy maximum was 32), and defaults to 2 unless\n"
            "# changed here.  Note that large PPQN values will require larger\n"
            "# zoom values in order to look good in the sequence editor.\n"
            "# Sequencer64 adapts the zoom to the PPQN, if zoom is set to 0.\n"
            "# \n"
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
            "\n"
            << (usr().inverse_colors() ? "1" : "0")
            << "      # inverse_colors\n"
            ;

        file << "\n"
            "# Specifies the window redraw rate for all windows that support\n"
            "# that concept.  The default is 40 ms.  Some windows used 25 ms.\n"
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
    }

    /*
     * [user-midi-settings]
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
            "# higher than that.  BEWARE:  STILL GETTING IT TO WORK!\n"
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
            "# is 120, and the legal range is 20 to 500.\n"
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
            "# command-line option. The default value is -1, which\n"
            "# means that there is no buss override.  If a value\n"
            "# from 0 to 31 is given, then that buss value overrides all\n"
            "# buss values specified in all sequences/patterns.\n"
            "# Change this value from -1 only to use a single\n"
            "# output buss, either for testing or convenience.  And don't\n"
            "# save the MIDI file afterwards, unless you really want to change\n"
            "# all of its buss values!\n"
            "\n"
            ;

        int bo = int(usr().midi_buss_override());   /* writing char no good */
        if (SEQ64_NO_BUSS_OVERRIDE(bo))
            file << "-1" << "       # midi_buss_override (disabled)\n";
        else
            file << bo   << "       # midi_buss_override (enabled, careful!)\n";
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

