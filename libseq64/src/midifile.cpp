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
 * \file          midifile.cpp
 *
 *  This module declares/defines the base class for MIDI files.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-02-10
 * \license       GNU GPLv2 or above
 *
 *  For a quick guide to the MIDI format, see, for example:
 *
 *  http://www.mobilefish.com/tutorials/midi/midi_quickguide_specification.html
 *
 *  It is important to note that most sequencers have taken a shortcut or
 *  two in reading the MIDI format.  For example, most will silently
 *  ignored an unadorned control tag (0x242400nn) which has not been
 *  package up as a proper sequencer-specific meta event.  The midicvt
 *  program (https://github.com/ahlstromcj/midicvt, derived from midicomp,
 *  midi2text, and mf2t/t2mf) does not ignore this lack, and hence we
 *  decided to provide a new, more strict input and output format for the
 *  the proprietary track in Sequencer24.
 */

#include <fstream>

#include "calculations.hpp"             /* beats_per_minute_from_tempo()    */
#include "perform.hpp"                  /* must precede midifile.hpp !      */
#include "midifile.hpp"                 /* seq64::midifile                  */
#include "sequence.hpp"                 /* seq64::sequence                  */

#define SEQ64_USE_MIDI_VECTOR           /* as opposed to the MIDI list      */
#if defined SEQ64_USE_MIDI_VECTOR
#include "midi_vector.hpp"              /* seq64::midi_vector container     */
#else
#include "midi_list.hpp"                /* seq64::midi_list container       */
#endif

/**
 *  A manifest constant for controlling the length of a line-reading
 *  array in a configuration file.
 */

#define SEQ64_MIDI_LINE_MAX         1024

/**
 *  The maximum length of a Seq24 track name.  This is a bit excessive.
 */

#define TRACKNAME_MAX                256

/**
 *  Highlights the MIDI file header value, "MThd".
 */

#define SEQ64_HEADER_TAG            0x4D546864      /* magic number 'MThd'  */

/**
 *  Highlights the MIDI file track-marker (chunk) value, "MTrk".
 */

#define SEQ64_TRACK_TAG             0x4D54726B      /* magic number 'MTrk'  */

/**
 *  The chunk header value for the Sequencer64 proprietary section.
 *  We might try other chunks, as well, since, as per the MIDI
 *  specification, unknown chunks should not cause an error in a sequencer
 *  (or our midicvt program).  For now, we stick with "MTrk".
 */

#define PROPRIETARY_CHUNK_TAG       SEQ64_TRACK_TAG

/**
 *  Provides the sequence number for the proprietary data when using the new
 *  format.  (There is no sequence number for the legacy format.)  Can't use
 *  numbers, such as 0xFFFF, that have MIDI meta tags in them, confuses
 *  our "proprietary" track parser.  This sequence number, 0x7777, is neither
 *  a valid nor legal sequence number.  No real sequence will ever have this
 *  number in Sequencer64.
 */

#define PROPRIETARY_SEQ_NUMBER      0x7777

/**
 *  Provides the track name for the "proprietary" data when using the new
 *  format.  (There is no track-name for the "proprietary" footer track when
 *  the legacy format is in force.)
 */

#define PROPRIETARY_TRACK_NAME      "Sequencer64-S"

namespace seq64
{

/**
 *  Principal constructor.
 *
 * \param name
 *      Provides the name of the MIDI file to be read or written.
 *
 * \param ppqn
 *      Provides the initial value of the PPQN setting.  It is handled
 *      differently for parsing (reading) versus writing the MIDI file.
 *      -   Reading.
 *          -   If set to SEQ64_USE_DEFAULT_PPQN, the legacy application
 *              behavior is used.  The m_ppqn member is set to the default
 *              PPQN, DEFAULT_PPQN.  The value read from the MIDI
 *              file, ppqn, is then use to scale the running-time of the
 *              sequence relative to DEFAULT_PPQN.
 *          -   Otherwise, m_ppqn is set to the value read from the MIDI file.
 *              No scaling is done.  Since the value gets written, specify
 *              ppqn as 0, an obviously bogus value, to get this behavior.
 *      -   Writing.  This value is written to the MIDI file in the header
 *          chunk of the song.  Note that the caller must query for the
 *          PPQN set during parsing, and pass it to the constructor when
 *          preparing to write the file.  See how it is done in the mainwnd
 *          class.
 *
 * \param oldformat
 *      If true, write out the MIDI file using the old Seq24 format, instead
 *      of the new MIDI-compliant sequencer-specific format, for the
 *      seq24-specific SeqSpec tags defined in the globals module.  This
 *      option is false by default.  Note that this option is only used in
 *      writing; reading can handle either format transparently.
 *
 * \param globalbgs
 *      If true, write any non-default values of the key, scale, and
 *      background sequence to the global "proprietary" section of the MIDI
 *      file, instead of to each sequence.  Note that this option is only used
 *      in writing; reading can handle either format transparently.
 */

midifile::midifile
(
    const std::string & name,
    int ppqn,
    bool oldformat,
    bool globalbgs
) :
    m_file_size             (0),
    m_error_message         (),
    m_error_is_fatal        (false),
    m_disable_reported      (false),
    m_pos                   (0),
    m_name                  (name),
    m_data                  (),
    m_char_list             (),
    m_new_format            (! oldformat),
    m_global_bgsequence     (globalbgs),
    m_ppqn                  (0),
    m_use_default_ppqn      (ppqn == SEQ64_USE_DEFAULT_PPQN),
    m_smf0_splitter         (ppqn)
{
    m_ppqn = choose_ppqn(ppqn);
}

/**
 *  A rote destructor.
 */

midifile::~midifile ()
{
    // empty body
}

/**
 *  Reads 4 bytes of data using read_byte().
 *
 * \warning
 *      This code looks endian-dependent and integer-size dependent.
 */

midilong
midifile::read_long ()
{
    midilong result = 0;
    result += (read_byte() << 24);
    result += (read_byte() << 16);
    result += (read_byte() << 8);
    result += (read_byte());
    return result;
}

/**
 *  Reads 2 bytes of data using read_byte().
 */

midishort
midifile::read_short ()
{
    midishort result = 0;
    result += (read_byte() << 8);
    result += (read_byte());
    return result;
}

/**
 *  Reads 1 byte of data directly from the m_data vector, incrementing
 *  m_pos after doing so.
 */

midibyte
midifile::read_byte ()
{
    if (m_pos < m_file_size)
    {
        return m_data[m_pos++];
    }
    else if (! m_disable_reported)
    {
        errdump("'End-of-file', further MIDI reading disabled");
        m_disable_reported = true;
    }
    return 0;
}

/**
 *  Read a MIDI Variable-Length Value (VLV), which has a variable number
 *  of bytes.  This function reads the bytes while bit 7 is set in each
 *  byte.  Bit 7 is a continuation bit.  See write_varinum() for more
 *  information.
 */

midilong
midifile::read_varinum ()
{
    midilong result = 0;
    midibyte c;
    while (((c = read_byte()) & 0x80) != 0x00)      /* while bit 7 is set  */
    {
        result <<= 7;                               /* shift result 7 bits */
        result += (c & 0x7F);                       /* add bits 0-6        */
    }
    result <<= 7;                                   /* bit was clear       */
    result += (c & 0x7F);
    return result;
}

/**
 *  This function opens a binary MIDI file and parses it into sequences
 *  and other application objects.
 *
 *  In addition to the standard MIDI track data in a normal track, Seq24
 *  adds four sequencer-specific events just before the end of the track:
 *
\verbatim
    c_triggers_new:     SeqSpec FF 7F 1C 24 24 00 08 00 00 ...
    c_midibus:          SeqSpec FF 7F 05 24 24 00 01 00
    c_timesig:          SeqSpec FF 7F 06 24 24 00 06 04 04
    c_midich:           SeqSpec FF 7F 05 24 24 00 02 06
\endverbatim
 *
 *  Standard MIDI provides for the port and channel specifications, but
 *  they are apparently considered obsolete:
 *
 *  Obsolete meta-event:                Replacement:
 *
\verbatim
    MIDI port (buss):   FF 21 01 po     Device (port) name: FF 09 len text
    MIDI channel:       FF 20 01 ch
\endverbatim
 *
 *  What do other applications use for specifying port/channel?
 *
 *  Note on the is-modified flag.  We now assume that the perform object is
 *  starting from scratch when parsing.  But we let mainwnd tell the perform
 *  object when to clear everything with perform::clear_all().  The mainwnd
 *  does this for a new file, opening a file, but not for a file import, which
 *  might be done simply to add more MIDI tracks to the current composition.
 *  So, if parsing succeeds, all we want to do is make sure the flag is set.
 *
 *  Parsing a file successfully is not always a modification of the setup.
 *  For instance, the first read of a MIDI file should start clean, not dirty.
 *
 * SysEx notes:
 *
 *      Some files (e.g. Dixie04.mid) do not always encode System Exclusive
 *      messages properly for a MIDI file.  Instead of a varinum length value,
 *      they are followed by extended IDs (0x7D, 0x7E, or 0x7F).
 *
 *      We've covered some of those cases by disabling access to m_data if the
 *      position passes the size of the file, but we want try to bypass these
 *      odd cases properly.  So we look ahead for one of these special values.
 *
 * \param p
 *      Provides a reference to the perform object into which sequences/tracks
 *      are to be added.
 *
 * \param screenset
 *      The screen-set offset to be used when loading a sequence (track) from
 *      the file.  This value ranges from -31 to 0 to +31 (32 is the maximum
 *      screen-set available in Seq24).  This offset is added to the sequence
 *      number read in for the sequence, to place it elsewhere in the imported
 *      tune, and locate it in a specific screen-set.  If this parameter is
 *      non-zero, the we will assume that the perform data is dirty.
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
midifile::parse (perform & p, int screenset)
{
    bool result = true;
    std::ifstream file
    (
        m_name.c_str(), std::ios::in | std::ios::binary | std::ios::ate
    );
    m_error_is_fatal = false;
    if (! file.is_open())
    {
        m_error_is_fatal = true;
        m_error_message = "Error opening MIDI file '";
        m_error_message += m_name;
        m_error_message += "'";
        errprint(m_error_message.c_str());
        return false;
    }

    int file_size = file.tellg();                   /* get end offset       */
    if (file_size <= 0)
    {
        m_error_is_fatal = true;
        m_error_message =
            "Invalid file size... are you trying to read a directory?";

        errprint(m_error_message.c_str());
        return false;
    }
    file.seekg(0, std::ios::beg);                   /* seek to start        */
    try
    {
        m_data.resize(file_size);                   /* allocate more data   */
        m_file_size = file_size;                    /* save for checking    */
    }
    catch (const std::bad_alloc & ex)
    {
        m_error_is_fatal = true;
        m_error_message = "Memory allocation failed in midifile::parse()";
        errprint(m_error_message.c_str());
        return false;
    }
    file.read((char *)(&m_data[0]), file_size);     /* vector == array :-)  */
    file.close();
    m_error_message.clear();
    m_disable_reported = false;
    m_smf0_splitter.initialize();

    midilong ID = read_long();                      /* read hdr chunk info  */
    midilong hdrlength = read_long();               /* stock MThd length    */
    if (ID != SEQ64_HEADER_TAG && hdrlength != 6)   /* magic number 'MThd'  */
    {
        m_error_is_fatal = true;
        errdump("Invalid MIDI header chunk detected", ID);
        return false;
    }

    midishort Format = read_short();                /* 0, 1, or 2           */
    if (Format == 0)
    {
        result = parse_smf_0(p, screenset);
    }
    else if (Format == 1)
    {
        result = parse_smf_1(p, screenset);
    }
    else
    {
        m_error_is_fatal = true;
        errdump("Unsupported MIDI format number", midilong(Format));
        result = false;
    }
    if (result)
    {
        if (file_size > m_pos)                          /* any more left?   */
            result = parse_proprietary_track(p, file_size);

        if (result && screenset != 0)
             p.modify();
    }
    return result;
}

/**
 *  This function parses an SMF 0 binary MIDI file as if it were an SMF 1
 *  file, then, if more than one MIDI channel was encountered in the sequence,
 *  splits all of the channels in the sequence out separate sequences, and
 *  deletes the original sequence.
 *
 * \param p
 *      Provides a reference to the perform object into which sequences/tracks
 *      are to be added.
 *
 * \param screenset
 *      The screen-set offset to be used when loading a sequence (track) from
 *      the file.
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
midifile::parse_smf_0 (perform & p, int screenset)
{
    bool result = parse_smf_1(p, screenset, true);  /* format 0 is flagged  */
    if (result)
    {
        result = m_smf0_splitter.split(p, screenset);
        if (result)
            p.modify();                     /* will prompt user for save    */
        else
            errdump("No SMF 0 main sequence found, bad file");
    }
    return result;
}

/**
 *  Internal function for simple calculation of a power of 2 without a lot of
 *  math.  Use for calculating the denominator of a time signature.
 *
 * \param logbase2
 *      Provides the power to which 2 is to be raised.  This integer is
 *      probably only rarely greater than 4 (which represents a denominator of
 *      16).
 *
 * \return
 *      Returns 2 raised to the logbase2 power.
 */

int
midifile::pow2 (int logbase2)
{
    int result;
    if (logbase2 == 0)
        result = 1;
    else
    {
        result = 2;
        for (int c = 1; c < logbase2; c++) 
            result *= 2;
    }
    return result;
}

/**
 *  Internal function to check for and report a bad length value.
 */

bool
midifile::checklen (midilong len, midibyte type)
{
    bool result = len > 0;
    if (! result)
    {
        char tmp[64];
        snprintf
        (
            tmp, sizeof tmp,
            "0 data length for meta type 0x%02X, bad MIDI data", type
        );
        errdump(tmp);
    }
    return result;
}

/**
 *  This function parses an SMF 1 binary MIDI file; it is basically the
 *  original seq25 midifile::parse() function.  It assumes the file-data has
 *  already been read into memory.  It also assumes that the ID, track-length,
 *  and format have already been read.
 *
 * \param p
 *      Provides a reference to the perform object into which sequences/tracks
 *      are to be added.
 *
 * \param screenset
 *      The screen-set offset to be used when loading a sequence (track) from
 *      the file.
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
midifile::parse_smf_1 (perform & p, int screenset, bool is_smf0)
{
    bool result = true;
    midishort NumTracks = read_short();
    midishort ppqn = read_short();

    /*
     * We should be good to load now, for each Track in the MIDI file.
     * Note that NumTracks doesn't count the Seq24 "proprietary" footer
     * section, even if it uses the new format, so that section will still
     * be read properly after all normal tracks have been processed.
     */

    char buss_override = usr().midi_buss_override();
    for (int curtrack = 0; curtrack < NumTracks; curtrack++)
    {
        midipulse Delta;                             /* time                 */
        midipulse RunningTime;
        midipulse CurrentTime;
        char TrackName[TRACKNAME_MAX];              /* track name from file */
        midilong ID = read_long();                  /* get track marker     */
        midilong TrackLength = read_long();         /* get track length     */
        if (ID == SEQ64_TRACK_TAG)                  /* magic number 'MTrk'  */
        {
            bool timesig_set = false;               /* seq24 style wins     */
            midishort seqnum = 0;
            midibyte status = 0;
            midibyte d0, d1;                        /* was data[2];         */
            midibyte laststatus;
            midilong proprietary = 0;               /* sequencer-specifics  */
            bool done = false;                      /* done for each track  */
            midilong len;                           /* important counter!   */
            sequence * s = new sequence(m_ppqn);    /* create new sequence  */
            if (s == nullptr)
            {
                errdump("MIDI file parsing: sequence allocation failed");
                return false;
            }
            sequence & seq = *s;                /* references are nicer     */
            seq.set_master_midi_bus(&p.master_bus());   /* set master buss  */
            RunningTime = 0;                    /* reset time               */
            while (! done)                      /* get each event in track  */
            {
                event e;                        /* safer here, if "slower"  */
                Delta = read_varinum();         /* get time delta           */
                laststatus = status;
                status = m_data[m_pos];         /* get status               */
                if ((status & 0x80) == 0x00)    /* is it a status bit ?     */
                    status = laststatus;        /* no, it's running status  */
                else
                    m_pos++;                    /* it's a status, increment */

                e.set_status(status);           /* set the members in event */

                /*
                 * Current time is ppqn according to the file, we have to
                 * adjust it to our own ppqn.  PPQN / ppqn gives us the
                 * ratio.  (This change is not enough; a song with a ppqn of
                 * 120 plays too fast in Seq24, which has a constant ppqn of
                 * 192.)
                 */

                RunningTime += Delta;           /* add in the time          */
                if (m_use_default_ppqn)         /* legacy handling of ppqn  */
                {
                    CurrentTime = RunningTime * m_ppqn / ppqn;
                    e.set_timestamp(CurrentTime);
                }
                else
                {
                    CurrentTime = RunningTime;
                    e.set_timestamp(CurrentTime);
                }

                midibyte eventcode = status & EVENT_CLEAR_CHAN_MASK;   /* F0 */
                midibyte channel = status & EVENT_GET_CHAN_MASK;       /* 0F */
                switch (eventcode)
                {
                case EVENT_NOTE_OFF:          /* cases for 2-data-byte events */
                case EVENT_NOTE_ON:
                case EVENT_AFTERTOUCH:
                case EVENT_CONTROL_CHANGE:
                case EVENT_PITCH_WHEEL:

                    d0 = read_byte();                     /* was data[0]      */
                    d1 = read_byte();                     /* was data[1]      */
                    if (eventcode == EVENT_NOTE_ON && d1 == 0)
                        e.set_status(EVENT_NOTE_OFF, channel); /* vel 0==off  */

                    e.set_data(d0, d1);                   /* set data and add */
                    seq.add_event(e);
                    seq.set_midi_channel(channel);        /* set midi channel */
                    if (is_smf0)
                        m_smf0_splitter.increment(channel);
                    break;

                case EVENT_PROGRAM_CHANGE:    /* cases for 1-data-byte events */
                case EVENT_CHANNEL_PRESSURE:

                    d0 = read_byte();                     /* was data[0]      */
                    e.set_data(d0);                       /* set data and add */
                    seq.add_event(e);
                    seq.set_midi_channel(channel);        /* set midi channel */
                    if (is_smf0)
                        m_smf0_splitter.increment(channel);
                    break;

                case 0xF0:                                /* Meta MIDI events */

                    if (status == 0xFF)
                    {
                        midibyte type = read_byte();      /* get meta type    */
                        len = read_varinum();             /* if 0 catch later */
                        switch (type)
                        {
                        case 0x7F:                        /* "proprietary"    */

                            if (len > 4)                  /* FF 7F len data   */
                            {
                                proprietary = read_long();
                                len -= 4;
                            }
                            else if (! checklen(len, type))
                                return false;

                            if (proprietary == c_midibus)
                            {
                                seq.set_midi_bus(read_byte());
                                --len;
                            }
                            else if (proprietary == c_midich)
                            {
                                midibyte channel = read_byte();
                                seq.set_midi_channel(channel);
                                if (is_smf0)
                                    m_smf0_splitter.increment(channel);

                                --len;
                            }
                            else if (proprietary == c_timesig)
                            {
                                timesig_set = true;
                                seq.set_beats_per_bar(read_byte());
                                seq.set_beat_width(read_byte());
                                len -= 2;
                            }
                            else if (proprietary == c_triggers)
                            {
                                printf("Old-style triggers event encountered\n");
                                int num_triggers = len / 4; /* why not 8??? */
                                for (int i = 0; i < num_triggers; i += 2)
                                {
                                    midilong on = read_long();
                                    midilong length = read_long() - on;
                                    len -= 8;
                                    seq.add_trigger(on, length, 0, false);
                                }
                            }
                            else if (proprietary == c_triggers_new)
                            {
                                /*
                                 *  If m_ppqn isn't set to the default value,
                                 *  then we should scale these triggers
                                 *  accordingly, just as is done for the MIDI
                                 *  events!  Should split this out into
                                 *  functions, too.  Too much indenting.
                                 */

                                int num_triggers = len / 12;
                                if (m_use_default_ppqn)
                                {
                                    for (int i = 0; i < num_triggers; i++)
                                    {
                                        midilong on = read_long();
                                        on = on * m_ppqn / ppqn;
                                        midilong off = read_long();
                                        off = off * m_ppqn / ppqn;
                                        midilong length = off - on + 1;
                                        midilong offset = read_long();
                                        offset = offset * m_ppqn / ppqn;
                                        len -= 12;
                                        seq.add_trigger(on, length, offset, false);
                                    }
                                }
                                else
                                {
                                    for (int i = 0; i < num_triggers; i++)
                                    {
                                        midilong on = read_long();
                                        midilong off = read_long();
                                        midilong length = off - on + 1;
                                        midilong offset = read_long();
                                        len -= 12;
                                        seq.add_trigger(on, length, offset, false);
                                    }
                                }
                            }
                            else if (proprietary == c_musickey)
                            {
                                seq.musical_key(read_byte());
                                --len;
                            }
                            else if (proprietary == c_musicscale)
                            {
                                seq.musical_scale(read_byte());
                                --len;
                            }
                            else if (proprietary == c_backsequence)
                            {
                                seq.background_sequence(int(read_long()));
                                len -= 4;
                            }
                            else if (SEQ64_IS_PROPTAG(proprietary))
                            {
                                errdump
                                (
                                    "Unsupported track SeqSpec, skipping...",
                                    proprietary
                                );
                            }
                            m_pos += len;               /* eat the rest     */
                            break;

                        case 0x58:                      /* Time Signature   */

                            if (! checklen(len, type))
                                return false;

                            if ((len == 4) && ! timesig_set)
                            {
                                int logbase2 = int(read_byte());    // dd
                                long bw = long(pow2(logbase2));
                                int bpm = int(read_byte());         // nn
#ifdef SEQ64_HANDLE_TIMESIG_AND_TEMPO
                                int cc = read_byte();               // cc
                                int bb = read_byte();               // bb
                                seq.set_beats_per_bar(bpm);
                                seq.set_beat_width(bw);
                                seq.clocks_per_metronome(cc);
                                seq.set_32nds_per_quarter(bb);

                                /*
                                 * @new ca 2016-02-10 Log for use with JACK.
                                 */

                                p.set_beats_per_bar(bpm);
                                p.set_beat_width(bw);
#else
                                (void) read_byte();                 // cc
                                (void) read_byte();                 // bb
                                seq.set_beats_per_bar(bpm);
                                seq.set_beat_width(bw);
#endif

#ifdef SEQ64_USE_DEBUG_OUTPUT
                                printf
                                (
                                    "Time Signature set to %d/%d\n",
                                    int(seq.get_beats_per_bar()),
                                    int(seq.get_beat_width())
                                );
#endif
                            }
                            else
                                m_pos += len;           /* eat it           */
                            break;

                        case 0x51:                      /* Set Tempo        */

                            if (! checklen(len, type))
                                return false;

                            if (len == 3)
                            {
                                unsigned tt = unsigned(read_byte());
                                tt = (tt * 256) + unsigned(read_byte());
                                tt = (tt * 256) + unsigned(read_byte());
                                int bpm = int
                                (
                                    beats_per_minute_from_tempo(double(tt))
                                );
                                p.set_beats_per_minute(bpm);
#ifdef SEQ64_HANDLE_TIMESIG_AND_TEMPO
                                seq.us_per_quarter_note(int(tt));
#endif
#ifdef SEQ64_USE_DEBUG_OUTPUT
                                printf("BPM set to %d\n", bpm);
#endif
                            }
                            else
                                m_pos += len;           /* eat it           */
                            break;

                        case 0x2F:                      /* End of Track     */

                            /*
                             * "If Delta is 0, then another event happened at
                             * the same time as track-end.  Class sequence
                             * discards the last note.  This fixes that.  A
                             * native Seq24 file will always have a Delta >= 1."
                             * Not true!  We've fixed the real issue by
                             * commenting this code:
                             *
                             *  if (Delta == 0)
                             *      ++CurrentTime;
                             */

                            seq.set_length(CurrentTime, false);
                            seq.zero_markers();
                            done = true;
                            break;

                        case 0x03:                      /* Track name       */

                            if (! checklen(len, type))
                                return false;

                            if (len > TRACKNAME_MAX)
                                len = TRACKNAME_MAX;    /* avoid a vuln     */

                            for (int i = 0; i < int(len); i++)
                                TrackName[i] = char(read_byte());

                            TrackName[len] = '\0';
                            seq.set_name(TrackName);
                            break;

                        case 0x00:                      /* sequence number  */

                            if (! checklen(len, type))
                                return false;

                            seqnum = read_short();
                            break;

                        default:

                            if (! checklen(len, type))
                                return false;

                            for (int i = 0; i < int(len); i++)
                                (void) read_byte();     /* ignore the rest  */
                            break;
                        }
                    }
                    else if (status == 0xF0)
                    {
                        /*
                         * Some files do not properly encode SysEx messages;
                         * see the function banner for notes.
                         */

                        midibyte check = read_byte();
                        if (is_sysex_special_id(check))
                        {
                            /*
                             * TMI: errdump("SysEx ID byte = 7D to 7F");
                             */
                        }
                        else                            /* handle normally  */
                        {
                            --m_pos;                    /* put byte back    */
                            len = read_varinum();       /* sysex            */
                            m_pos += len;               /* skip it          */
                            if (m_data[m_pos-1] != 0xF7)
                                errdump("SysEx terminator byte F7 not found");
                        }
                    }
                    else
                    {
                        errdump("Unexpected meta code", midilong(status));
                        return false;
                    }
                    break;

                default:

                    errdump("Unknown MIDI event", midilong(status));
                    return false;
                    break;
                }
            }                          /* while not done loading Trk chunk */

            if (buss_override != SEQ64_BAD_BUSS)
                seq.set_midi_bus(buss_override);

            /* Sequence has been filled, add it to the performance  */

            if (is_smf0)
                (void) m_smf0_splitter.log_main_sequence(seq, seqnum);
            else
                p.add_sequence(&seq, seqnum + (screenset * c_seqs_in_set));
        }
        else
        {
            /*
             * We don't know what kind of chunk it is.  It's not a MTrk, we
             * don't know how to deal with it, so we just eat it.  If this
             * happened on the first track, it is a fatal error.
             */

            if (curtrack > 0)                           /* non-fatal later  */
            {
                errdump("Unsupported MIDI track ID, skipping...", ID);
            }
            else                                        /* fatal in 1st one */
            {
                errdump("Unsupported MIDI track ID on first track.", ID);
                result = false;
                break;
            }
            m_pos += TrackLength;
        }
    }                                                   /* for each track   */
    return result;
}

/**
 *  Parse the proprietary header, figuring out if it is the new format, or
 *  the legacy format, for sequencer-specific data.
 *
 *  The new format creates a final track chunk, starting with "MTrk".
 *  Then comes the delta-time (here, 0), and the event.  An event is a
 *  MIDI event, a SysEx event, or a Meta event.
 *
 *  A MIDI Sequencer Specific meta message includes either a delta time or
 *  absolute time, and the MIDI Sequencer Specific event encoded as
 *  follows:
 *
\verbatim
        0x00 0xFF 0x7F length data
\endverbatim
 *
 *  For convenience, this function first checks the amount of file data left.
 *  If enough, then it reads a long value.  If the value starts with 0x00 0xFF
 *  0x7F, then that is a SeqSpec event, which signals usage of the new
 *  Sequencer64 "proprietary" format.  Otherwise, it is probably the old
 *  format, and the long value is a control tag (0x242400nn), which can be
 *  returned immedidately.
 *
 *  If it is the new format, we back up to the FF, then get the next byte,
 *  which should be a 7F.  If so, then we read the length (a variable
 *  length value) of the data, and then read the long value, which should
 *  be the control tag, which, again, is returned by this function.
 *
 * \note
 *      Most sequencers seem to be tolerant of both the lack of an "MTrk"
 *      marker and of the presence of an unwrapped control tag, and so can
 *      handle both the old and new formats of the final proprietary track.
 *
 * \param file_size
 *      The size of the data file.  This value is compared against the
 *      member m_pos (the position inside m_data[]), to make sure there is
 *      enough data left to process.
 *
 * \return
 *      Returns the control-tag value found.  These are the values, such as
 *      c_midich, found in the globals module, that indicate the type of
 *      sequencer-specific data that comes next.  If there is not enough
 *      data to process, then 0 is returned.
 */

midilong
midifile::parse_prop_header (int file_size)
{
    midilong result = 0;
    if ((file_size - m_pos) > int(sizeof(midilong)))
    {
        result = read_long();                   /* status (new), or C_tag   */
        midibyte status = (result & 0x00FF0000) >> 16;      /* 2-byte shift */
        if (status == 0xFF)
        {
            m_pos -= 2;                         /* back up to meta type     */
            midibyte type = read_byte();        /* get meta type            */
            if (type == 0x7F)                   /* SeqSpec event marker     */
            {
                (void) read_varinum();          /* prop section length      */
                result = read_long();           /* control tag              */
            }
            else
            {
                fprintf
                (
                    stderr,
                    "Bad meta type '%x' in prop section near offset %x\n",
                    int(type), m_pos
                );
            }
        }
    }
    return result;
}

/**
 *  After all of the conventional MIDI tracks are read, we're now at the
 *  "proprietary" Seq24 data section, which describes the various features
 *  that Seq24 supports.  It consists of series of tags:
 *
 *      -   c_midictrl
 *      -   c_midiclocks
 *      -   c_notes
 *      -   c_bpmtag (beats per minute)
 *      -   c_mutegroups
 *      -   c_musickey (new, added if usr() global_seq_feature() is true)
 *      -   c_musicscale (ditto)
 *      -   c_backsequence (ditto)
 *
 *  (There are more tags defined in the globals module, but they are not
 *  used in this function.  This doesn't quite make sense, as there are
 *  also some "triggers" values, and we're pretty sure the application
 *  uses them.  Oh, it turns out that they are set up by actions performed on
 *  each sequence, and are stored as sequencer-specific ("SeqSpec") data with
 *  each track's data as held in the MIDI container for the track.  See the
 *  midi_container module for more information.)
 *
 *  The format is (1) tag ID; (2) length of data; (3) the data.
 *
 *  First, we separate out this function for a little more clarity.  Then we
 *  added code to handle reading both the legacy Seq24 format and the new,
 *  MIDI-compliant format.  Note that even the new format is not quite
 *  correct, since it doesn't handle a MIDI manufacturer's ID, making it a
 *  single byte that is part of the data.  But it does have the "MTrk" marker
 *  and track name, so that must be processed for the new format.
 *
 *  Now, in our "midicvt" project, we have a test MIDI file,
 *  b4uacuse-non-mtrk.midi that is good, except for having a tag "MUnk"
 *  instead of "MTrk".  We should consider being more permissive, if possible.
 *  Otherwise, though, the only penality is that the "proprietary" chunk is
 *  completely skipped.
 *
 * \param p
 *      The performance object that is being set via the incoming MIDI file.
 *
 * \param file_size
 *      The file size as determined in the parse() function.
 *
 *  There are also implicit parameters, with the m_pos and m_new_format member
 *  variables.
 */

bool
midifile::parse_proprietary_track (perform & p, int file_size)
{
    bool result = true;
    midilong ID = read_long();                 /* Get ID + Length      */
    if (ID == PROPRIETARY_CHUNK_TAG)                /* magic number 'MTrk'  */
    {
        midilong tracklength = read_long();
        if (tracklength > 0)
        {
            int seqnum = read_seq_number();
            if (seqnum == PROPRIETARY_SEQ_NUMBER)   /* sanity check         */
            {
                std::string trackname = read_track_name();
                result = ! trackname.empty();

                /*
                 * This "sanity check" is probably a bit much.  It causes
                 * errors in Sequencer24 tracks, which are otherwise fine
                 * to scan in the new format.  Let the "MTrk" and 0x7777
                 * markers be enough.
                 *
                 * if (trackname != PROPRIETARY_TRACK_NAME)
                 *     result = false;
                 */
            }
            else if (seqnum == (-1))
            {
                m_error_is_fatal = false;
                errdump("No sequence number in proprietary track, extra data");
                result = false;
            }
            else
            {
                m_error_is_fatal = false;
                m_error_message = "Unexpected sequence number, proprietary track";
                result = false;
            }
        }
    }
    else
    {
        m_pos -= 4;                                 /* unread the "ID code" */
    }

    if (result)
    {
        midilong proprietary = parse_prop_header(file_size);
        if (proprietary == c_midictrl)
        {
            int seqs = int(read_long());

            /*
             * Some old code wrote some bad files, we need to work around that
             * and fix it.
             */

            if (seqs > c_max_sequence)
            {
                m_pos -= 4;
                errdump
                (
                    "Bad MIDI-control sequence count, fixing.\n"
                    "Please save the file now!",
                    midilong(seqs)
                );
                seqs = midilong(read_byte());
            }
            midibyte a[6];
            for (int i = 0; i < seqs; i++)
            {
                read_byte_array(a, 6);
                p.midi_control_toggle(i).set(a);

                read_byte_array(a, 6);
                p.midi_control_on(i).set(a);

                read_byte_array(a, 6);
                p.midi_control_off(i).set(a);
            }
        }
        proprietary = parse_prop_header(file_size);
        if (proprietary == c_midiclocks)
        {
            int busscount = int(read_long());

            /*
             * Some old code wrote some bad files, we need to work around that
             * and fix it.
             */

            if (busscount > SEQ64_DEFAULT_BUSS_MAX)
            {
                errdump("bad buss count, fixing; please save the file now");
                m_pos -= 4;
                busscount = int(read_byte());
            }
            for (int buss = 0; buss < busscount; ++buss)
            {
                bussbyte clocktype = read_byte();
                p.master_bus().set_clock(bussbyte(buss), (clock_e)(clocktype));
            }
        }
        proprietary = parse_prop_header(file_size);
        if (proprietary == c_notes)
        {
            midishort screen_sets = read_short();
            for (midishort x = 0; x < screen_sets; x++)
            {
                midishort len = read_short();           /* length of string */
                std::string notess;
                for (midishort i = 0; i < len; i++)
                    notess += read_byte();              /* unsigned!        */

                p.set_screen_set_notepad(x, notess);
            }
        }
        proprietary = parse_prop_header(file_size);
        if (proprietary == c_bpmtag)                    /* beats per minute */
        {
            long bpm = read_long();

            /*
             * We should check here for a conflict between the Tempo meta
             * event and this seq24 tag's value.  NOT YET.
             */

            p.set_beats_per_minute(bpm);                /* 2nd way to set!  */
        }

        /* Read in the mute group information. */

        proprietary = parse_prop_header(file_size);
        if (proprietary == c_mutegroups)
        {
            long length = read_long();
            if (c_gmute_tracks != length)
            {
                m_error_is_fatal = true;
                m_error_message = "Corrupt data in mute-group section";
                errdump(m_error_message.c_str());
                result = false;                         /* but keep going   */
            }
            for (int i = 0; i < c_seqs_in_set; i++)
            {
                midilong groupmute = read_long();
                p.select_group_mute(int(groupmute));
#ifdef SEQ64_USE_DEBUG_OUTPUT
                if (groupmute != 0)
                    fprintf(stderr, "group-mute[%d] = %ld\n", i, groupmute);
#endif
                for (int k = 0; k < c_seqs_in_set; ++k)
                {
                    midilong gmutestate = read_long();
                    p.set_group_mute_state(k, gmutestate != 0);
#ifdef SEQ64_USE_DEBUG_OUTPUT
                    if (gmutestate != 0)
                    {
                        fprintf
                        (
                            stderr, "group-mute-state[%d] = %ld\n",
                            k, gmutestate
                        );
                    }
#endif
                }
            }
        }

        /*
         * We will let Sequencer64 try to read this new stuff even if legacy mode
         * is in force, and the global-background sequence is in force.  These two
         * flags should affect only the writing of the MIDI file, not the reading.
         */

        proprietary = parse_prop_header(file_size);
        if (proprietary == c_musickey)
        {
            int key = int(read_byte());
            usr().seqedit_key(key);
        }
        proprietary = parse_prop_header(file_size);
        if (proprietary == c_musicscale)
        {
            int scale = int(read_byte());
            usr().seqedit_scale(scale);
        }
        proprietary = parse_prop_header(file_size);
        if (proprietary == c_backsequence)
        {
            int seqnum = int(read_long());
            usr().seqedit_bgsequence(seqnum);
        }

        /*
         * ADD NEW CONTROL TAGS AT THE END OF THE LIST HERE.
         */
    }
    return result;
}

/**
 *  Writes 4 bytes, using the write_byte() function.
 *
 * \warning
 *      This code looks endian-dependent.
 */

void
midifile::write_long (midilong a_x)
{
    write_byte((a_x & 0xFF000000) >> 24);
    write_byte((a_x & 0x00FF0000) >> 16);
    write_byte((a_x & 0x0000FF00) >> 8);
    write_byte((a_x & 0x000000FF));
}

/**
 *  Writes 2 bytes, using the write_byte() function.
 *
 * \warning
 *      This code looks endian-dependent.
 */

void
midifile::write_short (midishort a_x)
{
    write_byte((a_x & 0xFF00) >> 8);
    write_byte((a_x & 0x00FF));
}

/**
 *  Writes a MIDI Variable-Length Value (VLV), which has a variable number
 *  of bytes.
 *
 *  A MIDI file Variable Length Value is stored in bytes. Each byte has
 *  two parts: 7 bits of data and 1 continuation bit. The highest-order
 *  bit is set to 1 if there is another byte of the number to follow. The
 *  highest-order bit is set to 0 if this byte is the last byte in the
 *  VLV.
 *
 *  To recreate a number represented by a VLV, first you remove the
 *  continuation bit and then concatenate the leftover bits into a single
 *  number.
 *
 *  To generate a VLV from a given number, break the number up into 7 bit
 *  units and then apply the correct continuation bit to each byte.
 *
 *  In theory, you could have a very long VLV number which was quite
 *  large; however, in the standard MIDI file specification, the maximum
 *  length of a VLV value is 5 bytes, and the number it represents can not
 *  be larger than 4 bytes.
 *
 *  Here are some common cases:
 *
 *      -  Numbers between 0 and 127 (0x7F) are represented by a single
 *         byte.
 *      -  0x80 is represented as "0x81 0x00".
 *      -  0x0FFFFFFF (the largest number) is represented as "0xFF 0xFF
 *         0xFF 0x7F".
 *
 *  Also see the varinum_size() function.
 */

void
midifile::write_varinum (midilong value)
{
   midilong buffer = value & 0x7f;
   while ((value >>= 7) > 0)
   {
       buffer <<= 8;
       buffer |= 0x80;
       buffer += (value & 0x7f);
   }
   for (;;)
   {
      write_byte(midibyte(buffer & 0xff));
      if (buffer & 0x80)                            /* continuation bit?    */
         buffer >>= 8;                              /* yes                  */
      else
         break;                                     /* no, we are done      */
   }
}

/**
 *  Calculates the length of a variable length value.  This function is
 *  needed when calculating the length of a track.  Note that it handles
 *  only the following situations:
 *
 *  https://en.wikipedia.org/wiki/Variable-length_quantity
 *
\verbatim
       1 byte:  0x00 to 0x7F
       2 bytes: 0x80 to 0x3FFF
       3 bytes: 0x4000 to 0x001FFFFF
       4 bytes: 0x200000 to 0x0FFFFFFF
\endverbatim
 *
 * \return
 *      Returns values as noted above.  Anything beyond that range returns
 *      0.
 */

long
midifile::varinum_size (long len) const
{
    int result = 0;
    if (len >= 0x00 && len < 0x80)
        result = 1;
    else if (len >= 0x80 && len < 0x4000)
        result = 2;
    else if (len >= 0x4000 && len < 0x200000)
        result = 3;
    else if (len >= 0x200000 && len < 0x10000000)
        result = 4;

    return result;
}

/**
 * We want to write:
 *
 *  -   0x4D54726B.
 *      The track tag "MTrk".  The MIDI spec requires that software can skip
 *      over non-standard chunks. "Prop"?  Would require a fix to midicvt.
 *  -   0xaabbccdd.
 *      The length of the track.  This needs to be calculated somehow.
 *  -   0x00.  A zero delta time.
 *  -   0x7f7f.  Sequence number, a special value, well out of normal range.
 *  -   The name of the track:
 *      -   "Seq24-Spec"
 *      -   "Sequencer64-S"
 *
 *   Then follows the proprietary data, written in the normal manner.
 *   Finally, tack on the track-end meta-event.
 *
 *      Components of final track size:
 *
 *          -# Delta time.  1 byte, always 0x00.
 *          -# Sequence number.  5 bytes.  OPTIONAL.  We won't write it.
 *          -# Track name. 3 + 10 or 3 + 15
 *          -# Series of proprietary specs:
 *             -# Prop header:
 *                -# If legacy format, 4 bytes.
 *                -# Otherwise, 2 bytes + varinum_size(length) + 4 bytes.
 *                -# Length of the prop data.
 *          -# Track End. 3 bytes.
 */

/**
 *  Writes a "proprietary" Seq24 footer header in either the new
 *  MIDI-compliant format, or the legacy Seq24 format.  This function does not
 *  write the data.  It replaces calls such as "write_long(c_midich)" in the
 *  proprietary secton of write().
 *
 *  The legacy format just writes the control tag (0x242400xx).  The new
 *  format writes 0x00 0xFF 0x7F len 0x242400xx; the first 0x00 is the delta
 *  time.
 *
 *  In the new format, the 0x24 is a kind of "manufacturer ID".  At
 *  http://www.midi.org/techspecs/manid.php we see that most manufacturer IDs
 *  start with 0x00, and are thus three bytes long, or start with codes at
 *  0x40 and above.  Similary, this site shows that no manufacturer uses 0x24:
 *
 *      http://sequence15.blogspot.com/2008/12/midi-manufacturer-ids.html
 *
 * \warning
 *      Currently, the manufacturer ID is not handled; it is part of the
 *      data, which can be misleading in programs that analyze MIDI files.
 *
 * \param control_tag
 *      Determines the type of sequencer-specific section to be written.
 *      It should be one of the value in the globals module, such as
 *      c_midibus or c_mutegroups.
 *
 * \param data_length
 *      The amount of data that will be written.  This parameter does not
 *      count the length of the header itself.
 */

void
midifile::write_prop_header
(
    midilong control_tag,
    long data_length
)
{
    if (m_new_format)
    {
        int len = data_length + 4;          /* data + sizeof(control_tag);  */
        write_byte(0x00);                   /* delta time                   */
        write_byte(0xFF);
        write_byte(0x7F);
        write_varinum(len);
    }
    write_long(control_tag);                /* use legacy output call       */
}

/**
 *  Calculates the size of a proprietary item, as written by the
 *  write_prop_header() function, plus whatever is called to write the data.
 *  If using the new format, the length includes the sum of sequencer-specific
 *  tag (0xFF 0x7F) and the size of the variable-length value.  Then, for
 *  legacy and new format, 4 bytes are added for the Seq24 MIDI control
 *  value, and then the data length is added.
 */

long
midifile::prop_item_size (long data_length) const
{
    long result = 0;
    if (m_new_format)
    {
        int len = data_length + 4;          /* data + sizeof(control_tag);  */
        result += 3;                        /* count delta time, meta bytes */
        result += varinum_size(len);        /* count the length bytes       */
    }
    result += 4;                            /* write_long(control_tag);     */
    result += data_length;                  /* add the data size itself     */
    return result;
}

/**
 *  Write the whole MIDI data and Seq24 information out to the file.
 *
 * \param p
 *      Provides the object that will contain and manage the entire
 *      performance.
 *
 * \return
 *      Returns true if the write operations succeeded.
 */

bool
midifile::write (perform & p)
{
    bool result = true;
    int numtracks = 0;
    m_error_message.clear();
    printf("[Writing MIDI file, %d ppqn]\n", m_ppqn);
    for (int i = 0; i < c_max_sequence; i++) /* get number of active tracks */
    {
        if (p.is_active(i))
            numtracks++;
    }
    write_long(0x4D546864);                 /* MIDI Format 1 header MThd    */
    write_long(6);                          /* Length of the header         */
    write_short(1);                         /* MIDI Format 1                */
    write_short(numtracks);                 /* number of tracks             */
    write_short(m_ppqn);                    /* parts per quarter note       */

    /*
     * Write out the active tracks.  The value of c_max_sequence is 1024.
     */

    for (int curtrack = 0; curtrack < c_max_sequence; curtrack++)
    {
        if (p.is_active(curtrack))
        {
            sequence * s = p.get_sequence(curtrack);
            if (not_nullptr(s))
            {
                sequence & seq = *s;

#if defined SEQ64_USE_MIDI_VECTOR
                midi_vector lst(seq);
#else
                midi_list lst(seq);
#endif
                seq.fill_container(lst, curtrack);
                write_long(0x4D54726B);     /* magic number 'MTrk'          */
                write_long(lst.size());

                /**
                 * \note
                 *      Seq24 reverses the order of some events, due to
                 *      popping from its container.  Not an issue here.
                 */

                while (! lst.done())        /* write the track data         */
                    write_byte(lst.get());
            }
            else
            {
                m_error_message = "Error in MIDI buffer, failed to get sequence";
                result = false;
                break;
            }
        }
    }
    if (result)
    {
        (void) write_proprietary_track(p);
        std::ofstream file
        (
            m_name.c_str(), std::ios::out | std::ios::binary | std::ios::trunc
        );
        if (file.is_open())
        {
            char file_buffer[SEQ64_MIDI_LINE_MAX];  /* enable bufferization */
            file.rdbuf()->pubsetbuf(file_buffer, sizeof file_buffer);
            std::list<midibyte>::iterator it;
            for (it = m_char_list.begin(); it != m_char_list.end(); it++)
            {
                char c = *it;
                file.write(&c, 1);
            }
            m_char_list.clear();
        }
        else
        {
            m_error_message = "Error opening MIDI file for writing";
            result = false;
        }
    }
    if (result)
        p.is_modified(false);      /* it worked, tell perform about it */

    return result;
}

/**
 *  Writes out the proprietary section, using the new format if the
 *  legacy format is not in force.
 *
 *  The first thing to do, for the new format only, is calculate the length
 *  of this big section of data.  This was quite tricky; we tweaked and
 *  adjusted until the midicvt program handled the whole new-format file
 *  without emitting any errors.
 *
 *  Here's the basics of what Seq24 did for writing the data in this part of
 *  the file:
 *
 *      -#  Write the c_midictrl value, then write a 0.  To us, this looks like
 *          no one wrote any code to write this data.  And yet, the parsing
 *          code can handles a non-zero value, which is the number of sequences
 *          as a long value, not a byte.  So shouldn't we write 4 bytes, not
 *          one?  Yes, indeed, we made a mistake.  However, we should be
 *          writing out the full data set as well.  But not even Seq24 does
 *          that!  Perhaps they decided it was best kept in the "rc"
 *          configuration file.
 *      -#  MORE TO COME.
 */

bool
midifile::write_proprietary_track (perform & p)
{
    long tracklength = 0;
    int cnotesz = 2;                            /* first value is short     */
    for (int s = 0; s < c_max_sets; s++)
    {
        const std::string & note = p.get_screen_set_notepad(s);
        cnotesz += 2 + note.length();           /* short + note length      */
    }

    /*
     * We need a way to make the group mute data optional.  Why write 4096
     * bytes of zeroes?
     */

    int gmutesz = 4 + c_seqs_in_set * (4 + c_seqs_in_set * 4);
    if (m_new_format)                           /* calculate track size     */
    {
        tracklength += seq_number_size();       /* bogus sequence number    */
        tracklength += track_name_size(PROPRIETARY_TRACK_NAME);
        tracklength += prop_item_size(4);       /* c_midictrl               */
        tracklength += prop_item_size(4);       /* c_midiclocks             */
        tracklength += prop_item_size(cnotesz); /* c_notes                  */
        tracklength += prop_item_size(4);       /* c_bpmtag, beats/minute   */
        tracklength += prop_item_size(gmutesz); /* c_mutegroups             */
        if (m_global_bgsequence)
        {
            tracklength += prop_item_size(1);   /* c_musickey               */
            tracklength += prop_item_size(1);   /* c_musicscale             */
            tracklength += prop_item_size(4);   /* c_backsequence           */
        }
        tracklength += track_end_size();        /* Meta TrkEnd              */
    }
    if (m_new_format)                           /* write beginning of track */
    {
        write_long(PROPRIETARY_CHUNK_TAG);      /* "MTrk" or something else */
        write_long(tracklength);
        write_seq_number(PROPRIETARY_SEQ_NUMBER); /* bogus sequence number  */
        write_track_name(PROPRIETARY_TRACK_NAME); /* bogus track name       */
    }
    write_prop_header(c_midictrl, 4);           /* midi control tag + 4     */
    write_long(0);                              /* SEQ24 WRITES ZERO ONLY!  */
    write_prop_header(c_midiclocks, 4);         /* bus mute/unmute data + 4 */
    write_long(0);                              /* SEQ24 WRITES ZERO ONLY!  */
    write_prop_header(c_notes, cnotesz);        /* notepad data tag + data  */
    write_short(c_max_sets);                    /* data, not a tag          */
    for (int s = 0; s < c_max_sets; s++)        /* see "cnotesz" calc       */
    {
        const std::string & note = p.get_screen_set_notepad(s);
        write_short(note.length());
        for (unsigned n = 0; n < unsigned(note.length()); n++)
            write_byte(note[n]);
    }
    write_prop_header(c_bpmtag, 4);             /* bpm tag + long data      */
    write_long(p.get_beats_per_minute());       /* 4 bytes                  */
    write_prop_header(c_mutegroups, gmutesz);   /* the mute groups tag etc. */
    write_long(c_gmute_tracks);                 /* data, not a tag          */
    for (int j = 0; j < c_seqs_in_set; ++j)     /* should be optional!      */
    {
        p.select_group_mute(j);
        write_long(j);
        for (int i = 0; i < c_seqs_in_set; ++i)
            write_long(p.get_group_mute_state(i));
    }
    if (m_new_format)                           /* write beginning of track */
    {
        if (m_global_bgsequence)
        {
            write_prop_header(c_musickey, 1);               /* control tag+1 */
            write_byte(midibyte(usr().seqedit_key()));      /* key change    */
            write_prop_header(c_musicscale, 1);             /* control tag+1 */
            write_byte(midibyte(usr().seqedit_scale()));    /* scale change  */
            write_prop_header(c_backsequence, 4);           /* control tag+4 */
            write_long(long(usr().seqedit_bgsequence()));   /* background    */
        }
        write_track_end();
    }
    return true;
}

/**
 *  Writes out a track name.  Note that we have to precede this "event"
 *  with a delta time value, set to 0.
 */

void
midifile::write_track_name (const std::string & trackname)
{
    bool ok = ! trackname.empty();
    if (ok)
    {
        write_byte(0x00);                               /* delta time       */
        write_byte(0xFF);                               /* meta tag         */
        write_byte(0x03);                               /* second byte      */
        write_varinum(midilong(trackname.size()));
        for (int i = 0; i < int(trackname.size()); i++)
            write_byte(trackname[i]);
    }
}

/**
 *  Reads the track name.  Meant only for usage in the proprietary footer
 *  track, in the new file format.
 *
 * \return
 *      Returns the track name, or an empty string if there was a problem.
 */

std::string
midifile::read_track_name ()
{
    std::string result;
    (void) read_byte();                         /* throw-away delta time    */
    midibyte status = read_byte();              /* get the seq-spec marker  */
    if (status == 0xFF)
    {
        if (read_byte() == 0x03)
        {
            midilong tl = int(read_varinum());     /* track length     */
            if (tl > 0)
            {
                for (midilong i = 0; i < tl; i++)
                {
                    midibyte c = read_byte();
                    result += c;
                }
            }
        }
    }
    return result;
}

/**
 *  Calculates the size of a trackname and the meta event that specifies
 *  it.
 */

long
midifile::track_name_size (const std::string & trackname) const
{
    long result = 0;
    if (! trackname.empty())
    {
        result += 3;                                    /* 0x00 0xFF 0x03   */
        result += varinum_size(long(trackname.size())); /* variable length  */
        result += long(trackname.size());               /* data size        */
    }
    return result;
}

/**
 *  Writes out a sequence number.  The format is "00 FF 00 02 ss ss", where
 *  "02" is actually the constant length of the data.  We have to precede
 *  these values with a 0 delta time, of course.
 *
 *  Now, for sequence 0, an alternate format is "FF 00 00".  But that
 *  format can only occur in the first track, and the rest of the tracks then
 *  don't need a sequence number, since it is assumed to increment.  Our
 *  application doesn't bother with that shortcut.
 */

void
midifile::write_seq_number (midishort seqnum)
{
    write_byte(0x00);                           /* delta time               */
    write_byte(0xFF);                           /* meta tag                 */
    write_byte(0x00);                           /* second byte              */
    write_byte(0x02);                           /* finish sequence tag      */
    write_short(seqnum);                        /* write sequence number    */
}

/**
 *  Reads the sequence number.  Meant only for usage in the proprietary
 *  footer track, in the new file format.
 *
 * \return
 *      Returns the sequence number found, or -1 if it was not found.
 */

int
midifile::read_seq_number ()
{
    int result = -1;
    (void) read_byte();                         /* throw-away delta time    */
    midibyte status = read_byte();              /* get the seq-spec marker  */
    if (status == 0xFF)
    {
        if (read_byte() == 0x00 && read_byte() == 0x02)
            result = int(read_short());
    }
    return result;
}

/**
 *  Writes out the end-of-track marker.
 */

void
midifile::write_track_end ()
{
    write_byte(0xFF);                       /* meta tag                     */
    write_byte(0x2F);
    write_byte(0x00);
}

/**
 *  Helper function to emit more useful error messages.  It adds the file
 *  offset to the message.
 *
 * \param msg
 *      The main error message string, without an ending newline character.
 *
 * \return
 *      The constructed string is returned as a side-effect, in case we want
 *      to pass it along to the externally-accessible error-message buffer.
 */

void
midifile::errdump (const std::string & msg)
{
    char temp[32];
    snprintf(temp, sizeof temp, "Near offset 0x%x:  ", m_pos);
    std::string result = temp;
    result += msg;
    result += "\n";
    fprintf(stderr, result.c_str());
    m_error_message = result;
}

/**
 *  Helper function to emit more useful error messages for erroneous long
 *  values.  It adds the file offset to the message.
 *
 * \param msg
 *      The main error message string, without an ending newline character.
 *
 * \param value
 *      The long value to show as part of the message.
 *
 * \return
 *      The constructed string is returned as a side-effect, in case we want
 *      to pass it along to the externally-accessible error-message buffer.
 */

void
midifile::errdump (const std::string & msg, unsigned long value)
{
    char temp[64];
    snprintf
    (
        temp, sizeof temp, "Near offset 0x%x, bad value %lu (0x%lx):  ",
        m_pos, value, value
    );
    std::string result = temp;
    result += msg;
    result += "\n";
    fprintf(stderr, result.c_str());
    m_error_message = result;
}

}           // namespace seq64

/*
 * midifile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

