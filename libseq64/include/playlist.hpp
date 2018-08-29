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
 * \updates       2018-08-29
 * \license       GNU GPLv2 or above
 *
 */

#include <map>

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

    enum action_t
    {
        LIST_ZERO,
        LIST_NEXT,
        LIST_PREVIOUS,
        SONG_ZERO,
        SONG_NEXT,
        SONG_PREVIOUS
    };

private:

    /**
     *  Holds an entry describing a song, to be used as the "second" in a map.
     *  Also holds a copy of the key (index) value.  Do we want the user to be
     *  able to specify a title for the tune?
     */

    struct song_list_t
    {
        int ss_index;
        std::string ss_filename;
    };

    /**
     *  A type for holding a numerically ordered list of songs.
     */

    typedef std::map<int, song_list_t> song_list;

    /**
     *  Holds a playlist list entry to be used as the "second" in a map.
     *  Also holds a copy of the key (index) value.
     */

    struct play_list_t
    {
        int ls_index;
        std::string ls_list_name;
        std::string ls_file_directory;
        int ls_song_count;
        song_list ls_song_list;
    };

    /**
     *  A type for holding a numerically ordered list of playlists.
     */

    typedef std::map<int, play_list_t> play_list;

private:

    /**
     *  Holds a reference to the performer for this playlist.
     */

    perform & m_perform;

    /**
     *  Holds the [comments] section of the file. It is a list of concatenated
     *  lines.
     */

    std::string m_comments;

    /**
     *  The list of playlists.
     */

    play_list m_play_list;

    /**
     *  Indicates if we are in playlist mode.  Only true if the user specified
     *  a valid playlist file that was successfully loaded.
     */

    bool m_mode;

    /**
     *  Provides the directory location of the playlist file.  This value
     *  defaults to the modern "sequencer64" configuration directory in the
     *  user's HOME or AppData directory.  This is stored in canonical UNIX
     *  format (forward-slash directory separators, with a separator at the
     *  end), but might be stored here in the conventions of the operation
     *  system. We shall see.  Form: "C:/path/to/directory/" or
     *  "/home/user/path/to/directory/".  Made for easy concatenation.
     */

    std::string m_list_directory;

    /**
     *  Holds the basename (e.g. "sequencer64.playlist") of the file holding
     *  all of the desired playlists.  This is a bare file-name of the form
     *  "base.extension".  Made for easy concatenation.
     */

    std::string m_list_filename;

    /**
     *  Holds the index/number of the currently-active playlist, re 0.  The
     *  numbers need not be consecutive, as the main playlist items, each
     *  tagged by "[playlist]", are held in a map.  However, the numbers are
     *  used to order the playlist when writing it to a file.
     */

    int m_current_list_index;

    /**
     *  Holds the name of the currently-active playlist, which is not the
     *  filename, but a descriptive name for the playlist.  This string is
     *  trimmed of white-space or quotation characters at either end.
     */

    std::string m_current_list_name;

    /**
     *  Holds the name of the directory holding the songs for the currently
     *  active playlist.  All songs in a playlist must be in the same
     *  directory.  This is less flexible, but also a less confusing way
     *  to organize tunes.
     *
     *  However, if empty, every song in the playlist must specify the full or
     *  relative path to the file.  To represent this empty name in the
     *  playlist, two consecutive double quotes are used.
     */

    std::string m_song_directory;

    /**
     *  Hold the index of the currently-active song, re 0.
     */

    int m_current_song_index;

    /**
     *  Provides the base-name and the extension.
     */

    std::string m_current_song_filename;

    /**
     *  Holds the number of songs in the active plalist.
     */

    int m_current_song_count;

public:

    playlist (perform & p, const std::string & name);
    virtual ~playlist ();

    void show ();

public:

    bool parse ()
    {
        return parse(m_perform);
    }

    bool write ()
    {
        return write(m_perform);
    }

    bool mode () const
    {
        return m_mode;
    }

    const std::string & list_directory () const
    {
        return m_list_directory;
    }

    const std::string & list_filename () const
    {
        return m_list_filename;
    }

    int list_index () const
    {
        return m_current_list_index;
    }

    const std::string & list_name () const
    {
        return m_current_list_name;
    }

    const std::string & song_directory () const
    {
        return m_song_directory;
    }

    int song_index () const
    {
        return m_current_song_index;
    }

    const std::string & song_filename () const
    {
        return m_current_song_filename;
    }

    int song_count () const
    {
        return m_current_song_count;
    }

public:

    void clear ();
    bool open_playlist (const std::string & filename);
    bool verify_playlist ();
    bool select_list (int index);
    bool select_list (const std::string & name);
    bool next_list ();
    bool previous_list ();
    bool select_song (int index);
    bool select_song (const std::string & filename);
    bool next_song ();
    bool previous_song ();

private:        /* for internal use only, in this class */

    virtual bool parse (perform & p);
    virtual bool write (const perform & p);

protected:

    void mode (bool m)
    {
        m_mode = m;
    }

    void list_directory (const std::string & d)
    {
        m_list_directory = d;
    }

    void list_filename (const std::string & f)
    {
        m_list_filename = f;
    }

    void list_index (int i)
    {
        m_current_list_index = i;
    }

    void list_name (const std::string & n)
    {
        m_current_list_name = n;
    }

    void song_directory (const std::string & d)
    {
        m_song_directory = d;
    }

    void song_index (int i)
    {
        m_current_song_index = i;
    }

    void  song_filename (const std::string & f)
    {
        m_current_song_filename = f;
    }

    void song_count (int c)
    {
        m_current_song_count = c;
    }

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

