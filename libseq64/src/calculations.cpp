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
 * \file          calculations.cpp
 *
 *  This module declares/defines some utility functions and calculations
 *  needed by this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-07
 * \updates       2017-12-11
 * \license       GNU GPLv2 or above
 *
 *  This code was moved from the globals module so that other modules
 *  can include it only if they need it.
 *
 *  To convert the ticks for each MIDI note into a millisecond value to
 *  display the notes visually along a timeline, one needs to use the division
 *  and the tempo to determine the value of an individual tick.  That
 *  conversion looks like:
 *
\verbatim
        1 min    60 sec   1 beat     Z clocks
       ------- * ------ * -------- * -------- = seconds
       X beats   1 min    Y clocks       1
\endverbatim
 *
 *  X is the tempo (beats per minute, or BPM), Y is the division (pulses per
 *  quarter note, PPQN), and Z is the number of clocks from the incoming
 *  event. All of the units cancel out, yielding a value in seconds. The
 *  condensed version of that conversion is therefore:
 *
\verbatim
        (60 * Z) / (X * Y) = seconds
        seconds = 60 * clocks / (bpm * ppqn)
\endverbatim
 *
 *  The value given here in seconds is the number of seconds since the
 *  previous MIDI event, not from the sequence start.  One needs to keep a
 *  running total of this value to construct a coherent sequence.  Especially
 *  important if the MIDI file contains tempo changes.  Leaving the clocks (Z)
 *  out of the equation yields the periodicity of the clock.
 *
 *  The inverse calculation is:
 *
\verbatim
        clocks = seconds * bpm * ppqn / 60
\endverbatim
 *
 * \todo
 *      There are additional user-interface and MIDI scaling variables in the
 *      perfroll module that we need to move here.
 */

#include <cctype>                       /* std::isspace(), std::isdigit()   */
#include <math.h>                       /* C::floor(), C::log()             */
#include <stdlib.h>                     /* C::atoi(), C::strtol()           */
#include <string.h>                     /* C::memset()                      */
#include <time.h>                       /* C::strftime()                    */

#include "app_limits.h"
#include "calculations.hpp"
#include "settings.hpp"

#if ! defined PI
#define PI     3.14159265359
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Extracts up to 4 numbers from a colon-delimited string.
 *
 *      -   measures : beats : divisions
 *          -   "8" represents solely the number of pulses.  That is, if the
 *              user enters a single number, it is treated as the number of
 *              pulses.
 *          -   "8:1" represents a measure and a beat.
 *          -   "213:4:920"  represents a measure, a beat, and pulses.
 *      -   hours : minutes : seconds . fraction.  We really don't support
 *          this concept at present.  Beware!
 *          -   "2:04:12.14"
 *          -   "0:1:2"
 *
 * \warning
 *      This is not the most efficient implementation you'll ever see.
 *      At some point we will tighten it up.  This function is tested in the
 *      seq64-tests project, in the "calculations_unit_test" module.
 *      At present this test is surely BROKEN!
 *
 * \param s
 *      Provides the input time string, in measures or time format,
 *      to be processed.
 *
 * \param [out] part_1
 *      The destination reference for the first part of the time.
 *      In some contexts, this number alone is a pulse (ticks) value;
 *      in other contexts, it is a measures value.
 *
 * \param [out] part_2
 *      The destination reference for the second part of the time.
 *
 * \param [out] part_3
 *      The destination reference for the third part of the time.
 *
 * \param [out] fraction
 *      The destination reference for the fractional part of the time.
 *
 * \return
 *      Returns the number of parts provided, ranging from 0 to 4.
 */

int
extract_timing_numbers
(
    const std::string & s,
    std::string & part_1,
    std::string & part_2,
    std::string & part_3,
    std::string & fraction
)
{
    std::vector<std::string> tokens;
    int count = tokenize_string(s, tokens);
    part_1.clear();
    part_2.clear();
    part_3.clear();
    fraction.clear();
    if (count > 0)
        part_1 = tokens[0];

    if (count > 1)
        part_2 = tokens[1];

    if (count > 2)
        part_3 = tokens[2];

    if (count > 3)
        fraction = tokens[3];

    return count;
}

/**
 *  Tokenizes a string using the colon, space, or period as delimiters.  They
 *  are treated equally, and the caller must determine what to do with the
 *  parts.  Here are the steps:
 *
 *      -#  Skip any delimiters found at the beginning.  The position will
 *          either exist, or there will be nothing to parse.
 *      -#  Get to the next delimiter.  This will exist, or not.  Get all
 *          non-delimiters until the next delimiter or the end of the string.
 *      -#  Repeat until no more delimiters exist.
 *
 * \param source
 *      The string to be parsed and tokenized.
 *
 * \param tokens
 *      Provides a vector into which to push the tokens.
 *
 * \return
 *      Returns the number of tokens pushed (i.e. the final size of the tokens
 *      vector.
 */

int
tokenize_string
(
    const std::string & source,
    std::vector<std::string> & tokens
)
{
    static std::string s_delims = ":. ";
    int result = 0;
    tokens.clear();
    std::string::size_type pos = source.find_first_not_of(s_delims);
    if (pos != std::string::npos)
    {
        for (;;)
        {
            std::string::size_type depos = source.find_first_of(s_delims, pos);
            if (depos != std::string::npos)
            {
                tokens.push_back(source.substr(pos, depos - pos));
                pos = source.find_first_not_of(s_delims, depos +1);
                if (pos == std::string::npos)
                    break;
            }
            else
            {
                tokens.push_back(source.substr(pos));
                break;
            }
        }
        result = int(tokens.size());
    }
    return result;
}

/**
 *  Converts MIDI pulses (also known as ticks, clocks, or divisions) into a
 *  string.
 *
 * \todo
 *      Still needs to be unit tested.
 *
 * \param p
 *      The MIDI pulse/tick value to be converted.
 *
 * \return
 *      Returns the string as an unsigned ASCII integer number.
 */

std::string
pulses_to_string (midipulse p)
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%lu", (unsigned long)(p));
    return std::string(tmp);
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "measures:beats:ticks" ("measures:beats:division").
 *
 * \param p
 *      The number of MIDI pulses (clocks, divisions, ticks, you name it) to
 *      be converted.  If the value is SEQ64_NULL_MIDIPULSE, it is converted
 *      to 0, because callers don't generally worry about such niceties, and
 *      the least we can do is convert illegal measure-strings (like
 *      "000:0:000") to a legal value.
 *
 * \param seqparms
 *      This small structure provides the beats/measure, beat-width, and PPQN
 *      that hold for the sequence involved in this calculation.  These values
 *      are needed in the calculations.
 *
 * \return
 *      Returns the string, in measures notation, for the absolute pulses that
 *      mark this duration.
 */

std::string
pulses_to_measurestring (midipulse p, const midi_timing & seqparms)
{
    midi_measures measures;                 /* measures, beats, divisions   */
    char tmp[32];
    if (is_null_midipulse(p))
        p = 0;                              /* punt the runt!               */

    pulses_to_midi_measures(p, seqparms, measures); /* fill measures struct */
    snprintf
    (
        tmp, sizeof tmp, "%03d:%d:%03d",
        measures.measures(), measures.beats(), measures.divisions()
    );
    return std::string(tmp);
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "measures:beats:ticks" ("measures:beats:division").
 *
\verbatim
        m = p * W / (4 * P * B)
\endverbatim
 *
 * \param p
 *      Provides the MIDI pulses (as in "pulses per quarter note") that are to
 *      be converted to MIDI measures format.
 *
 * \param seqparms
 *      This small structure provides the beats/measure (B), beat-width (W),
 *      and PPQN (P) that hold for the sequence involved in this calculation.
 *      The beats/minute (T for tempo) value is not needed.
 *
 * \param [out] measures
 *      Provides the current MIDI song time structure to hold the results,
 *      which are the measures, beats, and divisions values for the time of
 *      interest.  Note that the measures and beats are corrected to be re 1,
 *      not 0.
 *
 * \return
 *      Returns true if the calculations were able to be made.  The P, B, and
 *      W values all need to be greater than 0.
 */

bool
pulses_to_midi_measures
(
    midipulse p,
    const midi_timing & seqparms,
    midi_measures & measures
)
{
    static const double s_epsilon = 0.000001;   /* HMMMMMMMMMMMMMMMMMMMMMMM */
    int W = seqparms.beat_width();
    int P = seqparms.ppqn();
    int B = seqparms.beats_per_measure();
    bool result = (W > 0) && (P > 0) && (B > 0);
    if (result)
    {
        double m = p * W / (4.0 * P * B);       /* measures, whole.frac     */
        double m_whole = floor(m);              /* holds integral measures  */
        m -= m_whole;                           /* get fractional measure   */
        double b = m * B;                       /* beats, whole.frac        */
        double b_whole = floor(b);              /* get integral beats       */
        b -= b_whole;                           /* get fractional beat      */
        double pulses_per_beat = 4 * P / W;     /* pulses/qn * qn/beat      */
        measures.measures(int(m_whole + s_epsilon) + 1);
        measures.beats(int(b_whole + s_epsilon) + 1);
        measures.divisions(int(b * pulses_per_beat + s_epsilon));
    }
    return result;
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "hours:minutes:seconds.fraction".  See the other pulses_to_timestring()
 *  overload.
 *
 * \todo
 *      Still needs to be unit tested.
 *
 * \param p
 *      Provides the number of ticks, pulses, or divisions in the MIDI
 *      event time.
 *
 * \param timinginfo
 *      Provides the tempo of the song, in beats/minute, and the
 *      pulse-per-quarter-note of the song.
 *
 * \return
 *      Returns the return-value of the other pulses_to_timestring() function.
 */

std::string
pulses_to_timestring (midipulse p, const midi_timing & timinginfo)
{
    return pulses_to_timestring
    (
        p, timinginfo.beats_per_minute(), timinginfo.ppqn()
    );
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "hours:minutes:seconds.fraction".  If the fraction part is 0, then it is
 *  not shown.  Examples:
 *
 *      -   "0:0:0"
 *      -   "0:0:0.102333"
 *      -   "12:3:1"
 *      -   "12:3:1.000001"
 *
 * \param p
 *      Provides the number of ticks, pulses, or divisions in the MIDI
 *      event time.
 *
 * \param bpm
 *      Provides the tempo of the song, in beats/minute.
 *
 * \param ppqn
 *      Provides the pulses-per-quarter-note of the song.
 *
 * \param showus
 *      If true (the default), shows the microseconds as well.
 *
 * \return
 *      Returns the time-string representation of the pulse (ticks) value.
 */

std::string
pulses_to_timestring (midipulse p, midibpm bpm, int ppqn, bool showus)
{
    unsigned long microseconds = ticks_to_delta_time_us(p, bpm, ppqn);
    int seconds = int(microseconds / 1000000UL);
    int minutes = seconds / 60;
    int hours = seconds / (60 * 60);
    minutes -= hours * 60;
    seconds -= (hours * 60 * 60) + (minutes * 60);
    microseconds -= (hours * 60 * 60 + minutes * 60 + seconds) * 1000000UL;

    char tmp[32];
    if (! showus || (microseconds == 0))
    {
        /*
         * Why the spaces?  It is inconsistent.  But see the
         * timestring_to_pulses() function first.
         */

        snprintf(tmp, sizeof tmp, "%03d:%d:%02d   ", hours, minutes, seconds);
    }
    else
    {
        snprintf
        (
            tmp, sizeof tmp, "%03d:%d:%02d.%02lu",
            hours, minutes, seconds, microseconds
        );
    }
    return std::string(tmp);
}

/**
 *  Converts a string that represents "measures:beats:division" to a MIDI
 *  pulse/ticks/clock value.
 *
 *  If the third value (the MIDI pulses or ticks value) is set to the dollar
 *  sign ("$"), then the pulses are set to PPQN-1, as a handy shortcut to
 *  indicate the end of the beat.
 *
 * \warning
 *      If only one number is provided, it is treated in this function like
 *      a measures value, not a pulses value.
 *
 * \param measures
 *      Provides the current MIDI song time in "measures:beats:divisions"
 *      format, where divisions are the MIDI pulses in
 *      "pulses-per-quarter-note".
 *
 * \param seqparms
 *      This small structure provides the beats/measure, beat-width, and PPQN
 *      that hold for the sequence involved in this calculation.
 *
 * \return
 *      Returns the absolute pulses that mark this duration.  If the input
 *      string is empty, then 0 is returned.
 */

midipulse
measurestring_to_pulses
(
    const std::string & measures,
    const midi_timing & seqparms
)
{
    midipulse result = 0;
    if (! measures.empty())
    {
        std::string m, b, d, dummy;
        int valuecount = extract_timing_numbers(measures, m, b, d, dummy);
        if (valuecount >= 1)
        {
            midi_measures meas_values;
            memset(&meas_values, 0, sizeof meas_values);
            meas_values.measures(atoi(m.c_str()));
            if (valuecount > 1)
            {
                meas_values.beats(atoi(b.c_str()));
                if (valuecount > 2)
                {
                    if (d == "$")
                        meas_values.divisions(seqparms.ppqn() - 1);
                    else
                        meas_values.divisions(atoi(d.c_str()));
                }
            }
            result = midi_measures_to_pulses(meas_values, seqparms);
        }
    }
    return result;
}

/**
 *  Converts a string that represents "measures:beats:division" to a MIDI
 *  pulse/ticks/clock value.
 *
 *  p = 4 * P * m * B / W
 *      p == pulse count (ticks or pulses)
 *      m == number of measures
 *      B == beats per measure (constant)
 *      P == pulses per quarter-note (constant)
 *      W == beat width in beats per measure (constant)
 *
 *  Note that the 0-pulse MIDI measure is "1:1:0", which means "at the
 *  beginning of the first beat of the first measure, no pulses'.  It is not
 *  "0:0:0" as one might expect.  If we get a 0 for measures or for beats, we
 *  treat them as if they were 1.  It is too easy for the user to mess up.
 *
 *  We should consider clamping the beats to the beat-width value as well.
 *
 * \param measures
 *      Provides the current MIDI song time structure holding the
 *      measures, beats, and divisions values for the time of interest.
 *
 * \param seqparms
 *      This small structure provides the beats/measure, beat-width, and PPQN
 *      that hold for the sequence involved in this calculation.
 *
 * \return
 *      Returns the absolute pulses that mark this duration.  If the
 *      pulse-value cannot be calculated, then SEQ64_NULL_MIDIPULSE is
 *      returned.
 */

midipulse
midi_measures_to_pulses
(
    const midi_measures & measures,
    const midi_timing & seqparms
)
{
    midipulse result = SEQ64_NULL_MIDIPULSE;
    int m = measures.measures() - 1;                /* true measure count   */
    int b = measures.beats() - 1;
    if (m < 0)
        m = 0;

    if (b < 0)
        b = 0;

    double qn_per_beat = 4.0 / seqparms.beat_width();
    result = 0;
    if (m > 0)
        result += int(m * seqparms.beats_per_measure() * qn_per_beat);

    if (b > 0)
        result += int(b * qn_per_beat);

    result *= seqparms.ppqn();
    result += measures.divisions();
    return result;
}

/**
 *  Converts a string that represents "hours:minutes:seconds.fraction" into a
 *  MIDI pulse/ticks/clock value.
 *
 * \param timestring
 *      The time value to be converted, which must be of the form
 *      "hh:mm:ss" or "hh:mm:ss.fraction".  That is, all four parts must
 *      be found.
 *
 * \param bpm
 *      The beats-per-minute tempo (e.g. 120) of the current MIDI song.
 *
 * \param ppqn
 *      The parts-per-quarter note precision (e.g. 192) of the current MIDI
 *      song.
 *
 * \return
 *      Returns 0 if an error occurred or if the number actually translated to
 *      0.
 */

midipulse
timestring_to_pulses (const std::string & timestring, midibpm bpm, int ppqn)
{
    midipulse result = 0;
    if (! timestring.empty())
    {
        std::string sh, sm, ss, us;
        if (extract_timing_numbers(timestring, sh, sm, ss, us) >= 4)
        {
            /**
             * This conversion assumes that the fractional parts of the
             * seconds is padded with zeroes on the left or right to 6 digits.
             */

            int hours = atoi(sh.c_str());
            int minutes = atoi(sm.c_str());
            int seconds = atoi(ss.c_str());
            double secfraction = atof(us.c_str());
            long sec = ((hours * 60) + minutes) * 60 + seconds;
            long microseconds = 1000000 * sec + long(1000000.0 * secfraction);
            double pulses = delta_time_us_to_ticks(microseconds, bpm, ppqn);
            result = midipulse(pulses);
        }
    }
    return result;
}

/**
 *  Converts a time string to pulses.  First, the type of string is deduced by
 *  the characters in the string.  If the string contains two colons and a
 *  decimal point, it is assumed to be a time-string ("hh:mm:ss.frac"); in
 *  addition ss will have to be less than 60. ???  Actually, now we only care if
 *  four numbers are provided.
 *
 *  If the string just contains two colons, then it is assumed to be a
 *  measure-string ("measures:beats:divisions").
 *
 *  If it has none of the above, it is assumed to be pulses.  Testing is not
 *  rigorous.
 *
 * \param s
 *      Provides the string to convert to pulses.
 *
 * \param mt
 *      Provides the structure needed to provide BPM and other values needed
 *      for some of the conversions done by this function.
 *
 * \return
 *      Returns the string as converted to MIDI pulses (or divisions, clocks,
 *      ticks, whatever you call it).
 */

midipulse
string_to_pulses
(
    const std::string & s,
    const midi_timing & mt
)
{
    midipulse result = 0;
    std::string s1;
    std::string s2;
    std::string s3;
    std::string fraction;
    int count = extract_timing_numbers(s, s1, s2, s3, fraction);
    if (count > 1)
    {
        if (fraction.empty() || atoi(s3.c_str()) >= 60)     // why???
            result = measurestring_to_pulses(s, mt);
        else
            result = timestring_to_pulses(s, mt.beats_per_minute(), mt.ppqn());
    }
    else
        result = atol(s.c_str());

    return result;
}

/**
 *  Converts a string to a MIDI byte.  This function bypasses characters until
 *  it finds a digit (whether part of the number or a "0x" construct), and
 *  then converts it.
 *
 * \param s
 *      Provides the string to convert to a MIDI byte.
 *
 * \return
 *      Returns the MIDI byte value represented by the string.
 */

midibyte
string_to_midibyte (const std::string & s)
{
    midibyte result = 0;
    if (! s.empty())
    {
        const char * numptr = s.c_str();
        while (! std::isdigit(*numptr))
            ++numptr;

        long value = strtol(numptr, nullptr, 0);    /* decimal/hex/octal */
        result = midibyte(value);
    }
    return result;
}

/**
 *  Shortens a file-specification to make sure it is no longer than the
 *  provided length value.  This is done by removing character in the middle,
 *  if necessary, and replacing them with an ellipse.
 *
 *  This function operates by first trying to find the <code> /home </code>
 *  directory.  If found, it strips off <code> /home/username </code> and replace
 *  it with the Linux <code> ~ </code> replacement for the <code> $HOME </code>
 *  environment variable.  This function assumes that the "username" portion
 *  <i> must </i> exist, and that there's no goofy stuff like double-slashes
 *  in the path.
 *
 * \param fpath
 *      The file specification, including the full path to the file, and the
 *      name of the file.
 *
 * \param leng
 *      Provides the length to which to limit the string.
 *
 * \return
 *      Returns the fpath parameter, possibly shortened to fit within the
 *      desired length.
 */

std::string
shorten_file_spec (const std::string & fpath, int leng)
{
    std::size_t fpathsize = fpath.size();
    if (fpathsize <= std::size_t(leng))
        return fpath;
    else
    {
        std::string ellipse("...");
        std::size_t halflength = (std::size_t(leng) - ellipse.size()) / 2 - 1;
        std::string result = fpath;
        std::size_t foundpos = result.find("/home");
        if (foundpos != std::string::npos)
        {
            foundpos = result.find_first_of('/', foundpos + 1);
            if (foundpos != std::string::npos)
            {
                foundpos = result.find_first_of('/', foundpos + 1);
                if (foundpos != std::string::npos)
                {
                    result.replace(0, foundpos /*length*/, "~");
                }
            }
        }
        result = result.substr(0, halflength);
        std::string lastpart = fpath.substr(fpathsize-halflength-1, halflength+1);
        result = result + ellipse + lastpart;
        return result;
    }
}

/**
 *  Tests that a string is not empty and has non-space characters.  Provides
 *  essentially the opposite test that string_is_void() provides.  The
 *  definition of white-space is provided by the std::isspace()
 *  function/macro.
 *
 * \param s
 *      The string pointer to check for emptiness.
 *
 * \return
 *      Returns true if the pointer is valid, the string has a non-zero
 *      length, and is not just white-space.
 */

bool
string_not_void (const std::string & s)
{
   bool result = false;
   if (! s.empty())
   {
      for (int i = 0; i < int(s.length()); ++i)
      {
         if (! std::isspace(s[i]))
         {
            result = true;
            break;
         }
      }
   }
   return result;
}

/**
 *  Tests that a string is empty or has only white-space characters.  Meant to
 *  have essentially the opposite result of string_not_void().  The meaning of
 *  empty is special here, as it refers to a string being useless as a token:
 *
 *      -  The string is of zero length.
 *      -  The string has only white-space characters in it, where the
 *         isspace() macro provides the definition of white-space.
 *
 * \param s
 *      The string pointer to check for emptiness.
 *
 * \return
 *      Returns true if the string has a zero length, or is only
 *      white-space.
 */

bool
string_is_void (const std::string & s)
{
   bool result = s.empty();
   if (! result)
      result = ! string_not_void(s);

   return result;
}

/**
 *  Compares two strings for a form of semantic equality, for the purposes of
 *  editable_event(), for example.  The strings_match() function returns true
 *  if the comparison items are identical, without case-sensitivity in
 *  character content up to the length of the secondary string.  This allows
 *  abbreviations to match. (And, in scanning routines, the first match is
 *  immediately accepted.)
 *
 * \param target
 *      The primary string in the comparison.  This is the target string, the
 *      one we hope to match.  It is <i> assumed </i> to be non-empty, and the
 *      result is false if it is empty..
 *
 * \param x
 *      The secondary string in the comparison.  It must be no longer than the
 *      target string, or the match is false.
 *
 * \return
 *      Returns true if both strings are are identical in characters, up to
 *      the length of the secondary string, with the case of the characters
 *      being insignificant.  Otherwise, false is returned.
 */

bool
strings_match (const std::string & target, const std::string & x)
{
    bool result = ! target.empty();
    if (result)
    {
        result = x.length() <= target.length();
        if (result)
        {
            for (int i = 0; i < int(x.length()); ++i)
            {
                if (std::tolower(x[i]) != std::tolower(target[i]))
                {
                    result = false;
                    break;
                }
            }
        }
    }
    return result;
}

/**
 *  Calculates the log-base-2 value of a number that is already a power of 2.
 *  Useful in converting a time signature's denominator to a Time Signature
 *  meta event's "dd" value.
 *
 * \param tsd
 *      The time signature denominator, which must be a power of 2:  2, 4, 8,
 *      16, or 32.
 *
 * \return
 *      Returns the power of 2 that achieves the \a tsd parameter value.
 */

int
log2_time_sig_value (int tsd)
{
    int result = 0;
    while (tsd > 1)
    {
        ++result;
        tsd >>= 1;
    }
    return result;
}

/**
 *  Calculates a suitable starting zoom value for the given PPQN value.  The
 *  default starting zoom is 2, but this value is suitable only for PPQN of
 *  192 and below.  Also, zoom currently works consistently only if it is a
 *  power of 2.  For starters, we scale the zoom to the selected ppqn, and
 *  then shift it each way to get a suitable power of two.
 *
 * \param ppqn
 *      The ppqn of interest.
 *
 * \return
 *      Returns the power of 2 appropriate for the given PPQN value.
 */

int
zoom_power_of_2 (int ppqn)
{
    int result = SEQ64_DEFAULT_ZOOM;
    if (ppqn > SEQ64_DEFAULT_PPQN)
    {
        int zoom = result * ppqn / SEQ64_DEFAULT_PPQN;
        zoom >>= 2;                     /* "divide" by 2    */
        zoom <<= 2;                     /* "multiply" by 2  */
        result = zoom;
        if (result > SEQ64_MAXIMUM_ZOOM)
            result = SEQ64_MAXIMUM_ZOOM;
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
beat_pow2 (int logbase2)
{
    int result;
    if (logbase2 == 0)
        result = 1;
    else
    {
        result = 2;
        for (int c = 1; c < logbase2; ++c)
            result *= 2;
    }
    return result;
}

/**
 *  Calculates the base-2 log of a number. This number is truncated to an
 *  integer byte value, as it is used in calculating values to be written to a
 *  MIDI file.
 *
 * \param value
 *      The integer value for which log2(value) is needed.
 *
 * \return
 *      Returns log2(value).
 */

midibyte
beat_log2 (int value)
{
    return midibyte(log(double(value)) / log(2.0));
}

/**
 *  Calculates the tempo in microseconds from the bytes read from a Tempo
 *  event in the MIDI file.
 *
 *  Is it correct to simply cast the bytes to a double value?
 *
 * \param tt
 *      Provides the 3-byte array of values making up the raw tempo data.
 *
 * \return
 *      Returns the result of converting the bytes to a double value.
 */

double
tempo_us_from_bytes (const midibyte tt[3])
{
    double result = double(tt[0]);
    result = (result * 256) + double(tt[1]);
    result = (result * 256) + double(tt[2]);
    return result;
}

/**
 *  Provide a way to convert a tempo value (microseconds per quarter note)
 *  into the three bytes needed as value in a Tempo meta event.  Recall the
 *  format of a Tempo event:
 *
 *  0 FF 51 03 t2 t1 t0 (tempo as number of microseconds per quarter note)
 *
 *  This code is the inverse of the lines of code around line 768 in
 *  midifile.cpp, which is basically
 *  <code> ((t2 * 256) + t1) * 256 + t0 </code>.
 *
 *  As a test case, note that the default tempo is 120 beats/minute, which is
 *  equivalent to tttttt=500000 (0x07A120).  The output of this function will
 *  be t[] = { 0x07, 0xa1, 0x20 } [the indices go 0, 1, 2].
 *
 * \param t
 *      Provides a small array of 3 elements to hold each tempo byte.
 *
 * \param tempo_us
 *      Provides the temp value in microseconds per quarter note.  This is
 *      always an integer, not a double, so do not get confused here.
 */

void
tempo_us_to_bytes (midibyte t[3], int tempo_us)
{
    t[2] = midibyte(tempo_us & 0x0000FF);
    t[1] = midibyte((tempo_us & 0x00FF00) >> 8);
    t[0] = midibyte((tempo_us & 0xFF0000) >> 16);
}

/**
 *  Converts a tempo value to a MIDI note value for the purpose of displaying
 *  a tempo value in the mainwid, seqdata section (hopefully!), and the
 *  perfroll.  It implements the following linear equation, with clamping just
 *  in case.
 *
\verbatim
                           N1 - N0
        N = N0 + (B - B0) ---------     where (N1 - N0) is always 127
                           B1 - B0
\endverbatim
 *
\verbatim
                        127
        N = (B - B0) ---------
                      B1 - B0
\endverbatim
 *
 *  where N0 = 0 (MIDI note 0 is the minimum), N1 = 127 (the maximum MIDI
 *  note), B0 is the value of usr().midi_bpm_minimum(),
 *  B1 is the value of usr().midi_bpm_maximum(), B is the input beats/minute,
 *  and N is the resulting note value.  As a precaution due to rounding error,
 *  we clamp the values between 0 and 127.
 *
 * \param tempovalue
 *      The tempo in beats/minute.
 *
 * \return
 *      Returns the tempo value scaled to the range 0 to 127, based on the
 *      configured BPM minimum and maximum values.
 */

midibyte
tempo_to_note_value (midibpm tempovalue)
{
    double slope = double(SEQ64_MAX_DATA_VALUE);
    slope /= usr().midi_bpm_maximum() - usr().midi_bpm_minimum();
    double note = (tempovalue - usr().midi_bpm_minimum()) * slope;
    if (note < 0.0)
        note = 0.0;
    else if (note > double(SEQ64_MAX_DATA_VALUE))
        note = double(SEQ64_MAX_DATA_VALUE);

    return midibyte(note);
}

/**
 *  The inverse of tempo_to_note_value().
 *
\verbatim
                  (N - N0) (B1 - B0)
        B = B0 + --------------------
                       N1 - N0
\endverbatim
 *
\verbatim
                    (B1 - B0)
        B = B0 + N -----------
                       127
\endverbatim
 *
 * \param note
 *      The note value used for displaying the tempo in the seqdata pane, the
 *      perfroll, and in a mainwid slot.
 *
 * \return
 *      Returns the tempo in beats/minute.
 */

midibpm
note_value_to_tempo (midibyte note)
{
    double slope = usr().midi_bpm_maximum() - usr().midi_bpm_minimum();
    slope *= double(note);
    slope /= double(SEQ64_MAX_DATA_VALUE);
    slope += usr().midi_bpm_minimum();
    return slope;
}

/**
 *  Calculates the quotient and remainder of a midipulse division, which is a
 *  common operation in Sequencer64.  This function also avoids division by
 *  zero (and currently ignores negative denominators, which are still
 *  possible with the current definition of the midipulse typedef.
 *
 * \param numerator
 *      Provides the numerator in the division operation.
 *
 * \param denominator
 *      Provides the denominator in the division operation.
 *
 * \param [out] remainder
 *      The remainder is written here.  If the division cannot be done, it is
 *      set to 0.
 *
 * \return
 *      Returns the result of the division.
 */

midipulse
pulse_divide (midipulse numerator, midipulse denominator, midipulse & remainder)
{
    midipulse result = 0;
    if (denominator > 0)
    {
        ldiv_t temp = ldiv(numerator, denominator);
        result = temp.quot;
        remainder = temp.rem;
    }
    else
        remainder = 0;

    return result;
}

/**
 *  Calculates a wave function for use as an LFO (low-frequency oscillator)
 *  for modifying data values in a sequence.  We extracted this function from
 *  mattias's lfownd module, as it is more generally useful.  The angle
 *  parameter is provided by the lfownd object.  It is calculated by
 *
\verbatim
                 speed * tick * BW
        angle = ------------------- + phase
                      seqlength
\endverbatim
 *
 *  The speed ranges from 0 to 16; the ratio of tick/seqlength ranges from 0
 *  to 1; BW (beat width) is generally 4; the phase ranges from 0 to 1.
 *
 * \param angle
 *      Provides the radial angle to be applied.  Units of radians,
 *      apparently.
 *
 * \param wavetype
 *      Provides the wave_type_t value to select the type of wave data-point
 *      to be generated.
 */

double
wave_func (double angle, wave_type_t wavetype)
{
    double result = 0.0;
    switch (wavetype)
    {
    case WAVE_SINE:
        result = sin(angle * PI * 2.0);
        break;

    case WAVE_SAWTOOTH:
        result = (angle - int(angle)) * 2.0 - 1.0;
        break;

    case WAVE_REVERSE_SAWTOOTH:
        result = (angle - int(angle)) * -2.0 + 1.0;
        break;

    case WAVE_TRIANGLE:
    {
        double tmp = angle * 2.0;
        result = (tmp - int(tmp));
        if ((int(tmp)) % 2 == 1)
            result = 1.0 - result;

        result = result * 2.0 - 1.0;
        break;
    }
    default:
        break;
    }
    return result;
}

/**
 *  Converts a wave type value to a string.  These names are short because I
 *  cannot figure out how to get the window pad out to show the longer names.
 *
 * \param wavetype
 *      The wave-type value to be displayed.
 *
 * \return
 *      Returns a short description of the wave type.
 */

std::string
wave_type_name (wave_type_t wavetype)
{
    std::string result = "None";
    switch (wavetype)
    {
    case WAVE_SINE:
        result = "Sine";
        break;

    case WAVE_SAWTOOTH:
        result = "Ramp Up Saw";
        break;

    case WAVE_REVERSE_SAWTOOTH:
        result = "Decay Saw";
        break;

    case WAVE_TRIANGLE:
        result = "Triangle";
        break;

    default:
        break;
    }
    return result;
}

/**
 *  Extracts the two names from the ALSA/JACK client/port name format,
 *  "[0] 128:0 clientname:portname".
 *
 *  It's a bit krufty to have to rely on that strict format; changes
 *  in the bus/port code could break this function.
 *
 *  And when a2jmidid is running, indeed this function breaks.  The
 *  name of a port changes to
 *
 *      a2j:Midi Through [14] (playback): Midi Through Port-0
 *
 *  with "a2j" as the client name and the rest, including the second colon, as
 *  the port name.
 *
 *  TODO:  FIX ME!!!!!!!
 *
 * \param fullname
 *      The full port specification to be split.
 *
 * \param [out] clientname
 *      The destination for the client name portion, "clientname".
 *
 * \param [out] portname
 *      The destination for the port name portion, "portname".
 *
 * \return
 *      Returns true if all items are non-empty after the process.
 */

bool
extract_port_names
(
    const std::string & fullname,
    std::string & clientname,
    std::string & portname
)
{
    bool result = ! fullname.empty();
    clientname.clear();
    portname.clear();
    if (result)
    {
        std::string cname;
        std::string pname;
        std::size_t colonpos = fullname.find_first_of(":"); /* not last! */
        if (colonpos != std::string::npos)
        {
            /*
             * The client name consists of all characters up the the first
             * colon.  Period.  The port name consists of all characters
             * after that colon.  Period.
             */

            cname = fullname.substr(0, colonpos);
            pname = fullname.substr(colonpos+1);
            result = ! cname.empty() && ! pname.empty();
        }
        else
            pname = fullname;

        clientname = cname;
        portname = pname;
    }
    return result;
}

/**
 *  Extracts the buss name from "bus:port".  Sometimes we don't need both
 *  parts at once.
 *
 *  However, when a2jmidid is active. the port name will have a colon in it.
 *
 * \param fullname
 *      The "bus:port" name.
 *
 * \return
 *      Returns the "bus" portion of the string.  If there is no colon, then
 *      it is assumed there is no buss name, so an empty string is returned.
 */

std::string
extract_bus_name (const std::string & fullname)
{
    std::size_t colonpos = fullname.find_first_of(":");  /* not last! */
    return (colonpos != std::string::npos) ?
        fullname.substr(0, colonpos) : std::string("");
}

/**
 *  Extracts the port name from "bus:port".  Sometimes we don't need both
 *  parts at once.
 *
 *  However, when a2jmidid is active. the port name will have a colon in it.
 *
 * \param fullname
 *      The "bus:port" name.
 *
 * \return
 *      Returns the "port" portion of the string.  If there is no colon, then
 *      it is assumed that the name is a port name, and so \a fullname is
 *      returned.
 */

std::string
extract_port_name (const std::string & fullname)
{
    std::size_t colonpos = fullname.find_first_of(":");  /* not last! */
    return (colonpos != std::string::npos) ?
        fullname.substr(colonpos + 1) : fullname ;
}

/**
 *  Gets the current date/time.
 *
 * \return
 *      Returns
 */

std::string
current_date_time ()
{
    static char s_temp[64];
    static const char * const s_format = "%Y-%m-%d %H:%M:%S";
    time_t t;
    memset(s_temp, 0, sizeof s_temp);
    time(&t);

    struct tm * tm = localtime(&t);
    strftime(s_temp, sizeof s_temp - 1, s_format, tm);
    return std::string(s_temp);
}

}       // namespace seq64

/*
 * calculations.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

