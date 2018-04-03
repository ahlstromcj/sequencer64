#ifndef SEQ64_QSTRIGGEREDITOR_HPP
#define SEQ64_QSTRIGGEREDITOR_HPP

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
 * \file          qstriggereditor.hpp
 *
 *  This module declares/defines the base class for the Performance window
 *  piano roll.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-03-07
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.
 */

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QPen>

#include "midibyte.hpp"                 /* seq64::midibyte and other typedefs   */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class sequence;
    class qseqdata;

/**
 * Displays the triggers for MIDI events
 * e.g. Modwheel, pitchbend etc
 */

class qstriggereditor : public QWidget
{
    Q_OBJECT

public:

    explicit qstriggereditor
    (
        sequence & seq,
        qseqdata & seqdata_wid,
        QWidget * parent = 0,
        int keyHeight = 12
    );
    void zoom_in ();
    void zoom_out ();
    void set_data_type (midibyte a_status, midibyte a_control);

protected:

    void paintEvent (QPaintEvent *);
    void mousePressEvent (QMouseEvent * event);
    void mouseReleaseEvent (QMouseEvent * event);
    void mouseMoveEvent (QMouseEvent * event);
    void keyPressEvent (QKeyEvent * event);
    void keyReleaseEvent (QKeyEvent * event);
    QSize sizeHint () const;

signals:

public slots:

private:

    /* checks mins / maxes..  the fills in x,y and width and height */

    void x_to_w (int x1, int x2, int & x, int & w);
    void start_paste ();
    void convert_x (int x, midipulse & tick);
    void convert_t (midipulse ticks, int & x);
    void drop_event (midipulse tick);
    void snap_y (int & y);
    void snap_x (int & x);
    void set_adding (bool adding);

private:

    sequence & m_seq;
    qseqdata & m_seqdata_wid;
    QRect * m_old;
    QRect * m_selected;
    QTimer * mTimer;
    QFont mFont;

    int m_zoom;             /* one pixel == m_zoom ticks */
    int m_snap;
    int m_window_x;
    int m_window_y;
    int keyY;

    bool m_selecting;       /* when highlighting a bunch of events */
    bool m_moving_init;
    bool m_moving;
    bool m_growing;
    bool m_painting;
    bool m_paste;
    bool m_adding;

    int m_drop_x;           /* where the dragging started */
    int m_drop_y;
    int m_current_x;
    int m_current_y;
    int m_move_snap_offset_x;

    midibyte m_status;      /* what is seqdata currently editing? */
    midibyte m_cc;

};          // class qstriggereditor

}           // namespace seq64

#endif      // SEQ64_QSTRIGGEREDITOR_HPP

/*
 * qstriggereditor.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

