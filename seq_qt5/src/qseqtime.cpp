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
 * \updates       2018-06-27
 * \license       GNU GPLv2 or above
 *
 */

#include "Globals.hpp"
#include "perform.hpp"
#include "qseqtime.hpp"
#include "sequence.hpp"
#include "settings.hpp"                 /* seq64::usr().key_height(), etc.  */

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
    qseqbase                (p, seq, zoom, SEQ64_DEFAULT_SNAP),
    m_timer                 (nullptr),
    m_font                  ()
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_timer = new QTimer(this);                             // redraw timer !!!
    m_timer->setInterval(2 * usr().window_redraw_rate());   // 50
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->start();
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::needs_update().
 */

void
qseqtime::conditional_update ()
{
    if (needs_update())
        update();
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

    int bpbar = seq().get_beats_per_bar();
    int bwidth = seq().get_beat_width();
    int ticks_per_beat = (4 * perf().ppqn()) / bwidth;
    int ticks_per_bar = bpbar * ticks_per_beat;
    int measures_per_line = zoom() * bwidth * bpbar * 2;
    if (measures_per_line <= 0)
        measures_per_line = 1;

    int ticks_per_step = 6 * zoom();
    int starttick = scroll_offset_ticks() -
            (scroll_offset_ticks() % ticks_per_step);
    int endtick = width() * zoom() + scroll_offset_ticks();

    pen.setColor(Qt::black);
    painter.setPen(pen);
    for (int tick = starttick; tick <= endtick; tick += ticks_per_step)
    {
        char bar[8];
        int x_offset = tick / zoom() + c_keyboard_padding_x - scroll_offset_x();

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

    int end_x = seq().get_length() / zoom() +
        c_keyboard_padding_x - scroll_offset_x();

    /*
     * Draw end of seq label, label background.
     */

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
    return QSize(seq().get_length() / zoom() + 100 + c_keyboard_padding_x, 22);
}

}           // namespace seq64

/*
 * qseqtime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

