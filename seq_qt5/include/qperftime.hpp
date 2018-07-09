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
 * \updates       2018-03-03
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

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 * The time bar for the song editor
 */

class qperftime : public QWidget
{
    Q_OBJECT

public:

    explicit qperftime (perform & a_perf, QWidget * parent);

    virtual ~qperftime ()
    {
        // no code needed
    }

    void zoom_in();
    void zoom_out();
    void set_guides(int a_snap, int a_measure);

protected:

    //override painting event to draw on the frame
    void paintEvent(QPaintEvent *);

    //override mouse events for interaction
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

    //override the sizehint to set our own defaults
    QSize sizeHint() const;

    const perform & perf () const
    {
        return m_mainperf;
    }

    perform & perf ()
    {
        return m_mainperf;
    }

private:

    perform & m_mainperf;
    QTimer * mTimer;
    QFont mFont;

    int m_4bar_offset;
    int m_snap;
    int m_measure_length;
    int zoom;

signals:

public slots:

};          // class qperftime

}           // namespace seq64

#endif      // SEQ64_QPERFTIME_HPP

/*
 * qperftime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

