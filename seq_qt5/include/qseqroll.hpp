#ifndef SEQ64_QSEQROLL_HPP
#define SEQ64_QSEQROLL_HPP

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
 * \file          qseqroll.hpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-04-03
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 */

#include <QWidget>
#include <QPainter>
#include <QPen>
#include <QTimer>
#include <QMouseEvent>

#include "rect.hpp"
#include "sequence.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
//  class sequence;

/**
 * The MIDI note grid in the sequence editor
 */

class qseqroll : public QWidget
{
    Q_OBJECT

public:

    explicit qseqroll
    (
        perform & perf,
        sequence & seq,
        QWidget * parent = 0,
        seq64::edit_mode_t mode = EDIT_MODE_NOTE
    );

    void set_snap (int snap);
    int length () const;
    void set_note_length (int length);
    void zoom_in();
    void zoom_out();

protected:
    // override painting event to draw on the frame

    void paintEvent (QPaintEvent *);

    // override mouse events for interaction

    void mousePressEvent (QMouseEvent * event);
    void mouseReleaseEvent (QMouseEvent * event);
    void mouseMoveEvent (QMouseEvent * event);

    // override keyboard events for interaction

    void keyPressEvent (QKeyEvent * event);
    void keyReleaseEvent (QKeyEvent * event);

    // override the sizehint to set our own defaults

    QSize sizeHint () const;

private:

    void snap_y (int & y);
    void snap_x (int & x);

    /* takes screen corrdinates, give us notes and ticks */

    void convert_xy (int x, int y, midipulse & ticks, int & note);
    void convert_tn (midipulse ticks, int note, int & x, int & a_y);
    void convert_tn_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int note_h, int note_l,
        seq64::rect & r
    );

    void set_adding (bool a_adding);
    void start_paste();

    perform & perf ()
    {
        return m_perform;
    }

private:

    perform & m_perform;
    sequence & m_seq;
    seq64::rect m_old;
    seq64::rect m_selected;
    QPen * mPen;
    QBrush * mBrush;
    QPainter * mPainter;
    QTimer * mTimer;
    QFont mFont;

    int m_scale;
    int m_key;
    int m_zoom;
    int m_snap;
    int m_note_length;

    bool m_selecting; /* when highlighting a bunch of events */
    bool m_adding;
    bool m_moving;
    bool m_moving_init;
    bool m_growing;
    bool m_painting;
    bool m_paste;
    bool m_is_drag_pasting;
    bool m_is_drag_pasting_start;
    bool m_justselected_one;

    int m_drop_x;           // mouse tracking
    int m_drop_y;
    int m_move_delta_x;
    int m_move_delta_y;
    int m_current_x;
    int m_current_y;
    int m_move_snap_offset_x;

    int m_old_progress_x;   // playhead tracking
    int m_background_sequence;
    bool m_drawing_background_seq;
    seq64::edit_mode_t editMode;

    int note_x;             // note drawing variables
    int note_width;
    int note_y;
    int note_height;

    int keyY;               // dimensions
    int keyAreaY;

signals:

public slots:

    void updateEditMode (seq64::edit_mode_t mode);

};          // class qseqroll

}           // namespace seq64

#endif      // SEQ64_QSEQROLL_HPP

/*
 * qseqroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

