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
 * \file          playlist.cpp
 *
 *  This module declares/defines the base class for managing the <code>
 *  ~/.seq24rc </code> legacy configuration file or the new <code>
 *  ~/.config/sequencer64/sequencer64.rc </code> ("rc") configuration file.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-08-26
 * \updates       2018-08-26
 * \license       GNU GPLv2 or above
 */

#include <string.h>                     /* memset()                         */

#include "playlist.hpp"
#include "perform.hpp"
#include "settings.hpp"                 /* seq64::rc()                      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Principal constructor.
 *
 * \param name
 *      Provides the name of the options file; this is usually a full path
 *      file-specification.
 */

playlist::playlist (const std::string & name)
 :
    configfile  (name)               // base class constructor
{
    // Empty body
}

/**
 *  A rote destructor.
 */

playlist::~playlist ()
{
    // Empty body
}

/**
 *  Helper function for error-handling.  It assembles a message and then
 *  passes it to set_error_message().
 *
 * \param sectionname
 *      Provides the name of the section for reporting the error.
 *
 * \param additional
 *      Additional context information to help in finding the error.
 *
 * \return
 *      Always returns false.
 */

bool
playlist::error_message
(
    const std::string & sectionname,
    const std::string & additional
)
{
    std::string msg = "BAD DATA in playlist [";
    msg += sectionname;
    msg += "]: ";
    if (! additional.empty())
        msg += additional;

    errprint(msg.c_str());
    set_error_message(msg);
    return false;
}

/**
 *  Parse the ~/.config/sequencer64/playlist.lst file.
 *
 *  [midi-control]
 *
 * \param p
 *      Provides the performance object to which all of these options apply.
 *
 * \return
 *      Returns true if the file was able to be opened for reading.
 *      Currently, there is no indication if the parsing actually succeeded.
 */

bool
playlist::parse (perform & p)
{
    std::ifstream file(m_name.c_str(), std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        printf("? error opening [%s] for reading\n", m_name.c_str());
        return false;
    }
    file.seekg(0, std::ios::beg);                           /* seek to start */

    /*
     * [comments]
     *
     * Header commentary is skipped during parsing.  However, we now try to
     * read an optional comment block.
     */

    if (line_after(file, "[comments]"))                 /* gets first line  */
    {
#if 0
        rc().clear_comments();
        do
        {
            rc().append_comment_line(m_line);
            rc().append_comment_line("\n");

        } while (next_data_line(file));
#endif
    }

    /*
     * This call causes parsing to skip all of the header material.  Please note
     * that the line_after() function always starts from the beginning of the
     * file every time.  A lot a rescanning!  But it goes fast these days.
     */

    unsigned sequences = 0;                                 /* seq & ctrl #s */
    if (line_after(file, "[midi-control]"))                 /* find section  */
    {
        sscanf(m_line, "%u", &sequences);
    }
    file.close();           /* done parsing the "rc" configuration file */
    return true;
}

/**
 *  This options-writing function is just about as complex as the
 *  options-reading function.
 *
 * \param p
 *      Provides a const reference to the main perform object.  However,
 *      we have to cast away the constness, because too many of the
 *      perform getter functions are used in non-const contexts.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
playlist::write (const perform & p)
{
    std::ofstream file(m_name.c_str(), std::ios::out | std::ios::trunc);
    perform & ucperf = const_cast<perform &>(p);
    if (! file.is_open())
    {
        printf("? error opening [%s] for writing\n", m_name.c_str());
        return false;
    }

    /*
     * Initial comments and MIDI control section.
     */

    file <<
        "# Sequencer64 0.96.0 (and above) playlist file\n"
        "#\n"
        "# This file holds a playlist for Sequencer64.\n"
        ;

    file << "#\n"
        "# The [comments] section can document this file.  Lines starting\n"
        "# with '#' and '[' are ignored.  Blank lines are ignored.  Show a\n"
        "# blank line by adding a space character to the line.\n"
        ;

    /*
     * [comments]

    file << "\n"
        << "[comments]\n"
        << "\n"
        << comments_block() << "\n"
        ;
    file << "\n"
        "[midi-control]\n"
        "\n"
        ;
     */

    file
        << "# End of " << m_name << "\n#\n"
        << "# vim: sw=4 ts=4 wm=4 et ft=sh\n"   /* ft=sh for nice colors */
        ;

    file.close();
    return true;
}

}           // namespace seq64

/*
 * playlist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

