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
 * \file          qseqkeys.cpp
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

#include "qseqkeys.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

qseqkeys::qseqkeys
(
    sequence & seq, QWidget * parent, int keyheight, int keyareaheight
) :
    QWidget             (parent),
    m_seq               (seq),
    m_timer             (nullptr),
    m_font              (),
    m_key               (0),
    keyY                (keyheight),
    keyAreaY            (keyareaheight),
    mPreviewing         (false),
    mPreviewKey         (-1)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    setMouseTracking(true);
}

/**
 *
 */

void
qseqkeys::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush (Qt::SolidPattern);
    pen.setStyle(Qt::SolidLine);
    brush.setColor(Qt::lightGray);
    m_font.setPointSize(6);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);

    // draw keyboard border

    painter.drawRect(0, 0, qc_keyarea_x, keyAreaY);
    for (int i = 0; i < qc_num_keys; i++)
    {
        pen.setColor(Qt::black);                // draw keys
        pen.setStyle(Qt::SolidLine);
        brush.setColor(Qt::white);
        brush.setStyle(Qt::SolidPattern);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawRect(qc_keyoffset_x+1, keyY*i + 1, qc_key_x-2, keyY-1);

        int key = (qc_num_keys - i - 1) % 12; /* the the key in the octave */
        if (key == 1 || key == 3 || key == 6 || key == 8 || key == 10)
        {
            if (qc_num_keys - (i + 1) == mPreviewKey)
            {
                // pen.setStyle(Qt::NoPen);
                // brush.setColor(Qt::red);
                // painter.setPen(*m_pen);
                // painter.setBrush(*m_brush);
                // painter.drawRect(qc_keyoffset_x + 3,
                //     keyY * i + 5, qc_key_x - 6, keyY - 8);
            }
            pen.setStyle(Qt::SolidLine); // draw black keys
            pen.setColor(Qt::black);
            brush.setColor(Qt::black);
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.drawRect(qc_keyoffset_x+1, keyY*i + 3, qc_key_x-4, keyY-5);
        }

        if (qc_num_keys - (i + 1) == mPreviewKey) // highlight note preview
        {
            brush.setColor(Qt::red);
            pen.setStyle(Qt::NoPen);
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.drawRect(qc_keyoffset_x+3, keyY*i + 3, qc_key_x-5, keyY-4);
        }

        char notes[20];
        if (key == m_key)
        {
            int octave = ((qc_num_keys - i - 1) / 12) - 1;      /* notes */
            if (octave < 0)
                octave *= -1;

            /*
             * TODO:  use the scales header file to get this
             */

            snprintf(notes, sizeof(notes), "%2s%1d", c_key_text[key], octave);

            pen.setColor(Qt::black);            // draw "Cx" octave labels
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
            painter.drawText(2, keyY * i + 11, notes);
        }
    }
}

/**
 *
 */

void
qseqkeys::mousePressEvent (QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton)
    {
        int note;
        int y = event->y();
        mPreviewing = true;
        convert_y(y, note);
        mPreviewKey = note;
        m_seq.play_note_on(note);
    }
    update();
}

/**
 *
 */

void
qseqkeys::mouseReleaseEvent (QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton && mPreviewing)
    {
        m_seq.play_note_off(mPreviewKey);
        mPreviewing = false;
        mPreviewKey = -1;
    }
    update();
}

/**
 *
 */

void
qseqkeys::mouseMoveEvent (QMouseEvent * event)
{
    int note;
    int y = event->y();
    convert_y(y, note);
    if (mPreviewing)
    {
        if (note != mPreviewKey)
        {
            m_seq.play_note_off(mPreviewKey);
            m_seq.play_note_on(note);
            mPreviewKey = note;
        }
    }
    update();
}

/**
 *
 */

QSize
qseqkeys::sizeHint () const
{
    return QSize(qc_keyarea_x, keyAreaY);
}

/**
 *
 */

void
qseqkeys::convert_y (int y, int & note)
{
    note = (keyAreaY - y - 2) / keyY;
}

}           // namespace seq64

/*
 * qseqkeys.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

