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
 * \updates       2015-09-27
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
 *  PLATFORM_DEBUG is defined.
 */

static void
dump_setting_summary ()
{
    g_user_settings.dump_summary();
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
    line_after(file, "[user-midi-bus-definitions]");    /* find the tag  */
    int buses = 0;
    sscanf(m_line, "%d", &buses);                       /* atavistic!    */
    for (int bus = 0; bus < buses; ++bus)
    {
        std::string label = make_section_name("user-midi-bus", bus);
        line_after(file, label);
        if (g_user_settings.add_bus(m_line))
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
                g_user_settings.bus_instrument(bus, channel, instrument);
            }
        }
        else
        {
            fprintf
            (
                stderr, "? error adding %s (line = '%s')", label.c_str(), m_line
            );
        }
    }

    line_after(file, "[user-instrument-definitions]");
    int instruments = 0;
    sscanf(m_line, "%d", &instruments);
    for (int i = 0; i < instruments; i++)
    {
        std::string label = make_section_name("user-instrument", i);
        line_after(file, label);
        if (g_user_settings.add_instrument(m_line))
        {
            next_data_line(file);
            char ccname[SEQ64_LINE_MAX];
            int ccs = 0;
            sscanf(m_line, "%d", &ccs);
            for (int j = 0; j < ccs; j++)
            {
                int c = 0;
                next_data_line(file);
                sscanf(m_line, "%d", &c);
                sscanf(m_line, "%[^\n]", ccname);
                g_user_settings.instrument_controllers
                (
                    i, c, std::string(ccname), true
                );
            }
        }
        else
        {
            fprintf
            (
                stderr, "? error adding %s (line = '%s')", label.c_str(), m_line
            );
        }
    }

    /*
     * TODO: More (new) variables to follow!
     */

    g_user_settings.set_globals();
    dump_setting_summary();
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
    g_user_settings.get_globals();
    dump_setting_summary();
    if (g_rc_settings.legacy_format())
        file << "# Seq24 0.9.2 user configuration file (legacy format)\n";
    else
        file << "# Sequencer26 0.9.9.4 (and above) user configuration file\n";

    file
        << "#\n"
        << "# In the following MIDI buss definitions, the channels are counted\n"
        << "# from 0 to 15, not 1 to 16.  Instruments not specified are set to\n"
        << "# -1 (GM_INSTRUMENT_FLAG) and are GM (General MIDI).\n"
        ;

    file
        << "\n"
        << "[user-midi-bus-definitions]\n\n"
        <<  g_user_settings.bus_count()
        << "     # number of user-defined MIDI busses\n"
        ;

    if (g_user_settings.bus_count() == 0)
    {
        file << "\n\n";
    }

    for (int buss = 0; buss < g_user_settings.bus_count(); ++buss)
    {
        file <<  "\n[user-midi-bus-" << buss << "]\n\n";
        const user_midi_bus & umb = g_user_settings.bus(buss);
        if (umb.is_valid())
        {
            file
                << "# Device name for this buss:\n"
                << umb.name() << "\n\n"
                << "# Number of channels:\n"
                << umb.channel_count() << "\n\n"
                << "# channel and instrument (program) number\n\n"
                ;

            for (int channel = 0; channel < umb.channel_count(); ++channel)
            {
                if (umb.instrument(channel) != GM_INSTRUMENT_FLAG)
                    file << channel << " " << umb.instrument(channel) << "\n";
            }
        }
        else
        {
            file << "? This buss specification is invalid\n";
        }
        file << "\n# End of buss definition " << buss << "\n\n";
    }

    file
        << "#\n"
        << "# In the following MIDI instrument definitions, active controller\n"
        << "# numbers (i.e. one supported by the instrument) are paired with\n"
        << "# the (optional) name of the controller supported.\n"
        ;

    file
        << "\n"
        << "[user-instrument-definitions]\n\n"
        <<  g_user_settings.instrument_count() << "     # instrument list count\n"
        ;

    if (g_user_settings.instrument_count() == 0)
    {
        file << "\n\n";
    }

    for (int inst = 0; inst < g_user_settings.instrument_count(); ++inst)
    {
        file <<  "\n[user-instrument-" << inst << "]\n\n";
        const user_instrument & uin = g_user_settings.instrument(inst);
        if (uin.is_valid())
        {
            file
                << "# Name of the instrument:\n"
                << uin.name() << "\n\n"
                << "# Number of MIDI controller values:\n"
                << uin.controller_count() << "\n\n"
                << "# controller number and (optional) name:\n\n"
                ;

            for (int ctlr = 0; ctlr < uin.controller_count(); ++ctlr)
            {
                if (uin.controller_active(ctlr))
                    file << ctlr << " " << uin.controller_name(ctlr) << "\n";
            }
        }
        else
        {
            file << "? This instrument specification is invalid\n";
        }
        file << "\n# End of instrument/controllers definition " << inst << "\n\n";
    }

    file
        << "# End of " << m_name << "\n#\n"
        << "# vim: sw=4 ts=4 wm=8 et ft=sh\n"
        ;
    file.close();
    return true;
}

}           // namespace seq64

/*
 * userfile.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
