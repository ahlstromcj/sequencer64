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
 * \updates       2018-04-01
 * \license       GNU GPLv2 or above
 *
 *  This is actually an elegant little parser, and works well as long as one
 *  respects its limitations.
 */

#include <fstream>
#include <string>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

class perform;

/**
 *  A manifest constant for controlling the length of a line-reading
 *  array in a configuration file.  This value was 1024, but
 *  realistically, 128 is more than enough.  We provide safety anyway.
 *  Actually, not.  A line longer than 132 (e.g. in the recent-files list of
 *  the "rc" file) causes std::ifstream::getline() to go off into the ozone.
 */

#define SEQ64_LINE_MAX          1024    // 132 is *not* enough

/**
 *    This class is the abstract base class for optionsfile and userfile.
 */

class configfile
{

private:

    /**
     *  Holds the last error message, if any.  Not a 100% foolproof yet.
     */

    std::string m_error_message;

protected:

    /**
     *  Provides the name of the configuration file.
     */

    std::string m_name;

    /**
     *   Points to an allocated buffer that holds the data for the
     *   configuration file.
     */

    char * m_d;

    /**
     *  The current line of text being processed.  This member receives
     *  an input line, and so needs to be a character buffer.
     */

    char m_line[SEQ64_LINE_MAX];

protected:

    bool next_data_line (std::ifstream & file);
    bool line_after (std::ifstream & file, const std::string & tag);

    /**
     *  Sometimes we need to know if there are new data lines at the end of an
     *  existing section.  One clue that there is not is that we're at the
     *  next section marker.  This function tests for that condition.
     *
     * \return
     *      Returns true if m_line[0] is the left-bracket character.
     */

    bool at_section_start () const
    {
        return m_line[0] == '[';
    }

public:

    configfile (const std::string & name);

    /**
     *  A rote destructor needed for a base class.
     */

    virtual ~configfile()
    {
        // empty body
    }

    virtual bool parse (perform & perf) = 0;
    virtual bool write (const perform & perf) = 0;

    const std::string & get_error_message () const
    {
        return m_error_message;
    }

protected:

    void set_error_message (const std::string & msg)
    {
        m_error_message = msg;
    }

};

}           // namespace seq64

#endif      // SEQ64_CONFIGFILE_HPP

/*
 * configfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

