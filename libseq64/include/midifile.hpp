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
 * \updates       2021-05-13
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
#include <vector>

#include "globals.h"                    /* SEQ64_USE_DEFAULT_PPQN           */
#include "midibyte.hpp"                 /* midishort, midibyte, etc.        */
#include "midi_splitter.hpp"            /* seq64::midi_splitter             */
#include "mutex.hpp"                    /* seq64::mutex, automutex          */

/**
 *  A feature to use the song/performance triggers to write out the MIDI
 *  data as laid out in the perfedit window.  Now permanent.
 *
 *      #define SEQ64_STAZED_EXPORT_SONG
 */

/**
 *  A manifest constant for controlling the length of a line-reading
 *  array in a configuration file.
 */

#define SEQ64_MIDI_LINE_MAX         1024

/**
 *  The maximum length of a Seq24 track name.  This is a bit excessive.
 */

#define SEQ64_TRACKNAME_MAX          256

/**
 *  The maximum allowed variable length value for a MIDI file, which allows
 *  the length to fit in a 32-bit integer.
 */

#define SEQ64_VARLENGTH_MAX         0x0FFFFFFF

/**
 *  Highlights the MIDI file header value, "MThd".
 */

#define SEQ64_MTHD_TAG              0x4D546864      /* magic number 'MThd'  */

/**
 *  Highlights the MIDI file track-marker (chunk) value, "MTrk".
 */

#define SEQ64_MTRK_TAG              0x4D54726B      /* magic number 'MTrk'  */

/**
 *  The chunk header value for the Sequencer64 proprietary/SeqSpec section.
 *  We might try other chunks, as well, since, as per the MIDI
 *  specification, unknown chunks should not cause an error in a sequencer
 *  (or our midicvt program).  For now, we stick with "MTrk".
 */

#define PROP_CHUNK_TAG              SEQ64_MTRK_TAG

/**
 *  Provides the sequence number for the proprietary/SeqSpec data when using
 *  the new format.  (There is no sequence number for the legacy format.)
 *  Can't use numbers, such as 0xFFFF, that have MIDI meta tags in them,
 *  confuses our "proprietary" track parser.
 */

#define PROP_SEQ_NUMBER             0x3FFF
#define PROP_SEQ_NUMBER_OLD         0x7777

/**
 *  Provides the track name for the "proprietary" data when using the new
 *  format.  (There is no track-name for the "proprietary" footer track when
 *  the legacy format is in force.)  This is more useful for examining a hex
 *  dump of a Sequencer64 song than for checking its validity.  It's overkill
 *  that causes needless error messages.
 */

#define PROP_TRACK_NAME             "Sequencer64-S"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    /*
     * Forward references.
     */

    class midi_splitter;
    class perform;
    class midi_vector;

/**
 *  This class handles the parsing and writing of MIDI files.  In addition to
 *  the standard MIDI tracks, it also handles some "private" or "proprietary"
 *  tracks specific to Seq24.  It does not, however, handle SYSEX events.
 */

class midifile
{

public:

    /**
     *  Instead of having two save options, we now have three.
     */

    typedef enum
    {
        FILE_SAVE_AS_NORMAL,
        FILE_SAVE_AS_EXPORT_SONG,
        FILE_SAVE_AS_EXPORT_MIDI

    } SaveOption;

private:

    /**
     *  Provides locking for the sequence.  Made mutable for use in
     *  certain locked getter functions.
     */

    mutable mutex m_mutex;

    /**
     *  Indicates if we are reading this file simply to verify it.  If so,
     *  then the song data will be removed after checking, via a call to
     *  perform::clear_all().
     */

    bool m_verify_mode;

    /**
     *  Holds the size of the MIDI file.  This variable was added when loading
     *  a file that caused an attempt to load data well beyond the file-size
     *  of the midicvt test file Dixie04.mid.
     */

    size_t m_file_size;

    /**
     *  Holds the last error message, useful for trouble-shooting without
     *  having Sequencer64 running in a console window.  If empty, there's no
     *  pending error.  Currently most useful in the parse() function.
     */

    std::string m_error_message;

    /**
     *  Indicates if the error should be considered fatal.  The caller can
     *  query for this value after getting the return value from parse().
     */

    bool m_error_is_fatal;

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

    size_t m_pos;

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
     *  Indicates that we are rescaling the PPQN of a file as it is read in.
     */

    bool m_use_scaled_ppqn;

    /**
     *  Provides the current value of the PPQN, which used to be constant
     *  and is now only the macro SEQ64_DEFAULT_PPQN.
     */

    int m_ppqn;

    /**
     *  The value of the PPQN from the file itself.
     */

    int m_file_ppqn;

    /**
     *  Provides support for SMF 0. This object holds all of the information
     *  needed to split a multi-channel sequence.
     */

    midi_splitter m_smf0_splitter;

public:

    midifile
    (
        const std::string & name,
        int ppqn            = SEQ64_USE_DEFAULT_PPQN,
        bool oldformat      = false,
        bool globalbgs      = true,
        bool playlistmode   = false
    );
    virtual ~midifile ();

    virtual bool parse (perform & p, int screenset = 0, bool importing = false);
    virtual bool write (perform & p, bool doseqspec = true);

    bool write_song (perform & p);

    /**
     * \getter m_error_message
     */

    const std::string & error_message () const
    {
        return m_error_message;
    }

    /**
     * \getter m_error_is_fatal
     */

    bool error_is_fatal () const
    {
        return m_error_is_fatal;
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

    /**
     * \getter m_file_ppqn
     */

    int file_ppqn () const
    {
        return m_file_ppqn;
    }

    bool scaled () const
    {
        return m_use_scaled_ppqn;
    }

    /**
     * \getter m_pos
     *
     *  Current position in the data stream.
     */

    size_t get_file_pos ()
    {
        return m_pos;           // return m_d->m_IOStream->device()->pos();
    }

protected:

    virtual sequence * initialize_sequence (perform & p);
    virtual void finalize_sequence
    (
        perform & p, sequence & seq, int seqnum, int screenset
    );

    /**
     * \getter m_verify_mode;
     */

    bool verify_mode () const
    {
        return m_verify_mode;
    }

    /**
     * \setter m_error_message
     */

    void clear_errors ()
    {
        m_error_message.clear();
        m_disable_reported = false;
    }

    /**
     * \setter m_ppqn
     */

    void ppqn (int p)
    {
        m_ppqn = p;
    }

    /**
     * \setter m_file_ppqn
     */

    void file_ppqn (int p)
    {
        m_file_ppqn = p;
    }

    /**
     *  Checks if the data stream pointer has reached the end position
     *
     * \return
     *      Returns true if the read pointer is at the end.
     */

    bool at_end () const
    {
        return m_pos >= m_file_size;
    }

    bool grab_input_stream (const std::string & tag);
    bool parse_smf_0 (perform & p, int screenset);
    bool parse_smf_1 (perform & p, int screenset, bool is_smf0 = false);
    midilong parse_prop_header (int file_size);
    bool parse_proprietary_track (perform & a_perf, int file_size);
    bool checklen (midilong len, midibyte type);
    void add_trigger (sequence & seq, midishort ppqn, bool tposable);
    void add_old_trigger (sequence & seq);
    bool read_seek (size_t pos);
    midilong read_long ();
    midilong read_split_long (unsigned & highbytes, unsigned & lowbytes);
    midishort read_short ();
    midibyte read_byte ();
    midilong read_varinum ();
    bool read_byte_array (midibyte * b, size_t len);
    bool read_byte_array (midistring & b, size_t len);
    void read_gap (size_t sz);

    void write_long (midilong value);
    void write_split_long (unsigned highbytes, unsigned lowbytes);
    void write_triple (midilong value);
    void write_short (midishort value);

    /**
     *  Writes 1 byte.  The byte is written to the m_char_list member, using a
     *  call to push_back().
     *
     * \param c
     *      The MIDI byte to be "written".
     */

    void write_byte (midibyte c)
    {
        m_char_list.push_back(c);
    }

    void write_varinum (midilong);
    void write_track_name (const std::string & trackname);
    std::string read_track_name();
    void write_seq_number (midishort seqnum);
    int read_seq_number ();
    void write_track_end ();
    bool write_header (int numtracks);
#ifdef USE_WRITE_START_TEMPO
    void write_start_tempo (midibpm start_tempo);
#endif
#ifdef USE_WRITE_TIME_SIG
    void write_time_sig (int beatsperbar, int beatwidth);
#endif
    void write_prop_header (midilong tag, long len);
    bool write_proprietary_track (perform & a_perf);
    long varinum_size (long len) const;
    long prop_item_size (long datalen) const;
    long track_name_size (const std::string & trackname) const;
    bool set_error (const std::string & msg);
    bool set_error_dump (const std::string & msg);
    bool set_error_dump (const std::string & msg, unsigned long p);
    void write_track (const midi_vector & lst);

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

/*
 *  Free functions related to midifile.
 */

extern bool open_midi_file
(
    perform & p,
    const std::string & fn,
    int & ppqn,
    std::string & errmsg
);
extern bool save_midi_file
(
    perform & p,
    const std::string & fn,
    std::string & errmsg
);

}           // namespace seq64

#endif      // SEQ64_MIDIFILE_HPP

/*
 * midifile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

