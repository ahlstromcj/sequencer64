#ifndef SEQ64_QSEQDATA_HPP
#define SEQ64_QSEQDATA_HPP

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
 * \file          qseqdata.hpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-04-02
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 */

#include <QWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

#include "midibyte.hpp"                 /* midibyte, midipulse typedefs     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class sequence;

/**
 * Displays the values for MIDI events
 * e.g. Modwheel, pitchbend etc
 */

class qseqdata : public QWidget
{
    Q_OBJECT

    friend class qseqroll;
    friend class qstriggereditor;

public:

    explicit qseqdata (sequence & seq, QWidget * parent = 0);

    void zoom_in ();
    void zoom_out ();
    void set_data_type (midibyte a_status, midibyte a_control);

protected:

    // override painting event to draw on the frame

    void paintEvent (QPaintEvent *);

    // override mouse events for interaction

    void mousePressEvent (QMouseEvent * event);
    void mouseReleaseEvent (QMouseEvent * event);
    void mouseMoveEvent (QMouseEvent * event);

    //override the sizehint to set our own defaults

    QSize sizeHint () const;

signals:

public slots:

private:

    void convert_x (int x, midipulse & tick);

private:

    sequence & m_seq;
    QRect * mOld;
    QTimer * mTimer;
    QString mNumbers;
    QFont mFont;
    int m_zoom;
    int mDropX;
    int mDropY;
    int mCurrentX;
    int mCurrentY;

    /* what is the data window currently editing ? */

    midibyte m_status;
    midibyte m_cc;

    // interaction states

    bool mLineAdjust;           // dragging a new-level adjustment slope
    bool mRelativeAdjust;       // relative adjusting notes by dragging

};          // class qseqdata

}           // namespace seq64

#endif      // SEQ64_QSEQDATA_HPP

/*
 * qseqdata.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

