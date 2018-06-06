#ifndef SEQ64_WRKFILE_HPP
#define SEQ64_WRKFILE_HPP

/*
 *  WRK File component
 *  Copyright (C) 2010-2018, Pedro Lopez-Cabanillas <plcl@users.sf.net>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file          wrkfile.hpp
 *
 *  This module declares/defines the class for reading WRK files.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-06-04
 * \updates       2018-06-04
 * \license       GNU GPLv2 or above
 *
 *  For a quick guide to the WRK format, see, for example:
 *
 *      WRK Cakewalk WRK File Parser (Input).
 */

// #include "macros.h"
#include <string>
// #include <QObject>

// class QDataStream;

#include "midifile.hpp"                /* seq64::midifile base class */

/**
 * @{
 */

namespace seq64
{

/**
 *    Record types within a WRK file.
 */

enum wrk_chunk_t
{
    WC_TRACK_CHUNK = 1,     ///< Track prefix
    WC_STREAM_CHUNK = 2,    ///< Events stream
    WC_VARS_CHUNK = 3,      ///< Global variables
    WC_TEMPO_CHUNK = 4,     ///< Tempo map
    WC_METER_CHUNK = 5,     ///< Meter map
    WC_SYSEX_CHUNK = 6,     ///< System exclusive bank
    WC_MEMRGN_CHUNK = 7,    ///< Memory region
    WC_COMMENTS_CHUNK = 8,  ///< Comments
    WC_TRKOFFS_CHUNK = 9,   ///< Track offset
    WC_TIMEBASE_CHUNK = 10, ///< Timebase. If present is the first chunk in the file.
    WC_TIMEFMT_CHUNK = 11,  ///< SMPTE time format
    WC_TRKREPS_CHUNK = 12,  ///< Track repetitions
    WC_TRKPATCH_CHUNK = 14, ///< Track patch
    WC_NTEMPO_CHUNK = 15,   ///< New Tempo map
    WC_THRU_CHUNK = 16,     ///< Extended thru parameters
    WC_LYRICS_CHUNK = 18,   ///< Events stream with lyrics
    WC_TRKVOL_CHUNK = 19,   ///< Track volume
    WC_SYSEX2_CHUNK = 20,   ///< System exclusive bank
    WC_STRTAB_CHUNK = 22,   ///< Table of text event types
    WC_METERKEY_CHUNK = 23, ///< Meter/Key map
    WC_TRKNAME_CHUNK = 24,  ///< Track name
    WC_VARIABLE_CHUNK = 26, ///< Variable record chunk
    WC_NTRKOFS_CHUNK = 27,  ///< Track offset
    WC_TRKBANK_CHUNK = 30,  ///< Track bank
    WC_NTRACK_CHUNK = 36,   ///< Track prefix
    WC_NSYSEX_CHUNK = 44,   ///< System exclusive bank
    WC_NSTREAM_CHUNK = 45,  ///< Events stream
    WC_SGMNT_CHUNK = 49,    ///< Segment prefix
    WC_SOFTVER_CHUNK = 74,  ///< Software version which saved the file
    WC_END_CHUNK = 255      ///< Last chunk, end of file
};

// const midibyte HEADER ("CAKEWALK"); ///< Cakewalk WRK File header id

/**
 *  Cakewalk WRK File header id.
 */

const std::string CakewalkHeader("CAKEWALK");

/**
 * Cakewalk WRK file format (input only)
 *
 * This class is used to parse Cakewalk WRK Files
 * @since 0.3.0
 */

class wrkfile : public midifile
{

public:

    wrkfile
    (
        const std::string & name,
        int ppqn = SEQ64_USE_DEFAULT_PPQN
    );
    virtual ~wrkfile ();

    virtual bool parse (perform & p, int ascreenset = 0, bool importing = false);

    void readFromFile(const std::string& fileName);
    double get_real_time (midipulse ticks) const;

private:

    // midibyte readByte();
    midishort to_16_bit(midibyte c1, midibyte c2);
    midilong to_32_bit(midibyte c1, midibyte c2, midibyte c3, midibyte c4);
    // midishort read16bit();
    midilong read_24_bit();
    // midilong read32bit();
    std::string read_string (int len);
    std::string read_var_string ();
    void read_raw_data (int size);
    void read_gap (int size);
    bool at_end ();
    void seek (long pos);

    int read_check ();
    void Track_chunk();
    void Vars_chunk();
    void Timebase_chunk();
    void processNoteArray(int track, int events);
    void Stream_chunk();
    void Meter_chunk();
    void Tempo_chunk(int factor = 1);
    void Sysex_chunk();
    void Sysex2_chunk();
    void NewSysex_chunk();
    void Thru_chunk();
    void TrackOffset();
    void TrackReps();
    void TrackPatch();
    void TrackBank();
    void TimeFormat();
    void Comments();
    void processVariableRecord(int max);
    void NewTrack();
    void SoftVer();
    void TrackName();
    void StringTable();
    void LyricsStream();
    void TrackVol();
    void NewTrackOffset();
    void MeterKey_chunk();
    void Segment_chunk();
    void NewStream();
    void Unknown(int id);
    void End_chunk();

    struct RecTempo
    {
        long time;
        double tempo;
        double seconds;
    };

    class wrkfile_private;
    wrkfile_private * m_d;

};          // class wrkfile

}           // namespace seq64

#endif      // SEQ64_WRKFILE_HPP

/*
 * wrkfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

