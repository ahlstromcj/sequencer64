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
 * \updates       2018-09-26
 * \license       GNU GPLv2 or above
 *
 * \todo
 *      Add filepath to BAD playlist message.
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

private:

    /**
     *  Holds an entry describing a song, to be used as the "second" in a map.
     *  Also holds a copy of the key value.  Do we want the user to be
     *  able to specify a title for the tune?
     */

    struct song_spec_t
    {
        /**
         *  Provides an ordinal value that indicates the offset of the song in
         *  the list.
         */

        int ss_index;

        /**
         *  Provides a copy of the key, which is the MIDI control number that
         *  the user has applied to this song in the playlist.
         */

        int ss_midi_number;

        /**
         *  The directory where the song is located.  This either the default
         *  directory specified in the playlist, or the path specification that
         *  existed in the file-name of the song.
         */

        std::string ss_song_directory;

        /**
         *  The base file-name, of the form "base.ext".  When appended to
         *  ss_song_directory, this yields the full path to the file.
         */

        std::string ss_filename;
    };

    /**
     *  A type for holding a numerically ordered list of songs.
     */

    typedef std::map<int, song_spec_t> song_list;
    typedef std::map<int, song_spec_t>::iterator song_iterator;
    typedef std::map<int, song_spec_t>::const_iterator const_song_iterator;

    /**
     *  Holds a playlist list entry to be used as the "second" in a map.
     *  Also holds a copy of the key value.
     */

    struct play_list_t
    {
        /**
         *  Provides an ordinal value that indicates the offset of the playlist
         *  in the play-list file.
         */

        int ls_index;

        /**
         *  Provides a copy of the key, which is the MIDI control number that the
         *  user has applied to this playlist in the play-list file.
         */

        int ls_midi_number;

        /**
         *  Provides the human name for the playlist, it's meaningful title.
         */

        std::string ls_list_name;

        /**
         *  The default directory where each song in the playlist is located.
         *  If there is a path specification that exists in the file-name of a
         *  given song, that overrides this directory name.
         */

        std::string ls_file_directory;

        /**
         *  A quick way to get the number of songs in this playlist.
         */

        int ls_song_count;

        /**
         *  A container holding the list of information for the songs in the
         *  playlist.
         */

        song_list ls_song_list;
    };

    /**
     *  A type for holding a numerically ordered list of playlists.
     */

    typedef std::map<int, play_list_t> play_list;
    typedef std::map<int, play_list_t>::iterator play_iterator;
    typedef std::map<int, play_list_t>::const_iterator const_play_iterator;

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
     *  The full path to the file from which this object was filled.
     *  This item is not needed.  Instead, use the configfile::name()
     *  function.
     *
     *      std::string m_list_filespec;
     */

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

    /**
     *  If true, write the lists/songs to standard output.  This is
     *  useful to test the CLI/daemon version of Sequencer64.
     */

    bool m_show_on_stdout;

private:

    /*
     * Only the friend class perform is able to call these functions.
     */

    playlist
    (
        perform & p,
        const std::string & name,
        bool show_on_stdout = false
    );

public:

    virtual ~playlist ();               // how to hide this???

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

    int list_midi_number () const
    {
        return m_current_list != m_play_lists.end() ?
            m_current_list->second.ls_midi_number : (-1) ;
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

    int song_midi_number () const;
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
    bool add_list
    (
        int index, int midinumber,
        const std::string & name,
        const std::string & directory
    );
    bool remove_list (int index);
    bool select_list (int index, bool selectsong = false);
    bool next_list (bool selectsong = false);
    bool previous_list (bool selectsong = false);
    bool add_song
    (
        int index, int midinumber,
        const std::string & name,
        const std::string & directory
    );
    bool remove_song (int index);
    bool select_song (int index);
    bool next_song ();
    bool previous_song ();
    bool open_song (const std::string & filename, bool verifymode = false);
    bool open_select_song (int index, bool opensong = true);
    bool open_current_song ();
    bool open_next_list (bool opensong = true);
    bool open_previous_list (bool opensong = true);
    bool open_next_song (bool opensong = true);
    bool open_previous_song (bool opensong = true);

private:

    virtual bool parse (perform & p);
    virtual bool write (const perform & p);

    /*
     * We want to hide the internal structures from the caller.
     */

    bool add_list (play_list_t & plist);
    void show_list (const play_list_t & pl) const;

    std::string song_filepath (const song_spec_t & s) const;
    bool add_song (song_spec_t & sspec);                    // add to current list
    bool add_song (song_list & slist, song_spec_t & sspec);
    bool add_song (play_list_t & plist, song_spec_t & sspec);
    void show_song (const song_spec_t & pl) const;

    void reorder_play_list ();                              // current list
    void reorder_song_list (song_list & sl);                // current song list
    bool scan_song_file (int & song_number, std::string & song_file);
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

