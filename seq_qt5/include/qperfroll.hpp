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
 * \updates       2018-07-15
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

#include "globals.h"
#include "gui_palette_qt5.hpp"
#include "qperfbase.hpp"
#include "rect.hpp"

/*
 * TODO: allow runtime adjustment of PPQN here.
 */

const int c_perfroll_background_x = (SEQ64_DEFAULT_PPQN * 4 * 16) / c_perf_scale_x;
const int c_perfroll_size_box_w = 3;
const int c_perfroll_size_box_click_w = c_perfroll_size_box_w + 1 ;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class qperfeditframe;

/**
 * The grid in the song editor for setting out sequences
 */

class qperfroll : public QWidget, private gui_palette_qt5, public qperfbase
{
    friend class qperfeditframe;    /* for scrolling a horizontal page  */

    Q_OBJECT

public:

    qperfroll
    (
        perform & p,
        int zoom                = SEQ64_DEFAULT_PERF_ZOOM,
        int snap                = SEQ64_DEFAULT_SNAP,
        int ppqn                = SEQ64_USE_DEFAULT_PPQN,
        qperfeditframe * frame  = nullptr,
        QWidget * parent        = nullptr
    );

    virtual ~qperfroll ();

    void set_guides (int snap, int measure, int beat);
    void update_sizes ();
    void increment_size ();

protected:      // Qt event-function overrides

    void paintEvent (QPaintEvent *);    // override painting event to draw on frame
    void mousePressEvent (QMouseEvent * event);
    void mouseReleaseEvent (QMouseEvent * event);
    void mouseMoveEvent (QMouseEvent * event);
    void keyPressEvent (QKeyEvent * event);
    void keyReleaseEvent (QKeyEvent * event);
    QSize sizeHint () const;            // override sizehint to set our defaults

public slots:

    void undo ();
    void redo ();
    void conditional_update ();

private:

    virtual void set_adding (bool adding);

    // We could add these function to perform and here:
    //
    //  cut_selected_trigger()
    //  copy_selected_trigger()
    //  paste_trigger()

    void add_trigger (int seq, midipulse tick);
    void half_split_trigger (int seq, midipulse tick);
    void delete_trigger (int seq, midipulse tick);
    void follow_progress ();

private:

    qperfeditframe * m_parent_frame;
    QTimer * m_timer;
    QFont m_font;
    int m_measure_length;
    int m_beat_length;
    int m_roll_length_ticks;
    int m_drop_sequence;                    // sequence selection
    midipulse m_tick_s;                     // start of tick window
    midipulse m_tick_f;                     // end of tick window
    int m_seq_h;                            // highest seq in window
    int m_seq_l;                            // lowest seq in window
    midipulse m_drop_tick;
    midipulse m_drop_tick_trigger_offset;   // ticks clicked from trigger
    midipulse mLastTick;                    // tick using at last mouse event
    bool m_sequence_active[c_max_sequence];
    bool mBoxSelect;
    bool m_grow_direction;
    bool m_adding_pressed;

};          // class qperfroll

}           // namespace seq64

#endif      // SEQ64_QPERFROLL_HPP

/*
 * qperfroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

