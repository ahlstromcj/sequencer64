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
 * \updates       2018-06-05
 * \license       GNU GPLv2 or above
 *
 *  For a quick guide to the WRK format, see, for example:
 *
 *    Implementation of a class managing Cakewalk WRK Files input
 *
 * Emitted after reading the last event of a event stream
 *
 * void signalWRKStreamEnd(long time);
 *
 * Emitted after reading a Note message
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
 * Emitted after reading a Polyphonic Aftertouch message
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - pitch MIDI Note
 *      - press Pressure amount
 *
 * void signalWRKKeyPress(int track, long time, int chan, int pitch, int press);
 *
 * Emitted after reading a Control Change message
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - ctl MIDI Controller
 *      - value Control value
 *
 * void signalWRKCtlChange(int track, long time, int chan, int ctl, int value);
 *
 * Emitted after reading a Bender message
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - value Bender value
 *
 * void signalWRKPitchBend(int track, long time, int chan, int value);
 *
 * Emitted after reading a Program change message
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - patch Program number
 *
 * void signalWRKProgram(int track, long time, int chan, int patch);
 *
 * Emitted after reading a Channel Aftertouch message
 *
 *      - track track number
 *      - time musical time
 *      - chan MIDI Channel
 *      - press Pressure amount
 *
 * void signalWRKChanPress(int track, long time, int chan, int press);
 *
 * Emitted after reading a System Exclusive event
 *
 *      - track track number
 *      - time musical time
 *      - bank Sysex Bank number
 *
 * void signalWRKSysexEvent(int track, long time, int bank);
 *
 * Emitted after reading a System Exclusive Bank
 *
 *      - bank Sysex Bank number
 *      - name Sysex Bank name
 *      - autosend Send automatically after loading the song
 *      - port MIDI output port
 *      - data Sysex bytes
 *
 * void signalWRKSysex(int bank, const std::string& name, bool autosend, int port,
 *  const QByteArray& data);
 *
 * Emitted after reading a text message
 *
 *      - track track number
 *      - time musical time
 *      - type Text type
 *      - data Text data
 *
 * void signalWRKText(int track, long time, int type, const std::string& data);
 *
 * Emitted after reading a WRK Time signature
 *
 *      - bar Measure number
 *      - num Numerator
 *      - den Denominator (exponent in a power of two)
 *
 * void signalWRKTimeSig(int bar, int num, int den);
 *
 * Emitted after reading a WRK Key Signature
 *
 *      - bar Measure number
 *      - alt Number of alterations (negative=flats, positive=sharps)
 *
 * void signalWRKKeySig(int bar, int alt);
 *
 * Emitted after reading a Tempo Change message.
 *
 * Tempo units are given in beats * 100 per minute, so to obtain BPM
 * it is necessary to divide by 100 the tempo.
 *
 *      - time musical time
 *      - tempo beats per minute multiplied by 100
 *
 * void signalWRKTempo(long time, int tempo);
 *
 * Emitted after reading a track prefix chunk
 *
 *      - name1 track 1st name
 *      - name2 track 2nd name
 *      - trackno track number
 *      - channel track forced channel (-1=no forced)
 *      - pitch track pitch transpose in semitones (-127..127)
 *      - velocity track velocity increment (-127..127)
 *      - port track forced port
 *      - selected true if track is selected
 *      - muted true if track is muted
 *      - loop true if loop is enabled
 *
 * void signalWRKTrack(const std::string& name1,
 *                  const std::string& name2,
 *                  int trackno, int channel, int pitch,
 *                  int velocity, int port,
 *                  bool selected, bool muted, bool loop);
 *
 * Emitted after reading the timebase chunk
 *
 *      - timebase ticks per quarter note
 *
 * void signalWRKTimeBase(int timebase);
 *
 * Emitted after reading the global variables chunk.
 *
 * This record contains miscellaneous Cakewalk global variables that can
 * be retrieved using individual getters.
 *
 * See getNow(), getFrom(), getThru()
 *
 * void signalWRKGlobalVars();
 *
 * Emitted after reading a track offset chunk
 *
 *      - track track number
 *      - offset time offset
 *
 * void signalWRKTrackOffset(int track, int offset);
 *
 * Emitted after reading a track offset chunk
 *
 *      - track track number
 *      - reps number of repetitions
 *
 * void signalWRKTrackReps(int track, int reps);
 *
 * Emitted after reading a track patch chunk
 *
 *      - track track number
 *      - patch
 *
 * void signalWRKTrackPatch(int track, int patch);
 *
 * Emitted after reading a track bank chunk
 *
 *      - track track number
 *      - bank
 *
 * void signalWRKTrackBank(int track, int bank);
 *
 * Emitted after reading a SMPTE time format chunk
 *
 *      - frames frames/sec (24, 25, 29=30-drop, 30)
 *      - offset frames of offset
 *
 * void signalWRKTimeFormat(int frames, int offset);
 *
 * Emitted after reading a comments chunk
 *
 *      - data file text comments
 *
 * void signalWRKComments(const std::string& data);
 *
 * Emitted after reading a variable chunk.
 * This record may contain data in text or binary format.
 *
 *      - name record identifier
 *      - data record variable data
 *
 * void signalWRKVariableRecord(const std::string& name, const QByteArray& data);
 *
 * Emitted after reading a track volume chunk.
 *
 *      - track track number
 *      - vol initial volume
 *
 * void signalWRKTrackVol(int track, int vol);
 *
 * Emitted after reading a new track prefix
 *
 *      - name track name
 *      - trackno track number
 *      - channel forced MIDI channel
 *      - pitch Note transposition
 *      - velocity Velocity increment
 *      - port MIDI port number
 *      - selected track is selected
 *      - muted track is muted
 *      - loop track loop enabled
 *
 *
 * void signalWRKNewTrack(const std::string& name,
 *                     int trackno, int channel, int pitch,
 *                     int velocity, int port,
 *                     bool selected, bool muted, bool loop);
 *
 * Emitted after reading a software version chunk.
 *
 *      - version software version string
 *
 * void signalWRKSoftVer(const std::string& version);
 *
 * Emitted after reading a track name chunk.
 *
 *      - track track number
 *      - name track name
 *
 * void signalWRKTrackName(int track, const std::string& name);
 *
 * Emitted after reading a string event types chunk.
 *
 *      - strs list of declared string event types
 *
 * void signalWRKStringTable(const std::stringList& strs);
 *
 * Emitted after reading a segment prefix chunk.
 *
 *      - track track number
 *      - time segment time offset
 *      - name segment name
 *
 * void signalWRKSegment(int track, long time, const std::string& name);
 *
 * Emitted after reading a chord diagram chunk.
 *
 *      - track track number
 *      - time event time in ticks
 *      - name chord name
 *      - data chord data definition (not decoded)
 *
 * void signalWRKChord(int track, long time, const std::string& name, const
 *      QByteArray& data);
 *
 * Emitted after reading an expression indication (notation) chunk.
 *
 *      - track track number
 *      - time event time in ticks
 *      - code expression event code
 *      - text expression text
 *
 * void signalWRKExpression(int track, long time, int code, const std::string& text);
 *
 * Emitted after reading a hairpin symbol (notation) chunk.
 *
 *      - track track number
 *      - time event time in ticks
 *      - code hairpin code
 *      - dur duration
 *
 * void signalWRKHairpin(int track, long time, int code, int dur);
 */

#include <cmath>

#include "perform.hpp"                  /* must precede wrkfile.hpp !       */
#include "wrkfile.h"

namespace seq64
{

/**
 * wrkfile provides a mechanism to parse Cakewalk WRK Files, without
 * the burden of a policy forcing to use some internal sequence representation.
 *
 * This class is not related or based on the ALSA library.
 *
 * Fixed-point ratio value of tempo offset 1, 2, or 3
 *
 * NOTE: The offset ratios are expressed as a numerator in the expression
 * n/64.  To get a ratio from this number, divide the number by 64.  To get
 * this number from a ratio, multiply the ratio by 64.
 * Examples:
 *   32 ==>  32/64 = 0.5
 *   63 ==>  63/64 = 0.9
 *   64 ==>  64/64 = 1.0
 *  128 ==> 128/64 = 2.0
 */

class wrkfile::wrkfile_private
{

public:

    wrkfile_private () :
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
        m_codec         (0),
        m_IOStream      (0)
    {
       // no code
    }

    midilong m_Now;          ///< Now marker time
    midilong m_From;         ///< From marker time
    midilong m_Thru;         ///< Thru marker time
    midibyte m_KeySig;        ///< Key signature (0=C, 1=C#, ... 11=B)
    midibyte m_Clock;         ///< Clock Source (0=Int, 1=MIDI, 2=FSK, 3=SMPTE)
    midibyte m_AutoSave;      ///< Auto save (0=disabled, 1..256=minutes)
    midibyte m_PlayDelay;     ///< Play Delay
    bool m_ZeroCtrls;       ///< Zero continuous controllers?
    bool m_SendSPP;         ///< Send Song Position Pointer?
    bool m_SendCont;        ///< Send MIDI Continue?
    bool m_PatchSearch;     ///< Patch/controller search-back?
    bool m_AutoStop;        ///< Auto-stop?
    midilong m_StopTime;     ///< Auto-stop time
    bool m_AutoRewind;      ///< Auto-rewind?
    midilong m_RewindTime;   ///< Auto-rewind time
    bool m_MetroPlay;       ///< Metronome on during playback?
    bool m_MetroRecord;     ///< Metronome on during recording?
    bool m_MetroAccent;     ///< Metronome accents primary beats?
    midibyte m_CountIn;       ///< Measures of count-in (0=no count-in)
    bool m_ThruOn;          ///< MIDI Thru enabled? (only used if no THRU rec)
    bool m_AutoRestart;     ///< Auto-restart?
    midibyte m_CurTempoOfs;   ///< Which of the 3 tempo offsets is used: 0..2
    midibyte m_TempoOfs1;     ///< Fixed-point ratio value of offset 1
    midibyte m_TempoOfs2;     ///< Fixed-point ratio value of offset 2
    midibyte m_TempoOfs3;     ///< Fixed-point ratio value of offset 3
    bool m_PunchEnabled;    ///< Auto-Punch enabled?
    midilong m_PunchInTime;  ///< Punch-in time
    midilong m_PunchOutTime; ///< Punch-out time
    midilong m_EndAllTime;   ///< Time of latest event (incl. all tracks)

    int m_division;
    midibyte m_lastChunkData[1024];

//  QTextCodec * m_codec;
//  QDataStream * m_IOStream;
//  QByteArray m_lastChunkData;
//  QList<RecTempo> m_tempos;
};

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
    int ppqn,
) :
    midifile    (name, ppqn),
    m_d         (new wrkfile_private())
{
    //
}

/**
 *  \dtor
 */

wrkfile::~wrkfile()
{
    if (not_nullptr(m_d))
        delete d;
}

/**
 * Read the chunk raw data (undecoded)
 */

void
wrkfile::read_raw_data (int sz)
{
    // m_d->m_lastChunkData = m_d->m_IOStream->device()->read(sz);

    read_byte_array(m_d->m_lastChunkData, sz);
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
 *
 *  Unicode is handled.
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
        for (int i = 0; i < len && c != 0; ++i)
        {
            midibyte c = read_byte();
            if (c != 0)
                data.push_back(static_cast<char>(c));   // CAREFUL!!!
        }
        if (is_nullptr(m_d) || is_nullptr(m_d->m_codec))
            s = std::string(data);
        else
        {
            // TODO: handle Unicode
            // s = m_d->m_codec->toUnicode(data);
        }
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
    std::string s;
    std::string data;
    do
    {
        midibyte b = read_byte();
        if (b != 0)
            data.push_back(static_cast<char>(b));   // CAREFUL!!!
    }
    while (b != 0);

    if (is_nullptr(m_d) || is_nullptr(m_d->m_codec))
        s = std::string(data);
    else
    {
        // TODO
        // s = m_d->m_codec->toUnicode(data);
    }
    return s;
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
    bool result = true;
    m_disable_reported = false;

    std::string read_string(CakewalkHeader.length());
    if (hdr == CakewalkHeader)
    {
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
        while (ck_id != WC_WC_END_CHUNK);

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
 *
 */

void
wrkfile::Track_chunk()
{
    std::string name[2];
    trackno = read_short();
    for (int i = 0; i < 2; ++i)
    {
        int namelen = read_byte();
        name[i] = readString(namelen);
    }
    int channel = (midibyte) read_byte();
    int pitch = read_byte();
    int velocity = read_byte();
    int port = read_byte();
    midibyte flags = read_byte();
    bool selected = ((flags & 1) != 0);
    bool muted = ((flags & 2) != 0);
    bool loop = ((flags & 4) != 0);
    /*
    Q_EMIT signalWRKTrack(name[0], name[1], trackno, channel, pitch,
                          velocity, port, selected, muted, loop);
     */
}

void
wrkfile::Vars_chunk()
{
    m_d->m_Now = read_long();
    m_d->m_From = read_long();
    m_d->m_Thru = read_long();
    m_d->m_KeySig = read_byte();
    m_d->m_Clock = read_byte();
    m_d->m_AutoSave = read_byte();
    m_d->m_PlayDelay = read_byte();
    read_gap(1);
    m_d->m_ZeroCtrls = (read_byte() != 0);
    m_d->m_SendSPP = (read_byte() != 0);
    m_d->m_SendCont = (read_byte() != 0);
    m_d->m_PatchSearch = (read_byte() != 0);
    m_d->m_AutoStop = (read_byte() != 0);
    m_d->m_StopTime = read_long();
    m_d->m_AutoRewind = (read_byte() != 0);
    m_d->m_RewindTime = read_long();
    m_d->m_MetroPlay = (read_byte() != 0);
    m_d->m_MetroRecord = (read_byte() != 0);
    m_d->m_MetroAccent = (read_byte() != 0);
    m_d->m_CountIn = read_byte();
    read_gap(2);
    m_d->m_ThruOn = (read_byte() != 0);
    read_gap(19);
    m_d->m_AutoRestart = (read_byte() != 0);
    m_d->m_CurTempoOfs = read_byte();
    m_d->m_TempoOfs1 = read_byte();
    m_d->m_TempoOfs2 = read_byte();
    m_d->m_TempoOfs3 = read_byte();
    read_gap(2);
    m_d->m_PunchEnabled = (read_byte() != 0);
    m_d->m_PunchInTime = read_long();
    m_d->m_PunchOutTime = read_long();
    m_d->m_EndAllTime = read_long();

    Q_EMIT signalWRKGlobalVars();
}

void
wrkfile::Timebase_chunk()
{
    midishort timebase = read_short();
    m_d->m_division = timebase;
    Q_EMIT signalWRKTimeBase(timebase);
}

void
wrkfile::processNoteArray(int track, int events)
{
    midilong time = 0;
    midibyte  status = 0, data1 = 0, data2 = 0;
    midishort dur = 0;
    int value = 0, type = 0, channel = 0, len = 0;
    std::string text;
    QByteArray data;
    for (int i = 0; i < events; ++i)
    {
        time = read_24_bit();
        status = read_byte();
        dur = 0;
        if (status >= 0x90)
        {
            type = status & 0xf0;
            channel = status & 0x0f;
            data1 = read_byte();
            if (type == 0x90 || type == 0xA0  || type == 0xB0 || type == 0xE0)
                data2 = read_byte();
            if (type == 0x90)
                dur = read_short();
            switch (type)
            {
            case 0x90:
                Q_EMIT signalWRKNote(track, time, channel, data1, data2, dur);
                break;

            case 0xA0:
                Q_EMIT signalWRKKeyPress(track, time, channel, data1, data2);
                break;

            case 0xB0:
                Q_EMIT signalWRKCtlChange(track, time, channel, data1, data2);
                break;

            case 0xC0:
                Q_EMIT signalWRKProgram(track, time, channel, data1);
                break;

            case 0xD0:
                Q_EMIT signalWRKChanPress(track, time, channel, data1);
                break;

            case 0xE0:
                value = (data2 << 7) + data1 - 8192;
                Q_EMIT signalWRKPitchBend(track, time, channel, value);
                break;

            case 0xF0:
                Q_EMIT signalWRKSysexEvent(track, time, data1);
                break;

            }
        }
        else if (status == 5)
        {
            int code = read_short();
            len = read_long();
            text = readString(len);
            Q_EMIT signalWRKExpression(track, time, code, text);
        }
        else if (status == 6)
        {
            int code = read_short();
            dur = read_short();
            read_gap(4);
            Q_EMIT signalWRKHairpin(track, time, code, dur);
        }
        else if (status == 7)
        {
            len = read_long();
            text = readString(len);
            data.clear();
            for (int j = 0; j < 13; ++j)
            {
                int byte = read_byte();
                data += byte;
            }
            Q_EMIT signalWRKChord(track, time, text, data);
        }
        else if (status == 8)
        {
            len = read_short();
            data.clear();
            for (int j = 0; j < len; ++j)
            {
                int byte = read_byte();
                data += byte;
            }
            Q_EMIT signalWRKSysex(0, std::string(), false, 0, data);
        }
        else
        {
            len = read_long();
            text = readString(len);
            Q_EMIT signalWRKText(track, time, status, text);
        }
    }
    Q_EMIT signalWRKStreamEnd(time + dur);
}

void
wrkfile::Stream_chunk()
{
    long time = 0;
    int dur = 0, value = 0, type = 0, channel = 0;
    midibyte status = 0, data1 = 0, data2 = 0;
    midishort track = read_short();
    int events = read_short();
    for (int i = 0; i < events; ++i)
    {
        time = read_24_bit();
        status = read_byte();
        data1 = read_byte();
        data2 = read_byte();
        dur = read_short();
        type = status & 0xf0;
        channel = status & 0x0f;
        switch (type)
        {
        case 0x90:
            Q_EMIT signalWRKNote(track, time, channel, data1, data2, dur);
            break;

        case 0xA0:
            Q_EMIT signalWRKKeyPress(track, time, channel, data1, data2);
            break;

        case 0xB0:
            Q_EMIT signalWRKCtlChange(track, time, channel, data1, data2);
            break;

        case 0xC0:
            Q_EMIT signalWRKProgram(track, time, channel, data1);
            break;

        case 0xD0:
            Q_EMIT signalWRKChanPress(track, time, channel, data1);
            break;

        case 0xE0:
            value = (data2 << 7) + data1 - 8192;
            Q_EMIT signalWRKPitchBend(track, time, channel, value);
            break;

        case 0xF0:
            Q_EMIT signalWRKSysexEvent(track, time, data1);
            break;

        }
    }
    Q_EMIT signalWRKStreamEnd(time + dur);
}

void
wrkfile::Meter_chunk()
{
    int count = read_short();
    for (int i = 0; i < count; ++i)
    {
        read_gap(4);
        int measure = read_short();
        int  num = read_byte();
        int  den = pow(2.0, read_byte());
        read_gap(4);
        Q_EMIT signalWRKTimeSig(measure, num, den);
    }
}

void
wrkfile::MeterKey_chunk()
{
    int count = read_short();
    for (int i = 0; i < count; ++i)
    {
        int measure = read_short();
        int  num = read_byte();
        int  den = pow(2.0, read_byte());
        midibyte alt = read_byte();
        Q_EMIT signalWRKTimeSig(measure, num, den);
        Q_EMIT signalWRKKeySig(measure, alt);
    }
}

double
wrkfile::get_real_time (long ticks) const
{
    double division = 1.0 * m_d->m_division;
    RecTempo last;
    last.time = 0;
    last.tempo = 100.0;
    last.seconds = 0.0;
    if (!d->m_tempos.isEmpty())
    {
        foreach (const RecTempo& rec, m_d->m_tempos)
        {
            if (rec.time >= ticks)
                break;

            last = rec;
        }
    }
    return last.seconds + (((ticks - last.time) / division) * (60.0 / last.tempo));
}

void
wrkfile::Tempo_chunk(int factor)
{
    double division = 1.0 * m_d->m_division;
    int count = read_short();
    RecTempo last, next;
    for (int i = 0; i < count; ++i)
    {

        long time = read_long();
        read_gap(4);
        long tempo = read_short() * factor;
        read_gap(8);

        next.time = time;
        next.tempo = tempo / 100.0;
        next.seconds = 0.0;
        last.time = 0;
        last.tempo = next.tempo;
        last.seconds = 0.0;
        if (! m_d->m_tempos.isEmpty())
        {
            foreach (const RecTempo& rec, m_d->m_tempos)
            {
                if (rec.time >= time)
                    break;

                last = rec;
            }
            next.seconds = last.seconds +
                           (((time - last.time) / division) * (60.0 / last.tempo));
        }
        m_d->m_tempos.append(next);

        Q_EMIT signalWRKTempo(time, tempo);
    }
}

void
wrkfile::Sysex_chunk()
{
    int j;
    std::string name;
    QByteArray data;
    int bank = read_byte();
    int length = read_short();
    bool autosend = (read_byte() != 0);
    int namelen = read_byte();
    name = readString(namelen);
    for (j = 0; j < length; ++j)
    {
        int byte = read_byte();
        data += byte;
    }
    Q_EMIT signalWRKSysex(bank, name, autosend, 0, data);
}

void
wrkfile::Sysex2_chunk()
{
    int j;
    std::string name;
    QByteArray data;
    int bank = read_short();
    int length = read_long();
    midibyte b = read_byte();
    int port = (b & 0xf0) >> 4;
    bool autosend = ((b & 0x0f) != 0);
    int namelen = read_byte();
    name = readString(namelen);
    for (j = 0; j < length; ++j)
    {
        int byte = read_byte();
        data += byte;
    }
    Q_EMIT signalWRKSysex(bank, name, autosend, port, data);
}

void
wrkfile::NewSysex_chunk()
{
    int j;
    std::string name;
    QByteArray data;
    int bank = read_short();
    int length = read_long();
    int port = read_short();
    bool autosend = (read_byte() != 0);
    int namelen = read_byte();
    name = readString(namelen);
    for (j = 0; j < length; ++j)
    {
        int byte = read_byte();
        data += byte;
    }
    Q_EMIT signalWRKSysex(bank, name, autosend, port, data);
}

/**
 *  Handles reading an Extended Thru parameters chunk.  This item was introduced
 *  in Cakewalk version 4.0.  These parameters are intended to override the global
 *  varisbles' ThruOn value, so this record should come after the WC_VARS_CHUNK
 *  record. It is optional.
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
wrkfile::Thru_chunk()
{
    read_gap(2);
    midibyte port = read_byte();    // 0->127
    midibyte channel = read_byte(); // -1, 0->15
    midibyte keyPlus = read_byte(); // 0->127
    midibyte velPlus = read_byte(); // 0->127
    midibyte localPort = read_byte();
    midibyte mode = read_byte();
    Q_EMIT signalWRKThru(mode, port, channel, keyPlus, velPlus, localPort);
}

void
wrkfile::TrackOffset()
{
    midishort track = read_short();
    midishort offset = read_short();
    Q_EMIT signalWRKTrackOffset(track, offset);
}

void
wrkfile::TrackReps()
{
    midishort track = read_short();
    midishort reps = read_short();
    Q_EMIT signalWRKTrackReps(track, reps);
}

void
wrkfile::TrackPatch()
{
    midishort track = read_short();
    midibyte patch = read_byte();
    Q_EMIT signalWRKTrackPatch(track, patch);
}

void
wrkfile::TimeFormat()
{
    midishort fmt = read_short();
    midishort ofs = read_short();
    Q_EMIT signalWRKTimeFormat(fmt, ofs);
}

void
wrkfile::Comments()
{
    int len = read_short();
    std::string text = readString(len);
    Q_EMIT signalWRKComments(text);
}

void
wrkfile::processVariableRecord(int max)
{
    int datalen = max - 32;
    QByteArray data;
    std::string name = read_var_string();
    read_gap(31 - name.length());
    for (int i = 0; i < datalen; ++i)
        data += read_byte();
    Q_EMIT signalWRKVariableRecord(name, data);
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
wrkfile::Unknown(int id)
{
    // Q_EMIT signalWRKUnknownChunk(id, m_d->m_lastChunkData);
}

void
wrkfile::NewTrack()
{
    midishort bank = midishort(-1);
    midishort patch = midishort(-1);
    // midishort vol = midishort(-1);
    // midishort pan = midishort(-1);
    midibyte key = midibyte(-1);
    midibyte vel = 0;
    midibyte port = 0;
    midibyte channel = 0;
    bool selected = false;
    bool muted = false;
    bool loop = false;
    midishort track = read_short();
    midibyte len = read_byte();
    std::string name = readString(len);
    bank = read_short();
    patch = read_short();
    /*vol =*/ read_short();
    /*pan =*/ read_short();
    key = read_byte();
    vel = read_byte();
    read_gap(7);
    port = read_byte();
    channel = read_byte();
    muted = (read_byte() != 0);
    Q_EMIT signalWRKNewTrack(name, track, channel, key, vel, port, selected, muted, loop);
    if (bank > -1)
        Q_EMIT signalWRKTrackBank(track, bank);
    if (patch > -1)
    {
        if (channel > -1)
            Q_EMIT signalWRKProgram(track, 0, channel, patch);
        else
            Q_EMIT signalWRKTrackPatch(track, patch);
    }
}

void
wrkfile::SoftVer()
{
    int len = read_byte();
    std::string vers = readString(len);
    Q_EMIT signalWRKSoftVer(vers);
}

void
wrkfile::TrackName()
{
    int track = read_short();
    int len = read_byte();
    std::string name = readString(len);
    Q_EMIT signalWRKTrackName(track, name);
}

void
wrkfile::StringTable()
{
    std::stringList table;
    int rows = read_short();
    for (int i = 0; i < rows; ++i)
    {
        int len = read_byte();
        std::string name = readString(len);
        int idx = read_byte();
        table.insert(idx, name);
    }
    Q_EMIT signalWRKStringTable(table);
}

void
wrkfile::LyricsStream()
{
    midishort track = read_short();
    int events = read_long();
    processNoteArray(track, events);
}

void
wrkfile::TrackVol()
{
    midishort track = read_short();
    int vol = read_short();
    Q_EMIT signalWRKTrackVol(track, vol);
}

void
wrkfile::NewTrackOffset()
{
    midishort track = read_short();
    int offset = read_long();
    Q_EMIT signalWRKTrackOffset(track, offset);
}

void
wrkfile::TrackBank()
{
    midishort track = read_short();
    int bank = read_short();
    Q_EMIT signalWRKTrackBank(track, bank);
}

void
wrkfile::Segment_chunk()
{
    std::string name;
    int track = read_short();
    int offset = read_long();
    read_gap(8);
    int len = read_byte();
    name = readString(len);
    read_gap(20);
    Q_EMIT signalWRKSegment(track, offset, name);
    int events = read_long();
    processNoteArray(track, events);
}

void
wrkfile::NewStream()
{
    std::string name;
    int track = read_short();
    int len = read_byte();
    name = readString(len);
    Q_EMIT signalWRKSegment(track, 0, name);
    int events = read_long();
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
wrkfile::End_chunk()
{
    // emit signalWRKEnd();
}

int
wrkfile::read_chunk ()
{
    int ck = read_byte();
    if (ck != WC_WC_END_CHUNK)
    {
        int ck_len = read_long();
        size_t start_pos = get_file_pos();
        size_t final_pos = start_pos + ck_len;
        read_raw_data(ck_len);
        seek(start_pos);                    // TODO: check the return value
        switch (ck)
        {
        case WC_WC_TRACK_CHUNK:
            Track_chunk();
            break;

        case WC_WC_VARS_CHUNK:
            Vars_chunk();
            break;

        case WC_WC_TIMEBASE_CHUNK:
            Timebase_chunk();
            break;

        case WC_WC_STREAM_CHUNK:
            Stream_chunk();
            break;

        case WC_WC_METER_CHUNK:
            Meter_chunk();
            break;

        case WC_WC_TEMPO_CHUNK:
            Tempo_chunk(100);
            break;

        case WC_WC_NTEMPO_CHUNK:
            Tempo_chunk();
            break;

        case WC_WC_SYSEX_CHUNK:
            Sysex_chunk();
            break;

        case WC_WC_THRU_CHUNK:
            Thru_chunk();
            break;

        case WC_WC_TRKOFFS_CHUNK:
            TrackOffset();
            break;

        case WC_WC_TRKREPS_CHUNK:
            TrackReps();
            break;

        case WC_WC_TRKPATCH_CHUNK:
            TrackPatch();
            break;

        case WC_WC_TIMEFMT_CHUNK:
            TimeFormat();
            break;

        case WC_WC_COMMENTS_CHUNK:
            Comments();
            break;

        case WC_WC_VARIABLE_CHUNK:
            processVariableRecord(ck_len);
            break;

        case WC_WC_NTRACK_CHUNK:
            NewTrack();
            break;

        case WC_WC_SOFTVER_CHUNK:
            SoftVer();
            break;

        case WC_WC_TRKNAME_CHUNK:
            TrackName();
            break;

        case WC_WC_STRTAB_CHUNK:
            StringTable();
            break;

        case WC_WC_LYRICS_CHUNK:
            LyricsStream();
            break;

        case WC_WC_TRKVOL_CHUNK:
            TrackVol();
            break;

        case WC_WC_NTRKOFS_CHUNK:
            NewTrackOffset();
            break;

        case WC_WC_TRKBANK_CHUNK:
            TrackBank();
            break;

        case WC_WC_METERKEY_CHUNK:
            MeterKey_chunk();
            break;

        case WC_SYSEX2_CHUNK:
            Sysex2_chunk();
            break;

        case WC_WC_NSYSEX_CHUNK:
            NewSysex_chunk();
            break;

        case WC_WC_SGMNT_CHUNK:
            Segment_chunk();
            break;

        case WC_WC_NSTREAM_CHUNK:
            NewStream();
            break;

        default:
            Unknown(ck);
            break;
        }
        seek(final_pos);                    // TODO: check the return value
    }
    return ck;
}

}        // namespace seq64

/*
 * wrkfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

