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
 * \updates       2018-08-29
 * \license       GNU GPLv2 or above
 *
 *  Here is a skeletal representation of a Sequencer64 playlist:
 *
 *      [playlist]
 *
 *      0                       # playlist number, which can be arbitrary
 *      "Downtempo"             # playlist name, for display/selection
 *      /home/user/midifiles/   # directory where the songs are stored
 *      file1.mid
 *      file2.midi
 *      file3.midi
 *       . . .
 *
 */

#include <utility>                      /* std::make_pair()                 */
#include <string.h>                     /* memset()                         */

#include "file_functions.hpp"           /* functions for file-names         */
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

playlist::playlist (perform & p, const std::string & name)
 :
    configfile                  (name),              // base class constructor
    m_perform                   (p),
    m_comments                  (),
    m_play_list                 (),
    m_mode                      (false),
    m_list_directory            (),
    m_list_filename             (),
    m_current_list_index        (-1),
    m_current_list_name         (),
    m_song_directory            (),
    m_current_song_index        (-1),
    m_current_song_filename     (),
    m_current_song_count        (0)
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
 *  Parse the ~/.config/sequencer64/file.playlist file.
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
    bool result = false;
    std::ifstream file(m_name.c_str(), std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        printf("? error opening [%s] for reading\n", m_name.c_str());
        return false;
    }
    if (file.is_open())
    {
        file.seekg(0, std::ios::beg);                       /* seek to start    */
        m_play_list.clear();                                /* start fresh      */
        m_comments.clear();

        /*
         * [comments]
         *
         * Header commentary is skipped during parsing.  However, we now try
         * to read an optional comment block, for restoration when rewriting
         * the file.
         */

        if (line_after(file, "[comments]"))                 /* gets first line  */
        {
            do
            {
                m_comments += std::string(m_line);
                m_comments += std::string("\n");

            } while (next_data_line(file));
        }

        /*
         * The next_section() function is like line-after, but scans from the
         * current line in the file.  Necessary here because all the sections
         * have the same name.  After detecting the "[playlist]" section, the
         * following items need to be obtained:
         *
         *      -   Playlist number.  This number is used as the key value for
         *          the playlist. It can be any integer, and the order of the
         *          playlists is based on this integer.
         *      -   Playlist name.  A human-readable string describing the
         *          nick-name for the playlist.  This is an alternate way to
         *          look up the playlist.
         *      -   Song directory name.  The directory where the songs are
         *          stored.  If this name is empty, then the song file-names
         *          need to include the individual directories for each file.
         *      -   Song file-name, or path to the song file-name.
         *
         * Note that the call to next_section() already gets to the next line
         * of data, which should be the index number of the playlist.
         */

        bool ok = true;
        while (next_section(file, "[playlist]"))        /* find section     */
        {
            int count = 0;
            play_list_t plist;                          /* current playlist */
            sscanf(m_line, "%d", &plist.ls_index);      /* playlist number  */
            if (next_data_line(file))
            {
                song_list slist;
                std::string line = m_line;
                plist.ls_list_name = strip_quotes(line);
                if (next_data_line(file))
                {
                    /*
                     * Make sure the directory name is canonical and clean.  The
                     * existence of the file should be validated later.
                     */

                    line = m_line;
                    plist.ls_file_directory = clean_path(line);

                    slist.clear();
                    while (next_data_line(file))
                    {
                        std::string fname = m_line;
                        if (! fname.empty())
                        {
                            song_list_t sinfo;
                            sinfo.ss_index = count;
                            sinfo.ss_filename = fname;

#if __cplusplus >= 201103L
                            std::pair<int, song_list_t> s =
                                std::make_pair(count, sinfo);
#else
                            std::pair<int, song_list_t> s =
                                std::make_pair<int, song_list_t>(count, sinfo);
#endif
                            slist.insert(s);
                            ++count;
                        }
                    }
                    plist.ls_song_count = count;
                    plist.ls_song_list = slist;     /* copy the temp list   */
#if __cplusplus >= 201103L
                    std::pair<int, play_list_t> ls =
                        std::make_pair(plist.ls_index, plist);
#else
                    std::pair<int, play_list_t> ls =
                        std::make_pair<int, play_list_t>(plist.ls_index, plist);
#endif
                    m_play_list.insert(ls);
                }
                else
                {
                    ok = false;
                    break;
                }
            }
            else
            {
                ok = false;
                break;
            }
        }
        result = ok;
        file.close();           /* done parsing the "rc" configuration file */
    }
    else
        printf("? error opening [%s] for reading\n", m_name.c_str());

    m_mode = result;
    return result;
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
     */

    file << "\n"
        << "[comments]\n"
        << "\n"
        << m_comments << "\n"
        ;

    std::map<int, play_list_t>::const_iterator pci;
    for (pci = m_play_list.begin(); pci != m_play_list.end(); ++pci)
    {
        // TODO
    }

    file
        << "# End of " << m_name << "\n#\n"
        << "# vim: sw=4 ts=4 wm=4 et ft=sh\n"   /* ft=sh for nice colors */
        ;

    file.close();
    return true;
}

/**
 *  Performs a simple dump of the playlists, most for troubleshooting.
 */

void
playlist::show ()
{
    if (m_play_list.empty())
    {
        printf("No items in playist.\n");
    }
    else
    {
        std::map<int, play_list_t>::const_iterator pci;
        for (pci = m_play_list.begin(); pci != m_play_list.end(); ++pci)
        {
            const play_list_t & pl = pci->second;
            printf
            (
                "%d [playlist] %d:  '%s'\n",
                pci->first, pl.ls_index, pl.ls_list_name.c_str()
            );
            printf
            (
                "  Directory '%s', %d songs \n",
                pl.ls_file_directory.c_str(), pl.ls_song_count
            );
            // TODO: list the songs with indentation
        }
    }
}

}           // namespace seq64

/*
 * playlist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

