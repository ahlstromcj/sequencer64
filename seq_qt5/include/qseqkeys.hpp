#ifndef SEQ64_QSEQKEYS_HPP
#define SEQ64_QSEQKEYS_HPP

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
 * \file          qseqkeys.hpp
 *
 *  This module declares/defines the base class for the left-side piano of
 *  the pattern/sequence panel.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-04-02
 * \license       GNU GPLv2 or above
 *
 *      We've added the feature of a right-click toggling between showing the
 *      main octave values (e.g. "C1" or "C#1") versus the numerical MIDI
 *      values of the keys.
 */

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QPen>
#include <QSizePolicy>
#include <QMouseEvent>

#include "Globals.hpp"
#include "sequence.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

    class sequence;

/**
 * draws the piano keys in the sequence editor
 */

class qseqkeys : public QWidget
{
    Q_OBJECT

public:

    explicit qseqkeys
    (
        sequence & seq,
        QWidget * parent = 0,
        int keyHeight = 12,
        int keyAreaHeight = 12 * qc_num_keys + 1
    );

protected:

    // override painting event to draw on the frame

    void paintEvent (QPaintEvent *);

    // override mouse events for interaction

    void mousePressEvent (QMouseEvent * event);
    void mouseReleaseEvent (QMouseEvent * event);
    void mouseMoveEvent (QMouseEvent * event);

    // override the sizehint to set our own defaults

    QSize sizeHint() const;

signals:

public slots:

private:

    void convert_y (int y, int & note);

private:

    sequence & m_seq;
    QTimer * m_timer;
    QFont m_font;
    int m_key;
    int keyY;
    int keyAreaY;
    bool mPreviewing;
    int  mPreviewKey;

};          // class qseqkeys

}           // namespace seq64

#endif      // SEQ64_QSEQKEYS_HPP

/*
 * qseqkeys.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

