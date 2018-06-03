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
 * \file          qseqdata.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-05-27
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 */

#include "Globals.hpp"
#include "qseqdata.hpp"
#include "rect.hpp"                     /* seq64::rect::xy_to_rect_get()    */
#include "sequence.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *
 */

qseqdata::qseqdata (sequence & seq, QWidget * parent)
 :
    QWidget         (parent),
    m_seq           (seq),
    mOld            (new QRect()),
    mTimer          (nullptr),          // (new QTimer(this)),
    mNumbers        (),
    mFont           (),
    m_zoom          (1),
    mDropX          (0),
    mDropY          (0),
    mCurrentX       (0),
    mCurrentY       (0),
    m_status        (EVENT_NOTE_ON),   // edit note velocity for now
    m_cc            (1),
    mLineAdjust     (false),
    mRelativeAdjust (false)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mTimer = new QTimer(this);  // start refresh timer to queue regular redraws
    mTimer->setInterval(20);
    QObject::connect(mTimer, SIGNAL(timeout()), this, SLOT(update()));
    mTimer->start();
}

/**
 *
 */

void
qseqdata::zoom_in ()
{
    if (m_zoom > 1)
        m_zoom *= 0.5;
}

/**
 *
 */

void
qseqdata::zoom_out ()
{
    if (m_zoom < 32)
        m_zoom *= 2;
}

/**
 *
 */

QSize
qseqdata::sizeHint () const
{
    return QSize
    (
        m_seq.get_length() / m_zoom + 100 + c_keyboard_padding_x, qc_dataarea_y
    );
}

/**
 *
 * \note
 *      We had a weird issue with the following function, where d1 would be
 *      assigned a value inside the function, but d1 was 0 afterward.  So we
 *      decided to bite the bullet and ditch this call:
 *
 *      m_seq.get_next_event_kepler(m_status, m_cc, tick, d0, d1, selected)
 *
 *      Instead, we create an iterator and use sequence::get_next_event_ex().
 */

void
qseqdata::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush(Qt::lightGray, Qt::SolidPattern);
    mFont.setPointSize(6);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(mFont);

    event_list::const_iterator cev;
    int start_tick = 0;
    int end_tick = width() * m_zoom;
    painter.drawRect(0, 0, width() - 1, height() - 1);
    m_seq.reset_ex_iterator(cev);                   /* reset_draw_marker()  */
    while (m_seq.get_next_event_ex(m_status, m_cc, cev))
    {
        midipulse tick = cev->get_timestamp();
        if (tick >= start_tick && tick <= end_tick)
        {
            /*
             *  Convert to screen coordinates.
             */

            midibyte d0, d1;
            int event_x = tick / m_zoom + c_keyboard_padding_x;
            cev->get_data(d0, d1);

            int event_height = d1;              /* generate the value       */
            if (event::is_one_byte_msg(m_status))
                event_height = d0;

            pen.setWidth(2);                    /* draw vertical grid lines */
            painter.setPen(pen);
            painter.drawLine
            (
                event_x + 1, height() - event_height, event_x + 1, height()
            );

            QString val = QString::number(d1);  /* draw numbers             */
            pen.setColor(Qt::black);
            pen.setWidth(1);
            painter.setPen(pen);
            if (val.length() >= 1)
                painter.drawText(event_x + 3, qc_dataarea_y - 25, val.at(0));

            if (val.length() >= 2)
                painter.drawText(event_x + 3, qc_dataarea_y - 25 + 8, val.at(1));

            if (val.length() >= 3)
                painter.drawText(event_x + 3, qc_dataarea_y - 25 + 16, val.at(2));
        }
        ++cev;
    }

    if (mLineAdjust)                            // draw edit line
    {
        int x, y, w, h;
        pen.setColor(Qt::black);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        rect::xy_to_rect_get(mDropX, mDropY, mCurrentX, mCurrentY, x, y, w, h);
        mOld->setX(x);
        mOld->setY(y);
        mOld->setWidth(w);
        mOld->setHeight(h);
        painter.drawLine
        (
            mCurrentX + c_keyboard_padding_x,
            mCurrentY, mDropX + c_keyboard_padding_x, mDropY
        );
    }
}

/**
 *
 */

void
qseqdata::mousePressEvent (QMouseEvent *event)
{
    int mouseX = event->x() - c_keyboard_padding_x;
    int mouseY = event->y();

    // If near an event (4px), do relative adjustment

    midipulse tick_start, tick_finish;
    convert_x(mouseX - 2, tick_start);
    convert_x(mouseX + 2, tick_finish);

    m_seq.push_undo();
    if                      // check if these ticks would select an event
    (
        m_seq.select_events
        (
            tick_start, tick_finish, m_status, m_cc, sequence::e_would_select
        )
    )
    {
        mRelativeAdjust = true;
    }
    else                    // set new values for seqs under a line
    {
        mLineAdjust = true;
    }

    mDropX = mouseX;            /* set values for line */
    mDropY = mouseY;
    mOld->setX(0);              /* reset box that holds dirty redraw spot */
    mOld->setY(0);
    mOld->setWidth(0);
    mOld->setHeight(0);
}

/**
 *
 */

void
qseqdata::mouseReleaseEvent (QMouseEvent *event)
{
    mCurrentX = (int) event->x() - c_keyboard_padding_x;
    mCurrentY = (int) event->y();
    if (mLineAdjust)
    {
        midipulse tick_s, tick_f;
        if (mCurrentX < mDropX)
        {
            swap(mCurrentX, mDropX);
            swap(mCurrentY, mDropY);
        }

        /*
         * convert x,y to ticks, then set events in range
         */

        convert_x(mDropX, tick_s);
        convert_x(mCurrentX, tick_f);
        m_seq.change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc,
            qc_dataarea_y - mDropY - 1, qc_dataarea_y - mCurrentY - 1
        );

        mLineAdjust = false;
    }
    else if (mRelativeAdjust)
        mRelativeAdjust = false;
}

/**
 *
 */

void
qseqdata::mouseMoveEvent (QMouseEvent * event)
{
    mCurrentX = event->x() - c_keyboard_padding_x;
    mCurrentY = event->y();
    midipulse tick_s, tick_f;
    if (mLineAdjust)
    {
        int adj_x_min, adj_x_max, adj_y_min, adj_y_max;
        if (mCurrentX < mDropX)
        {
            adj_x_min = mCurrentX;
            adj_y_min = mCurrentY;
            adj_x_max = mDropX;
            adj_y_max = mDropY;
        }
        else
        {
            adj_x_max = mCurrentX;
            adj_y_max = mCurrentY;
            adj_x_min = mDropX;
            adj_y_min = mDropY;
        }

        convert_x(adj_x_min, tick_s);
        convert_x(adj_x_max, tick_f);
        m_seq.change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc,
            qc_dataarea_y - adj_y_min - 1, qc_dataarea_y - adj_y_max - 1
        );
    }
    else if (mRelativeAdjust)
    {
        convert_x(mDropX - 2, tick_s);
        convert_x(mDropX + 2, tick_f);

        ///// TODO
        ///// int adjY = mDropY - mCurrentY;
        ///// m_seq.change_event_data_relative(tick_s, tick_f, m_status, m_cc, adjY);

        // move the drop location so we increment properly on next mouse move

        mDropY = mCurrentY;
    }
}

/**
 *
 */

void
qseqdata::set_data_type (midibyte status, midibyte control = 0)
{
    m_status = status;
    m_cc = control;
}

/**
 *
 */

void
qseqdata::convert_x (int x, midipulse & tick)
{
    tick = x * m_zoom;
}

}           // namespace seq64

/*
 * qseqdata.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

