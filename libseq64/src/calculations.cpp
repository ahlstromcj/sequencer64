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
 * \updates       2015-12-05
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

#include <cctype>                       /* std::isspace(), etc.             */
#include <math.h>                       /* C::floor()                       */
#include <stdlib.h>                     /* C::atoi()                        */
#include <time.h>                       /* C::strftime()                    */

#include "calculations.hpp"

namespace seq64
{

/**
 *  Extracts up to 4 numbers from a colon-delimited string.
 *
 *      -   measures : beats : divisions
 *          -   "213:4:920"
 *          -   "0:1:0"
 *      -   hours : minutes : seconds . fraction
 *          -   "2:04:12.14"
 *          -   "0:1:2"
 *
 * \warning
 *      This is not the most efficient implementation you'll ever see.
 *      At some point we will tighten it up.  This function is tested in the
 *      seq64-tests project, in the "calculations_unit_test" module.
 *
 * \return
 *      Returns true if a reasonable portion (3 numbers) was good for
 *      extraction.  The fraction part will start with a period for easier
 *      conversion to fractional seconds.
 */

bool
extract_timing_numbers
(
    const std::string & s,
    std::string & part_1,
    std::string & part_2,
    std::string & part_3,
    std::string & fraction
)
{
    bool result = false;
    std::size_t colon_1_pos = s.find_first_of(":");
    part_1.clear();
    part_2.clear();
    part_3.clear();
    fraction.clear();
    if (colon_1_pos != std::string::npos)
    {
        std::size_t colon_2_pos = s.find_first_of(":", colon_1_pos + 1);
        if (colon_2_pos != std::string::npos)
        {
            std::size_t period_pos = s.find_first_of(".", colon_2_pos + 1);
            if (period_pos != std::string::npos)
            {
                fraction = s.substr(period_pos);
                std::size_t len = period_pos - colon_2_pos - 1;
                if (len > 0)
                {
                    part_3 = s.substr(colon_2_pos + 1, len);
                    result = true;
                }
            }
            else
            {
                part_3 = s.substr(colon_2_pos + 1);
                result = true;
            }
            std::size_t len = colon_2_pos - colon_1_pos - 1;
            if (len > 0)
                part_2 = s.substr(colon_1_pos + 1, len);
        }
        std::size_t number_pos = s.find_first_of("0123456789");
        if (number_pos != std::string::npos)
        {
            std::size_t len = colon_1_pos - number_pos;
            if (len > 0)
                part_1 = s.substr(number_pos, len);
        }
    }
    return result;
}

/**
 *  Converts MIDI pulses (also known as ticks, clocks, or divisions) into a
 *  string.
 *
 * \todo
 *      Still needs to be unit tested.
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
 * \param seqparms
 *      This small structure provides the beats/measure, beat-width, and PPQN
 *      that hold for the sequence involved in this calculation.
 *
 * \return
 *      Returns the absolute pulses that mark this duration.
 */

std::string
pulses_to_measurestring (midipulse p, const midi_timing & seqparms)
{
    midi_measures measures;
    char tmp[32];
    pulses_to_midi_measures(p, seqparms, measures); /* fill measures struct */
    snprintf
    (
        tmp, sizeof tmp, "%d:%d:%d",
        measures.measures(), measures.beats(), measures.divisions()
    );
    return std::string(tmp);
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "measures:beats:ticks" ("measures:beats:division").
 *
 *      m = p * W / (4 * P * B)
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
 * \param measures
 *      Provides the current MIDI song time structure holding the results,
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
 *  "hours:minutes:seconds.fraction".  See the pulses_to_timestring()
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
 *      Provides the pulse-per-quarter-note of the song.
 */

std::string
pulses_to_timestring (midipulse p, int bpm, int ppqn)
{
    unsigned long microseconds = ticks_to_delta_time_us(p, bpm, ppqn);
    int seconds = int(microseconds / 1000000UL);
    int minutes = seconds / 60;
    int hours = seconds / (60 * 60);
    minutes -= hours * 60;
    seconds -= (hours * 60 * 60) + (minutes * 60);
    microseconds -= (hours * 60 * 60 + minutes * 60 + seconds) * 1000000UL;

    char tmp[32];
    if (microseconds == 0)
    {
        snprintf(tmp, sizeof tmp, "%d:%d:%d", hours, minutes, seconds);
    }
    else
    {
        snprintf
        (
            tmp, sizeof tmp, "%d:%d:%d.%06lu",
            hours, minutes, seconds, microseconds
        );
    }
    return std::string(tmp);
}

/**
 *  Converts a string that represents "measures:beats:division" to a MIDI
 *  pulse/ticks/clock value.
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
 *      Returns the absolute pulses that mark this duration.
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
        if (extract_timing_numbers(measures, m, b, d, dummy))
        {
            midi_measures meas_values;
            meas_values.measures(atoi(m.c_str()));
            meas_values.beats(atoi(b.c_str()));
            meas_values.divisions(atoi(d.c_str()));
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
 *  "0:0:0" as one might expect.
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
 *      pulse-value cannot be calculated, then SEQ64_ILLEGAL_PULSE is
 *      returned.
 */

midipulse
midi_measures_to_pulses
(
    const midi_measures & measures,
    const midi_timing & seqparms
)
{
    midipulse result = SEQ64_ILLEGAL_PULSE;
    int m = measures.measures() - 1;                /* true measure count   */
    int b = measures.beats() - 1;
    if (m >= 0 && b >= 0)
    {
        double qn_per_beat = 4.0 / seqparms.beat_width();
        result = 0;
        if (m > 0)
            result += int(m * seqparms.beats_per_measure() * qn_per_beat);

        if (b > 0)
            result += int(b * qn_per_beat);

        result *= seqparms.ppqn();
        result += measures.divisions();

    }
    return result;
}

/**
 *  Converts a string that represents "hours:minutes:seconds.fraction" into a
 *  MIDI pulse/ticks/clock value.
 *
 * \param timestring
 *      The time value to be converted, which must be of the form
 *      "hh:mm:ss" or "hh:mm:ss.fraction".
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
timestring_to_pulses (const std::string & timestring, int bpm, int ppqn)
{
    midipulse result = 0;
    if (! timestring.empty())
    {
        std::string sh, sm, ss, us;
        if (extract_timing_numbers(timestring, sh, sm, ss, us))
        {
            int hours = atoi(sh.c_str());
            int minutes = atoi(sm.c_str());
            int seconds = atoi(ss.c_str());

            /*
             * This conversion assumes that the fractional parts of the seconds
             * is padded with zeroes on the left or right to 6 digits.
             */

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
 *  Shortens a file-specification to make sure it is no longer than the
 *  provided length value.  This is done by removing character in the middle,
 *  if necessary, and replacing them with an ellipse.
 *
 *  This function operates by first trying to find the <tt> /home </tt>
 *  directory.  If found, it strips off <tt> /home/username </tt> and replace
 *  it with the Linux <tt> ~ </tt> replacement for the <tt> $HOME </tt>
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
 *    Tests that a string is not empty and has non-space characters.
 *    Provides essentially the opposite test that string_is_void()
 *    provides.
 *
 * \param s
 *      The string pointer to check for emptiness.
 *
 * \return
 *    Returns 'true' if the pointer is valid, the string has a non-zero
 *    length, and is not just white-space.
 *
 *    The definition of white-space is provided by the isspace()
 *    function/macro.
 */

bool
string_not_void (const std::string & s)
{
   bool result = false;
   if (! s.empty())
   {
      for (int i = 0; i < int(s.length()); i++)
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
 *    Tests that a string is empty or has only white-space characters.  Meant
 *    to have essentially the opposite result of string_not_void().  The
 *    meaning of empty is special here, as it refers to a string being useless
 *    as a token:
 *
 *      -  The string is of zero length.
 *      -  The string has only white-space characters in it, where the
 *         isspace() macro provides the definition of white-space.
 *
 * \param s
 *      The string pointer to check for emptiness.
 *
 * \return
 *    Returns 'true' if the string has a zero length, or is only white-space.
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
 *    Compares two strings for a form of semantic equality, for the purposes
 *    of editable_event(), for example.  The strings_match() function returns
 *    true if the comparison items are identical, without case-sensitivity
 *    in character content up to the length of the secondary string.
 *    This allows abbreviations to match. (And, in scanning routines, the
 *    first match is immediately accepted.)
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
 *    Returns true if both strings are are identical in characters, up to the
 *    length of the secondary string, with the case of the characters being
 *    insignificant.  Otherwise, false is returned.
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
            for (int i = 0; i < int(x.length()); i++)
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

}       // namespace seq64

/*
 * calculations.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

