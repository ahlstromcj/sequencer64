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
 * \file          qseqtime.cpp
 *
 *  This module declares/defines the base class for drawing the
 *  time/measures bar at the top of the patterns/sequence editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-06-19
 * \license       GNU GPLv2 or above
 *
 */

#include "Globals.hpp"
#include "perform.hpp"
#include "qseqtime.hpp"
#include "sequence.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *
 */

qseqtime::qseqtime
(
    perform & p,
    sequence & seq,
    int zoom,
    QWidget * parent
) :
    QWidget                 (parent),
    m_perform               (p),
    m_seq                   (seq),
    m_timer                 (new QTimer(this)),  // timer to do regular redraws
    m_font                  (),
    m_zoom                  (zoom),
    m_scroll_offset_ticks   (0),
    m_scroll_offset_x       (0)
{
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
    m_timer->setInterval(50);
    m_timer->start();
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

/**
 *  Draws the time panel.
 */

void
qseqtime::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QBrush brush(Qt::lightGray, Qt::SolidPattern);
    QPen pen(Qt::black);
    m_font.setPointSize(6);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);

    /*
     * Draw the border
     */

    painter.drawRect
    (
        c_keyboard_padding_x, 0, size().width(), size().height() - 1
    );

    /*
     * The ticks_per_step value needs to be figured out.  Why 6 * m_zoom?  6
     * is the number of pixels in the smallest divisions in the default
     * seqroll background.
     *
     * This code needs to be put into a function.
     */

    int bpbar = m_seq.get_beats_per_bar();
    int bwidth = m_seq.get_beat_width();
    int ticks_per_beat = (4 * perf().ppqn()) / bwidth;
    int ticks_per_bar = bpbar * ticks_per_beat;
    int measures_per_line = m_zoom * bwidth * bpbar * 2;
    if (measures_per_line <= 0)
        measures_per_line = 1;

    int ticks_per_step = 6 * m_zoom;
    int starttick = m_scroll_offset_ticks -
            (m_scroll_offset_ticks % ticks_per_step);
    int endtick = width() * m_zoom + m_scroll_offset_ticks;

    pen.setColor(Qt::black);
    painter.setPen(pen);
    for (int tick = starttick; tick <= endtick; tick += ticks_per_step)
    {
        char bar[8];
        int x_offset = tick / m_zoom + c_keyboard_padding_x - m_scroll_offset_x;

        /*
         * Vertical line at each bar; number each bar.
         */

        if (tick % ticks_per_bar == 0)
        {
            painter.drawLine(x_offset, 0, x_offset, size().height());
            snprintf(bar, sizeof bar, "%d", tick / ticks_per_bar + 1);
#ifdef SEQ64_SOLID_PIANOROLL_GRID
            pen.setWidth(2);                    // two pixels
#endif
            painter.setPen(pen);
            painter.drawText(x_offset + 3, 10, bar);
        }
        else if (tick % ticks_per_beat == 0)
        {
#ifdef SEQ64_SOLID_PIANOROLL_GRID
            pen.setWidth(1);                    // two pixels
#endif
            painter.setPen(pen);
            pen.setStyle(Qt::SolidLine);        // pen.setColor(Qt::DashLine)
            painter.drawLine(x_offset, 0, x_offset, size().height());
        }
    }

    int end_x = m_seq.get_length() / m_zoom +
        c_keyboard_padding_x - m_scroll_offset_x;

    // draw end of seq label, label background

    pen.setColor(Qt::white);
    brush.setColor(Qt::white);
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawRect(end_x + 1, 13, 15, 8);         // white background
    pen.setColor(Qt::black);                        // black label text
    painter.setPen(pen);
    painter.drawText(end_x + 1, 21, tr("END"));
}

/**
 *
 */

void
qseqtime::mousePressEvent (QMouseEvent *)
{
    // no code
}

/**
 *
 */

void
qseqtime::mouseReleaseEvent (QMouseEvent *)
{
    // no code
}

/**
 *
 */

void
qseqtime::mouseMoveEvent(QMouseEvent *)
{
    // no code
}

/**
 *
 */

QSize
qseqtime::sizeHint() const
{
    return QSize(m_seq.get_length() / m_zoom + 100 + c_keyboard_padding_x, 22);
}

/**
 *
 */

void
qseqtime::zoom_in ()
{
    if (m_zoom > 1)
        m_zoom *= 0.5;
}

/**
 *
 */

void
qseqtime::zoom_out ()
{
    if (m_zoom < 32)
        m_zoom *= 2;
}

}           // namespace seq64

/*
 * qseqtime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

