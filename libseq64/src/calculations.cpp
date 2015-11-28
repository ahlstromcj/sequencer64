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
 * \updates       2015-11-28
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

#include "calculations.hpp"

namespace seq64
{

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

}       // namespace seq64

/*
 * calculations.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

