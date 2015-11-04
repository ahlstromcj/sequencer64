#ifndef SEQ64_USERFILE_HPP
#define SEQ64_USERFILE_HPP

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
 * \file          userfile.hpp
 *
 *  This module declares/defines the base class for
 *  managing the user's <tt> ~/.config/sequencer64/sequencer64.usr </tt>
 *  or <tt> ~/.seq24usr </tt> configuration file.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-04
 * \license       GNU GPLv2 or above
 *
 */

#include <string>

#include "configfile.hpp"

namespace seq64
{

class perform;

/**
 *    Supports the user's <tt> ~/.config/sequencer64/sequencer64.usr </tt> and
 *    <tt> ~/.seq24usr </tt> configuration file.
 */

class userfile : public configfile
{

public:

    userfile (const std::string & a_name);
    ~userfile ();

    bool parse (perform & a_perf);
    bool write (const perform & a_perf);

private:

    void dump_setting_summary ();

};

}           // namespace seq64

#endif      // SEQ64_USERFILE_HPP

/*
 * userfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

