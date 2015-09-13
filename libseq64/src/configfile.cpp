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
 * \file          configfile.cpp
 *
 *  This module defines the base class for configuration and options
 *  files.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-13
 * \license       GNU GPLv2 or above
 *
 *  We found a couple of unused members in this module and removed them.
 */

#include <iostream>

#include "easy_macros.h"
#include "configfile.hpp"

namespace seq64
{

/**
 *  Provides the string constructor for a configuration file.
 *
 * @param a_name
 *      The name of the configuration file.
 */

configfile::configfile (const std::string & a_name)
 :
    m_name  (a_name),
    m_d     (nullptr),
    m_line  ()                  // array of characters
{
   // empty body
}

/**
 *  Gets the next line of data from an input stream.
 *
 *  If the line starts with a number-sign, a space (!), or a null, it
 *  is skipped, to try the next line.  This occurs until an EOF is
 *  encountered.
 *
 *  We may try to convert this item to a reference; pointers can
 *  be subject to problems.  For example, what if someone passes a
 *  nullpointer?  For speed, we don't check it.
 *
 *  Member m_line is a "global" return value.
 *
 * @param a_file
 *      Points to an input stream.
 */

void
configfile::next_data_line (std::ifstream & a_file)
{
    a_file.getline(m_line, sizeof(m_line));
    while
    (
        (m_line[0] == '#' || m_line[0] == ' ' || m_line[0] == 0) && ! a_file.eof()
    )
    {
        a_file.getline(m_line, sizeof(m_line));
    }
}

/**
 * This function gets a specific line of text, specified as a tag.
 *
 * @param a_file
 *      Points to the input file stream.
 *
 * @param a_tag
 *      Provides a tag to be found.  Lines are read until a match occurs
 *      with this tag.
 */

void
configfile::line_after (std::ifstream & a_file, const std::string & a_tag)
{
    a_file.clear();
    a_file.seekg(0, std::ios::beg);
    a_file.getline(m_line, sizeof(m_line));
    while
    (
        strncmp(m_line, a_tag.c_str(), a_tag.length()) != 0  && ! a_file.eof()
    )
    {
        a_file.getline(m_line, sizeof(m_line));
    }
    next_data_line(a_file);
}

}           // namespace seq64

/*
 * configfile.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
