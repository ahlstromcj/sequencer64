#ifndef SEQ64_OPTIONSFILE_HPP
#define SEQ64_OPTIONSFILE_HPP

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
 * \file          optionsfile.hpp
 *
 *  This module declares/defines the base class for managind the ~/.seq24rc
 *  configuration file.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2019-06-10
 * \license       GNU GPLv2 or above
 *
 *  The ~/.seq24rc or ~/.config/sequencer64/sequencer64.rc files are
 *  referred to as the "rc" files.  Note that there are other variations on
 *  the name for the different versions of Sequencer64 that can be built.
 */

#include "configfile.hpp"
#include "midi_control_out.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *  Provides a file for reading and writing the application's main
 *  configuration file.  The settings that are passed around are provided
 *  or used by the perform class.
 */

class optionsfile : public configfile
{

public:

    optionsfile (const std::string & name);
    virtual ~optionsfile ();

    virtual bool parse (perform & p);
    virtual bool write (const perform & p);

    bool parse_mute_group_section (perform & p);

private:

    bool parse_midi_control_section (const std::string & fname, perform & p);
    bool parse_midi_control_out (const std::string & fname, perform & p);
    bool write_midi_control
    (
        const perform & p,
        std::ofstream & file
    );
    bool write_midi_control_out
    (
        const perform & p,
        std::ofstream & file
    );
    void read_ctrl_event
    (
        std::ifstream & file,
        midi_control_out * mctrl,
        midi_control_out::action a
    );
    void write_ctrl_event
    (
        std::ofstream & file,
        midi_control_out * mctrl,
        midi_control_out::action a
    );
    void read_ctrl_pair
    (
        std::ifstream & file,
        midi_control_out * mctrl,
        midi_control_out::action a1,
        midi_control_out::action a2
    );
    void write_ctrl_pair
    (
        std::ofstream & file,
        midi_control_out * mctrl,
        midi_control_out::action a1,
        midi_control_out::action a2
    );

    bool make_error_message
    (
        const std::string & sectionname,
        const std::string & additional = ""
    );

};          // class optionsfile

}           // namespace seq64

#endif      // SEQ64_OPTIONSFILE_HPP

/*
 * optionsfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

