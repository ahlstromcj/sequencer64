#ifndef SEQ64_QPERFNAMES_HPP
#define SEQ64_QPERFNAMES_HPP

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
 * \file          qperfnames.hpp
 *
 *  This module declares/defines the base class for performance names.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-03-02
 * \license       GNU GPLv2 or above
 *
 */

#include <QWidget>
#include <QPainter>
#include <QPen>

#include "globals.h"
#include "sequence.hpp"
#include "gui_palette_qt5.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 * Sequence labels for the side of the song editor
 */

class qperfnames : public QWidget, gui_palette_qt5
{
    Q_OBJECT

public:

    explicit qperfnames (perform & p, QWidget * parent);
    ~qperfnames();

protected:

    int name_x (int i)
    {
        return m_nametext_x + i;
    }

    int name_y (int i)
    {
        return m_nametext_y * i;
    }

protected:

    // override painting event to draw on the frame

    void paintEvent (QPaintEvent *);

    // override mouse events for interaction

    void mousePressEvent (QMouseEvent * event);
    void mouseReleaseEvent (QMouseEvent * event);
    void mouseMoveEvent (QMouseEvent * event);

    // override the sizehint to set our own defaults

    QSize sizeHint() const;

private:

    perform & perf ()
    {
        return mPerf;
    }

signals:

public slots:

private:

    perform & mPerf;

    QTimer * mTimer;
    QPen * mPen;
    QBrush * mBrush;
    QPainter * mPainter;
    QFont mFont;

    bool m_sequence_active[c_max_sequence];
    int m_nametext_x;
    int m_nametext_y;

};

}           // namespace seq64

#endif      // SEQ64_QPERFNAMES_HPP

/*
 * qperfnames.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

