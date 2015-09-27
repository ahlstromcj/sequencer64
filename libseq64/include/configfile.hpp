#ifndef SEQ64_CONFIGFILE_HPP
#define SEQ64_CONFIGFILE_HPP

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
 * \file          configfile.hpp
 *
 *  This module declares the abstract base class for configuration and
 *  options files.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-27
 * \license       GNU GPLv2 or above
 *
 */

#include <fstream>
#include <string>
#include <list>
#include <gtkmm/drawingarea.h>

namespace seq64
{

class perform;

/**
 *  A manifest constant for controlling the length of a line-reading
 *  array in a configuration file.  This value was 1024, but
 *  realistically, 128 is more than enough.  We provide safety anyway.
 */

#define SEQ64_LINE_MAX                  128

/**
 *    This class is the abstract base class for optionsfile and userfile.
 */

class configfile
{

protected:

    /**
     *  Provides the name of the file.
     */

    std::string m_name;

    /**
     *   Points to an allocated buffer that holds the data for the
     *   configuration file.
     */

    unsigned char * m_d;

    /**
     *  The current line of text being processed.  This member receives
     *  an input line, and so needs to be a character buffer.
     */

    char m_line[SEQ64_LINE_MAX];

protected:

    void next_data_line (std::ifstream & a_file);
    void line_after (std::ifstream & a_file, const std::string & a_tag);

public:

    configfile (const std::string & a_name);

    /**
     *  A rote constructor needed for a base class.
     */

    virtual ~configfile()
    {
        // empty body
    }

    virtual bool parse (perform & a_perf) = 0;
    virtual bool write (const perform & a_perf) = 0;

};

}           // namespace seq64

#endif      // SEQ64_CONFIGFILE_HPP

/*
 * configfile.hpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
