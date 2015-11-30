#ifndef SEQ64_MIDIBYTE_HPP
#define SEQ64_MIDIBYTE_HPP

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
 * \file          midibyte.hpp
 *
 *  This module declares a number of useful typedefs.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-23
 * \updates       2015-11-30
 * \license       GNU GPLv2 or above
 *
 *  These typedefiare intended to remove the ambiguity we have seen between
 *  signed and unsigned values.  MIDI bytes and pulses, ticks, or clocks are,
 *  by their nature, unsigned, and we should enforce that.
 *
 *  One minor issue is why we didn't tack on "_t" to most of these types, to
 *  adhere to C conventions.  Well, no real reason except to save a couple
 *  characters.  Besides, it is easy to set up vim to highlight these new
 *  types in a special color, making them stand out easily while reading the
 *  code.
 */

/*
 *
#include <limits.h>                     // ULONG_MAX and other limits   //
 *
 *  Since we're using unsigned variables for counting pulses, we can't do the
 *  occasional test for negativity, we have to use wraparound.  One way is to
 *  use this macro.  However, we will probably just ignore the issue of
 *  wraparound.  With 32-bit longs, we have a maximum of 4,294,967,295.
 *  Even at an insame PPQN of 9600, that's almost 450,000 quarter notes.
 *  And for 64-bit code?  Forgeddaboudid!
 *
#define IS_SEQ64_MIDIPULSE_WRAPAROUND(x)  ((x) > (ULONG_MAX / 2))
 *
 */
 
namespace seq64
{

/**
 *  Provides a fairly common type definition for a byte value.
 */

typedef unsigned char midibyte;

/**
 *  Distinguishes a bus number from other MIDI bytes.
 */

typedef unsigned char bussbyte;

/**
 *  Distinguishes a long value from the unsigned long values implicit in
 *  long-valued MIDI numbers.
 */

typedef unsigned long midilong;

/**
 *  Distinguishes a long value from the unsigned long values implicit in MIDI
 *  time measurements.
 *
 *  HOWEVER, CURRENTLY, if you make this value unsigned, then perfroll won't
 *  show any notes in the sequence bars!!!
 */

typedef long midipulse;

/**
 *  We anticipate the need to have a small structure holding the parameters
 *  needed to calculate MIDI times within an arbitrary song.  Although
 *  Seq24/Sequencer64 currently are heavily dependent on hard-wired values,
 *  that will be rectified eventually, so let us get ready for it.
 *
 * \var mt_beats_per_measure
 *      This value should match the numerator value selected when editing the
 *      sequence.  This value is most commonly set to 4.
 *
 * \var mt_beat_width
 *      This value should match the denominator value selected when editing the
 *      sequence.  This value is most commonly set to 4, meaning that the
 *      fundamental beat unit is the quarter note.
 *
 * \var mt_beats_per_mintue
 *      This value should match the BPM value selected when editing the
 *      song.  This value is most commonly set to 120, but is also read from
 *      the MIDI file.
 *
 * \var mt_ppqn
 *      This value provides the precision of the MIDI song.  
 *      This value is most commonly set to 192, but is also read from the MIDI
 *      file.  We are still working getting "non-standard" values to work.
 */

typedef struct
{
    int mt_beats_per_measure;
    int mt_beat_width;
    int mt_beats_per_minute;
    int mt_ppqn;

} midi_timing_t;


}           // namespace seq64

#endif      // SEQ64_MIDIBYTE_HPP

/*
 * midibyte.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

