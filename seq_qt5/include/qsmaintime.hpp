#ifndef SEQ64_QSMAINTIME_HPP
#define SEQ64_QSMAINTIME_HPP

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
 * \updates       2018-03-05
 * \license       GNU GPLv2 or above
 *
 */

#include <QWidget>
#include <QPainter>
#include <QTimer>

#include "perform.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 * A beat indicator widget
 */

class qsmaintime : public QWidget
{
    Q_OBJECT

public:

    qsmaintime
    (
        perform & perf,
        QWidget * parent,
        int beats_per_measure   = 4,
        int beat_width          = 4
    );

    ~qsmaintime();

    /**
     * \getter m_beats_per_measure
     */

    int get_beats_per_measure () const
    {
        return m_beats_per_measure;
    }

    /**
     * \setter m_beats_per_measure
     */

    void set_beats_per_measure (int bpm)
    {
        m_beats_per_measure = bpm;
    }

    /**
     *  \getter m_beat_width
     */

    int get_beat_width () const
    {
        return m_beat_width;
    }

    /**
     *  \setter m_beat_width
     */

    void set_beat_width (int bw)
    {
        m_beat_width = bw;
    }

    /*
     * Never defined, never called.
     *
     * void startRedrawTimer();
     * void setPlaying(bool mPlaying);
     * bool getPlaying() const;
     */

protected:

    void paintEvent (QPaintEvent * event);  // override to draw on the frame
    QSize sizeHint() const;                 // override to set our own defaults

private:

    const perform & m_main_perf;
    QColor * m_color;
    QFont mFont;
    int m_beats_per_measure;
    int m_beat_width;
    int m_lastMetro;
    int m_alpha;

};          // class qsmaintime

}           // namespace seq64

#endif      // SEQ64_QSMAINTIME_HPP

/*
 * qsmaintime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

