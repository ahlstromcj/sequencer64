#ifndef SEQ64_SCALES_H
#define SEQ64_SCALES_H

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
 * \file          scales.h
 *
 *  This module declares/defines just the scales-related global variables.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-06
 * \updates       2016-05-05
 * \license       GNU GPLv2 or above
 *
 *  These values were moved from the globals module.
 */

#include <string>

#include "easy_macros.h"                /* with platform_macros.h, too  */

/**
 *  A manifest constant for the normal number of semitones in an
 *  equally-tempered octave.
 */

#define SEQ64_OCTAVE_SIZE               12

/**
 *  A constant for clarification of the value of zero, which, in the context
 *  of a musical key, is the default key of C.
 */

#define SEQ64_KEY_OF_C                  0

namespace seq64
{

/**
 *  Corresponds to the small number of musical scales that the application
 *  can handle.  Scales can be shown in the piano roll as gray bars for
 *  reference purposes.
 *
 *  We've added three more scales; there are still a number of them that could
 *  be fruitfully added to the list of scales.
 *
 *  It would be good to offload this stuff into a new "scale" class.
 */

enum c_music_scales
{
    c_scale_off,
    c_scale_major,
    c_scale_minor,
    c_scale_harmonic_minor,
    c_scale_melodic_minor,
    c_scale_c_whole_tone,
    c_scale_size            // a "maximum" or "size of set" value.
};

/**
 *  Each value in the kind of scale is denoted by a true value in these
 *  arrays.  See the following sites for more information:
 *
 *      http://method-behind-the-music.com/theory/scalesandkeys/
 *
 *      https://en.wikipedia.org/wiki/Heptatonic_scale
 *
 *  Note that melodic minor descends in the same way as the natural minor
 *  scale, so it descends differently than it ascends.  We don't deal with
 *  that trick, at all.
 *
\verbatim
    Chromatic           C  C# D  D# E  F  F# G  G# A  A# B   Notes, chord
    Major               C  .  D  .  E  F  .  G  .  A  .  B
    Minor               C  .  D  Eb .  F  .  G  Ab .  Bb .
    Harmonic Minor      C  .  D  Eb .  F  .  G  Ab .  .  B
    Melodic Minor       C  .  D  Eb .  F  .  G  .  A  .  B   Descending diff.
    C Whole Tone        C  .  D  .  E  .  F# .  G# .  A# .   C+7 chord
    C Lydian Dominant   C  .  D  .  E  .  F# .  G  .  A  Bb  Unimplemented, A7
    A Mixolydian        C# .  D  .  E  .  F# .  G  .  A  .   Unimplemented, A7
    B Whole Tone        .  Db .  Eb .  F  .  G  .  A  .  B   Unimplemented
    G Whole Tone        .  C# .  D# .  F  .  G  .  A  .  B   Unimplemented, same
    G Octatonic         .  C# D  .  E  F  .  G  Ab .  Bb B   Unimplemented
\endverbatim
 */

const bool c_scales_policy[c_scale_size][SEQ64_OCTAVE_SIZE] =
{
    {                                                       /* off = chromatic */
        true, true, true, true, true, true,
        true, true, true, true, true, true
    },
    {                                                       /* major           */
        true, false, true, false, true, true,
        false, true, false, true, false, true
    },
    {                                                       /* minor           */
        true, false, true, true, false, true,
        false, true, true, false, true, false
    },
    {                                                       /* harmonic minor  */
        true, false, true, true, false, true,
        false, true, true, false, false, true
    },
    {                                                       /* melodic minor   */
        true, false, true, true, false, true,
        false, true, false, true, false, true
    },
    {                                                       /* whole tone      */
        true, false, true, false, true, false,
        true, false, true, false, true, false
    },
};

/**
 *  Increment values needed to transpose each scale up so that it remains
 *  in the same key.  For example, if we simply add 1 semitone to each
 *  note, it remains a minor key, but it is in a different minor key.
 *  Using the transpositions in these arrays, the minor key remains the
 *  same minor key.
 *
\verbatim
    Major               C  .  D  .  E  F  .  G  .  A  .  B
    Transpose up        2  0  2  0  1  2  0  2  0  2  0  1
    Result up           D  .  E  .  F  G  .  A  .  B  .  C

    Minor               C  .  D  D# .  F  .  G  G# .  A# .
    Transpose up        2  0  1  2  0  2  0  1  2  0  2  0
    Result up           D  .  D# F  .  G  .  G# A# .  C  .

    Harmonic minor      C  .  D  Eb .  F  .  G  Ab .  .  B
    Transpose up        2  .  1  2  .  2  .  1  3  .  .  1
    Result up           D  .  Eb F  .  G  .  Ab B  .  .  C

    Melodic minor       C  .  D  Eb .  F  .  G  .  A  .  B
    Transpose up        2  .  1  2  .  2  .  2  .  2  .  1
    Result up           D  .  Eb F  .  G  .  A  .  B  .  C

    C Whole Tone        C  .  D  .  E  .  F# .  G# .  A# .
    Transpose up        2  .  2  .  2  .  2  .  2  .  2  .
    Result up           D  .  E  .  F# .  G# .  A# .  C  .
\endverbatim
 */

const int c_scales_transpose_up[c_scale_size][SEQ64_OCTAVE_SIZE] =
{
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},              /* off = chromatic */
    { 2, 0, 2, 0, 1, 2, 0, 2, 0, 2, 0, 1},              /* major           */
    { 2, 0, 1, 2, 0, 2, 0, 1, 2, 0, 2, 0},              /* minor           */
    { 2, 0, 1, 2, 0, 2, 0, 1, 3, 0, 0, 1},              /* harmonic minor  */
    { 2, 0, 1, 2, 0, 2, 0, 2, 0, 2, 0, 1},              /* melodic minor   */
    { 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0},              /* C whole tone    */
};

const int c_scales_transpose_dn[c_scale_size][SEQ64_OCTAVE_SIZE] =
{
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},  /* off = chromatic */
    { -1, 0, -2, 0, -2, -1, 0, -2, 0, -2, 0, -2},       /* major           */
    { -2, 0, -2, -1, 0, -2, 0, -2, -1, 0, -2, 0},       /* minor           */
    { -1, -0, -2, -1, 0, -2, 0, -2, -1, 0, 0, -3},      /* harmonic minor  */
    { -1, 0, -2, -1, 0, -2, 0, -2, 0, -2, 0, -2},       /* melodic minor   */
    { -2, 0, -2, 0, -2, 0, -2, 0, -2, 0, -2, 0},        /* C whole tone    */
};

/**
 *  Making these positive makes it easier to read.  Just remember to
 *  negate each value before using it.
 *
\verbatim
    Major               C  .  D  .  E  F  .  G  .  A  .  B
    Transpose down      1  0  2  0  2  1  0  2  0  2  0  2
    Result down         B  .  C  .  D  E  .  F  .  G  .  A
\endverbatim
 *
\verbatim
    Minor               C  .  D  D# .  F  .  G  G# .  A# .
    Transpose down      2  0  2  1  0  2  0  2  1  0  2  0
    Result down         A# .  C  D  .  D# .  F  G  .  G# .
\endverbatim
 *
\verbatim
    Harmonic minor      C  .  D  Eb .  F  .  G  Ab .  .  B
    Transpose down      1  .  2  1  .  2  .  2  1  .  .  3
    Result down         B  .  C  D  .  Eb .  F  G  .  .  Ab
\endverbatim
 *
\verbatim
    Melodic minor       C  .  D  Eb .  F  .  G  .  A  .  B
    Transpose down      1  .  2  1  .  2  .  2  .  2  .  2
    Result down         B  .  C  D  .  Eb .  F  .  G  .  A
\endverbatim
 *
\verbatim
    C whole tone        C  .  D  .  E  .  F# .  G# .  A# .
    Transpose down      2  .  2  .  2  .  2  .  2  .  2  .
    Result down         A# .  C  .  D  .  E  .  F# .  G# .
\endverbatim
 */

const int c_scales_transpose_dn_neg[c_scale_size][SEQ64_OCTAVE_SIZE] =
{
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},                  /* off = chromatic */
    { 1, 0, 2, 0, 2, 1, 0, 2, 0, 2, 0, 2},                  /* major           */
    { 2, 0, 2, 1, 0, 2, 0, 2, 1, 0, 2, 0},                  /* minor           */
    { 1, 0, 2, 1, 0, 2, 0, 2, 1, 0, 0, 3},                  /* harmonic minor  */
    { 1, 0, 2, 1, 0, 2, 0, 2, 0, 2, 0, 2},                  /* melodic minor   */
    { 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0},                  /* C whole tone    */
};

/**
 *  The names of the currently-supported scales.
 */

const char c_scales_text[c_scale_size][32] =                /* careful!        */
{
    "Off (chromatic)",
    "Major",
    "Minor",
    "Harmonic Minor",
    "Melodic Minor",
    "Whole Tone",
};

/**
 *  Provides the entries for the Key dropdown menu in the Pattern Editor
 *  window.
 */

const char c_key_text[SEQ64_OCTAVE_SIZE][3] =
{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

/**
 *  Provides the entries for the Interval dropdown menu in the Pattern Editor
 *  window.
 */

const char c_interval_text[16][3] =
{
    "P1", "m2", "M2", "m3", "M3", "P4", "TT", "P5",
    "m6", "M6", "m7", "M7", "P8", "m9", "M9", ""
};

/**
 *  Provides the entries for the Chord dropdown menu in the Pattern Editor
 *  window.  However, we have not seen this menu in the GUI!  Ah, it only
 *  appears if the user has selected a musical scale like Major or Minor.
 */

const char c_chord_text[8][5] =
{
    "I", "II", "III", "IV", "V", "VI", "VII", "VIII"
};

}           // namespace seq64

#endif      // SEQ64_SCALES_H

/*
 * scales.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

