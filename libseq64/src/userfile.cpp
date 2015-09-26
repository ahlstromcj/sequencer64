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
 * \updates       2015-09-26
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
        printf("? error opening [%s]\n", m_name.c_str());
        return false;
    }
    file.seekg(0, std::ios::beg);                       /* seek to start */
    line_after(file, "[user-midi-bus-definitions]");    /* find the tag  */
    int buses = 0;
    sscanf(m_line, "%d", &buses);                       /* atavistic!    */
    for (int i = 0; i < buses; i++)
    {
        char bus_num[4];
        snprintf(bus_num, sizeof(bus_num), "%d", i);
        line_after(file, "[user-midi-bus-" + std::string(bus_num) + "]");

        // global_user_midi_bus_definitions[i].alias = m_line;
        // g_user_settings.bus_alias(i, m_line);

        (void) g_user_settings.add_bus(m_line);
        next_data_line(file);
        int instruments = 0;
        int instrument;
        int channel;
        sscanf(m_line, "%d", &instruments);
        for (int j = 0; j < instruments; j++)
        {
            next_data_line(file);
            sscanf(m_line, "%d %d", &channel, &instrument);

            // global_user_midi_bus_definitions[i].instrument[channel] =
            //      instrument;

            /*
             * This call assumes that add_bus() succeeded.  Otherwise, i
             * would not match the size of the vector.
             */

            g_user_settings.bus_instrument(i, channel, instrument);
        }
    }

    line_after(file, "[user-instrument-definitions]");
    int instruments = 0;
    sscanf(m_line, "%d", &instruments);
    for (int i = 0; i < instruments; i++)
    {
        char instrument_num[4];
        snprintf(instrument_num, sizeof(instrument_num), "%d", i);
        line_after(file, "[user-instrument-"+std::string(instrument_num)+"]");

        // global_user_instrument_definitions[i].instrument = m_line;
        // g_user_settings.instrument_name(i, m_line);

        (void) g_user_settings.add_instrument(m_line);
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

        //  global_user_instrument_definitions[i].controllers_active[c] = true;
        //  global_user_instrument_definitions[i].controllers[c] =
        //      std::string(ccname);

            /*
             * This call assumes that add_instrument() succeeded.
             * Otherwise, i would not match the size of the vector.
             */

            g_user_settings.instrument_controllers
            (
                i, c, std::string(ccname), true
            );
        }
    }

    /*
     * TODO: More (new) variables to follow!
     */

    g_user_settings.set_globals();
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
        printf("? error opening [%s] for writing\n", m_name.c_str());
        return false;
    }
    if (g_rc_settings.legacy_format())
    {
        file << "# Seq24 0.9.2 user configuration file (legacy format)\n";
    }
    else
    {
        file << "# Sequencer26 0.9.9.4 (and above) user configuration file\n";
    }

    //////////////////////////////////////////////////////////////

    file
        << "\n[user-midi-bus-definitions]\n\n"
        <<  g_user_settings.bus_count() << "     # MIDI buss list count\n"
        ;

    for (int buss = 0; buss < g_user_settings.bus_count(); ++buss)
    {
        char bus_num[4];
        snprintf(bus_num, sizeof(bus_num), "%d", buss);
        file <<  "\n[user-midi-bus-" << std::string(bus_num) << "]\n\n";


        // MORE TO COME
    }

    //////////////////////////////////////////////////////////////

    file
        << "\n[user-instrument-definitions]\n\n"
        <<  g_user_settings.instrument_count() << "     # instrument list count\n"
        ;

    for (int inst = 0; inst < g_user_settings.instrument_count(); ++inst)
    {
        char inst_num[4];
        snprintf(inst_num, sizeof(inst_num), "%d", inst);
        file <<  "\n[user-instrument-" << std::string(inst_num) << "]\n\n";


        // MORE TO COME
    }

    //////////////////////////////////////////////////////////////

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
