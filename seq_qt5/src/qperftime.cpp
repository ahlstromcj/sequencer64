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
 * \updates       2018-07-30
 * \license       GNU GPLv2 or above
 *
 */

#include "qperftime.hpp"
#include "settings.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *
 */

qperftime::qperftime
(
    perform & p,
    int zoom,
    int snap,
    int appqn,
    QWidget * parent
) :
    QWidget             (parent),
    qperfbase           (p, zoom, snap, appqn, 1, 1 * 1),
    m_timer             (new QTimer(this)), // refresh timer for redraws
    m_font              (),
    m_4bar_offset       (0)
{
    m_font.setBold(true);
    QObject::connect
    (
        m_timer, SIGNAL(timeout()), this, SLOT(conditional_update())
    );
    m_timer->setInterval(usr().window_redraw_rate());    // 50
    m_timer->start();
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

/**
 *
 */

void
qperftime::conditional_update ()
{
    if (needs_update())
    {
        update();
    }
}

/**
 *
 */

void
qperftime::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QBrush brush(Qt::lightGray, Qt::SolidPattern);
    QPen pen(Qt::black);
    pen.setStyle(Qt::SolidLine);
    m_font.setPointSize(6);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);
    painter.drawRect(0, 0, width(), height());

    /*
     *  Draw the vertical lines for the measures and the beats.
     */

    midipulse tick0 = scroll_offset_ticks();
    midipulse windowticks = length_ticks(width());
    midipulse tick1 = tick0 + windowticks;
    midipulse tickstep = 1;
    int measure = 0;            // TODO

#ifdef PLATFORM_DEBUG_TMI
    printf
    (
        "ticks %ld to %ld step %ld; measure = %ld, beat = %ld\n",
        tick0, tick1, tickstep, measure_length(), beat_length()
    );
#endif

    for (midipulse tick = tick0; tick < tick1; tick += tickstep)
    {
        if (tick % measure_length() == 0)
        {
            int x_pos = position_pixel(tick);
            pen.setColor(Qt::black);                        /* measure */
            pen.setWidth(2);
            painter.setPen(pen);
            painter.drawLine(x_pos, 0, x_pos, height());

            /*
             * Draw the measure numbers if they will fit.  Determined
             * currently by trial and error.
             */

            if (zoom() >= 1 && zoom() <= 16)
            {
                QString bar(QString::number(measure + 1));
                pen.setColor(Qt::black);
                painter.setPen(pen);
                painter.drawText(x_pos + 2, 9, bar);
            }
            ++measure;
        }
#if 0
        else if (tick % beat_length() == 0)
        {
            int x_pos = position_pixel(tick);
            pen.setColor(Qt::black);                        /* beat    */
            pen.setWidth(1);
            painter.setPen(pen);
            painter.drawLine(x_pos, 0, x_pos, height());
        }
#endif
    }

    int left = position_pixel(perf().get_left_tick());
    int right = position_pixel(perf().get_right_tick());
    if (left >= scroll_offset_x() && left <= scroll_offset_x() + width())
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
    if (right >= scroll_offset_x() && right <= scroll_offset_x() + width())
    {
        pen.setColor(Qt::black);
        brush.setColor(Qt::black);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawRect(right - 7, height() - 9, 7, 10);
        pen.setColor(Qt::white);
        painter.setPen(pen);
        painter.drawText(right - 7 + 1, 21, "R");
    }
}

/**
 *
 */

QSize
qperftime::sizeHint () const
{
    int height = 24;    // 22
    int width = horizSizeHint();
    return QSize(width, height);
}

/**
 *
 */

void
qperftime::mousePressEvent (QMouseEvent * event)
{
    midipulse tick = midipulse(event->x());
    tick *= scale_zoom();
    tick += (m_4bar_offset * 16 * ppqn());
    tick -= (tick % snap());
    if (event->y() > height() * 0.5)
    {
        if (event->button() == Qt::LeftButton)  // move L/R markers
            perf().set_left_tick(tick);

        if (event->button() == Qt::RightButton)
            perf().set_right_tick(tick + snap());
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
qperftime::set_guides (int snap, int measure)
{
    set_snap(snap);
    m_measure_length = measure;
    set_dirty();
}

}           // namespace seq64

/*
 * qperftime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

