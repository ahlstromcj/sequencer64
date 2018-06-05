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
        midibyte c = 0xff;
        QByteArray data;
        for (int i = 0; i < len && c != 0; ++i)
        {
            c = read_byte();
            if (c != 0)
                data += c;
        }
        if (d->m_codec == NULL)
            s = std::string(data);
        else
            s = m_d->m_codec->toUnicode(data);
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
    QByteArray data;
    midibyte b;
    do
    {
        b = read_byte();
        if (b != 0)
            data += b;
    }
    while (b != 0);
    if (d->m_codec == NULL)
        s = std::string(data);
    else
        s = m_d->m_codec->toUnicode(data);
    return s;
}

/**
 *  Current position in the data stream.
 *
 * \return current position
 */

long
wrkfile::get_file_pos()
{
    return m_d->m_IOStream->device()->pos();
}

/**
 *  Seeks to a new position in the data stream.
 *
 * \param pos new position
 */

void
wrkfile::seek (long pos)
{
    m_d->m_IOStream->device()->seek(pos);
}

/**
 *  Checks if the data stream pointer has reached the end position
 *
 * \return true if the read pointer is at end
 */

bool wrkfile::at_end ()
{
    return false;   // m_d->m_IOStream->atEnd();
}

/**
 * Jumps the given size in the data stream
 * \param size the gap size
 */

void
wrkfile::read_gap (int size)
{
    if (size > 0)
        seek(get_file_pos() + size);
}

/**
 * Reads a stream.
 * \param stream Pointer to an existing and opened stream
 */
void wrkfile::readFromStream(QDataStream *stream)
{
    m_d->m_IOStream = stream;
    wrkRead();
}

/**
 * Reads a stream from a disk file.
 * \param fileName Name of an existing file.
 */
void wrkfile::readFromFile(const std::string& fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QDataStream ds(&file);
    readFromStream(&ds);
    file.close();
}

void
wrkfile::processTrackChunk()
{
    int namelen;
    std::string name[2];
    int trackno;
    int channel;
    int pitch;
    int velocity;
    int port;
    bool selected;
    bool muted;
    bool loop;

    trackno = read_short();
    for (int i = 0; i < 2; ++i)
    {
        namelen = read_byte();
        name[i] = readString(namelen);
    }
    channel = (qint8) read_byte();
    pitch = read_byte();
    velocity = read_byte();
    port = read_byte();
    midibyte flags = read_byte();
    selected = ((flags & 1) != 0);
    muted = ((flags & 2) != 0);
    loop = ((flags & 4) != 0);
    Q_EMIT signalWRKTrack(name[0], name[1],
                          trackno, channel, pitch,
                          velocity, port, selected,
                          muted, loop);
}

void
wrkfile::processVarsChunk()
{
    m_d->m_Now = read_long();
    m_d->m_From = read_long();
    m_d->m_Thru = read_long();
    m_d->m_KeySig = read_byte();
    m_d->m_Clock = read_byte();
    m_d->m_AutoSave = read_byte();
    m_d->m_PlayDelay = read_byte();
    readGap(1);
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
    readGap(2);
    m_d->m_ThruOn = (read_byte() != 0);
    readGap(19);
    m_d->m_AutoRestart = (read_byte() != 0);
    m_d->m_CurTempoOfs = read_byte();
    m_d->m_TempoOfs1 = read_byte();
    m_d->m_TempoOfs2 = read_byte();
    m_d->m_TempoOfs3 = read_byte();
    readGap(2);
    m_d->m_PunchEnabled = (read_byte() != 0);
    m_d->m_PunchInTime = read_long();
    m_d->m_PunchOutTime = read_long();
    m_d->m_EndAllTime = read_long();

    Q_EMIT signalWRKGlobalVars();
}

void wrkfile::processTimebaseChunk()
{
    midishort timebase = read_short();
    m_d->m_division = timebase;
    Q_EMIT signalWRKTimeBase(timebase);
}

void wrkfile::processNoteArray(int track, int events)
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
            readGap(4);
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

void wrkfile::processStreamChunk()
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

void wrkfile::processMeterChunk()
{
    int count = read_short();
    for (int i = 0; i < count; ++i)
    {
        readGap(4);
        int measure = read_short();
        int  num = read_byte();
        int  den = pow(2.0, read_byte());
        readGap(4);
        Q_EMIT signalWRKTimeSig(measure, num, den);
    }
}

void wrkfile::processMeterKeyChunk()
{
    int count = read_short();
    for (int i = 0; i < count; ++i)
    {
        int measure = read_short();
        int  num = read_byte();
        int  den = pow(2.0, read_byte());
        qint8 alt = read_byte();
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

void wrkfile::processTempoChunk(int factor)
{
    double division = 1.0 * m_d->m_division;
    int count = read_short();
    RecTempo last, next;
    for (int i = 0; i < count; ++i)
    {

        long time = read_long();
        readGap(4);
        long tempo = read_short() * factor;
        readGap(8);

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

void wrkfile::processSysexChunk()
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

void wrkfile::processSysex2Chunk()
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

void wrkfile::processNewSysexChunk()
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

void wrkfile::processThruChunk()
{
    readGap(2);
    qint8 port = read_byte();    // 0->127
    qint8 channel = read_byte(); // -1, 0->15
    qint8 keyPlus = read_byte(); // 0->127
    qint8 velPlus = read_byte(); // 0->127
    qint8 localPort = read_byte();
    qint8 mode = read_byte();
    Q_EMIT signalWRKThru(mode, port, channel, keyPlus, velPlus, localPort);
}

void wrkfile::processTrackOffset()
{
    midishort track = read_short();
    qint16 offset = read_short();
    Q_EMIT signalWRKTrackOffset(track, offset);
}

void wrkfile::processTrackReps()
{
    midishort track = read_short();
    midishort reps = read_short();
    Q_EMIT signalWRKTrackReps(track, reps);
}

void wrkfile::processTrackPatch()
{
    midishort track = read_short();
    qint8 patch = read_byte();
    Q_EMIT signalWRKTrackPatch(track, patch);
}

void wrkfile::processTimeFormat()
{
    midishort fmt = read_short();
    midishort ofs = read_short();
    Q_EMIT signalWRKTimeFormat(fmt, ofs);
}

void wrkfile::processComments()
{
    int len = read_short();
    std::string text = readString(len);
    Q_EMIT signalWRKComments(text);
}

void wrkfile::processVariableRecord(int max)
{
    int datalen = max - 32;
    QByteArray data;
    std::string name = read_var_string();
    readGap(31 - name.length());
    for (int i = 0; i < datalen; ++i)
        data += read_byte();
    Q_EMIT signalWRKVariableRecord(name, data);
}

void wrkfile::processUnknown(int id)
{
    Q_EMIT signalWRKUnknownChunk(id, m_d->m_lastChunkData);
}

void wrkfile::processNewTrack()
{
    qint16 bank = -1;
    qint16 patch = -1;
    //qint16 vol = -1;
    //qint16 pan = -1;
    qint8 key = -1;
    qint8 vel = 0;
    midibyte port = 0;
    qint8 channel = 0;
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
    readGap(7);
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

void wrkfile::processSoftVer()
{
    int len = read_byte();
    std::string vers = readString(len);
    Q_EMIT signalWRKSoftVer(vers);
}

void wrkfile::processTrackName()
{
    int track = read_short();
    int len = read_byte();
    std::string name = readString(len);
    Q_EMIT signalWRKTrackName(track, name);
}

void wrkfile::processStringTable()
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

void wrkfile::processLyricsStream()
{
    midishort track = read_short();
    int events = read_long();
    processNoteArray(track, events);
}

void wrkfile::processTrackVol()
{
    midishort track = read_short();
    int vol = read_short();
    Q_EMIT signalWRKTrackVol(track, vol);
}

void wrkfile::processNewTrackOffset()
{
    midishort track = read_short();
    int offset = read_long();
    Q_EMIT signalWRKTrackOffset(track, offset);
}

void wrkfile::processTrackBank()
{
    midishort track = read_short();
    int bank = read_short();
    Q_EMIT signalWRKTrackBank(track, bank);
}

void wrkfile::processSegmentChunk()
{
    std::string name;
    int track = read_short();
    int offset = read_long();
    readGap(8);
    int len = read_byte();
    name = readString(len);
    readGap(20);
    Q_EMIT signalWRKSegment(track, offset, name);
    int events = read_long();
    processNoteArray(track, events);
}

void wrkfile::processNewStream()
{
    std::string name;
    int track = read_short();
    int len = read_byte();
    name = readString(len);
    Q_EMIT signalWRKSegment(track, 0, name);
    int events = read_long();
    processNoteArray(track, events);
}

void
wrkfile::processEndChunk()
{
    emit signalWRKEnd();
}

int
wrkfile::read_chunk ()
{
    long start_pos, final_pos;
    int ck_len, ck = read_byte();
    if (ck != WC_WC_END_CHUNK)
    {
        ck_len = read_long();
        start_pos = get_file_pos();
        final_pos = start_pos + ck_len;
        read_raw_data(ck_len);
        seek(start_pos);
        switch (ck)
        {
        case WC_WC_TRACK_CHUNK:
            processTrackChunk();
            break;
        case WC_WC_VARS_CHUNK:
            processVarsChunk();
            break;
        case WC_WC_TIMEBASE_CHUNK:
            processTimebaseChunk();
            break;
        case WC_WC_STREAM_CHUNK:
            processStreamChunk();
            break;
        case WC_WC_METER_CHUNK:
            processMeterChunk();
            break;
        case WC_WC_TEMPO_CHUNK:
            processTempoChunk(100);
            break;
        case WC_WC_NTEMPO_CHUNK:
            processTempoChunk();
            break;
        case WC_WC_SYSEX_CHUNK:
            processSysexChunk();
            break;
        case WC_WC_THRU_CHUNK:
            processThruChunk();
            break;
        case WC_WC_TRKOFFS_CHUNK:
            processTrackOffset();
            break;
        case WC_WC_TRKREPS_CHUNK:
            processTrackReps();
            break;
        case WC_WC_TRKPATCH_CHUNK:
            processTrackPatch();
            break;
        case WC_WC_TIMEFMT_CHUNK:
            processTimeFormat();
            break;
        case WC_WC_COMMENTS_CHUNK:
            processComments();
            break;
        case WC_WC_VARIABLE_CHUNK:
            processVariableRecord(ck_len);
            break;
        case WC_WC_NTRACK_CHUNK:
            processNewTrack();
            break;
        case WC_WC_SOFTVER_CHUNK:
            processSoftVer();
            break;
        case WC_WC_TRKNAME_CHUNK:
            processTrackName();
            break;
        case WC_WC_STRTAB_CHUNK:
            processStringTable();
            break;
        case WC_WC_LYRICS_CHUNK:
            processLyricsStream();
            break;
        case WC_WC_TRKVOL_CHUNK:
            processTrackVol();
            break;
        case WC_WC_NTRKOFS_CHUNK:
            processNewTrackOffset();
            break;
        case WC_WC_TRKBANK_CHUNK:
            processTrackBank();
            break;
        case WC_WC_METERKEY_CHUNK:
            processMeterKeyChunk();
            break;
        case WC_SYSEX2_CHUNK:
            processSysex2Chunk();
            break;
        case WC_WC_NSYSEX_CHUNK:
            processNewSysexChunk();
            break;
        case WC_WC_SGMNT_CHUNK:
            processSegmentChunk();
            break;
        case WC_WC_NSTREAM_CHUNK:
            processNewStream();
            break;
        default:
            processUnknown(ck);
        }
        seek(final_pos);
    }
    return ck;
}

void wrkfile::wrkRead()
{
    int vma, vme;
    int ck_id;
    QByteArray hdr(HEADER.length(), ' ');
    m_d->m_tempos.clear();
    m_d->m_IOStream->device()->read(hdr.data(), HEADER.length());
    if (hdr == HEADER)
    {
        readGap(1);
        vme = read_byte();
        vma = read_byte();
        Q_EMIT signalWRKHeader(vma, vme);
        do
        {
            ck_id = read_chunk();
        }
        while (ck_id != WC_WC_END_CHUNK);
        if (!at_end())
            Q_EMIT signalWRKError("Corrupted file");
        else
            processEndChunk();
    }
    else
        Q_EMIT signalWRKError("Invalid file format");
}

}        // namespace seq64

/*
 * wrkfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

