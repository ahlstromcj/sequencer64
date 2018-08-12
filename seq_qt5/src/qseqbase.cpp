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
 * \file          qseqbase.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-08-11
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 */

#include "perform.hpp"
#include "qseqbase.hpp"
#include "settings.hpp"                 /* seq64::choose_ppqn()             */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *
 */

qseqbase::qseqbase
(
    perform & p,
    sequence & seq,
    int zoom,
    int snap,
    int /*ppqn*/,
    int unit_height,
    int total_height
) :
    m_perform               (p),
    m_seq                   (seq),
    m_old                   (),
    m_selected              (),
    m_zoom                  (zoom),
    m_snap                  (snap),
    m_ppqn                  (p.get_ppqn()),
    m_selecting             (false),
    m_adding                (false),
    m_moving                (false),
    m_moving_init           (false),
    m_growing               (false),
    m_painting              (false),
    m_paste                 (false),
    m_is_drag_pasting       (false),
    m_is_drag_pasting_start (false),
    m_justselected_one      (false),
    m_drop_x                (0),
    m_drop_y                (0),
    m_move_delta_x          (0),
    m_move_delta_y          (0),
    m_current_x             (0),
    m_current_y             (0),
    m_move_snap_offset_x    (0),
    m_progress_x            (0),
    m_old_progress_x        (0),
    m_scroll_page           (0),
    m_progress_follow       (false),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_key     (0),
    m_scroll_offset_x       (0),
    m_scroll_offset_y       (0),
    m_unit_height           (unit_height),
    m_total_height          (total_height),
    m_is_dirty              (true)
{
    set_snap(m_seq.get_snap_tick());
}

#ifdef USE_SCROLLING_CODE    // not ready for this class

/**
 *  Sets the horizontal scroll value according to the current value of the
 *  horizontal scroll-bar.
 */

void
qseqbase::set_scroll_x (int x)
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
qseqbase::set_scroll_y (int y)
{
    m_scroll_offset_y = y;
    m_scroll_offset_key * c_key_y;          // m_unit_height
    m_scroll_offset_key = y / c_key_y;      // m_unit_height
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
qseqbase::snap_x (int & x)
{
    int mod = (m_snap / m_zoom);
    if (mod <= 0)
        mod = 1;

    x -= x % mod;
}

/**
 *  Handles changes to the PPQN value in one place.
 *
 *  The m_ticks_per_bar member replaces the global ppqn times 16.  This
 *  construct is parts-per-quarter-note times 4 quarter notes times 4
 *  sixteenth notes in a bar.  (We think...)
 *
 *  The m_perf_scale_x member starts out at c_perf_scale_x, which is 32 ticks
 *  per pixel at the default tick rate of 192 PPQN.  We adjust this now.
 *  But note that this calculation still involves the c_perf_scale_x constant.
 *
 * \todo
 *      Resolve the issue of c_perf_scale_x versus m_perf_scale_x in perfroll.
 */

void
qseqbase::set_ppqn (int ppqn)
{
    if (ppqn_is_valid(ppqn))
    {
        m_ppqn = choose_ppqn(ppqn);
    }
}

/**
 *
 */

bool
qseqbase::needs_update () const
{
    bool dirty = const_cast<qseqbase *>(this)->check_dirty();
    perform & ncp = const_cast<perform &>(perf());
    return ncp.needs_update(seq().number()) || dirty;
}

/**
 *  Set the measures value, using the given parameter, and some internal
 *  values passed to apply_length().
 *
 * \param len
 *      Provides the sequence length, in measures.
 */

void
qseqbase::set_measures (int len)
{
    seq().apply_length(len);
    set_dirty();
}

/**
 *
 */

int
qseqbase::get_measures ()
{
    return seq().get_measures();
}

/**
 *
 */

void
qseqbase::convert_xy (int x, int y, midipulse & tick, int & note)
{
    tick = x * m_zoom;
    note = (m_total_height - y - 2) / m_unit_height;
}

/**
 *
 */

void
qseqbase::convert_tn (midipulse ticks, int note, int & x, int & y)
{
    x = ticks /  m_zoom;
    y = m_total_height - ((note + 1) * m_unit_height) - 1;
}

/**
 *  See seqroll::convert_sel_box_to_rect() for a potential upgrade.
 *
 * \param tick_s
 *      The starting tick of the rectangle.
 *
 * \param tick_f
 *      The finishing tick of the rectangle.
 *
 * \param note_h
 *      The high note of the rectangle.
 *
 * \param note_l
 *      The low note of the rectangle.
 *
 * \param [out] r
 *      The destination rectangle for the calculations.
 */

void
qseqbase::convert_tn_box_to_rect
(
    midipulse tick_s, midipulse tick_f, int note_h, int note_l,
    seq64::rect & r
)
{
    int x1, y1, x2, y2;
    convert_tn(tick_s, note_h, x1, y1);         /* convert box to X,Y values */
    convert_tn(tick_f, note_l, x2, y2);
    rect::xy_to_rect(x1, y1, x2, y2, r);
    r.height_incr(m_unit_height);
}

/**
 *  Get the box that selected elements are in, then adjust for clipboard
 *  being shifted to tick 0.
 */

void
qseqbase::start_paste ()
{
    snap_x(m_current_x);
    snap_y(m_current_x);
    m_drop_x = m_current_x;
    m_drop_y = m_current_y;
    m_paste = true;

    midipulse tick_s, tick_f;
    int note_l, note_h;
    m_seq.get_clipboard_box(tick_s, note_h, tick_f, note_l);
    convert_tn_box_to_rect(tick_s, tick_f, note_h, note_l, m_selected);
    m_selected.xy_incr(m_drop_x, m_drop_y - m_selected.y());
}

}           // namespace seq64

/*
 * qseqbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

