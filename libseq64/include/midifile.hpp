#ifndef SEQ64_MIDIFILE_HPP
#define SEQ64_MIDIFILE_HPP

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
 * \file          midifile.hpp
 *
 *  This module declares/defines the base class for MIDI files.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-11-24
 * \license       GNU GPLv2 or above
 *
 *  The Seq24 MIDI file is a standard, Format 1 MIDI file, with some extra
 *  "proprietary" tracks that hold information needed to set up the song
 *  in Seq24.
 *
 *  Sequencer64 can write out the Seq24 file with the "proprietary" tracks
 *  written in a format more palatable for strict MIDI programs, such as
 *  midicvt (a MIDI-to-ASCII conversion program available at the
 *  https://github.com/ahlstromcj/midicvt.git repository.
 *
 *  Sequencer64 can also split an SMF 0 file into multiple tracks, effectively
 *  converting it to SMF 1.
 */

#include <string>
#include <list>
// #include <map>
#include <vector>

#include "globals.h"                    /* SEQ64_USE_DEFAULT_PPQN   */
#include "midi_splitter.hpp"            /* seq64::midi_splitter     */

namespace seq64
{

class perform;                          /* forward reference        */
// class sequence;                         /* forward reference        */

/**
 *  This class handles the parsing and writing of MIDI files.  In addition to
 *  the standard MIDI tracks, it also handles some "private" or "proprietary"
 *  tracks specific to Seq24.  It does not, however, handle SYSEX events.
 */

class midifile
{

private:

    /**
     *  Holds the size of the MIDI file.  This variable was added when loading
     *  a file that caused an attempt to load data well beyond the file-size
     *  of the midicvt test file Dixie04.mid.
     */

    int m_file_size;

    /**
     *  Holds the last error message, useful for trouble-shooting without
     *  having Sequencer64 running in a console window.  If empty, there's no
     *  pending error.  Currently most useful in the parse() function.
     */

    std::string m_error_message;

    /**
     *  Indicates that file reading has already been disabled (due to serious
     *  errors), so don't complain about it anymore.  Once is enough.
     */

    bool m_disable_reported;

    /**
     *  Holds the position in the MIDI file.  This is at least a 31-bit
     *  value in the recent architectures running Linux and Windows, so it
     *  will handle up to 2 Gb of data.  This member is used as the offset
     *  into the m_data vector.
     */

    int m_pos;

    /**
     *  The unchanging name of the MIDI file.
     */

    const std::string m_name;

    /**
     *  This vector of characters holds our MIDI data.  We could also use
     *  a string of characters, unsigned.  This member is resized to the
     *  putative size of the MIDI file, in the parse() function.  Then the
     *  whole file is read into it, as if it were an array.  This member is an
     *  input buffer.
     */

    std::vector<midibyte> m_data;

    /**
     *  Provides a list of characters.  The class pushes each MIDI byte into
     *  this list using the write_byte() function.  Also note that the write()
     *  function calls sequence::fill_list() to fill a temporary
     *  std::list<char> (!) buffer, then writes that data <i> backwards </i> to
     *  this member.  This member is an output buffer.
     */

    std::list<midibyte> m_char_list;

    /**
     *  Use the new format for the proprietary footer section of the Seq24
     *  MIDI file.
     *
     *  In the new format, each sequencer-specfic value (0x242400xx, as
     *  defined in the globals module) is preceded by the sequencer-specific
     *  prefix, 0xFF 0x7F len id/date). By default, the new format is used,
     *  but the user can specify the --legacy (-l) option, or make a soft link
     *  to the sequence24 binary called "seq24",  to write the data in the old
     *  format. [We will eventually add the --legacy option to the "rc"
     *  configuration file.]  Note that reading can handle either format
     *  transparently.
     */

    bool m_new_format;

    /**
     *  Indicates to store the new key, scale, and background
     *  sequence in the global, "proprietary" section of the MIDI song.
     */

    bool m_global_bgsequence;

    /**
     *  Provides the current value of the PPQN, which used to be constant
     *  and is now only the macro DEFAULT_PPQN.
     */

    int m_ppqn;

    /**
     *  Indicates that the default PPQN is in force.
     */

    bool m_use_default_ppqn;

    /**
     *  Provides support for SMF 0. This object holds all of the information
     *  needed to split a multi-channel sequence.
     */

    midi_splitter m_smf0_splitter;

public:

    midifile
    (
        const std::string & name,
        int ppqn = SEQ64_USE_DEFAULT_PPQN,
        bool oldformat = false,
        bool globalbgs = true
    );
    ~midifile ();

    bool parse (perform & a_perf, int a_screen_set = 0);
    bool write (perform & a_perf);

    /**
     * \getter m_error_message
     */

    const std::string & error_message () const
    {
        return m_error_message;
    }

    /**
     * \getter m_ppqn
     *      Provides a way to get the actual value of PPQN used in processing
     *      the sequences when parse() was called.  The PPQN will be either
     *      the global ppqn (legacy behavior) or the value read from the
     *      file, depending on the ppqn parameter passed to the midifile
     *      constructor.
     */

    int ppqn () const
    {
        return m_ppqn;
    }

private:

    bool parse_smf_0 (perform & p, int screenset);
    bool parse_smf_1 (perform & p, int screenset, bool is_smf0 = false);
    unsigned long parse_prop_header (int file_size);
    bool parse_proprietary_track (perform & a_perf, int file_size);
    unsigned long read_long ();
    unsigned short read_short ();
    midibyte read_byte ();
    unsigned long read_varinum ();
    void write_long (unsigned long);
    void write_short (unsigned short);

    /**
     *  A helper function to simplify reading midi_control data from the MIDI
     *  file.
     *
     * \param b
     *      The byte array to receive the data.
     *
     * \param len
     *      The number of bytes in the array, and to be read.
     */

    void read_byte_array (midibyte * b, int len)
    {
        for (int i = 0; i < len; ++i)
            *b++ = read_byte();
    }

    /**
     *  Writes 1 byte.  The byte is written to the m_char_list member, using a
     *  call to push_back().
     */

    void write_byte (midibyte c)
    {
        m_char_list.push_back(c);
    }

    void write_varinum (unsigned long);
    void write_track_name (const std::string & trackname);
    std::string read_track_name();
    void write_seq_number (unsigned short seqnum);
    int read_seq_number ();
    void write_track_end ();
    void write_prop_header (unsigned long tag, long len);
    bool write_proprietary_track (perform & a_perf);
    long varinum_size (long len) const;
    long prop_item_size (long datalen) const;
    long track_name_size (const std::string & trackname) const;
    void errdump (const std::string & msg);
    void errdump (const std::string & msg, unsigned long p);

    /**
     *  Returns the size of a sequence-number event, which is always 5
     *  bytes, plus one byte for the delta time that precedes it.
     */

    long seq_number_size () const
    {
        return 6;
    }

    /**
     *  Returns the size of a track-end event, which is always 3 bytes.
     */

    long track_end_size () const
    {
        return 3;
    }

    /**
     *  Check for special SysEx ID byte.
     *
     * \param ch
     *      Provides the byte to be checked against 0x7D through 0x7F.
     *
     * \return
     *      Returns true if the byte is SysEx special ID.
     */

    bool is_sysex_special_id (midibyte ch)
    {
        return ch >= 0x7D && ch <= 0x7F;
    }

};          // class midifile

}           // namespace seq64

#endif      // SEQ64_MIDIFILE_HPP

/*
 * midifile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

