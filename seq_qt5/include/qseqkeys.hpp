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
 * \updates       2018-10-26
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

#include "globals.h"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

    class perform;
    class sequence;

/**
 *  Draws the piano keys in the sequence editor.
 */

class qseqkeys : public QWidget
{
    Q_OBJECT

    friend class qseqroll;

public:

    qseqkeys
    (
        perform & perf,
        sequence & seq,
        QWidget * parent,
        int keyHeight       = 12,
        int keyAreaHeight   = 12 * c_num_keys + 1
    );

    virtual ~qseqkeys ()
    {
        // no code needed
    }

protected:      // Qt overrides

    virtual void paintEvent (QPaintEvent *);
    virtual void mousePressEvent (QMouseEvent * event);
    virtual void mouseReleaseEvent (QMouseEvent * event);
    virtual void mouseMoveEvent (QMouseEvent * event);
    virtual QSize sizeHint() const;                 // sizehint to set defaults
    virtual void wheelEvent (QWheelEvent * ev);

signals:

public slots:

    // void conditional_update ();

private:

    void convert_y (int y, int & note);

    /**
     *  Detects a black key.
     *
     * \param key
     *      The key to analyze.
     *
     * \return
     *      Returns true if the key is black (value 1, 3, 6, 8, or 10).
     */

    bool is_black_key (int key) const
    {
        return key == 1 || key == 3 || key == 6 || key == 8 || key == 10;
    }

    /**
     * \getter m_seq
     */

    sequence & seq ()
    {
        return m_seq;
    }

private:

    perform & m_perform;
    sequence & m_seq;
    QFont m_font;

    /**
     *  The default value is to show the octave letters on the vertical
     *  virtual keyboard.  If false, then the MIDI key numbers are shown
     *  instead.  This is a new feature of Sequencer64.
     */

    bool m_show_octave_letters;

    bool m_is_previewing;
    int m_key;
    int m_key_y;
    int m_key_area_y;
    int m_preview_key;

};          // class qseqkeys

}           // namespace seq64

#endif      // SEQ64_QSEQKEYS_HPP

/*
 * qseqkeys.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

