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
 * \updates       2018-09-14
 * \license       GNU GPLv2 or above
 *
 *  We found a couple of unused members in this module and removed them.
 */

#include <iostream>
#include <string.h>                     /* strncmp() function needed!   */

#include "easy_macros.h"
#include "configfile.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Provides the string constructor for a configuration file.
 *
 * \param name
 *      The name of the configuration file.
 */

configfile::configfile (const std::string & name)
 :
    m_error_message (),
    m_name          (name),
    m_d             (nullptr),
    m_line          ()          /* array of characters              */
{
   m_line[0] = 0;               /* guarantee a legal empty string   */
}

/**
 *  A useful intermediate function to save a call and allow for debugging.
 *
 * \param file
 *      The refernce to the opened input file-stream.
 *
 * \return
 *      Returns the value of file.good().
 */

bool
configfile::get_line (std::ifstream & file)
{
    file.getline(m_line, sizeof m_line);
#ifdef PLATFORM_DEBUG_TMI
    std::cout << "Line = " << m_line << std::endl;
#endif
    return file.good();
}

/**
 *  Gets the next line of data from an input stream.  If the line starts with
 *  a number-sign, a space (!), or a null, it is skipped, to try the next
 *  line.  This occurs until an EOF is encountered.
 *
 *  Member m_line is a "global" return value.
 *
 * \param file
 *      Points to an input stream.  We converted this item to a reference;
 *      pointers can be subject to problems.  For example, what if someone
 *      passes a null pointer?  Nonetheless, this is a small library.  Since
 *      this function has this parameter, the caller can deal with multiple
 *      files at the same time.
 *
 * \return
 *      Returns true if a presumed data line was found.  False is returned if
 *      not found before an EOF or a section marker ("[") is found.  This is a
 *      a new (ca 2016-02-14) feature of this function, to assist in adding
 *      new data to the file without crapping out on old-style configuration
 *      files.
 */

bool
configfile::next_data_line (std::ifstream & file)
{
    bool result = get_line(file);
    if (result)
    {
        char ch = m_line[0];
        while ((ch == '#' || ch == '[' || ch == 0) && ! file.eof())
        {
            if (m_line[0] == '[')
            {
                result = false;
                break;
            }
            if (get_line(file))
                ch = m_line[0];
            else
            {
                result = false;
                break;
            }
        }
        if (file.eof())
            result = false;
    }
    return result;
}

/**
 *  Looks for the next named section.  Unlike line_after(), it does not
 *  restart from the beginning of the file.  Like next_data_line(), it starts
 *  at the current line in the file.  This makes it useful in parsing files,
 *  such as a playlist, that has multiple sections with the same name.
 *
 *  Note one other quirk.  If we are on a line matching the tag, then we do
 *  not search, but instead use that line.  The reason is that the
 *  next_data_line() function for the previous section will often end up
 *  at the beginning of the next section.  Especially important with
 *  play-lists.
 *
 * \param file
 *      Points to the input file stream.  Since this function has this
 *      parameter, the caller can deal with multiple files at the same time.
 *      However, the caller must either close the file explicitly or open it
 *      on the stack.
 *
 * \param tag
 *      Provides a tag to be found.  Lines are read until a match occurs
 *      with this tag.  Normally, the tag is a section marker, such as
 *      "[user-interface]".  Best to assume an exact match is needed.
 *
 * \return
 *      Returns true if the tag was found.  Otherwise, false is returned.
 */

bool
configfile::next_section (std::ifstream & file, const std::string & tag)
{
    bool result = false;
    file.clear();
    if (tag == m_line)
    {
        result = true;
    }
    else
    {
        bool ok = get_line(file);
        while (ok)                      /* includes the EOF check           */
        {
            result = strncmp(m_line, tag.c_str(), tag.length()) == 0;
            if (result)
                break;
            else
            {
                if (file.bad())
                {
                    errprint("bad file stream reading config file");
                }
                else
                    ok = get_line(file);
            }
        }
    }
    if (result)
        result = next_data_line(file);

    return result;
}

/**
 *  This function gets a specific line of text, specified as a tag.
 *  Then it gets the next non-blank line (i.e. data line) after that.
 *
 *  This function always starts from the beginning of the file.  Therefore,
 *  it can handle reading Sequencer64 configuration files that have had
 *  their tagged sections arranged in a different order.  This feature makes
 *  the configuration file a little more robust against errors.
 *
 * \param file
 *      Points to the input file stream.  Since this function has this
 *      parameter, the caller can deal with multiple files at the same time.
 *      However, the caller must either close the file explicitly or open it
 *      on the stack.
 *
 * \param tag
 *      Provides a tag to be found.  Lines are read until a match occurs
 *      with this tag.  Normally, the tag is a section marker, such as
 *      "[user-interface]".  Best to assume an exact match is needed.
 *
 * \return
 *      Returns true if the tag was found.  Otherwise, false is returned.
 */

bool
configfile::line_after (std::ifstream & file, const std::string & tag)
{
    bool result = false;
    file.clear();
    file.seekg(0, std::ios::beg);
    bool ok = get_line(file);
    while (ok)                          /* includes the EOF check           */
    {
        result = strncmp(m_line, tag.c_str(), tag.length()) == 0;
        if (result)
        {
            break;
        }
        else
        {
            if (file.bad())
            {
                errprint("bad file stream reading config file");
            }
            else
                ok = get_line(file);
        }
    }
    if (result)
        result = next_data_line(file);

    return result;
}

}           // namespace seq64

/*
 * configfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

