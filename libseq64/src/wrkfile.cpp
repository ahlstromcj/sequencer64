/*
 *  WRK File component
 *  Copyright (C) 2010-2018, Pedro Lopez-Cabanillas <plcl\users.sf.net>
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

 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file          wrkfile.cpp
 *
 *  This module declares/defines the class for reading WRK files.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-06-04
 * \updates       2018-06-07
 * \license       GNU GPLv2 or above
 *
 *  For a quick guide to the WRK format, see, for example:
 *
 *    Implementation of a class managing Cakewalk WRK Files input.
 *
 * Format:
 *
 *  Using drumstick-dumpwrk, the overall structure of an (old, very old) WRK
 *  file is:
 *
 *  -#  First track with ticks, track number, channel, events, and data.
 *      In our sample, ticks = 0, and track and channel are empty here.
 *      -#  Wrk file version.
 *      -#  PPQN.
 *      -#  Global variables, if any.
 *      -#  Variable record with string "FILESTATS"
 *      -#  Comment = "@"
 *  -#  Song name, such as '"Memories of Professor Longhair" by Mac Rebenak...'
 *  -#  Some more data:
\verbatim
          0    -- -- Unknown Chunk 25 (0x19)   size=66
          0    -- -- String Table              MCICmd, Wave, Text, Lyric
          0    -- -- Unknown Chunk 28 (0x1c)   size=6
          0    -- -- Tempo                     115.00
          0    -- -- Time Signature            bar=1, 4/4
          0    -- -- Time Signature            bar=1, 4/4
          0    -- -- Key Signature             bar=1, alt=0
          0    -- -- SMPTE Time Format         30 frames/second, offset=0
          0    -- -- Unknown Chunk 17 (0x11)   size=14
          0    -- -- Thru Mode   mode=-1 port=0 chan=9 key+=0 vel+=10 port=-1
          0    -- -- Unknown Chunk 31 (0x1f)   size=4
\endverbatim
 *	-# Track data:
\verbatim
		  0     0  7 Track       name1='Right hand  (99' name2=' | 00 Stereo Pi'
		  0     0 -- Track Name                Right hand  (99 | 00 Stereo Piano
		 60     0  0 Note                      key=55 vel=64 dur=15
		 75     0  0 Note                      key=56 vel=64 dur=15
          . . .
	  47040     0  0 Note                      key=63 vel=64 dur=480
	  47040     0  0 Note                      key=55 vel=64 dur=480
		  0     0 -- Track Patch               0
		  0     0 -- Track Volume              127
		  0    -- -- Unknown Chunk 29 (0x1d)   size=6
\endverbatim
 *
 *  And the rest of the tracks follow the same pattern.  A couple empty tracks
 *  (Track, Track Name, and Unknown Chunk only) end the tune.  There seems to
 *  be no way of knowing the number of tracks before parsing them all.
 */

#include <cmath>

#include "perform.hpp"                  /* must precede wrkfile.hpp !       */
#include "sequence.hpp"                 /* seq64::sequence                  */
#include "settings.hpp"                 /* seq64::rc().show_midi()          */
#include "wrkfile.hpp"                  /* seq64::wrkfile                   */

namespace seq64
{

/**
 * wrkfile provides a mechanism to parse Cakewalk WRK Files, without
 * the burden of a policy forcing to use some internal sequence representation.
 *
 * This class is not related to or based on the ALSA library.
 */

wrkfile::wrkfile_private::wrkfile_private ()
 :
    m_Now           (0),
    m_From          (0),
    m_Thru          (11930),
    m_KeySig        (0),
    m_Clock         (0),
    m_AutoSave      (0),
    m_PlayDelay     (0),
    m_ZeroCtrls     (false),
    m_SendSPP       (true),
    m_SendCont      (true),
    m_PatchSearch   (false),
    m_AutoStop      (false),
    m_StopTime      (4294967295U),
    m_AutoRewind    (false),
    m_RewindTime    (0),
    m_MetroPlay     (false),
    m_MetroRecord   (true),
    m_MetroAccent   (false),
    m_CountIn       (1),
    m_ThruOn        (true),
    m_AutoRestart   (false),
    m_CurTempoOfs   (1),
    m_TempoOfs1     (32),
    m_TempoOfs2     (64),
    m_TempoOfs3     (128),
    m_PunchEnabled  (false),
    m_PunchInTime   (0),
    m_PunchOutTime  (0),
    m_EndAllTime    (0),
    m_division      (120),
//  m_codec         (0),
    m_tempos        ()
{
   // no code
}

/**
 *  Principal constructor.
 *
 * \param name
 *      Provides the name of the WRK file to be read or written.
 *
 * \param ppqn
 *      Provides the initial value of the PPQN setting.  It is handled
 *      differently for parsing (reading) versus writing the MIDI file.
 *      See the midifile class.
 */

wrkfile::wrkfile
(
    const std::string & name,
    int ppqn
) :
    midifile        (name, ppqn),
    m_data          (),
    m_seq_number    (0)
{
    //
}

/**
 *  \dtor
 *
 *  Nothing to delete, m_data is now a member, not a pointer.  Some might
 *  argue that it is better, as Pedro did in his "Drumstick" project,
 *  to hide the data implement in the C++ file, rather than risk rebuilds by
 *  putting it in the header file.
 */

wrkfile::~wrkfile()
{
    // no code
}

/**
 * Read the chunk raw data (undecoded)
 */

void
wrkfile::read_raw_data (int sz)
{
    // m_data.m_lastChunkData = m_data.m_IOStream->device()->read(sz);

    read_byte_array(m_data.m_lastChunkData, sz);
}

/**
 *  Converts two bytes into a single 16-bit value.
 *
 * \param c1
 *      First byte, the most significant byte.
 *
 * \param c2
 *      Second byte.
 *
 * \return
 *      The 16-bit value is returned.
 */

midishort
wrkfile::to_16_bit (midibyte c1, midibyte c2)
{
    midishort value = c1 << 8;
    value += c2;
    return value;
}

/**
 *  Converts four bytes into a single 32-bit value.
 *
 * \param c1
 *      1st byte, the most significant byte.
 *
 * \param c2
 *      2nd byte.
 *
 * \param c3
 *      3rd byte.
 *
 * \param c4
 *      4th byte.
 *
 * \return
 *      The 32-bit value is returned.
 */

midilong
wrkfile::to_32_bit (midibyte c1, midibyte c2, midibyte c3, midibyte c4)
{
    midilong value = c1 << 24;
    value += c2 << 16;
    value += c3 << 8;
    value += c4;
    return value;
}

/**
 *  Reads a 24-bit value.
 *
 * \return
 *      The 32-bit value, calculated from three bytes and a zero, is returned.
 */

midilong
wrkfile::read_24_bit ()
{
    midibyte c1 = read_byte();
    midibyte c2 = read_byte();
    midibyte c3 = read_byte();
    return to_32_bit(0, c3, c2, c1);
}

/**
 *  Reads a string.
 *  Unicode will be handled.
 *  Compared this function to midifile::read_byte_array().
 *
 * \param len
 *      Provides the length to be read.
 *
 * \return
 *      Returns a string of unsigned bytes.
 */

std::string
wrkfile::read_string (int len)
{
    std::string s;
    if (len > 0)
    {
        std::string data;
        midibyte c = 0xff;
        for (int i = 0; i < len && c != 0; ++i)
        {
            c = read_byte();
            if (c != 0)
                data.push_back(static_cast<char>(c));   // CAREFUL!!!
        }
#ifdef USE_UNICODE_SUPPORT
        if (is_nullptr(m_data.m_codec))
            s = std::string(data);
        else
        {
            // TODO: handle Unicode
            // s = m_data.m_codec->toUnicode(data);
        }
#else
        s = std::string(data);
#endif
    }
    return s;
}

/**
 *  Reads a variable length string (C-style).
 *
 *  Unicode is handled.
 *
 * \return
 *      Returns a string.
 */

std::string
wrkfile::read_var_string ()
{
    std::string result;
    std::string data;
    midibyte b;
    do
    {
        b = read_byte();
        if (b != 0)
            data.push_back(static_cast<char>(b));   // CAREFUL!!!
    }
    while (b != 0);

#ifdef USE_UNICODE_SUPPORT
    if (is_nullptr(m_data.m_codec))
        result = std::string(data);
    else
    {
        // TODO
        // result = m_data.m_codec->toUnicode(data);
    }
#else
    result = std::string(data);
#endif

    return result;
}

/**
 *  After reading a WRK header:
 *
 *      - verh WRK file format version major
 *      - verl WRK file format version minor
 *
 * void signalWRKHeader(int verh, int verl);
 *
 *  Note that the filename is set during the construction of this
 *  object.
 */

bool
wrkfile::parse (perform & p, int screenset, bool importing)
{
    bool result = grab_input_stream(std::string("WRK"));
    if (result)
    {
        std::string hdr = read_string(int(CakewalkHeader.length()));
        result = hdr == CakewalkHeader;
    }
    if (result)
    {
        clear_errors();
        read_gap(1);

#ifdef USE_WRK_VERSION_NUMBERS
        int vme = read_byte();          /* minor WRK version number         */
        int vma = read_byte();          /* major WRK version number         */
#else
        (void) read_byte();             /* minor WRK version number         */
        (void) read_byte();             /* major WRK version number         */
#endif

        int ck_id;
        do
        {
            ck_id = read_chunk();
        }
        while (ck_id != WC_END_CHUNK && ! at_end());

        if (! at_end())
            result = set_error("Corrupted WRK file.");
        else
            End_chunk();
    }
    else
        result = set_error("Invalid WRK file format.");

    return result;
}

/**
 *  This function seems to get an element that described a track and how it is
 *  to be played.
 *
 *  Emitted after reading a track prefix chunk:
 *
 *      - name1. Track 1st name.  Do not confuse with the TrackName() function!
 *      - name2. Track 2nd name.  Ditto.
 *      - trackno. Track number.
 *      - channel. Track forced channel (-1 = not forced).
 *      - pitch. Track pitch transpose in semitones (-127 to 127).
 *      - velocity. Track velocity increment (-127 to 127).
 *      - port. Track forced port.
 *      - selected. True if track is selected.
 *      - muted. True if track is muted.
 *      - loop. True if loop is enabled.
 *
\verbatim
   void signalWRKTrack
   (
        const std::string& name1,
        const std::string& name2,
        int trackno, int channel, int pitch, int velocity, int port,
        bool selected, bool muted, bool loop
   );
\endverbatim
 *
 * Not yet sure what to do with this function.
 */

void
wrkfile::Track_chunk ()
{
    std::string name[2];
    int trackno = int(read_short());
    for (int i = 0; i < 2; ++i)
    {
        int namelen = read_byte();
        name[i] = read_string(namelen);
    }
    int channel = read_byte();
    int pitch = read_byte();
    int velocity = read_byte();
    int port = read_byte();
    midibyte flags = read_byte();
    bool selected = ((flags & 1) != 0);
    bool muted = ((flags & 2) != 0);
    bool loop = ((flags & 4) != 0);

    // Q_EMIT signalWRKTrack(name[0], name[1], trackno, channel, pitch, ...);

    if (rc().show_midi())
    {
        std::string track_name = name[0] + " " + name[1];
        printf
        (
            "Track       : Tr. %d '%s' ch. %d port %d "
            "selected %s muted %s loop %s\n",
            trackno, track_name.c_str(), channel, port, bool_string(selected),
            bool_string(muted), bool_string(loop)
        );
        printf("            : pitch %d velocity %d\n", pitch, velocity);

    }
}

/**
 * Emitted after reading the global variables chunk:
 *
 * This record contains miscellaneous Cakewalk global variables that can
 * be retrieved using individual getters.
 *
 * See getNow(), getFrom(), getThru(), etc.  However, we will expose only
 * the values needed by Sequencer64.
 *
 * Fixed-point ratio value of tempo offset 1, 2, or 3.
 *
 * \note
 *      The offset ratios are expressed as a numerator in the expression n/64.
 *      To get a ratio from this number, divide the number by 64.  To get this
 *      number from a ratio, multiply the ratio by 64.
 *
 * Examples:
 *
 *      -  32 ==>  32/64 = 0.5
 *      -  63 ==>  63/64 = 0.9
 *      -  64 ==>  64/64 = 1.0
 *      - 128 ==> 128/64 = 2.0
 *
 * void signalWRKGlobalVars();
 */

void
wrkfile::Vars_chunk ()
{
    m_data.m_Now            = read_long();
    m_data.m_From           = read_long();
    m_data.m_Thru           = read_long();
    m_data.m_KeySig         = read_byte();
    m_data.m_Clock          = read_byte();
    m_data.m_AutoSave       = read_byte();
    m_data.m_PlayDelay      = read_byte();
    read_gap(1);
    m_data.m_ZeroCtrls      = read_byte() != 0;
    m_data.m_SendSPP        = read_byte() != 0;
    m_data.m_SendCont       = read_byte() != 0;
    m_data.m_PatchSearch    = read_byte() != 0;
    m_data.m_AutoStop       = read_byte() != 0;
    m_data.m_StopTime       = read_long();
    m_data.m_AutoRewind     = read_byte() != 0;
    m_data.m_RewindTime     = read_long();
    m_data.m_MetroPlay      = read_byte() != 0;
    m_data.m_MetroRecord    = read_byte() != 0;
    m_data.m_MetroAccent    = read_byte() != 0;
    m_data.m_CountIn        = read_byte();
    read_gap(2);
    m_data.m_ThruOn         = read_byte() != 0;
    read_gap(19);
    m_data.m_AutoRestart    = read_byte() != 0;
    m_data.m_CurTempoOfs    = read_byte();
    m_data.m_TempoOfs1      = read_byte();
    m_data.m_TempoOfs2      = read_byte();
    m_data.m_TempoOfs3      = read_byte();
    read_gap(2);
    m_data.m_PunchEnabled   = read_byte() != 0;
    m_data.m_PunchInTime    = read_long();
    m_data.m_PunchOutTime   = read_long();
    m_data.m_EndAllTime     = read_long();

    // Q_EMIT signalWRKGlobalVars();

    if (rc().show_midi())
    {
        printf
        (
            "Global Vars : now = %ld, end = %ld (and many more)\n",
            m_data.m_Now, m_data.m_EndAllTime
        );
    }
}

/**
 *
 * Emitted after reading the timebase chunk:
 *
 *      - timebase ticks per quarter note
 *
 * void signalWRKTimeBase(int timebase);
 */

void
wrkfile::Timebase_chunk ()
{
    midishort timebase = read_short();
    m_data.m_division = timebase;

    // Q_EMIT signalWRKTimeBase(timebase);

    if (rc().show_midi())
    {
        printf("Time Base   : %d PPQN\n", int(timebase));
    }
}

/**
 *  Provides the processing of events for a number of other functions.
 *
 * Emitted after reading a text message:
 *
 *      - track track number
 *      - time musical time
 *      - type Text type
 *      - data Text data
 *
 * void signalWRKText(int track, long time, int type, const std::string& data);
 *
 * Emitted after reading a track bank chunk:
 *
 *      - track track number
 *      - bank
 *
 * void signalWRKTrackBank(int track, int bank);
 *
 * Emitted after reading a track volume chunk:
 *
 *      - track track number
 *      - vol initial volume
 *
 * void signalWRKTrackVol(int track, int vol);
 *
 * Emitted after reading a chord diagram chunk:
 *
 *      - track track number
 *      - time event time in ticks
 *      - name chord name
 *      - data chord data definition (not decoded)
 *
 * void signalWRKChord(int track, long time, const std::string& name, const
 *      QByteArray& data);
 *
 * Emitted after reading an expression indication (notation) chunk:
 *
 *      - track track number
 *      - time event time in ticks
 *      - code expression event code
 *      - text expression text
 *
 * void signalWRKExpression(int track, long time, int code, const std::string&
 * text);
 *
 * Emitted after reading a hairpin symbol (notation) chunk:
 *
 *      - track track number
 *      - time event time in ticks
 *      - code hairpin code
 *      - dur duration
 *
 * void signalWRKHairpin(int track, long time, int code, int dur);
 */

void
wrkfile::processNoteArray (int track, int events)
{
    int value = 0;
    int len = 0;
    std::string text;
    midistring mdata;
    const char * format =
        "%12s: Tr. %d tick %ld event 0x%02X ch. %d data %d.%d value %d dur %d\n";

    for (int i = 0; i < events; ++i)
    {
        midibyte data1 = 0;
        midibyte data2 = 0;
        midishort dur = 0;
        midibyte type = 0;
        midibyte channel = 0;
        midilong time = read_24_bit();
        midibyte status = read_byte();
        dur = 0;

        /*
         * This check leaves out Note Off events.  This seems wrong, but it looks
         * like Cakewalk encodes note events as a Note On and a duration.
         */

        if (status >= EVENT_NOTE_ON)                            // 0x90
        {
            type = status & 0xf0;
            channel = status & 0x0f;
            data1 = read_byte();
            if (event::is_two_byte_msg(type))   // note on/off, control, pitch...
                data2 = read_byte();

            if (type == EVENT_NOTE_ON)                          // 0x90
                dur = read_short();                             // Cakewalk thing

            switch (type)
            {
            case EVENT_NOTE_ON:                                 // 0x90
                // Q_EMIT signalWRKNote(track, time, channel, data1, data2, dur);
                break;

            case EVENT_AFTERTOUCH:                              // 0xA0
                // Q_EMIT signalWRKKeyPress(track, time, channel, data1, data2);
                break;

            case EVENT_NOTE_OFF:                                // 0xB0
                // Q_EMIT signalWRKCtlChange(track, time, channel, data1, data2);
                break;

            case EVENT_PROGRAM_CHANGE:                          // 0xC0
                // Q_EMIT signalWRKProgram(track, time, channel, data1);
                break;

            case EVENT_CHANNEL_PRESSURE:                        // 0xD0
                // Q_EMIT signalWRKChanPress(track, time, channel, data1);
                break;

            case EVENT_PITCH_WHEEL:                             // 0xE0
                value = (data2 << 7) + data1 - 8192;
                // Q_EMIT signalWRKPitchBend(track, time, channel, value);
                break;

            case EVENT_MIDI_SYSEX:                              // 0xF0
                // Q_EMIT signalWRKSysexEvent(track, time, data1);
                break;
            }

            if (rc().show_midi())
            {
                printf
                (
                    format, "Note Array",
                    track, long(time), type, channel, data1, data2, value, dur
                );
            }
        }
        else if (status == 5)
        {
            int code = read_short();
            len = read_long();
            text = read_string(len);

            // Q_EMIT signalWRKExpression(track, time, code, text);

            if (rc().show_midi())
            {
                printf
                (
                    format, "Expression",
                    track, long(time), type, channel, data1, data2, value, dur
                );
                printf
                (
                    "        Text: code %d len %d, '%s'\n",
                    code, len, text.c_str()
                );
            }
        }
        else if (status == 6)
        {
            int code = int(read_short());
            dur = read_short();
            read_gap(4);

            // Q_EMIT signalWRKHairpin(track, time, code, dur);

            if (rc().show_midi())
            {
                printf
                (
                    format, "Hairpin",
                    track, long(time), type, channel, data1, data2, value, dur
                );
                printf("        Code: code %d\n", code);
            }
        }
        else if (status == 7)
        {
            len = read_long();
            text = read_string(len);
            mdata.clear();
            for (int j = 0; j < 13; ++j)    // TODO: use read_byte_array
            {
                midibyte byte = read_byte();
                mdata += byte;
            }

            // Q_EMIT signalWRKChord(track, time, text, mdata);

            if (rc().show_midi())
            {
                printf
                (
                    format, "Chord",
                    track, long(time), type, channel, data1, data2, value, dur
                );
                printf("        Text: len %d, '%s'\n", len, text.c_str());
            }
        }
        else if (status == 8)
        {
            len = read_short();
            mdata.clear();
            for (int j = 0; j < len; ++j)   // TODO: use read_byte_array
            {
                midibyte byte = read_byte();
                mdata += byte;
            }

            // Q_EMIT signalWRKSysex(0, std::string(), false, 0, mdata);

            if (rc().show_midi())
            {
                printf
                (
                    format, "SysEx",
                    track, long(time), type, channel, data1, data2, value, dur
                );
                // printf("        Text: len %d, '%s'\n", len, mdata.c_str());
            }
        }
        else
        {
            len = read_long();
            text = read_string(len);

            // Q_EMIT signalWRKText(track, time, status, text);

            if (rc().show_midi())
            {
                printf
                (
                    format, "Text",
                    track, long(time), type, channel, data1, data2, value, dur
                );
                printf("        Text: len %d, '%s'\n", len, text.c_str());
            }
        }
    }
    // Q_EMIT signalWRKStreamEnd(time + dur);
}

/**
 *
 * Emitted after reading a Note message:
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - pitch MIDI Note
 *      - vol Velocity
 *      - dur Duration
 *
 * void signalWRKNote(int track, long time, int chan, int pitch, int vol, int dur);
 *
 * Emitted after reading a Polyphonic Aftertouch message:
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - pitch MIDI Note
 *      - press Pressure amount
 *
 * void signalWRKKeyPress(int track, long time, int chan, int pitch, int press);
 *
 * Emitted after reading a Control Change message:
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - ctl MIDI Controller
 *      - value Control value
 *
 * void signalWRKCtlChange(int track, long time, int chan, int ctl, int value);
 *
 * Emitted after reading a Bender message:
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - value Bender value
 *
 * void signalWRKPitchBend(int track, long time, int chan, int value);
 *
 * Emitted after reading a Program change message:
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - patch Program number
 *
 * void signalWRKProgram(int track, long time, int chan, int patch);
 *
 * Emitted after reading a Channel Aftertouch message:
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - press Pressure amount
 *
 * void signalWRKChanPress(int track, long time, int chan, int press);
 *
 * Emitted after reading a System Exclusive event:
 *
 *      - track track number
 *      - time musical time
 *      - bank Sysex Bank number
 *
 * void signalWRKSysexEvent(int track, long time, int bank);
 *
 * Emitted after reading the last event of a event stream:
 *
 * void signalWRKStreamEnd(long time);
 *
 */

void
wrkfile::Stream_chunk ()
{
    midishort track = read_short();
    int events = read_short();
    for (int i = 0; i < events; ++i)
    {
        long time = read_24_bit();
        midibyte status = read_byte();
        midibyte data1 = read_byte();
        midibyte data2 = read_byte();
        int dur = read_short();
        int type = status & 0xf0;
        int channel = status & 0x0f;
        int value = 0;
        switch (type)
        {
        case 0x90:
            // Q_EMIT signalWRKNote(track, time, channel, data1, data2, dur);
            break;

        case 0xA0:
            // Q_EMIT signalWRKKeyPress(track, time, channel, data1, data2);
            break;

        case 0xB0:
            // Q_EMIT signalWRKCtlChange(track, time, channel, data1, data2);
            break;

        case 0xC0:
            // Q_EMIT signalWRKProgram(track, time, channel, data1);
            break;

        case 0xD0:
            // Q_EMIT signalWRKChanPress(track, time, channel, data1);
            break;

        case 0xE0:
            value = (data2 << 7) + data1 - 8192;
            // Q_EMIT signalWRKPitchBend(track, time, channel, value);
            break;

        case 0xF0:
            // Q_EMIT signalWRKSysexEvent(track, time, data1);
            break;
        }
        if (rc().show_midi())
        {
            const char * format =
                "%12s: Tr. %d tick %ld event 0x%02X ch. %d "
                "data %d.%d value %d dur %d\n";

            printf
            (
                format, "Stream",
                track, long(time), type, channel, data1, data2, value, dur
            );
        }
    }
    // Q_EMIT signalWRKStreamEnd(time + dur);
}

/**
 *  See MeterKey_chunk().
 */

void
wrkfile::Meter_chunk ()
{
    int count = read_short();
    for (int i = 0; i < count; ++i)
    {
        read_gap(4);
        int measure = read_short();
        int num = read_byte();
        int den = pow(2.0, read_byte());
        read_gap(4);

        // Q_EMIT signalWRKTimeSig(measure, num, den);

        if (rc().show_midi())
        {
            printf
            (
                "Time Sig    : measure = %d timesig %d/%d\n", measure, num, den
            );
        }
    }
}

/**
 * Emitted after reading a WRK Time signature:
 *
 *      - bar. Measure number.
 *      - num. Numerator.
 *      - den. Denominator (exponent in a power of two).
 *
 * void signalWRKTimeSig(int bar, int num, int den);
 *
 * Emitted after reading a WRK Key Signature:
 *
 *      - bar. Measure number.
 *      - alt. Number of alterations (negative=flats, positive=sharps).
 *
 * void signalWRKKeySig(int bar, int alt);
 */

void
wrkfile::MeterKey_chunk ()
{
    int count = read_short();
    for (int i = 0; i < count; ++i)
    {
        int measure = read_short();
        int num = read_byte();
        int den = pow(2.0, read_byte());
        midibyte alt = read_byte();

        // Q_EMIT signalWRKTimeSig(measure, num, den);
        // Q_EMIT signalWRKKeySig(measure, alt);

        if (rc().show_midi())
        {
            printf
            (
                "Time Sig/Key: measure %d timesig %d/%d key %u\n",
                measure, num, den, unsigned(alt)
            );
        }
    }
}

/**
 *
 */

double
wrkfile::get_real_time (long ticks) const
{
    double division = 1.0 * m_data.m_division;
    RecTempo last;
    last.time = 0;
    last.tempo = 100.0;
    last.seconds = 0.0;
    /*
     * TODO TODO TODO
     *
    if (! m_data.m_tempos.empty())
    {
        for (const RecTempo & rec : m_data.m_tempos)
        {
            if (rec.time >= ticks)
                break;

            last = rec;
        }
    }
     */

    return last.seconds +
    (
        ((ticks - last.time) / division) * (60.0 / last.tempo)
    );
}

/**
 * Emitted after reading a Tempo Change message:
 *
 * Tempo units are given in beats * 100 per minute, so to obtain BPM
 * it is necessary to divide by 100 the tempo.
 *
 *      - time musical time
 *      - tempo beats per minute multiplied by 100
 *
 * void signalWRKTempo(long time, int tempo);
 */

void
wrkfile::Tempo_chunk (int factor)
{
    double division = 1.0 * m_data.m_division;
    int count = read_short();
    for (int i = 0; i < count; ++i)
    {
        long time = read_long();
        read_gap(4);

        long tempo = read_short() * factor;
        read_gap(8);

        RecTempo next;
        next.time = time;
        next.tempo = tempo / 100.0;             /* the true BPM???  */
        next.seconds = 0.0;

        RecTempo last;
        last.time = 0;
        last.tempo = next.tempo;
        last.seconds = 0.0;
        if (! m_data.m_tempos.empty())
        {
            for (const RecTempo & rec : m_data.m_tempos)
            {
                if (rec.time >= time)
                    break;

                last = rec;
            }
            next.seconds = last.seconds +
            (
                ((time - last.time) / division) * (60.0 / last.tempo)
            );
        }
        m_data.m_tempos.push_back(next);

        // Q_EMIT signalWRKTempo(time, tempo);

        if (rc().show_midi())
        {
            printf("Tempo       : time %ld tempo %ld\n", time, tempo);
        }
    }
}

/**
 * Emitted after reading a System Exclusive Bank:
 *
 *      - bank Sysex Bank number
 *      - name Sysex Bank name
 *      - autosend Send automatically after loading the song
 *      - port MIDI output port
 *      - data Sysex bytes
 *
 * void signalWRKSysex(int bank, const std::string& name, bool autosend, int port,
 *      const QByteArray& data);
 */

void
wrkfile::Sysex_chunk ()
{
    midistring data;
    int bank = read_byte();
    int len = read_short();
    bool autosend = (read_byte() != 0);
    int namelen = read_byte();
    std::string name = read_string(namelen);
    for (int j = 0; j < len; ++j)           // TODO: use read_byte_array()
    {
        midibyte byte = read_byte();
        data += byte;
    }

    // Q_EMIT signalWRKSysex(bank, name, autosend, 0, data);

    if (rc().show_midi())
    {
        printf
        (
            "Sysex chunk : bank %d length %d name-length %d '%s' autosend %s\n",
            bank, len, namelen, name.c_str(), bool_string(autosend)
        );
    }
}

/**
 *
 */

void
wrkfile::Sysex2_chunk ()
{
    midistring data;
    int bank = read_short();
    int len = read_long();
    midibyte b = read_byte();
    int port = (b & 0xf0) >> 4;
    bool autosend = (b & 0x0f) != 0;
    int namelen = read_byte();
    std::string name = read_string(namelen);
    for (int j = 0; j < len; ++j)           // TODO: use read_byte_array()
    {
        int byte = read_byte();
        data += byte;
    }

    // Q_EMIT signalWRKSysex(bank, name, autosend, port, data);

    if (rc().show_midi())
    {
        printf
        (
            "Sysex2 chunk: bank %d length %d name-length %d '%s' "
            "port %d autosend %s\n",
            bank, len, namelen, name.c_str(), port, bool_string(autosend)
        );
    }
}

/**
 *
 */

void
wrkfile::NewSysex_chunk ()
{
    std::string name;
    midistring data;
    int bank = read_short();
    int len = int(read_long());
    int port = read_short();
    bool autosend = (read_byte() != 0);
    int namelen = read_byte();
    name = read_string(namelen);
    for (int j = 0; j < len; ++j)           // TODO: use read_byte_array()
    {
        int byte = read_byte();
        data += byte;
    }

    // Q_EMIT signalWRKSysex(bank, name, autosend, port, data);

    if (rc().show_midi())
    {
        printf
        (
            "New Sysex   : bank %d length %d name-length %d"
            "'%s' port %d autosend %s\n",
            bank, len, namelen, name.c_str(), port, bool_string(autosend)
        );
    }
}

/**
 *  Handles reading an Extended Thru parameters chunk.  This item was
 *  introduced in Cakewalk version 4.0.  These parameters are intended to
 *  override the global variables' ThruOn value, so this record should come
 *  after the WC_VARS_CHUNK record. It is optional.
 *
 *      -  mode (auto, off, on)
 *      -  port MIDI port
 *      -  channel MIDI channel
 *      -  keyPlus Note transpose
 *      -  velPlus Velocity transpose
 *      -  localPort MIDI local port
 *
 *  void signalWRKThru (int mode, int port, int channel, int keyPlus, int velPlus,
 *  int localPort);
 */

void
wrkfile::Thru_chunk ()
{
    read_gap(2);
    midibyte port = read_byte();            // 0 -> 127
    midibyte channel = read_byte();         // -1, 0 -> 15
    midibyte keyplus = read_byte();         // 0 -> 127
    midibyte velplus = read_byte();         // 0 -> 127
    midibyte localport = read_byte();
    midibyte mode = read_byte();

    // Q_EMIT signalWRKThru(mode, port, channel, keyplus, velplus, localport);

    if (rc().show_midi())
    {
        printf
        (
            "Thru        : mode %u port %u channel %u key+ %u vel+ %u "
            "localport %u\n",
            unsigned(mode), unsigned(port), unsigned(channel),
            unsigned(keyplus), unsigned(velplus), unsigned(localport)
        );
    }
}

/**
 * Emitted after reading a track offset chunk:
 *
 *      - track track number
 *      - offset time offset
 *
 * void signalWRKTrackOffset(int track, int offset);
 */

void
wrkfile::TrackOffset ()
{
    midishort track = read_short();
    midishort offset = read_short();

    // Q_EMIT signalWRKTrackOffset(track, offset);

    if (rc().show_midi())
    {
        printf("Track Offset: Tr. %d offset %d\n", int(track), int(offset));
    }
}

/**
 * Emitted after reading a track repetition chunk.
 *
 *      - track track number
 *      - reps number of repetitions
 *
 * void signalWRKTrackReps(int track, int reps);
 */

void
wrkfile::TrackReps ()
{
    midishort track = read_short();
    midishort reps = read_short();

    // Q_EMIT signalWRKTrackReps(track, reps);

    if (rc().show_midi())
    {
        printf("Track Reps  : Tr. %d reps %d\n", int(track), int(reps));
    }
}

/**
 * Emitted after reading a track patch chunk:
 *
 *      - track track number
 *      - patch
 *
 * void signalWRKTrackPatch(int track, int patch);
 */

void
wrkfile::TrackPatch ()
{
    midishort track = read_short();
    midibyte patch = read_byte();

    // Q_EMIT signalWRKTrackPatch(track, patch);

    if (rc().show_midi())
    {
        printf("Track Patch : Tr. %d patch %d\n", int(track), int(patch));
    }
}

/**
 * Emitted after reading a SMPTE time format chunk:
 *
 *      - frames frames/sec (24, 25, 29=30-drop, 30)
 *      - offset frames of offset
 *
 * void signalWRKTimeFormat(int frames, int offset);
 */

void
wrkfile::TimeFormat ()
{
    midishort fmt = read_short();
    midishort ofs = read_short();

    // Q_EMIT signalWRKTimeFormat(fmt, ofs);

    if (rc().show_midi())
    {
        printf("Time Format : format %d offset %d\n", int(fmt), int(ofs));
    }
}

/**
 * Emitted after reading a comments chunk:
 *
 *      - data file text comments
 *
 * void signalWRKComments(const std::string& data);
 */

void
wrkfile::Comments ()
{
    int len = read_short();
    std::string text = read_string(len);

    // Q_EMIT signalWRKComments(text);

    if (rc().show_midi())
    {
        printf("Comments    : length %d, '%s'\n", len, text.c_str());
    }
}

/**
 * Emitted after reading a variable chunk:
 *
 * This record may contain data in text or binary format.
 *
 *      - name record identifier
 *      - data record variable data
 *
 * void signalWRKVariableRecord(const std::string& name, const QByteArray& data);
 */

void
wrkfile::ProcessVariableRecord (int max)
{
    int datalen = max - 32;
    midistring data;
    std::string name = read_var_string();
    read_gap(31 - name.length());
    for (int i = 0; i < datalen; ++i)   // TODO: use read_byte_array()
        data += read_byte();

    // Q_EMIT signalWRKVariableRecord(name, data);

    if (rc().show_midi())
    {
        printf("Variable Rec: '%s' (data not shown)\n", name.c_str());
    }
}

/**
 *  Handles reading an unknown chunk.
 *
 *      -  type chunk type
 *      -  data chunk data (not decoded)
 *
 * void signalWRKUnknownChunk(int type, const QByteArray& data);
 */

void
wrkfile::Unknown (int id)
{
    // Q_EMIT signalWRKUnknownChunk(id, m_data.m_lastChunkData);

    if (rc().show_midi())
    {
        printf("Unknown     : id %d (data not shown)\n", id);
    }
}

/**
 * Emitted after reading a new track prefix:
 *
 *      - name. Track name.
 *      - trackno. Track number.
 *      - channel. Forced MIDI channel.
 *      - pitch. Note transposition.
 *      - velocity. Velocity increment.
 *      - port. MIDI port number.
 *      - selected. Track is selected.
 *      - muted. Track is muted.
 *      - loop. Track loop enabled.
 *
 * void signalWRKNewTrack
 * (
 *      const std::string& name,
 *      int trackno, int channel, int pitch,
 *      int velocity, int port,
 *      bool selected, bool muted, bool loop
 * );
 */

void
wrkfile::NewTrack ()
{
    bool selected = false;
    bool loop = false;
    midishort track = read_short();
    midibyte len = read_byte();
    std::string name = read_string(len);
    midishort bank = read_short();
    midishort patch = read_short();
    midishort vol = read_short();
    midishort pan = read_short();
    midibyte key = read_byte();
    midibyte vel = read_byte();
    read_gap(7);

    midibyte port = read_byte();
    midibyte channel = read_byte();
    bool muted = read_byte() != 0;

    // Q_EMIT signalWRKNewTrack
    // (
    //      name, track, channel, key, vel, port, selected, muted, loop
    // );

    if (rc().show_midi())
    {
        printf
        (
            "New Track   : Tr. %d ch. %d key %d port %d "
            "selected %s muted %s loop %s\n",
            int(track), int(channel), int(key), int(port),
            bool_string(selected), bool_string(muted), bool_string(loop)
        );
        printf
        (
            "            : volume %d velocity %d pan %d\n",
            int(vol), int(vel), int(pan)
        );
    }

    if (short(bank) >= 0)
    {
        // Q_EMIT signalWRKTrackBank(track, bank);
    }
    if (short(patch) >= 0)
    {
        if (short(channel) >= 0)
        {
            // Q_EMIT signalWRKProgram(track, 0, channel, patch);
        }
        else
        {
            // Q_EMIT signalWRKTrackPatch(track, patch);
        }
    }
}

/**
 * Emitted after reading a software version chunk:
 *
 *      - version software version string
 *
 * void signalWRKSoftVer(const std::string& version);
 */

void
wrkfile::SoftVer()
{
    int len = read_byte();
    std::string vers = read_string(len);

    // Q_EMIT signalWRKSoftVer(vers);

    if (rc().show_midi())
    {
        printf("Software Ver: %s\n", vers.c_str());
    }
}

/**
 * Emitted after reading a track name chunk:
 *
 *      - track track number
 *      - name track name
 *
 * void signalWRKTrackName(int track, const std::string& name);
 */

void
wrkfile::TrackName ()
{
    int track = read_short();
    int len = read_byte();
    std::string name = read_string(len);

    // Q_EMIT signalWRKTrackName(track, name);

    if (rc().show_midi())
    {
        printf
        (
            "Track Name  : Tr. %d name-length %d name '%s'\n",
            track, len, name.c_str()
        );
    }
}

/**
 * Emitted after reading a string event types chunk:
 *
 *      - strs. List of declared string event types.
 *
 * void signalWRKStringTable(const std::stringList& strs);
 */

void
wrkfile::StringTable()
{
    std::list<std::string> table;
    int rows = read_short();
    for (int i = 0; i < rows; ++i)
    {
        int len = read_byte();
        std::string name = read_string(len);
        int idx = read_byte();
        table.push_back(name);      // TODO: table.insert(idx, name);
    }

    // Q_EMIT signalWRKStringTable(table);

    if (rc().show_midi())
    {
        printf("String Table: %d rows (event types not shown)\n", rows);
    }
}

/**
 *
 */

void
wrkfile::LyricsStream ()
{
    midishort track = read_short();
    int events = read_long();
    processNoteArray(track, events);
}

/**
 *  Gets Cakewalk style track volume.
 */

void
wrkfile::TrackVol ()
{
    midishort track = read_short();
    int vol = read_short();

    // Q_EMIT signalWRKTrackVol(track, vol);

    if (rc().show_midi())
    {
        printf("Track Volume: Tr. %d volume %d\n", int(track), vol);
    }
}

/**
 *  See TrackOffset().  This version reads a long offset, instead of a short
 *  offset.
 */

void
wrkfile::NewTrackOffset ()
{
    midishort track = read_short();
    int offset = read_long();

    // Q_EMIT signalWRKTrackOffset(track, offset);

    if (rc().show_midi())
    {
        printf("N track offs: Tr. %d offset %d\n", int(track), offset);
    }
}

/**
 *  Gets the bank ID of the track.
 */

void
wrkfile::TrackBank ()
{
    int track = int(read_short());
    int bank = int(read_short());

    // Q_EMIT signalWRKTrackBank(track, bank);

    if (rc().show_midi())
    {
        printf("Track Bank  : Tr. %d bank %d\n", track, bank);
    }
}

/**
 *
 * Emitted after reading a segment prefix chunk:
 *
 *      - track track number
 *      - time segment time offset
 *      - name segment name
 *
 * void signalWRKSegment(int track, long time, const std::string& name);
 *
 */

void
wrkfile::Segment_chunk ()
{
    int track = read_short();
    int offset = read_long();
    read_gap(8);

    int len = int(read_byte());
    std::string name = read_string(len);
    read_gap(20);

    // Q_EMIT signalWRKSegment(track, offset, name);

    if (rc().show_midi())
    {
        printf
        (
            "Segment     : Tr. %d offset %d name-length %d name '%s'\n",
            track, offset, len, name.c_str()
        );
    }

    int events = read_long();
    processNoteArray(track, events);
}

/**
 *
 */

void
wrkfile::NewStream()
{
    int track = read_short();
    int len = int(read_byte());
    std::string name = read_string(len);

    // Q_EMIT signalWRKSegment(track, 0, name);

    if (rc().show_midi())
    {
        printf
        (
            "New Stream  : Tr. %d name-length %d name '%s'\n",
            track, len, name.c_str()
        );
    }

    int events = int(read_long());
    processNoteArray(track, events);
}

/**
 *  After reading the last chunk of a WRK file
 *
 * void signalWRKEnd();
 *
 *  Nothing to do here.  The original emitted a signal, but that's not needed
 *  when simply reading a WRK file in the Sequencer64 library.
 */

void
wrkfile::End_chunk ()
{
    // Q_EMIT signalWRKEnd();

    if (rc().show_midi())
    {
        printf("End chunk   : at seq number %d\n", m_seq_number);
    }
}

/**
 *
 */

int
wrkfile::read_chunk ()
{
    int ck = int(read_byte());
    if (ck != WC_END_CHUNK)
    {
        int ck_len = read_long();
        size_t start_pos = get_file_pos();
        size_t final_pos = start_pos + ck_len;
        read_raw_data(ck_len);
        read_seek(start_pos);       // TODO: check the return value
        switch (ck)
        {
        case WC_TRACK_CHUNK:
            Track_chunk();          // names, number, velocity, mute/loop status
            break;

        case WC_VARS_CHUNK:
            Vars_chunk();           // Cakewalk global variables
            break;

        case WC_TIMEBASE_CHUNK:
            Timebase_chunk();       // gets PPQN value m_division for whole tune
            break;

        case WC_STREAM_CHUNK:
            Stream_chunk();         // note, control, program, pitchbend, etc.
            break;

        case WC_METER_CHUNK:
            Meter_chunk();          // gets a time signature
            break;

        case WC_TEMPO_CHUNK:
            Tempo_chunk(100);       // gets the BPM tempo
            break;

        case WC_NTEMPO_CHUNK:
            Tempo_chunk();          // gets the BPM tempo
            break;

        case WC_SYSEX_CHUNK:
            Sysex_chunk();          // handle SysEx messages (TODO)
            break;

        case WC_THRU_CHUNK:
            Thru_chunk();           // Extended Thru: mode, port, channel, ...
            break;

        case WC_TRKOFFS_CHUNK:
            TrackOffset();          // "short" track offset
            break;

        case WC_TRKREPS_CHUNK:
            TrackReps();            // repetition count for a track
            break;

        case WC_TRKPATCH_CHUNK:
            TrackPatch();           // track number and patch number
            break;

        case WC_TIMEFMT_CHUNK:
            TimeFormat();           // SMPTE frames, frames/sec, offset
            break;

        case WC_COMMENTS_CHUNK:
            Comments();             // data file text comments
            break;

        case WC_VARIABLE_CHUNK:
            ProcessVariableRecord(ck_len);  // record identifier & variable data
            break;

        case WC_NTRACK_CHUNK:
            NewTrack();             // track #, channel, pitch, mute/loop...
            break;

        case WC_SOFTVER_CHUNK:
            SoftVer();              // software version string
            break;

        case WC_TRKNAME_CHUNK:
            TrackName();            // track number and name
            break;

        case WC_STRTAB_CHUNK:
            StringTable();          // list of declared string event types
            break;

        case WC_LYRICS_CHUNK:
            LyricsStream();         // processes the note array
            break;

        case WC_TRKVOL_CHUNK:
            TrackVol();             // Cakewalk style track volume
            break;

        case WC_NTRKOFS_CHUNK:
            NewTrackOffset();       // "long" track offset
            break;

        case WC_TRKBANK_CHUNK:
            TrackBank();            // the bank ID of the track
            break;

        case WC_METERKEY_CHUNK:
            MeterKey_chunk();       // gets a time signature and key (scale)
            break;

        case WC_SYSEX2_CHUNK:
            Sysex2_chunk();         // handle SysEx messages (TODO)
            break;

        case WC_NSYSEX_CHUNK:
            NewSysex_chunk();
            break;

        case WC_SGMNT_CHUNK:
            Segment_chunk();        // processes a note array
            break;

        case WC_NSTREAM_CHUNK:
            NewStream();            // processes a note array
            break;

        default:
            Unknown(ck);
            break;
        }
        read_seek(final_pos);           // TODO: check the return value
    }
    return ck;
}

}        // namespace seq64

/*
 * wrkfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

