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
 *  <tt> ~/.seq24usr </tt> or <tt> ~/.config/sequencer64/sequencer64.rc
 *  </tt> configuration file.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-05
 * \license       GNU GPLv2 or above
 *
 *  Note that the parse function has some code that is not yet enabled.
 */

#include <iostream>

/*
 * Not yet actually used!
 *
 * #include "perform.hpp"
 */

#include "globals.h"
#include "userfile.hpp"

namespace seq64
{

class perform;          // temporary forward reference

/**
 *  Principal constructor.
 */

userfile::userfile (const std::string & a_name)
  :
    configfile (a_name)
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
 *  Provides a debug dump of basic information to help debug a
 *  surprisingly intractable problem with all busses having the name and
 *  values of the last buss in the configuration.  Does work only if
 *  PLATFORM_DEBUG is defined; see the user_settings class..
 */

void
userfile::dump_setting_summary ()
{
    usr().dump_summary();
}

/**
 *  Parses a "usr" file, filling in the given perform object.
 *  This function opens the file as a text file (line-oriented).
 *
 * \param a_perf
 *      The performance object, currently unused.
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
     * Header commentary.  Skipped during parsing.
     */

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
            next_data_line(file);
            int instruments = 0;
            int instrument;
            int channel;
            sscanf(m_line, "%d", &instruments);
            for (int j = 0; j < instruments; j++)
            {
                next_data_line(file);
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

    /*
     * [user-instrument-definitions]
     */

    line_after(file, "[user-instrument-definitions]");
    int instruments = 0;
    sscanf(m_line, "%d", &instruments);

    /*
     * [user-instrument-x]
     */

    for (int i = 0; i < instruments; i++)
    {
        std::string label = make_section_name("user-instrument", i);
        line_after(file, label);
        if (usr().add_instrument(m_line))
        {
            next_data_line(file);
            char ccname[SEQ64_LINE_MAX];
            int ccs = 0;
            sscanf(m_line, "%d", &ccs);
            for (int j = 0; j < ccs; j++)
            {
                int c = 0;
                next_data_line(file);
                ccname[0] = 0;                              // clear the buffer
                sscanf(m_line, "%d %[^\n]", &c, ccname);
                if (c >= 0 && c < MIDI_CONTROLLER_MAX)      // 128
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
     * These are new items stored in the user file.
     *
     * Only variables whose effects we can be completely sure of are read
     * from this section, and used, at this time.
     */

    if (! rc().legacy_format())
    {
        line_after(file, "[user-interface-settings]");
        int scratch = 0;
        sscanf(m_line, "%d", &scratch);
        usr().mainwnd_rows(scratch);

        next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().mainwnd_cols(scratch);

        next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().max_sets(scratch);

        next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().mainwid_border(scratch);

        next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().mainwid_spacing(scratch);

        next_data_line(file);
        sscanf(m_line, "%d", &scratch);
        usr().control_height(scratch);

        usr().normalize();    /* calculate derived values */
    }

    /*
     *                  [user-midi-settings]
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
     * We have all of the data, now distribute the values to any legacy global
     * variables that are still being used.  Then close the file.
     */

    usr().set_globals();
    dump_setting_summary();

    /*
     * End of file.
     */

    file.close();
    return true;
}

/**
 *  This function just returns false, as there is no "perform" information
 *  in the user-file yet.
 *
 * \param a_perf
 *      The performance object, currently unused.
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

    /*
     * Any legacy global variables still outstanding?  These might have been
     * modified by older code, and we need to make sure we get those changes
     * into the user-settings object before we write to the file.
     */

    usr().get_globals();
    dump_setting_summary();

    /*
     * Header commentary.  Write out comments about the nature of this file.
     */

    if (rc().legacy_format())
        file << "# Seq24 0.9.2 user configuration file (legacy format)\n";
    else
        file << "# Sequencer26 0.9.9.7 (and above) user configuration file\n";

    file << "#\n"
        "# Created by reading the following file and writing it out via the\n"
        "# sequencer64 application:\n"
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
        "# -1 (GM_INSTRUMENT_FLAG) and are GM (General MIDI).\n"
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
        file << "\n\n";

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
                if (umb.instrument(channel) != GM_INSTRUMENT_FLAG)
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

    file << "#\n"
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
        file << "\n\n";

    /*
     * [user-instrument-x]
     */

    for (int inst = 0; inst < usr().instrument_count(); ++inst)
    {
        file << "\n[user-instrument-" << inst << "]\n"
        << "\n";
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
        file << "\n# End of instrument/controllers definition " << inst << "\n";
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
            "#   ========================================================\n"
            "#   ======== Sequencer64-Specific Variables Section ========\n"
            "#   ========================================================\n"
            "\n"
            ;

        file << "\n"
            "[user-interface-settings]\n"
            "\n"
            "# These settings specify the soon-to-be-modifiable sizes of\n"
            "# the Sequencer64 user-interface elements.\n"
            "\n"
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
    }

    /*
     *                  [user-midi-settings]
     */

    if (! rc().legacy_format())
    {
        file << "\n"
            "[user-midi-settings]\n"
            "\n"
            "# These settings specify MIDI-specific value that might be\n"
            "# better off as variables, rather than constants.\n"
            "\n"
            ;

        file << "\n"
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
            "# Specifies the buss-number override. The default value is -1,\n"
            "# which means that there is no buss override.  If a value\n"
            "# from 0 to 31 is given, then that buss value overrides all\n"
            "# buss values specified in all sequences/patterns.\n"
            "# Change this value from -1 only if you want to use a single\n"
            "# output buss, either for testing or convenience.  And don't\n"
            "# save the MIDI afterwards, unless you really want to change\n"
            "# all of its buss values.\n"
            "\n"
            ;

        int bo = int(usr().midi_buss_override());   /* writing char no good */
        if (NO_BUSS_OVERRIDE(bo))
            file << "-1" << "       # midi_buss_override\n";
        else
            file << bo   << "       # midi_buss_override\n";
    }

    /*
     * End of file.
     */

    file << "\n"
        << "# End of " << m_name
        << "\n#\n"
        << "# vim: sw=4 ts=4 wm=4 et ft=sh\n"
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
