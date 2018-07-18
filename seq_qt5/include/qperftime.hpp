#ifndef SEQ64_QPERFTIME_HPP
#define SEQ64_QPERFTIME_HPP

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
 * \file          qperftime.hpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-07-15
 * \license       GNU GPLv2 or above
 *
 */

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QObject>
#include <QPen>
#include <QMouseEvent>

#include "globals.h"
#include "perform.hpp"
#include "qperfbase.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 * The time bar for the song editor
 */

class qperftime : public QWidget, public qperfbase
{
    friend class qperfeditframe;    /* for scrolling a horizontal page  */

    Q_OBJECT

public:

    qperftime
    (
        perform & a_perf,
        int zoom            = SEQ64_DEFAULT_PERF_ZOOM,
        int snap            = SEQ64_DEFAULT_SNAP,
        int ppqn            = SEQ64_USE_DEFAULT_PPQN,
        QWidget * parent    = nullptr
    );

    virtual ~qperftime ()
    {
        // no code needed
    }

    void set_guides (int snap, int measure);

protected:      // override Qt event handlers

    void paintEvent (QPaintEvent *);
    void mousePressEvent (QMouseEvent * event);
    void mouseReleaseEvent (QMouseEvent * event);
    void mouseMoveEvent (QMouseEvent * event);
    QSize sizeHint() const;

private:

    QTimer * m_timer;
    QFont m_font;
    int m_4bar_offset;
    int m_measure_length;

signals:

    // no signals

public slots:

    void conditional_update ();

};          // class qperftime

}           // namespace seq64

#endif      // SEQ64_QPERFTIME_HPP

/*
 * qperftime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

