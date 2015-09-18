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
 *  This module declares/defines the base class for
 *  managing the user's ~/.seq24usr configuration file.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-18
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
        return false;

    file.seekg(0, std::ios::beg);                       /* seek to start */
    line_after(file, "[user-midi-bus-definitions]");    /* find the tag  */
    int buses = 0;
    sscanf(m_line, "%d", &buses);                       /* atavistic!    */
    char bus_num[4];
    for (int i = 0; i < buses; i++)
    {
        snprintf(bus_num, sizeof(bus_num), "%d", i);
        line_after(file, "[user-midi-bus-" + std::string(bus_num) + "]");
        global_user_midi_bus_definitions[i].alias = m_line;
        next_data_line(file);
        int instruments = 0;
        int instrument;
        int channel;
        sscanf(m_line, "%d", &instruments);
        for (int j = 0; j < instruments; j++)
        {
            next_data_line(file);
            sscanf(m_line, "%d %d", &channel, &instrument);
            global_user_midi_bus_definitions[i].instrument[channel] = instrument;
        }
    }
    line_after(file, "[user-instrument-definitions]");
    int instruments = 0;
    sscanf(m_line, "%d", &instruments);
    char instrument_num[4];
    for (int i = 0; i < instruments; i++)
    {
        snprintf(instrument_num, sizeof(instrument_num), "%d", i);
        line_after(file, "[user-instrument-"+std::string(instrument_num)+"]");
        global_user_instrument_definitions[i].instrument = m_line;
        next_data_line(file);
        int ccs = 0;
        int cc = 0;
        char cc_name[SEQ64_LINE_MAX];
        sscanf(m_line, "%d", &ccs);
        for (int j = 0; j < ccs; j++)
        {
            next_data_line(file);
            sscanf(m_line, "%d", &cc);
            sscanf(m_line, "%[^\n]", cc_name);
            global_user_instrument_definitions[i].controllers[cc] =
                std::string(cc_name);

            global_user_instrument_definitions[i].controllers_active[cc] = true;
        }
    }
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
    return false;
}

}           // namespace seq64

/*
 * userfile.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
