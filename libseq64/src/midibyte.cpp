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
 * \file          midibyte.cpp
 *
 *  This module declares a couple of useful data classes.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-12-05
 * \updates       2016-05-07
 * \license       GNU GPLv2 or above
 *
 *  These classes were originally structures, but now they are "constant"
 *  classes, filled in at construction time and accessed only through getter
 *  functions.
 */

#include "app_limits.h"                 /* SEQ64_DEFAULT_PPQN               */
#include "midibyte.hpp"                 /* seq64::midi_timing, _measures    */

namespace seq64
{

/**
 *  Default constructor for midi_measures.
 */

midi_measures::midi_measures ()
 :
    m_measures      (0),
    m_beats         (0),
    m_divisions     (0)
{
    // Empty body
}

/**
 *  Principal constructor for midi_measures.
 *
 * \param measures
 *      Copied into the m_measures member.
 *
 * \param beats
 *      Copied into the m_beats member.
 *
 * \param divisions
 *      Copied into the m_divisions member.
 */

midi_measures::midi_measures
(
    int measures,
    int beats,
    int divisions
) :
    m_measures      (measures),
    m_beats         (beats),
    m_divisions     (divisions)
{
    // Empty body
}

/**
 *  Defaults constructor for midi_timing.
 */

midi_timing::midi_timing ()
 :
    m_beats_per_minute      (0),
    m_beats_per_measure     (0),
    m_beat_width            (0),
    m_ppqn                  (0)
{
    // Empty body
}

/**
 *  Principal constructor for midi_timing.
 *
 * \param bpminute
 *      Copied into the m_beats_per_minute member.
 *
 * \param bpmeasure
 *      Copied into the m_beats_per_measure member.
 *
 * \param beatwidth
 *      Copied into the m_beat_width member.
 *
 * \param ppqn
 *      Copied into the m_ppqn member.
 */

midi_timing::midi_timing
(
    int bpminute,
    int bpmeasure,
    int beatwidth,
    int ppqn
) :
    m_beats_per_minute      (bpminute),
    m_beats_per_measure     (bpmeasure),
    m_beat_width            (beatwidth),
    m_ppqn                  (ppqn)
{
    // Empty body
}

}           // namespace seq64

/*
 * midibyte.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

