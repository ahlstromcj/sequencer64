#ifndef SEQ64_QPERFROLL_HPP
#define SEQ64_QPERFROLL_HPP

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
 * \file          qperfroll.hpp
 *
 *  This module declares/defines the base class for the Qt 5 version of the
 *  Performance window piano roll.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-04-02
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.
 */

#include <QWidget>
#include <QTimer>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QMouseEvent>

#include "Globals.hpp"
#include "globals.h"
#include "gui_palette_qt5.hpp"
#include "rect.hpp"

const int c_perfroll_background_x = (c_ppqn * 4 * 16) / c_perf_scale_x;
const int c_perfroll_size_box_w = 3;
const int c_perfroll_size_box_click_w = c_perfroll_size_box_w + 1 ;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 * The grid in the song editor for setting out sequences
 */

class qperfroll : public QWidget, gui_palette_qt5
{
    Q_OBJECT

public:

    explicit qperfroll (perform & p, QWidget * parent);

    int getSnap () const;
    void set_snap (int getSnap);
    void set_guides (int snap, int measure, int beat);
    void update_sizes ();
    void increment_size ();
    void zoom_in ();
    void zoom_out ();

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

public slots:

    void undo ();
    void redo ();

private:

    const perform & perf () const
    {
        return mPerf;
    }

    perform & perf ()
    {
        return mPerf;
    }

    void convert_xy (int x, int y, midipulse & ticks, int & seq);
    void convert_x (int x, midipulse & ticks);
    void snap_x (int & x);
    void snap_y (int & y);

    // We could add these function to perform and here:
    //
    //  cut_selected_trigger()
    //  copy_selected_trigger()
    //  paste_trigger()

    void add_trigger (int seq, midipulse tick);
    void half_split_trigger (int seq, midipulse tick);
    void delete_trigger (int seq, midipulse tick);
    void set_adding (bool adding);

private:

    perform & mPerf;
    QTimer * mTimer;
    QFont mFont;
    seq64::rect m_old;      // why do we need the namespace here?
    int m_snap;
    int m_measure_length;
    int m_beat_length;
    int m_roll_length_ticks;
    int m_drop_x, m_drop_y;
    int m_current_x, m_current_y;
    int m_drop_sequence;
    int m_zoom;

    // sequence selection

    midipulse m_tick_s;                     // start of tick window
    midipulse m_tick_f;                     // end of tick window
    int m_seq_h;                            // highest seq in window
    int m_seq_l;                            // lowest seq in window
    midipulse m_drop_tick;
    midipulse m_drop_tick_trigger_offset;   // ticks clicked from trigger
    midipulse mLastTick;                    // tick using at last mouse event
    bool m_sequence_active[c_max_sequence];
    bool m_moving;
    bool mBoxSelect;
    bool m_growing;
    bool m_grow_direction;
    bool m_adding;
    bool m_adding_pressed;

};          // class qperfroll

}           // namespace seq64

#endif      // SEQ64_QPERFROLL_HPP

/*
 * qperfroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

