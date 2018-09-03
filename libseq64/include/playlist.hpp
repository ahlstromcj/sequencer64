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
 * \updates       2018-09-03
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

    friend class perform;

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

    struct song_spec_t
    {
        int ss_index;
        std::string ss_song_directory;
        std::string ss_filename;
    };

    /**
     *  A type for holding a numerically ordered list of songs.
     */

    typedef std::map<int, song_spec_t> song_list;
    typedef std::map<int, song_spec_t>::const_iterator song_iterator;

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
    typedef std::map<int, play_list_t>::const_iterator play_iterator;

private:

    /**
     *  Provides an empty map to use to access the end() function.
     */

    static song_list sm_dummy;

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

    play_list m_play_lists;

    /**
     *  Indicates if we are in playlist mode.  Only true if the user specified
     *  a valid playlist file that was successfully loaded.
     */

    bool m_mode;

    /**
     *  Provides an iterator to the current playlist.  If valid, it provides
     *  access to the name of the playlist, its file-directory, and its list
     *  of songs.
     */

    play_iterator m_current_list;

    /**
     *  Provides an iterator to the current song.  It can only be valid if the
     *  current playlist is valid, otherwise it "points" to a static empty
     *  song structure, sm_dummy. If valid, it provides
     *  access to the file-name for the song and its file-directory.
     */

    song_iterator m_current_song;

private:

    /*
     * Only the friend class perform is able to call these functions.
     */

    playlist (perform & p, const std::string & name);

public:

    virtual ~playlist ();               // how to hide this???

    void show_list (const play_list_t & pl) const;
    void show_song (const song_spec_t & pl) const;
    void show () const;
    void test ();

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

    void mode (bool m)
    {
        m_mode = m;
    }

    int list_index () const
    {
        return m_current_list != m_play_lists.end() ?
            m_current_list->second.ls_index : (-1) ;
    }

    const std::string & list_name () const
    {
        static std::string s_dummy;
        return m_current_list != m_play_lists.end() ?
            m_current_list->second.ls_list_name : s_dummy ;
    }

    int list_count () const
    {
        return int(m_play_lists.size());
    }

    const std::string & song_directory () const
    {
        static std::string s_dummy;
        return m_current_list != m_play_lists.end() ?
            m_current_list->second.ls_file_directory : s_dummy ;
    }

    int song_index () const;

    /*
     *  Normally, play_list_t holds the name of the directory holding the
     *  songs for the currently active playlist.  All songs in a playlist must
     *  be in the same directory.  This is less flexible, but also a less
     *  confusing way to organize tunes.
     *
     *  However, if empty, every song in that playlist must specify the full
     *  or relative path to the file.  To represent this empty name in the
     *  playlist, two consecutive double quotes are used.
     */

    const std::string & song_filename () const;     // base-name only
    std::string song_filepath (const song_spec_t & s) const;
    std::string song_filepath () const;             // for current song

    int song_count () const
    {
        return m_current_list != m_play_lists.end() ?
            m_current_list->second.ls_song_count : (-1) ;
    }

    std::string current_song () const;

public:

    void clear ();
    bool reset ();
    bool open (bool verify_it = true);
    bool open (const std::string & filename, bool verify_it = true);
    bool select_list (int index, bool selectsong = false);
    bool next_list (bool selectsong = false);
    bool previous_list (bool selectsong = false);
    bool select_song (int index);
    bool next_song ();
    bool previous_song ();
    bool open_song (const std::string & filename, bool playlistmode = false);
    bool open_current_song ();
    bool open_next_list ();
    bool open_previous_list ();
    bool open_next_song ();
    bool open_previous_song ();

    virtual bool parse (perform & p);
    virtual bool write (const perform & p);

private:

    bool make_error_message (const std::string & additional);
    bool make_file_error_message
    (
        const std::string & fmt,
        const std::string & filename
    );
    bool verify (bool strong = true);

};          // class playlist

}           // namespace seq64

#endif      // SEQ64_PLAYLIST_HPP

/*
 * playlist.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

