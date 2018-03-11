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
 * \file          qperftime.cpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-03-11
 * \license       GNU GPLv2 or above
 *
 */

#include "Globals.hpp"
#include "qperftime.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *
 */

qperftime::qperftime(perform & p, QWidget * parent)
 :
    QWidget             (parent),
    m_mainperf          (p),
    mTimer              (new QTimer(this)), // refresh timer for redraws
    mFont               (),
    m_4bar_offset       (0),
    m_snap              (c_ppqn),
    m_measure_length    (c_ppqn * 4),
    zoom                (1)
{
    QObject::connect(mTimer, SIGNAL(timeout()), this, SLOT(update()));
    mTimer->setInterval(50);
    mTimer->start();
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

/**
 *
 */

void
qperftime::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush(Qt::lightGray, Qt::SolidPattern);

    mFont.setPointSize(6);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(mFont);
    painter.drawRect(0, 0, width(), height());

    /* draw vert lines */

    midipulse tick_offset = 0;
    int first_measure = 0;
    int last_measure = 1 + first_measure +
        (width() * c_perf_scale_x / (m_measure_length));

    for (int i = first_measure; i < last_measure; ++i)
    {
        int x_pos = ((i * m_measure_length) - tick_offset) /
            (c_perf_scale_x * zoom);

        pen.setColor(Qt::black);          /* beat */
        painter.setPen(pen);
        painter.drawLine(x_pos, 0, x_pos, height());

        if (zoom <= 2)          // only draw these numbers if they'll fit
        {
            QString bar(QString::number(i + 1));
            pen.setColor(Qt::black);
            painter.setPen(pen);
            painter.drawText(x_pos + 2, 9, bar);
        }
    }

    midipulse left = perf().get_left_tick();
    midipulse right = perf().get_right_tick();
    left -= (m_4bar_offset * 16 * c_ppqn);
    left /= c_perf_scale_x * zoom;
    right -= (m_4bar_offset * 16 * c_ppqn);
    right /= c_perf_scale_x * zoom;
    if (left >= 0 && left <= width())
    {
        pen.setColor(Qt::black);
        brush.setColor(Qt::black);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawRect(left, height() - 9, 7, 10);
        pen.setColor(Qt::white);
        painter.setPen(pen);
        painter.drawText(left + 1, 21, "L");
    }
    if (right >= 0 && right <= width())
    {
        pen.setColor(Qt::black);
        brush.setColor(Qt::black);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawRect(right - 6, height() - 9, 7, 10);
        pen.setColor(Qt::white);
        painter.setPen(pen);
        painter.drawText(right - 6 + 1, 21, "R");
    }
}

/**
 *
 */

QSize
qperftime::sizeHint () const
{
    return QSize
    (
        perf().get_max_trigger() / (zoom * c_perf_scale_x) + 2000, 22
    );
}

/**
 *
 */

void
qperftime::mousePressEvent (QMouseEvent * event)
{
    midipulse tick = midipulse(event->x());
    tick *= c_perf_scale_x * zoom;
    tick += (m_4bar_offset * 16 * c_ppqn);
    tick -= (tick % m_snap);
    if (event->y() > height() * 0.5)
    {
        if (event->button() == Qt::LeftButton)  // move L/R markers
            perf().set_left_tick(tick);

        if (event->button() == Qt::RightButton)
            perf().set_right_tick(tick + m_snap);
    }
    else
        perf().set_tick(tick);              // reposition timecode
}

/**
 *
 */

void
qperftime::mouseReleaseEvent (QMouseEvent *)
{
    // no code
}

/**
 *
 */

void
qperftime::mouseMoveEvent (QMouseEvent *)
{
    // no code
}

/**
 *
 */

void
qperftime::zoom_in ()
{
    if (zoom > 1)
        zoom *= 0.5;
}

/**
 *
 */

void
qperftime::zoom_out ()
{
    zoom *= 2;
}

/**
 *
 */

void
qperftime::set_guides (int snap, int measure)
{
    m_snap = snap;
    m_measure_length = measure;
}

}           // namespace seq64

/*
 * qperftime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

