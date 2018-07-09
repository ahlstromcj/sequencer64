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
 * \updates       2018-07-06
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

#include "app_limits.h"                 /* SEQ64_SEQKEY_HEIGHT macro            */
#include "midibyte.hpp"                 /* seq64::midibyte and other typedefs   */
#include "qseqbase.hpp"                 /* seq64::qseqbase base class           */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class sequence;
    class qseqdata;

/**
 *  Displays the triggers for MIDI events (e.g. Mod Wheel, Pitch Bend) in the
 *  event pane underneath the qseqroll pane.
 *
 *  Note that the qeqbase mixin class is publicly inherited so that the
 *  qseqeditrame classes can access the public member of this class.
 */

class qstriggereditor : public QWidget, public qseqbase
{
    Q_OBJECT

public:

    qstriggereditor
    (
        perform & perf,
        sequence & seq,
        qseqdata * seqdata_wid  = nullptr,
        int zoom                = SEQ64_DEFAULT_ZOOM,
        int snap                = SEQ64_DEFAULT_SNAP,
        int keyHeight           = SEQ64_SEQKEY_HEIGHT,
        QWidget * parent        = nullptr
    );

    virtual ~qstriggereditor ()
    {
        // no code needed
    }

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

    void conditional_update ();

private:

    /* checks mins / maxes..  the fills in x,y and width and height */

    void x_to_w (int x1, int x2, int & x, int & w);
    void start_paste ();
    void convert_x (int x, midipulse & tick);
    void convert_t (midipulse ticks, int & x);
    void drop_event (midipulse tick);
    void set_adding (bool adding);

private:

    qseqdata * m_seqdata_wid;
    QTimer * m_timer;
    QFont m_font;
    int m_key_y;
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

