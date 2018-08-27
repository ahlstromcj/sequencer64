#ifndef SEQ64_PLAYLIST_HPP
#define SEQ64_PLAYLIST_HPP

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
 * \file          playlist.hpp
 *
 *  This module declares/defines the base class for a playlist file and
 *  a playlist manager.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-08-26
 * \updates       2018-08-26
 * \license       GNU GPLv2 or above
 *
 */

#include "configfile.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *  Provides a file for reading and writing the application' main
 *  configuration file.  The settings that are passed around are provided
 *  or used by the perform class.
 */

class playlist : public configfile
{

public:

    playlist (const std::string & name);
    virtual ~playlist ();

    bool parse (perform & p);
    bool write (const perform & p);

private:

    bool error_message
    (
        const std::string & sectionname,
        const std::string & additional = ""
    );

};          // class playlist

}           // namespace seq64

#endif      // SEQ64_PLAYLIST_HPP

/*
 * playlist.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

