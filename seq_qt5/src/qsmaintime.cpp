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
 * \file          qsmaintime.hpp
 *
 *  This module declares/defines the base class for the "time" progress
 *  window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-07-31
 * \license       GNU GPLv2 or above
 *
 */

#include "Globals.hpp"
#include "qsmaintime.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *
 */

qsmaintime::qsmaintime
(
    perform & perf,
    QWidget * parent,
    int beats_per_measure,
    int beat_width
) :
    QWidget             (parent),
    m_main_perf         (perf),
    m_color             (new QColor(Qt::red)),
    m_font              (),
    m_beats_per_measure (beats_per_measure),
    m_beat_width        (beat_width),
    m_last_metro        (0),
    m_alpha             (0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_font.setPointSize(9);
    m_font.setBold(true);
}

/**
 *  what about the pens, brushes, etc???
 */

qsmaintime::~qsmaintime ()
{
    if (not_nullptr(m_color))
        delete m_color;
}

/**
 *
 */

void
qsmaintime::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QPen pen(Qt::darkGray);
    QBrush brush(Qt::NoBrush);

    painter.setPen(pen);
    painter.setFont(m_font);
    painter.setBrush(brush);

    midipulse tick = m_main_perf.get_tick();
    int boxwidth = (width() - 1) / m_beats_per_measure;
    int metro = (tick / (perf().get_ppqn() / 4 * m_beat_width)) %
        m_beats_per_measure;

    /*
     * Flash on beats (i.e. if the metronome has changed, or we've just started
     * playing).
     *
     * \todo
     *      We need to select a color from the palette.
     */

    if (metro != m_last_metro || (tick < 50 && tick > 0))
    {
        m_alpha = 230;
        if (metro == 0)
            m_color->setRgb(255, 50, 50);       // red on first beat in bar
        else
            m_color->setRgb(255, 255, 255);     // white on others
    }

    for (int i = 0; i < m_beats_per_measure; ++i)       // draw beat blocks
    {
        int offsetX = boxwidth * i;
        if (i == metro && m_main_perf.is_running())    // flash on current beat
        {
            brush.setStyle(Qt::SolidPattern);
            pen.setColor(Qt::black);
        }
        else
        {
            brush.setStyle(Qt::NoBrush);
            pen.setColor(Qt::darkGray);
        }

        m_color->setAlpha(m_alpha);
        brush.setColor(*m_color);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawRect
        (
            offsetX + pen.width() - 1, pen.width() - 1,
            boxwidth - pen.width(), height() - pen.width()
        );
    }
    if (m_beats_per_measure < 10)       // draw beat number (if there's space)
    {
        pen.setColor(Qt::black);
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawText
        (
            (metro + 1) * boxwidth - (m_font.pointSize() + 2),
            height() * 0.3 + m_font.pointSize(), QString::number(metro + 1)
        );
    }

    /*
     * Lessen alpha on each redraw to have smooth fading done as a factor of
     * the BPM to get useful fades.  However, we have to scale this
     * differently than 300, because Sequencer64 allows BPM higher than 300.
     */

    m_alpha *= 0.7 - m_main_perf.bpm() / SEQ64_MAXIMUM_BPM;     /* 600 */
    if (m_alpha < 0)
        m_alpha = 0;

    m_last_metro = metro;
}

/**
 *
 */

QSize
qsmaintime::sizeHint () const
{
    return QSize(150, m_font.pointSize() * 2.4);
}

}           // namespace seq64

/*
 * qsmaintime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

