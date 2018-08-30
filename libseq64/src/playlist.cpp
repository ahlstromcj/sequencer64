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
 * \updates       2018-08-30
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

#include <iostream>                     /* std::cout                        */
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
 *
 */

playlist::song_list playlist::sm_dummy;

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
    m_play_lists                (),
    m_mode                      (false),
    m_current_list              (),                 // play-list iterator
    m_current_song              ()                  // song-list iterator
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
 *  Opens the play-list file and optionally verifies it.
 *
 * \param filename
 *      The name of the file to be opened and parsed.
 *
 * \param verify_it
 *      If true (the default), call verify() to make sure the playlist is
 *      sane.
 *
 * \return
 *      Returns true if the file was parseable and verifiable.
 */

bool
playlist::open (const std::string & filename, bool verify_it)
{
    std::string old_filename = m_name;
    m_name = filename;

    bool result = parse(m_perform);
    if (result)
    {
        if (verify_it)
            result = verify();
    }
    if (! result)
        m_name = old_filename;

    return result;
}

/**
 *  Parse the ~/.config/sequencer64/file.playlist file.
 *
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
    if (file.is_open())
    {
        file.seekg(0, std::ios::beg);                   /* seek to start    */
        m_play_lists.clear();                           /* start fresh      */
        m_comments.clear();

        /*
         * [comments]
         *
         * Header commentary is skipped during parsing.  However, we now try
         * to read an optional comment block, for restoration when rewriting
         * the file.
         */

        if (line_after(file, "[comments]"))             /* gets first line  */
        {
            do
            {
                m_comments += std::string(m_line);
                m_comments += std::string("\n");

            } while (next_data_line(file));
        }

        /*
         * See banner notes.
         */

        bool have_section = line_after(file, "[playlist]");
        while (have_section)
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

                    bool emptydir = plist.ls_file_directory.empty();
                    while (next_data_line(file))
                    {
                        std::string fname = m_line;
                        if (! fname.empty())
                        {
                            song_spec_t sinfo;
                            sinfo.ss_index = count;
                            if (emptydir)
                            {
                                std::string path;
                                std::string filebase;
                                filename_split(fname, path, filebase);
                                sinfo.ss_song_directory = path;
                                sinfo.ss_filename = filebase;
                            }
                            else
                            {
                                sinfo.ss_song_directory = plist.ls_file_directory;
                                sinfo.ss_filename = fname;
                            }

#if __cplusplus >= 201103L
                            std::pair<int, song_spec_t> s =
                                std::make_pair(count, sinfo);
#else
                            std::pair<int, song_spec_t> s =
                                std::make_pair<int, song_spec_t>(count, sinfo);
#endif
                            slist.insert(s);
                            ++count;
                            result = true;
                        }
                    }
                    if (count > 0)
                    {
                        plist.ls_song_count = count;
                        plist.ls_song_list = slist; /* copy the temp list   */
#if __cplusplus >= 201103L
                        std::pair<int, play_list_t> ls =
                            std::make_pair(plist.ls_index, plist);
#else
                        std::pair<int, play_list_t> ls =
                            std::make_pair<int, play_list_t>(plist.ls_index, plist);
#endif
                        m_play_lists.insert(ls);
                    }
                    else
                    {
                        result = false;
                        break;
                    }
                }
                else
                    break;
            }
            else
                break;

            have_section = next_section(file, "[playlist]");
        }
        file.close();           /* done parsing the "playlist" file */
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
 *      currently not used.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
playlist::write (const perform & /*p*/)
{
    std::ofstream file(m_name.c_str(), std::ios::out | std::ios::trunc);
    if (! file.is_open())
    {
        printf("? error opening [%s] for writing\n", m_name.c_str());
        return false;
    }

    /*
     * Initial comments and MIDI control section.
     */

    file
        << "# Sequencer64 0.96.0 (and above) playlist file\n"
        << "#\n"
        << "# Written on " << current_date_time() << "\n"
        << "#\n"
        << "# This file holds a playlist for Sequencer64.\n"
        ;

    file << "#\n"
        "# The [comments] section can document this file.  Lines starting\n"
        "# with '#' are ignored.  Blank lines are ignored.  Show a\n"
        "# blank line by adding a space character to the line.\n"
        ;

    /*
     * [comments]
     */

    file
        << "\n"
        << "[comments]\n"
        << "\n"
        << m_comments << "\n"
        ;

    for
    (
        play_iterator pci = m_play_lists.begin();
        pci != m_play_lists.end(); ++pci
    )
    {
        const play_list_t & pl = pci->second;
        file
        << "\n"
        << "[playlist]\n"
        << "\n"
        << "# Playlist number, arbitrary but unique. 0 to 127 recommended.\n"
        << "# for use with MIDI control.\n"
        << pl.ls_index << "\n\n"
        << "# Display name of this play list.\n"
        << "\"" << pl.ls_list_name << "\"\n\n"
        << "# Storage directory for the song-files in this play list.\n"
        << pl.ls_file_directory << "\n"
        << "\n"
        << "# The base file-names (tune.midi) of songs in this playlist:\n"
        ;

        const song_list & sl = pl.ls_song_list;
        for (song_iterator sci = sl.begin(); sci != sl.end(); ++sci)
        {
            const song_spec_t & s = sci->second;
            file << s.ss_filename << "\n";
        }
    }

    file
        << "\n"
        << "# End of " << m_name << "\n#\n"
        << "# vim: sw=4 ts=4 wm=4 et ft=sh\n"   /* ft=sh for nice colors */
        ;

    file.close();
    return true;
}

/**
 *  Goes through all of the playlists and makes sure that all of the song files
 *  are accessible.
 */

bool
playlist::verify ()
{
    bool result = true;
    for
    (
        play_iterator pci = m_play_lists.begin();
        pci != m_play_lists.end(); ++pci
    )
    {
        const song_list & sl = pci->second.ls_song_list;
        for (song_iterator sci = sl.begin(); sci != sl.end(); ++sci)
        {
            const song_spec_t & s = sci->second;
            if (! file_exists(s.ss_filename))
            {
#ifdef PLATFORM_DEBUG
                printf
                (
                    "playlist '%s', song '%s' is missing\n",
                    pci->second.ls_list_name.c_str(), s.ss_filename.c_str()
                );
#endif
                result = false;
                break;
            }
        }
    }
    return result;
}

/**
 *
 */

void
playlist::clear ()
{
    m_comments.clear();
    m_play_lists.clear();
    m_mode = false;
    m_current_list = m_play_lists.end();
}

/**
 *
 */

bool
playlist::reset ()
{
    bool result = ! m_play_lists.empty();
    if (result)
    {
    }
    return result;
}

/**
 *
 */

bool
playlist::select_list (int index, bool selectsong)
{
    play_iterator pci = m_play_lists.find(index);
    bool result = pci != m_play_lists.end();
    if (result)
    {
        const play_list_t & pl = pci->second;
#ifdef PLATFORM_DEBUG
        show_list(pl);
#endif
        if (selectsong)
        {
            /*
             * Similar to select_song(0), but we don't need to check so many
             * iterators.
             */

            // TODO
        }
    }
    return result;
}

/**
 *  Moves to the next play-list.  If the iterator reaches the end, this
 *  function wraps around to the beginning.  Also see the other return value
 *  conditions.
 *
 * \param selectsong
 *      If true (the default), the first song in the play-list is selected.
 *
 * \return
 *      Returns true if the play-list iterator was able to be moved, or if
 *      there was only one play-list, so that movement was unnecessary. If the
 *      there are no play-lists, then false is returned.
 */

bool
playlist::next_list (bool selectsong)
{
    bool result = m_play_lists.size() > 0;
    if (m_play_lists.size() > 1)
    {
        ++m_current_list;
        if (m_current_list == m_play_lists.end())
            m_current_list = m_play_lists.begin();

        if (selectsong)
        {
            /*
             * Similar to select_song(0), but we don't need to check so many
             * iterators.
             */

            // TODO
        }
    }
    return result;
}

/**
 *  Moves to the previous play-list.  If the iterator reaches the beginning,
 *  this function wraps around to the end.  Also see the other return value
 *  conditions.
 *
 * \param selectsong
 *      If true (the default), the first song in the play-list is selected.
 *
 * \return
 *      Returns true if the play-list iterator was able to be moved, or if
 *      there was only one play-list, so that movement was unnecessary. If the
 *      there are no play-lists, then false is returned.
 */

bool
playlist::previous_list (bool selectsong)
{
    bool result = m_play_lists.size() > 0;
    if (m_play_lists.size() > 1)
    {
        if (m_current_list == m_play_lists.begin())
            m_current_list = std::prev(m_play_lists.end());
        else
            --m_current_list;

        if (selectsong)
        {
            /*
             * Similar to select_song(0), but we don't need to check so many
             * iterators.
             */

            // TODO
        }
    }
    return result;
}

/**
 *  Obtains the current song index, which is a number starting at 0 that
 *  indicates the songs position in the list.  If the current-list iterator is
 *  invalid, or the current-song iterator is invalid, then (-1)
 *  is returned.
 */

int
playlist::song_index () const
{
    int result = (-1);
    if (m_current_list != m_play_lists.end())
    {
        if (m_current_song != m_current_list->second.ls_song_list.end())
            result = m_current_song->second.ss_index;
    }
    return result;
}

/**
 *
 */

const std::string &
playlist::song_filename () const
{
    static std::string s_dummy;
    if (m_current_list != m_play_lists.end())
    {
        if (m_current_song != m_current_list->second.ls_song_list.end())
            return m_current_song->second.ss_filename;
        else
            return s_dummy;
    }
    else
        return s_dummy;
}

/**
 *
 */

std::string
playlist::song_filepath () const
{
    std::string result;
    if (m_current_list != m_play_lists.end())
    {
        if (m_current_song != m_current_list->second.ls_song_list.end())
        {
            result = m_current_song->second.ss_song_directory;
            result += m_current_song->second.ss_filename;
        }
    }
    return result;
}

/**
 *
 */

bool
playlist::select_song (int index)
{
    bool result = m_current_list != m_play_lists.end();
    if (result)
    {
        song_iterator sci = m_current_list->second.ls_song_list.find(index);

        result = sci != m_current_list->second.ls_song_list.end();
        if (result)
        {
#ifdef PLATFORM_DEBUG
            const song_spec_t & s = sci->second;
            show_song(s);
#endif
        }
    }
    return result;
}

/**
 *
 */

void
playlist::show_list (const play_list_t & pl) const
{
    std::cout
        << "[playlist] " << pl.ls_index
        << ": '" << pl.ls_list_name
        << "', directory '" << pl.ls_file_directory
        << "', " << pl.ls_song_count << " songs"
        << std::endl
        ;
}

/**
 *
 */

void
playlist::show_song (const song_spec_t & s) const
{
    std::cout
        << "    Song file " << s.ss_index
        << ": '" << s.ss_song_directory << s.ss_filename
        << std::endl
        ;
}


/**
 *  Performs a simple dump of the playlists, most for troubleshooting.
 */

void
playlist::show () const
{
    if (m_play_lists.empty())
    {
        printf("No items in playist.\n");
    }
    else
    {
        for
        (
            play_iterator pci = m_play_lists.begin();
            pci != m_play_lists.end(); ++pci
        )
        {
            const play_list_t & pl = pci->second;
            show_list(pl);

            const song_list & sl = pl.ls_song_list;
            song_iterator sci;
            for (sci = sl.begin(); sci != sl.end(); ++sci)
            {
                const song_spec_t & s = sci->second;
                show_song(s);
            }
        }
    }
}

}           // namespace seq64

/*
 * playlist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

