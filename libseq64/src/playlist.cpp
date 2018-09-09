/*
 *  This file is part of seq24/sequencer64.
 *
 *  sequencer64 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  sequencer64 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with sequencer64; if not, write to the Free Software
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
 * \updates       2018-09-08
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

#include <cctype>                       /* std::toupper() function          */
#include <iostream>                     /* std::cout                        */
#include <utility>                      /* std::make_pair()                 */
#include <string.h>                     /* memset()                         */

#include "file_functions.hpp"           /* functions for file-names         */
#include "playlist.hpp"
#include "perform.hpp"
#include "settings.hpp"                 /* seq64::rc()                      */
#include "wrkfile.hpp"                  /* seq64::midifile & seq64::wrkfile */

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
    configfile                  (name),             // base class constructor
    m_perform                   (p),                // the owner of this object
    m_comments                  (),
    m_play_lists                (),
    m_mode                      (false),
    m_current_list              (),                 // play-list iterator
    m_current_song              ()                  // song-list iterator
{
    // No code needed
}

/**
 *  This destructor unregisters this playlist from the perform object.
 */

playlist::~playlist ()
{
    // No code needed
}

/**
 *  Helper function for error-handling.  It assembles a message and then
 *  passes it to set_error_message().
 *
 * \param additional
 *      Additional context information to help in finding the error.
 *
 * \return
 *      Always returns false.
 */

bool
playlist::make_error_message (const std::string & additional)
{
    std::string msg = "BAD [playlist]";
    if (! additional.empty())
    {
        msg += ": ";
        msg += additional;
    }
    errprint(msg.c_str());
    set_error_message(msg);
    return false;
}

/**
 *  Opens the current play-list file and optionally verifies it.
 *
 * \param verify_it
 *      If true (the default), call verify() to make sure the playlist is
 *      sane.
 *
 * \return
 *      Returns true if the file was parseable and verifiable.
 */

bool
playlist::open (bool verify_it)
{
    bool result = parse(m_perform);
    if (result)
    {
        if (verify_it)
            result = verify();
    }
    mode(result);
    return result;
}

/**
 *  Opens the play-list file and optionally verifies it.  Provisionally sets
 *  m_name to the provided file-name, but will clear the current play-list if
 *  something bad happens.  We cannot easily recover from a bad play-list.
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
    bool result = open(verify_it);
    if (! result)
        clear();

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
 *          But even if not empty, the play-list directory is not used if
 *          the song file-name includes a path, as indicated by "/" or "\".
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
    std::ifstream file(name().c_str(), std::ios::in | std::ios::ate);
    if (file.is_open())
    {
        file.seekg(0, std::ios::beg);                   /* seek to start    */
        m_play_lists.clear();                           /* start fresh      */
        m_comments.clear();                             /* ditto            */

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

        int listcount = 0;
        bool have_section = line_after(file, "[playlist]");
        if (! have_section)
        {
            result = make_error_message("empty section");
        }
        while (have_section)
        {
            int listnumber = -1;
            int songcount = 0;
            play_list_t plist;                          /* current playlist */
            sscanf(m_line, "%d", &listnumber);          /* playlist number  */
            if (next_data_line(file))
            {
                std::string line = m_line;
                song_list slist;
                plist.ls_list_name = strip_quotes(line);
                if (next_data_line(file))
                {
                    /*
                     * We need to get the song's MIDI control number and it's
                     * directory name.  Make sure the directory name is
                     * canonical and clean.  The existence of the file should
                     * be validated later.  Also determine if the song
                     * file-name already has a directory before using the
                     * play-list's directory.
                     */

                    line = m_line;
                    plist.ls_file_directory = clean_path(line);
                    slist.clear();
                    while (next_data_line(file))
                    {
                        int songnumber = -1;
                        std::string fname;
                        result = scan_song_file(songnumber, fname);
                        if (result)
                        {
                            song_spec_t sinfo;
                            sinfo.ss_index = songcount;
                            sinfo.ss_midi_number = songnumber;
                            if (name_has_directory(fname))
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
                            (void) add_song(slist, sinfo);
                            ++songcount;
                        }
                        else
                            break;
                    }

                    /*
                     * Need to deal with a false result still....
                     */

                    if (songcount > 0)
                    {
                        plist.ls_index = listcount;         /* ordinal      */
                        plist.ls_midi_number = listnumber;  /* MIDI mapping */
                        plist.ls_song_count = songcount;
                        plist.ls_song_list = slist;         /* copy temp    */
                        result = add_list(plist);
                    }
                    else
                    {
                        result = make_error_message("no songs");
                        break;
                    }
                }
                else
                {
                    std::string msg = "no list directory in playlist #" +
                        std::to_string(listnumber);

                    result = make_error_message(msg);
                    break;
                }
            }
            else
            {
                std::string msg = "no data in playlist #" +
                    std::to_string(listnumber);

                result = make_error_message(msg);
                break;
            }
            ++listcount;
            have_section = next_section(file, "[playlist]");
        }
        file.close();           /* done parsing the "playlist" file */
    }
    else
    {
        std::string msg = "error opening file [" + name() + "]";
        result = make_error_message(msg);
    }
    if (result)
        result = reset();

    mode(result);
    return result;
}

/**
 *  Encapsulates some groty code for the parse() function.  It assumes that
 *  next_data_line() has retrieved a file-name line for a song.
 *
 * \param [out] song_number
 *      Holds the song number that was retrieved.  Use it only if not equal
 *      to -1 and if this function returns true.
 *
 * \param [out] song_file
 *      Holds the song file-name that was retrieved.  Use it only if not
 *      empty and if this function returns true.
 *
 * \return
 *      Returns true if this function succeeded.  If false, an error message is
 *      set up.
 */

bool
playlist::scan_song_file (int & song_number, std::string & song_file)
{
    bool result = false;
    int songnumber = -1;
    const char * dirname = &m_line[0];
    int sscount = sscanf(m_line, "%d", &songnumber);
    if (sscount == EOF || sscount == 0)
    {
        song_number = -1;                                   /* side-effect  */
        song_file.clear();                                  /* side-effect  */
        result = make_error_message("song number missing");
    }
    else
    {
        while (! std::isspace(*dirname))
        {
            if (*dirname == 0)
                break;

            ++dirname;
        }
        while (std::isspace(*dirname))
        {
            if (*dirname == 0)
                break;

            ++dirname;
        }
        bool gotit = std::isalnum(*dirname) || std::ispunct(*dirname);
        if (gotit)
        {
            song_number = songnumber;                       /* side-effect  */
            song_file = dirname;                            /* side-effect  */
            result = true;
        }
        else
        {
            song_number = -1;                               /* side-effect  */
            song_file.clear();                              /* side-effect  */
            result = make_error_message("song file-path missing");
        }
    }
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
    std::ofstream file(name().c_str(), std::ios::out | std::ios::trunc);
    if (! file.is_open())
    {
        errprintf("error opening [%s] for writing\n", name().c_str());
        return false;
    }

    /*
     * Initial comments and MIDI control section.
     */

    file
        << "# Sequencer64 0.96.0 (and above) playlist file\n"
        << "#\n"
        << "# " << name() << "\n"
        << "# Written on " << current_date_time() << "\n"
        << "#\n"
        << "# This file holds a playlist for Sequencer64. It consists of one\n"
        << "# or more '[playlist]' sections.  Each section has a user-specified\n"
        << "# number.  This number should range from 0 to 127, but it can go\n"
        << "# higher if the user doesn't need to use MIDI control to select\n"
        << "# a playlist. Ultimately, the playlists are sorted by this number.\n"
        << "#\n"
        << "# Next comes a display name for this list, with or without quotes.\n"
        << "#\n"
        << "# Next comes the name of the directory, always using the UNIX-style\n"
        << "# separator, a forward slash (solidus).  It can optionally be\n"
        << "# terminated with a slash.\n"
        << "#\n"
        << "# The last item is a line containing the MIDI song-control number,\n"
        << "# followed by the name of the MIDI files.  They are sorted by the\n"
        << "# control number, starting from 0.  They can be simple 'base.ext'\n"
        << "# file-names; the playlist directory will be prepended before the\n"
        << "# song is accessed.\n"
        << "#\n"
        << "# If the MIDI file-name already has a directory name, that will be\n"
        << "# used instead.\n"
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
        const_play_iterator pci = m_play_lists.begin();
        pci != m_play_lists.end(); ++pci
    )
    {
        const play_list_t & pl = pci->second;
        file
        << "\n"
        << "[playlist]\n"
        << "\n"
        << "# Playlist number, arbitrary but unique. 0 to 127 recommended\n"
        << "# for use with the MIDI playlist control.\n"
        << pl.ls_midi_number << "\n\n"
        << "# Display name of this play list.\n\n"
        << "\"" << pl.ls_list_name << "\"\n\n"
        << "# Default storage directory for the song-files in this playlist.\n\n"
        << pl.ls_file_directory << "\n"
        << "\n"
        << "# Provides the MIDI song-control number, and also the\n"
        << "# base file-name (tune.midi) of each song in this playlist.\n"
        << "# The playlist directory is used, unless the file-name contains its\n"
        << "# own path.\n\n"
        ;

        /*
         * Here, we need to write the MIDI control number, following only by
         * the file-name, which could include the path-name.
         */

        const song_list & sl = pl.ls_song_list;
        for (const_song_iterator sci = sl.begin(); sci != sl.end(); ++sci)
        {
            const song_spec_t & s = sci->second;
            file << s.ss_midi_number << " " << s.ss_filename << "\n";
        }
    }

    file
        << "\n"
        << "# End of " << name() << "\n#\n"
        << "# vim: sw=4 ts=4 wm=4 et ft=sh\n"   /* ft=sh for nice colors */
        ;

    file.close();
    return true;
}

/**
 *  Given a file-name, opens that file as a song.  This function holds common
 *  code.
 *
 *  Before the song is loaded, the current song is cleared from memory.
 *  Remember that clear_all() will fail if it detects a sequence being edited.
 *  In that case, this function will fail as well.
 *
 * \param fname
 *      The full path to the file to be opened.
 *
 * \param playlistmode
 *      If true, open the file in play-list mode.  Currently, this means
 *      is that some output from the file-opening process is suppressed, and
 *      the perform::clear_all() function is called right after parsing the
 *      song file.
 */

bool
playlist::open_song (const std::string & fname, bool playlistmode)
{
    bool result = m_perform.clear_all();
    if (result)
    {
        bool is_wrk = file_extension_match(fname, "wrk");
        if (is_wrk)
        {
            wrkfile m(fname, SEQ64_USE_DEFAULT_PPQN, playlistmode);
            result = m.parse(m_perform);
        }
        else
        {
            midifile m(fname, SEQ64_USE_DEFAULT_PPQN, false, true, playlistmode);
            result = m.parse(m_perform);
        }
        if (playlistmode)
            (void) m_perform.clear_all();
    }
    return result;
}

/**
 *  Goes through all of the playlists and makes sure that all of the song files
 *  are accessible.
 *
 * \param strong
 *      If true, also make sure the MIDI files open without error as well.
 *      The code is similar to open_midi_file() in the midifile module, but it
 *      does not make configuration settings.
 *
 * \return
 *      Returns true if all of the MIDI files are verifiable.
 */

bool
playlist::verify (bool strong)
{
    bool result = ! m_play_lists.empty();
    if (result)
    {
        for
        (
            const_play_iterator pci = m_play_lists.begin();
            pci != m_play_lists.end(); ++pci
        )
        {
            const song_list & sl = pci->second.ls_song_list;
            for (const_song_iterator sci = sl.begin(); sci != sl.end(); ++sci)
            {
                const song_spec_t & s = sci->second;
                const std::string fname = song_filepath(s);
                if (file_exists(fname))
                {
                    if (strong)
                    {
                        /*
                         * The file is parsed.  If the result is false, then the
                         * play-list mode end up false.  Although we don't
                         * really need a playlist-mode flag here, it is useful
                         * to cut down on console output.  Let the caller do
                         * the reporting on errors only.
                         */

                        result = open_song(fname, true);
                        if (! result)
                        {
                            make_file_error_message("song '%s' missing", fname);
                            break;
                        }
                    }
                }
                else
                {
                    std::string fmt = pci->second.ls_list_name;
                    fmt += ": song '%s' is missing";
                    result = make_file_error_message(fmt, fname);
                    break;
                }
            }
            if (! result)
                break;
        }
    }
    else
    {
        std::string msg = "empty list file '";
        msg += name();
        msg += "'";
        make_error_message(msg);
    }
    return result;
}

/**
 *  Opens/loads the current song.
 *
 * \return
 *      Returns true if there was a song to be opened, and it opened properly.
 */

bool
playlist::open_current_song ()
{
    bool result = false;
    if (m_current_list != m_play_lists.end())
    {
        if (m_current_song != m_current_list->second.ls_song_list.end())
        {
            const std::string fname = song_filepath(m_current_song->second);
            result = open_song(fname);
            if (! result)
                (void) make_file_error_message("could not open song '%s'", fname);
        }
    }
    return result;
}

/**
 *
 */

bool
playlist::open_next_list (bool opensong)
{
    bool result = next_list(true);      /* select the next list, first song */
    if (result && opensong)
        result = open_current_song();

    return result;
}

/**
 *
 */

bool
playlist::open_previous_list (bool opensong)
{
    bool result = previous_list(true);  /* select the prev. list, first song */
    if (result && opensong)
        result = open_current_song();

    return result;
}

/**
 *
 */

bool
playlist::open_next_song (bool opensong)
{
    bool result = next_song();
    if (result && opensong)
        result = open_current_song();

    return result;
}

/**
 *
 */

bool
playlist::open_previous_song (bool opensong)
{
    bool result = previous_song();
    if (result && opensong)
        result = open_current_song();

    return result;
}

/**
 *  Makes a file-error message.
 */

bool
playlist::make_file_error_message
(
    const std::string & fmt,
    const std::string & filename
)
{
    char tmp[256];
    snprintf(tmp, sizeof tmp, fmt.c_str(), filename.c_str());
    make_error_message(tmp);
    return false;
}

/**
 *
 */

void
playlist::clear ()
{
    m_comments.clear();
    m_play_lists.clear();
    mode(false);
    m_current_list = m_play_lists.end();
}

/**
 *  Resets to the first play-list and the first-song in that playlist.
 *
 * \return
 *      Returns true if the play-lists where present and the first song of the
 *      first play-list was able to be selected.
 */

bool
playlist::reset ()
{
    bool result = ! m_play_lists.empty();
    if (result)
    {
        m_current_list = m_play_lists.begin();
        result = select_song(0);
    }
    else
        clear();

    return result;
}

/*
 *  List-container functions.
 */

/**
 *  Adds an already set playlist structure.  It is copied into the list of
 *  play-lists.  It assumes all of the fields in the play-list have been set,
 *  including an empty song-list.
 *
 * \param plist
 *      Provides a reference to the new playlist to be added.  This parameter
 *      is copied into the list.
 *
 * \return
 *      Returns true if the count of playlists has changed.  However, if a
 *      playlist was simply being modified, this value is false.  So the usage
 *      of the return parameter is dependent upon the context of the call.
 */

bool
playlist::add_list (play_list_t & plist)
{
    bool result = false;
    int count = int(m_play_lists.size());
    int listnumber = plist.ls_midi_number;      /* MIDI control number  */
    if (listnumber >= 0)
    {
#if __cplusplus >= 201103L
        std::pair<int, play_list_t> ls =
            std::make_pair(listnumber, plist);
#else
        std::pair<int, play_list_t> ls =
            std::make_pair<int, play_list_t>(listnumber, plist);
#endif
        m_play_lists.insert(ls);
        result = int(m_play_lists.size()) == count + 1;
    }
    return result;
}

/**
 *  Selects a play-list with the given index.
 *
 * \param index
 *      The index of the play-list re 0.  Generally should be restricted to
 *      the range of 0 to 127, to be suitable for MIDI control.
 *
 * \param selectsong
 *      If true, then the first (0th) song in the play-list is selected.
 *
 * \return
 *      Returns true if the selected play-list is valid.  If true, then the
 *      m_current_list iterator points to the current list.
 */

bool
playlist::select_list (int index, bool selectsong)
{
    play_iterator pci = m_play_lists.find(index);
    bool result = pci != m_play_lists.end();
    if (result)
    {
#ifdef PLATFORM_DEBUG_TMI
        show_list(pci->second);
#endif
        m_current_list = pci;
        if (selectsong)
            select_song(0);
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

#ifdef PLATFORM_DEBUG_TMI
        show_list(m_current_list->second);
#endif

        if (selectsong)
            select_song(0);
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

#ifdef PLATFORM_DEBUG_TMI
        show_list(m_current_list->second);
#endif

        if (selectsong)
            select_song(0);
    }
    return result;
}

/**
 *  The following four functions are to be used for a playlist editor, though
 *  they can also be used for parsing/loading the playlist.  Kind of a low
 *  priority, but we are leaving room for the concept.
 *
 *  In usage, the playlist and its songs are ordered by MIDI control number.
 *  That is, the MIDI control number is the key, which allows for the fast
 *  lookup of the MIDI control number during a live performance involving MIDI
 *  control from the musician.
 *
 *  In editing, the playlist and its songs are ordered by MIDI control number.
 *  When a playlist is added, we are adding a row to the table, keyed by the
 *  control number.  We then have to re-order the playlist and repopulate the
 *  table.  The song-list for the new playlist is empty, and the playlist
 *  table must reflect that.
 *
 *  Adding a song to the new playlist, if it is indeed new, should be
 *  straightforward.
 *
 *  Adding a song to an existing playlist (which might be a new playlist)
 *  means inserting the song and re-ordering the playlist's songs as
 *  necessary).
 *
 *  The add_list() function adds a playlist and re-orders the list of
 *  playlists.  The playlist is initially empty, and an empty song-list is
 *  created.
 *
 * \todo
 *
 * \return
 *      Returns the return-value of the add_list() overload, which returns
 *      true if the count has changed.
 */

/**
 *  An overloaded function to encapsulare adding a playlist and make the
 *  callers simpler.  The inserted list has an empty song-list.  This function
 *  is intended for use by a playlist editor.
 */

bool
playlist::add_list
(
    int index,
    int midinumber,
    const std::string & name,
    const std::string & directory
)
{
    play_list_t plist;                  /* will be copied upon insertion    */
    plist.ls_index = index;             /* an ordinal value from list table */
    plist.ls_midi_number = midinumber;  /* MIDI control number to use       */
    plist.ls_list_name = name;
    plist.ls_file_directory = directory;
    plist.ls_song_count = 0;            /* no songs to start in new list    */

    /*
     * Song list is empty at first, created by the playlist default constructor.
     *
     *      plist.ls_song_list = slist;
     */

    return add_list(plist);
}

/**
 *  This function removes a playlist at the given index.  The index is an
 *  ordinal, not a key, therefore we have to iterate through the whole list
 *  until we encounter the desired index.  Useful for removing the selected
 *  playlist in a table.
 *
 *  This function works by iterating to the index'th element in the playlist
 *  and deleting it.
 *
 * \param index
 *      The ordinal value (not a key) of the desired table row.
 *
 * \return
 *      Returns true if the desired list was found and removed.
 */

bool
playlist::remove_list (int index)
{
    bool result = false;
    int count = 0;
    play_iterator pci;
    for (pci = m_play_lists.begin(); pci != m_play_lists.end(); /*++pci,*/ ++count)
    {
        if (count == index)
        {
            pci = m_play_lists.erase(pci);
            result = true;
            break;
        }
        else
            ++pci;
    }
    if (result)
        reorder_play_list(m_play_lists);

    return result;
}

/**
 *  Moves through the play-list container in key (MIDI control number) order,
 *  modifying the index value of each playlist in the main list.
 */

void
playlist::reorder_play_list (play_list & pl)
{
    int index = 0;
    for (play_iterator pci = pl.begin(); pci != pl.end(); ++pci, ++index)
    {
        play_list_t & p = pci->second;
        p.ls_index = index;
    }
}

/*
 *  Song-container functions.
 */

/**
 *  Obtains the current song index, which is a number starting at 0 that
 *  indicates the song's position in the list.  This value is useful to put
 *  the song information at the right place in a table, for example. If the
 *  current-list iterator is invalid, or the current-song iterator is invalid,
 *  then (-1) is returned.
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
 *  Obtains the current song MIDI control number, which is a number ranging
 *  from 0 to 127 that indicates the control number that triggers the song. If
 *  the current-list iterator is invalid, or the current-song iterator is
 *  invalid, then (-1) is returned.
 */

int
playlist::song_midi_number () const
{
    int result = (-1);
    if (m_current_list != m_play_lists.end())
    {
        if (m_current_song != m_current_list->second.ls_song_list.end())
            result = m_current_song->second.ss_midi_number;
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
playlist::song_filepath (const song_spec_t & sinfo) const
{
    std::string result = clean_path(sinfo.ss_song_directory);
    result += sinfo.ss_filename;
    return result;
}

/**
 *  Gets the current song-specification from the current play-list, and, if
 *  valid concatenates the song's directory and file-name.
 *
 * \return
 *      Returns the song's directory and file-name as a full path
 *      specification.  However, if there's an error, then an empty string is
 *      returned.
 */

std::string
playlist::song_filepath () const
{
    std::string result;
    if (m_current_list != m_play_lists.end())
    {
        if (m_current_song != m_current_list->second.ls_song_list.end())
            result = song_filepath(m_current_song->second);
    }
    return result;
}

/**
 *  Provides a one-line description containing the current play-list name and
 *  song file.
 *
 * \return
 *      Returns the play-list name and song file-name.  If not in playlist
 *      mode, or an item cannot be found, then an empty string is returned.
 */

std::string
playlist::current_song () const
{
    std::string result;
    if (mode())
    {
        if (m_current_list != m_play_lists.end())
        {
            if (m_current_song != m_current_list->second.ls_song_list.end())
            {
                result = m_current_list->second.ls_list_name;
                result += ": ";
                result += m_current_song->second.ss_filename;
            }
        }
    }
    return result;
}

/**
 *  Selects a song with the given index.
 *
 * \param index
 *      The index of the song re 0.  Generally should be restricted to the
 *      range of 0 to 127, to be suitable for MIDI control.
 *
 * \return
 *      Returns true if the current play-list and the current song are valid.
 *      If true, then the m_current_song iterator points to the current song.
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
#ifdef PLATFORM_DEBUG_TMI
            show_song(sci->second);
#endif
            m_current_song = sci;
        }
    }
    return result;
}

/**
 *
 */

bool
playlist::next_song ()
{
    bool result = m_current_list != m_play_lists.end();
    if (result)
    {
        ++m_current_song;
        if (m_current_song == m_current_list->second.ls_song_list.end())
            m_current_song = m_current_list->second.ls_song_list.begin();
    }
    return result;
}

/**
 *
 */

bool
playlist::previous_song ()
{
    bool result = m_current_list != m_play_lists.end();
    if (result)
    {
        if (m_current_song == m_current_list->second.ls_song_list.begin())
            m_current_song = std::prev(m_current_list->second.ls_song_list.end());
        else
            --m_current_song;
    }
    return result;
}

/**
 *  Adds a song to the current playlist, if available.
 */

bool
playlist::add_song (song_spec_t & sspec)
{
    bool result = m_current_list != m_play_lists.end();
    if (result)
        result = add_song(m_current_list->second.ls_song_list, sspec);

    return result;
}

/**
 *
 */

bool
playlist::add_song (song_list & slist, song_spec_t & sspec)
{
    bool result = false;
    int count = int(slist.size());
    int songnumber = sspec.ss_midi_number;
#if __cplusplus >= 201103L              // C++11
    std::pair<int, song_spec_t> s = std::make_pair(songnumber, sspec);
#else
    std::pair<int, song_spec_t> s =
        std::make_pair<int, song_spec_t>(songnumber, sspec);
#endif
    slist.insert(s);
    result = int(slist.size()) == count + 1;
    return result;
}

/**
 *
 */

bool
playlist::add_song (play_list_t & plist, song_spec_t & sspec)
{
    song_list & sl = plist.ls_song_list;
    bool result = add_song(sl, sspec);
    return result;
}

/**
 *  This function removes a song from the current playlist at the given index.
 *  The index is an
 *  ordinal, not a key, therefore we have to iterate through the whole list
 *  until we encounter the desired index.  Useful for removing the selected
 *  song in a table.
 *
 *  This function works by iterating to the index'th element in the song-list
 *  and deleting it.
 *
 * \param index
 *      The ordinal value (not a key) of the desired table row.
 *
 * \return
 *      Returns true if the desired list was found and removed.
 */

bool
playlist::remove_song (int index)
{
    bool result = false;
    if (m_current_list != m_play_lists.end())
    {
        int count = 0;
        song_list & slist = m_current_list->second.ls_song_list;
        song_iterator sci;
        for (sci = slist.begin(); sci != slist.end(); /*++sci,*/ ++count)
        {
            if (count == index)
            {
                sci = slist.erase(sci);
                result = true;
                break;
            }
            else
                ++sci;
        }
        if (result)
            reorder_song_list(slist);
    }
    return result;
}

/**
 *  Moves through the song-list container in key (MIDI control number) order,
 *  modifying the index value of each song in the list.
 */

void
playlist::reorder_song_list (song_list & sl)
{
    int index = 0;
    for (song_iterator sci = sl.begin(); sci != sl.end(); ++sci, ++index)
    {
        song_spec_t & s = sci->second;
        s.ss_index = index;
    }
}

/**
 *
 */

void
playlist::show_list (const play_list_t & pl) const
{
    std::cout
        << "[playlist MIDI #" << pl.ls_midi_number
        << "] at slot " << pl.ls_index
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
        << "    Song MIDI #" << s.ss_midi_number << " at slot " << s.ss_index
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
            const_play_iterator pci = m_play_lists.begin();
            pci != m_play_lists.end(); ++pci
        )
        {
            const play_list_t & pl = pci->second;
            show_list(pl);

            const song_list & sl = pl.ls_song_list;
            const_song_iterator sci;
            for (sci = sl.begin(); sci != sl.end(); ++sci)
            {
                const song_spec_t & s = sci->second;
                show_song(s);
            }
        }
    }
}

/**
 *  A function for running tests of the play-list handling.
 */

void
playlist::test ()
{
    show();
    show_list(m_current_list->second);
    show_song(m_current_song->second);
    for (int i = 0; i < 8; ++i)
    {
        if (next_song())
        {
            std::cout << "Next song: ";
            show_song(m_current_song->second);
        }
        else
            break;
    }
    for (int i = 0; i < 8; ++i)
    {
        if (previous_song())
        {
            std::cout << "Prev song: ";
            show_song(m_current_song->second);
        }
        else
            break;
    }
    for (int i = 0; i < 8; ++i)
    {
        if (next_list())
        {
            std::cout << "Next list: ";
            show_list(m_current_list->second);
        }
        else
            break;
    }
    for (int i = 0; i < 8; ++i)
    {
        if (previous_list())
        {
            std::cout << "Prev list: ";
            show_list(m_current_list->second);
        }
        else
            break;
    }
    reset();
    write();
}

}           // namespace seq64

/*
 * playlist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

