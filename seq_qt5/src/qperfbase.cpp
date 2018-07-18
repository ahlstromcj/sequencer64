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
 * \file          qperfbase.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the song editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-07-14
 * \updates       2018-07-17
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 */

#include "perform.hpp"
#include "qperfbase.hpp"
#include "settings.hpp"                 /* seq64::choose_ppqn()             */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *
 */

qperfbase::qperfbase
(
    perform & perf,
    int zoom,
    int snap,
    int ppqn,
    int unit_height,
    int total_height
) :
    m_perform               (perf),
    m_old                   (),
    m_selected              (),
    m_zoom                  (zoom),
    m_scale                 (c_perf_scale_x / 4),
    m_scale_zoom            (m_scale * m_zoom),
    m_snap                  (snap),
    m_ppqn                  (0),
    m_beat_length           (0),
    m_measure_length        (0),
    m_selecting             (false),
    m_adding                (false),
    m_moving                (false),
    m_moving_init           (false),
    m_growing               (false),
    m_window_width          (0),            // m_window_x
    m_window_height         (0),            // m_window_y
    m_drop_x                (0),
    m_drop_y                (0),
    m_current_x             (0),
    m_current_y             (0),
    m_progress_x            (0),
    m_old_progress_x        (0),
#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    m_scroll_page           (0),
    m_progress_follow       (false),
#endif
    m_scroll_offset_ticks   (0),
    m_scroll_offset_seq     (0),
    m_scroll_offset_x       (0),
    m_scroll_offset_y       (0),
    m_unit_height           (unit_height),
    m_total_height          (total_height),
    m_is_dirty              (true)
{
    set_ppqn(ppqn);
}

/**
 *
 */

void
qperfbase::zoom_in ()
{
    if (m_zoom > 1)
    {
        m_zoom /= 2;
        m_scale_zoom = m_zoom * m_scale;
        set_dirty();
    }
}

/**
 *
 */

void
qperfbase::zoom_out ()
{
    if (m_zoom < 64)    // 32
    {
        m_zoom *= 2;
        m_scale_zoom = m_zoom * m_scale;
        set_dirty();
    }
}

/**
 *
 */

void
qperfbase::set_zoom (int z)
{
    if (z != m_zoom)
    {
        m_zoom = z;         // must be validated by the caller
        m_scale_zoom = m_zoom * m_scale;
        set_dirty();
    }
}

/**
 *  Handles changes to the PPQN value in one place.
 *
 *  The m_ticks_per_bar member replaces the global ppqn times 16.  This
 *  construct is parts-per-quarter-note times 4 quarter notes times 4
 *  sixteenth notes in a bar.  (We think...)
 *
 *  The m_scale member starts out at c_perf_scale_x, which is 32 ticks
 *  per pixel at the default tick rate of 192 PPQN.  We adjust this now.
 *  But note that this calculation still involves the c_perf_scale_x constant.
 *
 * \todo
 *      Resolve the issue of c_perf_scale_x versus m_scale in perfroll.
 */

void
qperfbase::set_ppqn (int ppqn)
{
    if (ppqn_is_valid(ppqn))
    {
        m_ppqn = choose_ppqn(ppqn);
        // m_ticks_per_bar = m_ppqn * m_divs_per_beat;             /* 16 */
        // m_background_x = (m_ppqn * 4 * 16) / c_perf_scale_x;
        // m_scale = m_zoom * m_ppqn / SEQ64_DEFAULT_PPQN;
        // m_w_scale_x = sm_perfroll_size_box_click_w * m_scale;
        // if (m_scale == 0)
        //     m_scale = 1;
        ///// m_scale = c_perf_scale_x * m_ppqn / SEQ64_DEFAULT_PPQN;
        m_scale_zoom = m_zoom * m_scale;
        m_beat_length = m_ppqn;
        m_measure_length = m_beat_length * 4;
    }
}

#ifdef USE_SCROLLING_CODE    // not ready for this class

/**
 *  Sets the horizontal scroll value according to the current value of the
 *  horizontal scroll-bar.
 */

void
qperfbase::set_scroll_x (int x)
{
    m_scroll_offset_x = x;
    m_scroll_offset_ticks = x * m_zoom;
}

/**
 *  Sets the vertical scroll value according to the current value of the
 *  vertical scroll-bar.
 *
 *  TODO:  use the height member....
 */

void
qperfbase::set_scroll_y (int y)
{
    m_scroll_offset_y = y;
    m_scroll_offset_seq * m_unit_height;        // c_key_y;
    m_scroll_offset_seq = y / m_unit_height;    // c_key_y;
}

#endif  // USE_SCROLLING_CODE

/**
 *
 *  snap = number pulses to snap to
 *
 *  m_zoom = number of pulses per pixel
 *
 *  so snap / m_zoom  = number pixels to snap to
 */

void
qperfbase::snap_x (int & x)
{
    int mod = m_snap / m_scale_zoom;
    if (mod <= 0)
        mod = 1;

    x -= x % mod;
}

/**
 *  Checks to see if the song is running or if the "dirty" flag had been
 *  set.  The obtuse code here helps in debugging.
 */

bool
qperfbase::needs_update () const
{
    bool result = const_cast<qperfbase *>(this)->check_dirty();
    if (! result)
        result = perf().is_running();

    return result;
}

/**
 *
 */

void
qperfbase::convert_x (int x, midipulse & tick)
{
    midipulse tick_offset = 0;                  // it's always this!!!
    tick = x * m_scale_zoom;
    tick += tick_offset;
}

/**
 *
 */

void
qperfbase::convert_xy (int x, int y, midipulse & tick, int & seq)
{
//  tick = x * m_zoom;
//  seq = (m_total_height - y - 2) / m_unit_height;

    midipulse tick_offset =  0;                 // again, always 0!!!
    tick = x * m_scale_zoom;
    seq = y / c_names_y;
    tick += tick_offset;
    if (seq >= c_max_sequence)
        seq = c_max_sequence - 1;

    if (seq < 0)
        seq = 0;
}

/**
 *
 */

void
qperfbase::convert_ts (midipulse ticks, int seq, int & x, int & y)
{
    x = ticks /  m_zoom;
    y = m_total_height - ((seq + 1) * m_unit_height) - 1;
}

/**
 *
 * \param tick_s
 *      The starting tick of the rectangle.
 *
 * \param tick_f
 *      The finishing tick of the rectangle.
 *
 * \param seq_h
 *      The high sequence row of the rectangle.
 *
 * \param seq_l
 *      The low sequence row of the rectangle.
 *
 * \param [out] r
 *      The destination rectangle for the calculations.
 */

void
qperfbase::convert_ts_box_to_rect
(
    midipulse tick_s, midipulse tick_f, int seq_h, int seq_l,
    seq64::rect & r
)
{
    int x1, y1, x2, y2;
    convert_ts(tick_s, seq_h, x1, y1);         /* convert box to X,Y values */
    convert_ts(tick_f, seq_l, x2, y2);
    rect::xy_to_rect(x1, y1, x2, y2, r);
    r.height_incr(m_unit_height);
}

}           // namespace seq64

/*
 * qperfbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

