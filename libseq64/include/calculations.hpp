#ifndef SEQ64_CALCULATIONS_HPP
#define SEQ64_CALCULATIONS_HPP

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
 * \file          calculations.hpp
 *
 *  This module declares/defines some common calculations needed by the
 *  application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-11-07
 * \updates       2018-04-28
 * \license       GNU GPLv2 or above
 *
 *  These items were moved from the globals.h module so that only the modules
 *  that need them need to include them.  Also included are some minor
 *  "utility" functions dealing with strings.
 *
 *  Many of the functions are defined in this header file, as inline code.
 */

#include <string>
#include <vector>

#include "app_limits.h"                 /* SEQ64_DEFAULT_PPQN           */
#include "easy_macros.h"                /* with platform_macros.h, too  */
#include "midibyte.hpp"                 /* midipulse typedef            */

/**
 *  The MIDI beat clock (also known as "MIDI timing clock" or "MIDI clock") is
 *  a clock signal that is broadcast via MIDI to ensure that several
 *  MIDI-enabled devices or sequencers stay in synchronization.  Do not
 *  confuse it with "MIDI timecode".
 *
 *  The standard MIDI beat clock ticks every 24 times every quarter note
 *  (crotchet).
 *
 *  Unlike MIDI timecode, the MIDI beat clock is tempo-dependent. Clock events
 *  are sent at a rate of 24 ppqn (pulses per quarter note). Those pulses are
 *  used to maintain a synchronized tempo for synthesizers that have
 *  BPM-dependent voices and also for arpeggiator synchronization.
 *
 *  The following macro represents the standard MIDI clock rate in
 *  pulses-per-quarter-note.
 */

#define SEQ64_MIDI_CLOCK_IN_PPQN               24

/*
 * Global functions in the seq64 namespace for MIDI timing calculations.
 */

namespace seq64
{

/**
 *  Provides a clear enumation of wave types supported by the wave function.
 *  We still have to clarify these type values, though.
 */

enum wave_type_t
{
    WAVE_NONE               = 0,    /**< No waveform, never used.           */
    WAVE_SINE               = 1,    /**< Sine wave modulation.              */
    WAVE_SAWTOOTH           = 2,    /**< Saw-tooth (ramp) modulation.       */
    WAVE_REVERSE_SAWTOOTH   = 3,    /**< Reverse saw-tooth (decay).         */
    WAVE_TRIANGLE           = 4     /**< No waveform, never used.           */
};

/*
 * Free functions in the seq64 namespace.
 */

extern std::string wave_type_name (wave_type_t wv);
extern int extract_timing_numbers
(
    const std::string & s,
    std::string & part_1,
    std::string & part_2,
    std::string & part_3,
    std::string & fraction
);
extern int tokenize_string
(
    const std::string & source,
    std::vector<std::string> & tokens
);
extern std::string pulses_to_string (midipulse p);
extern std::string pulses_to_measurestring
(
    midipulse p,
    const midi_timing & seqparms
);
extern bool pulses_to_midi_measures
(
    midipulse p,
    const midi_timing & seqparms,
    midi_measures & measures
);
extern std::string pulses_to_timestring
(
    midipulse p,
    const midi_timing & timinginfo
);
extern std::string pulses_to_timestring
(
    midipulse pulses, midibpm bpm, int ppqn, bool showus = true
);
extern midipulse measurestring_to_pulses
(
    const std::string & measures,
    const midi_timing & seqparms
);
extern midipulse midi_measures_to_pulses
(
    const midi_measures & measures,
    const midi_timing & seqparms
);
extern midipulse timestring_to_pulses
(
    const std::string & timestring,
    int bpm, int ppqn
);
extern midipulse string_to_pulses
(
    const std::string & s,
    const midi_timing & mt
);
extern midibyte string_to_midibyte (const std::string & s);
extern std::string shorten_file_spec (const std::string & fpath, int leng);
extern bool string_not_void (const std::string & s);
extern bool string_is_void (const std::string & s);
extern bool strings_match (const std::string & target, const std::string & x);
extern int log2_time_sig_value (int tsd);
extern int zoom_power_of_2 (int ppqn);
extern int beat_pow2 (int logbase2);
extern midibyte beat_log2 (int value);
extern double tempo_us_from_bytes (const midibyte tt[3]);
extern void tempo_us_to_bytes (midibyte t[3], int tempo_us);
extern midibyte tempo_to_note_value (midibpm tempo);
extern midibpm note_value_to_tempo (midibyte note);

/**
 *  Common code for handling PPQN settings.  Validates a PPQN value.
 *
 * \param ppqn
 *      Provides the PPQN value to be used.
 *
 * \return
 *      Returns true if the ppqn parameter is between MINIMUM_PPQN and
 *      MAXIMUM_PPQN, or is set to SEQ64_USE_DEFAULT_PPQN (-1).
 */

inline bool
ppqn_is_valid (int ppqn)
{
    return
    (
        ppqn == SEQ64_USE_DEFAULT_PPQN ||
        (ppqn >= SEQ64_MINIMUM_PPQN && ppqn <= SEQ64_MAXIMUM_PPQN)
    );
}

/**
 *  Converts tempo (e.g. 120 beats/minute) to microseconds.
 *  This function is the inverse of bpm_from_tempo_us().
 *
 * \param bpm
 *      The value of beats-per-minute.  If this value is 0, we'll get an
 *      arithmetic exception.
 *
 * \return
 *      Returns the tempo in qn/us.  If the bpm value is 0, then 0 is
 *      returned.
 */

inline double
tempo_us_from_bpm (midibpm bpm)
{
    return bpm > 0.0 ? (60000000.0 / bpm) : 0.0 ;
}

/**
 *  This function calculates the effective beats-per-minute based on the value
 *  of a Tempo meta-event.  The tempo event's numeric value is given in 3
 *  bytes, and is in units of microseconds-per-quarter-note (us/qn).
 *
 * \param tempous
 *      The value of the Tempo meta-event, in units of us/qn.  If this value
 *      is 0, we'll get an arithmetic exception.
 *
 * \return
 *      Returns the beats per minute.  If the tempo value is 0, then 0 is
 *      returned.
 */

inline midibpm
bpm_from_tempo_us (double tempous)
{
    return tempous > 0.0 ? (60000000.0 / tempous) : 0.0 ;
}

/**
 *  Provides a direct conversion from a midibyte array to the beats/minute
 *  value.
 *
 *  It might be worthwhile to provide an std::vector version at some point.
 *
 * \param t
 *      The 3 tempo midibytes that were read directly from a MIDI file.
 */

inline midibpm
bpm_from_bytes (midibyte t[3])
{
    return bpm_from_tempo_us(tempo_us_from_bytes(t));
}

/**
 *  Calculates pulse-length from the BPM (beats-per-minute) and PPQN
 *  (pulses-per-quarter-note) values.  The formula for the pulse-length in
 *  seconds is:
 *
\verbatim
                 60
        P = ------------
             BPM * PPQN
\endverbatim
 *
 * \param bpm
 *      Provides the beats-per-minute value.  No sanity check is made.  If
 *      this value is 0, we'll get an arithmetic exception.
 *
 * \param ppqn
 *      Provides the pulses-per-quarter-note value.  No sanity check is
 *      made.  If this value is 0, we'll get an arithmetic exception.
 *
 * \return
 *      Returns the pulse length in microseconds.  If either parameter is
 *      invalid, then this function will crash. :-D
 */

inline double
pulse_length_us (midibpm bpm, int ppqn)
{
    /*
     * Let's use the original notation for now.
     *
     * return 60000000.0 / double(bpm * ppqn);
     */

    return 60000000.0 / ppqn / bpm;
}

/**
 *  Converts delta time in microseconds to ticks.  This function is the
 *  inverse of ticks_to_delta_time_us().
 *
 *  Please note that terms "ticks" and "pulses" are equivalent, and refer to
 *  the "pulses" in "pulses per quarter note".
 *
\verbatim
             beats       pulses           1 minute       1 sec
    P = 120 ------ * 192 ------ * T us *  ---------  * ---------
            minute       beats            60 sec       1,000,000 us
\endverbatim
 *
 *  Note that this formula assumes that a beat is a quarter note.  If a beat
 *  is an eighth note, then the P value would be halved, because there would
 *  be only 96 pulses per beat.  We will implement an additional function to
 *  account for the beat; the current function merely blesses some
 *  calculations made in the application.
 *
 * \param us
 *      The number of microseconds in the delta time.
 *
 * \param bpm
 *      Provides the beats-per-minute value, otherwise known as the "tempo".
 *
 * \param ppqn
 *      Provides the pulses-per-quarter-note value, otherwise known as the
 *      "division".
 *
 * \return
 *      Returns the tick value.
 */

inline double
delta_time_us_to_ticks (unsigned long us, midibpm bpm, int ppqn)
{
    return double(bpm * ppqn * (us / 60000000.0f));
}

/**
 *  Converts the time in ticks ("clocks") to delta time in microseconds.
 *  The inverse of delta_time_us_to_ticks().
 *
 *  Please note that terms "ticks" and "pulses" are equivalent, and refer to
 *  the "pulses" in "pulses per quarter note".
 *
 *  Old:  60000000.0 * double(delta_ticks) / (double(bpm) * double(ppqn));
 *
 * \param delta_ticks
 *      The number of ticks or "clocks".
 *
 * \param bpm
 *      Provides the beats-per-minute value, otherwise known as the "tempo".
 *
 * \param ppqn
 *      Provides the pulses-per-quarter-note value, otherwise known as the
 *      "division".
 *
 * \return
 *      Returns the time value in microseconds.
 */

inline double
ticks_to_delta_time_us (midipulse delta_ticks, midibpm bpm, int ppqn)
{
    return double(delta_ticks) * pulse_length_us(bpm, ppqn);
}

/**
 *  Calculates the duration of a clock tick based on PPQN and BPM settings.
 *
 * \deprecated
 *      This is a somewhat bogus calculation used only for "statistical"
 *      output in the old perform module.  Name changed to reflect this
 *      unfortunate fact.  Use pulse_length_us() instead.
 *
\verbatim
                       60000000 ppqn
        us = ---------------------------------
              MIDI_CLOCK_IN_PPQN * bpm * ppqn
\endverbatim
 *
 *  MIDI_CLOCK_IN_PPQN is 24.
 *
 * \param bpm
 *      Provides the beats-per-minute value.  No sanity check is made.  If
 *      this value is 0, we'll get an arithmetic exception.
 *
 * \param ppqn
 *      Provides the pulses-per-quarter-note value.  No sanity check is
 *      made.  If this value is 0, we'll get an arithmetic exception.
 *
 * \return
 *      Returns the clock tick duration in microseconds.  If either parameter
 *      is invalid, this will crash.  Who wants to waste time on value checks
 *      here? :-D
 */

inline double
clock_tick_duration_bogus (midibpm bpm, int ppqn)
{
    return (ppqn / SEQ64_MIDI_CLOCK_IN_PPQN) * 60000000.0 / (bpm * ppqn);
}

/**
 *  A simple calculation to convert PPQN to MIDI clock ticks.
 *
 * \param ppqn
 *      The number of pulses per quarter note.  For example, the default value
 *      for Seq24 is 192.
 *
 * \return
 *      The integer value of ppqn / 24 [MIDI_CLOCK_IN_PPQN] is returned.
 */

inline int
clock_ticks_from_ppqn (int ppqn)
{
    return ppqn / SEQ64_MIDI_CLOCK_IN_PPQN;
}

/**
 *  A simple calculation to convert PPQN to MIDI clock ticks.  The same as
 *  clock_ticks_from_ppqn(), but returned as a double float.
 *
 * \param ppqn
 *      The number of pulses per quarter note.
 *
 * \return
 *      The double value of ppqn / 24 [SEQ64_MIDI_CLOCK_IN_PPQN]_is returned.
 */

inline double
double_ticks_from_ppqn (int ppqn)
{
    return ppqn / double(SEQ64_MIDI_CLOCK_IN_PPQN);
}

/**
 *  Calculates the pulses per measure.  This calculation is extremely simple,
 *  and it provides an important constraint to pulse (ticks) calculations:
 *  the number of pulses in a measure is always 4 times the PPQN value,
 *  regardless of the time signature.  The number pulses in a 7/8 measure is the
 *  the same as in a 4/4 measure.
 */

inline midipulse
pulses_per_measure (int ppqn = SEQ64_DEFAULT_PPQN)
{
    return 4 * ppqn;
}

/**
 *  Calculates the length of an integral number of measures, in ticks.
 *  This function is called in seqedit::apply_length(), when the user
 *  selects a sequence length in measures.  That function calculates the
 *  length in ticks.  The number of pulses is given by the number of quarter
 *  notes times the pulses per quarter note.  The number of quarter notes is
 *  given by the measures times the quarter notes per measure.  The quarter
 *  notes per measure is given by the beats per measure times 4 divided by
 *  beat_width beats.  So:
 *
\verbatim
    p = 4 * P * m * B / W
        p == pulse count (ticks or pulses)
        m == number of measures
        B == beats per measure (constant)
        P == pulses per quarter-note (constant)
        W == beat width in beats per measure (constant)
\endverbatim
 *
 *  For our "b4uacuse" MIDI file, M can be about 100 measures, B is 4,
 *  P can be 192 (but we want to support higher values), and W is 4.
 *  So p = 100 * 4 * 4 * 192 / 4 = 76800 ticks.
 *
 *  Note that 4 * P is a constraint encapsulated by the inline function
 *  pulses_per_measure().
 *
 * \param bpb
 *      The B value in the equation, beats/measure or beats/bar.
 *
 * \param ppqn
 *      The P value in the equation, pulses/qn.
 *
 * \param bw
 *      The W value in the equation, the denominator of the time signature.
 *      If this value is 0, we'll get an arithmetic exception (crash), so we
 *      just return 0 in this case.
 *
 * \param measures
 *      The M value in the equation.  It defaults to 1, in case one desires a
 *      simple "ticks per measure" number.
 *
 * \return
 *      Returns the L value (ticks or pulses) as calculated via the given
 *      equation.  If bw is 0, then 0 is returned.
 */

inline midipulse
measures_to_ticks (int bpb, int ppqn, int bw, int measures = 1)
{
    return (bw > 0) ? midipulse(4 * ppqn * measures * bpb / bw) : 0 ;
}

/**
 *  The inverse of measures_to_ticks.
 *
 * \param bpb
 *      The B value in the equation, beats/measure or beats/bar.
 *
 * \param ppqn
 *      The P value in the equation, pulses/qn.
 *
 * \param bw
 *      The W value in the equation, the denominator of the time signature.
 *      If this value is 0, we'll get an arithmetic exception (crash), so we
 *      just return 0 in this case.
 *
 * \param ticks
 *      The p (pulses) value in the equation.  It defaults to 192, in case one
 *      desires a default "ticks per measure" number.
 *
 * \return
 *      Returns the M value (measures or bars) as calculated via the inverse
 *      equation.  If ppqn or bpb are 0, then 0 is returned.
 */

inline int
ticks_to_measures (int bpb, int ppqn, int bw, midipulse ticks = 192)
{
    return (ppqn > 0 && bpb > 0.0) ?
        (ticks * bw) / (4.0 * ppqn * bpb) : 0 ;
}

/*
 *  Free functions in the seq64 namespace.
 */

extern midipulse pulse_divide
(
    midipulse numerator, midipulse denominator, midipulse & remainder
);
extern double wave_func (double angle, wave_type_t wavetype);
extern bool extract_port_names
(
    const std::string & fullname,
    std::string & clientname,
    std::string & portname
);
extern std::string extract_bus_name (const std::string & fullname);
extern std::string extract_port_name (const std::string & fullname);
extern std::string current_date_time ();

}           // namespace seq64

#endif      // SEQ64_CALCULATIONS_HPP

/*
 * calculations.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

