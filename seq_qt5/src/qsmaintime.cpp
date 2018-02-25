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
 * \updates       2018-02-23
 * \license       GNU GPLv2 or above
 *
 */

#include "qsmaintime.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

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
    mPen                (nullptr),
    mBrush              (nullptr),
    m_Color             (new QColor(Qt::red)),
    mFont               (),
    m_beats_per_measure (beats_per_measure),
    m_beat_width        (beat_width),
    m_lastMetro         (0),
    m_alpha             (0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    mFont.setPointSize(9);
    mFont.setBold(true);
}

/**
 *
 */

qsmaintime::~qsmaintime ()
{
    // what about the pens, brushes, etc???
}

/**
 *
 */

void
qsmaintime::paintEvent (QPaintEvent *)
{
    mPainter    = new QPainter(this);
    mBrush      = new QBrush(Qt::NoBrush);
    mPen        = new QPen(Qt::darkGray);

    mPainter->setPen(*mPen);
    mPainter->setFont(mFont);
    mPainter->setBrush(*mBrush);

    midipulse tick = m_main_perf->get_tick();
    int metro = (tick / (c_ppqn / 4 * m_beat_width)) % m_beats_per_measure;
    int divX = (width() - 1) / m_beats_per_measure;

    /*
     * Flash on beats (i.e. if the metronome has changed, or we've just started
     * playing).
     *
     * \todo
     *      We need to select a color from the palette.
     */

    if (metro != m_lastMetro || (tick < 50 && tick > 0))
    {
        m_alpha = 230;
        if (metro == 0)
            mColour->setRgb(255, 50, 50);       // red on first beat in bar
        else
            mColour->setRgb(255, 255, 255);     // white on others
    }

    for (int i = 0; i < m_beats_per_measure; ++i)       // draw beat blocks
    {
        int offsetX = divX * i;
        if (i == metro && m_main_perf->is_running())    // flash on current beat
        {
            mBrush->setStyle(Qt::SolidPattern);
            mPen->setColor(Qt::black);
        }
        else
        {
            mBrush->setStyle(Qt::NoBrush);
            mPen->setColor(Qt::darkGray);
        }

        mColour->setAlpha(m_alpha);
        mBrush->setColor(*mColour);
        mPainter->setPen(*mPen);
        mPainter->setBrush(*mBrush);
        mPainter->drawRect
        (
            offsetX + mPen->width() - 1, mPen->width() - 1,
            divX - mPen->width(), height() - mPen->width()
        );
    }
    if (m_beats_per_measure < 10)       // draw beat number (if there's space)
    {
        mPen->setColor(Qt::black);
        mPen->setStyle(Qt::SolidLine);
        mPainter->setPen(*mPen);
        mPainter->drawText
        (
            (metro + 1) * divX - (mFont.pointSize() + 2),
            height() * 0.3 + mFont.pointSize(), QString::number(metro + 1)
        );
    }

    /*
     * lessen alpha on each redraw to have smooth fading done as a factor of
     * the bpm to get useful fades
     */

    m_alpha *= 0.7 - m_main_perf->get_bpm() / 300;
    m_lastMetro = metro;
    delete mPainter;
    delete mBrush;
    delete mPen;
}

/**
 *
 */

QSize
qsmaintime::sizeHint () const
{
    return QSize(150, mFont.pointSize() * 2.4);
}

}           // namespace seq64

/*
 * qsmaintime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

