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
 * \updates       2015-11-30
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
 * \return
 *      Returns true if a reasonable portion (3 numbers) was good for
 *      extraction.  The fraction part will start with a period for easier
 *      conversion to
 *      seconds.
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
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "measures:beats:ticks" ("measures:beats:division").
 */

std::string
pulses_to_measures (midipulse /*pulses*/, int /*ppqn*/)
{
    std::string result;

    return result;
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "hours:minutes:seconds.fraction".
 */

std::string
pulses_to_time (midipulse pulses, int bpm, int ppqn)
{
    unsigned long microseconds = ticks_to_delta_time_us(pulses, bpm, ppqn);
    int seconds = long(microseconds * (1.0 / 1000000.0));
    int minutes = seconds / 60;
    int hours = minutes / 60;
    minutes -= hours * 60;
    seconds -= minutes * 60;
    microseconds -= seconds * 1000000.0;
    char tmp[32];
    snprintf
    (
        tmp, sizeof tmp, "%d:%d:%d.%06lu",
        hours, minutes, seconds, microseconds
    );
    return std::string(tmp);
}

/**
 *  Converts a string that represents "measures:beats:ticks" to a MIDI
 *  pulse/ticks/clock value.
 *
 *  24 seconds in a 120 bpm, 4/4 project:
 *
 *  beats_per_measure = ts_num;                 // = 4
 *  position = 24 / 60 * 120;                   // = 48
 *  meas_num = floor(position /4) +1;           // = 13
 *  beat_num = floor(position - (meas_num-1) * 4) +1;              // =1
 *  tick_num = (position - (meas_num-1) * 4 - (beat_num-1)) * 960; // = 0
 *
 */

midipulse
measures_to_pulses_4_4 (const std::string & measures, int /*ppqn*/)
{
    midipulse result = 0;
    if (! measures.empty())
    {
        // todo
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
time_to_pulses (const std::string & timestring, int bpm, int ppqn)
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
 *
 *//*-------------------------------------------------------------------------*/

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

