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
 * \updates       2018-11-04
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 */

#include "Globals.hpp"
#include "perform.hpp"
#include "qseqdata.hpp"
#include "rect.hpp"                     /* seq64::rect::xy_to_rect_get()    */
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

qseqdata::qseqdata
(
    perform & p,
    sequence & seq,
    int zoom,
    int snap,
    int ppqn,
    QWidget * parent
) :
    QWidget             (parent),
    qseqbase            (p, seq, zoom, ppqn, snap),
    mTimer              (nullptr),          // (new QTimer(this)),
    mNumbers            (),
    mFont               (),
    m_status            (EVENT_NOTE_ON),    // edit note velocity for now
    m_cc                (1),
    m_line_adjust       (false),
    m_relative_adjust   (false),
    m_dragging          (false)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    mTimer = new QTimer(this);                          // redraw timer !!!
    mTimer->setInterval(usr().window_redraw_rate());    // 20
    QObject::connect(mTimer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    mTimer->start();
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::needs_update().
 */

void
qseqdata::conditional_update ()
{
    if (needs_update())
        update();
}

/**
 *
 */

QSize
qseqdata::sizeHint () const
{
    return QSize
    (
        seq().get_length() / zoom() + 100 + c_keyboard_padding_x,
        c_dataarea_y
    );
}

/**
 *
 * \note
 *      We had a weird issue with the following function, where d1 would be
 *      assigned a value inside the function, but d1 was 0 afterward.  So we
 *      decided to bite the bullet and ditch this call:
 *
 *      seq().get_next_event_kepler(m_status, m_cc, tick, d0, d1, selected)
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
    painter.drawRect(0, 0, width() - 1, height() - 1);

    event_list::const_iterator cev;
    int starttick = 0;
    int endtick = width() * zoom();
    seq().reset_ex_iterator(cev);               /* reset_draw_marker()      */
    while (seq().get_next_event_match(m_status, m_cc, cev))
    {
        midipulse tick = cev->get_timestamp();
        if (tick >= starttick && tick <= endtick)
        {
            /*
             *  Convert to screen coordinates.
             */

            midibyte d0, d1;
            int event_x = tick / zoom();        /* + c_keyboard_padding_x;  */
            bool selected = cev->is_selected();
            cev->get_data(d0, d1);

            int event_height = d1;              /* generate the value       */
            if (event::is_one_byte_msg(m_status))
                event_height = d0;

            pen.setWidth(2);                    /* draw vertical grid lines */
            if (selected)
                pen.setColor("orange");
            else
                pen.setColor(Qt::black);

            painter.setPen(pen);
            painter.drawLine
            (
                event_x + 1, height() - event_height, event_x + 1, height()
            );

            char tmp[4];
            snprintf(tmp, sizeof tmp, "%3d", d1);   /* to draw digits       */
            pen.setColor(Qt::black);
            pen.setWidth(1);
            painter.setPen(pen);

            int x_offset = event_x + 3;
            int y_offset = c_dataarea_y - 25;
            QString val = tmp;
            painter.drawText(x_offset, y_offset,      val.at(0));
            painter.drawText(x_offset, y_offset +  8, val.at(1));
            painter.drawText(x_offset, y_offset + 16, val.at(2));
        }
        ++cev;
    }

    if (m_line_adjust)                            // draw edit line
    {
        int x, y, w, h;
        pen.setColor(Qt::black);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        rect::xy_to_rect_get
        (
            drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
        );
        old_rect().set(x, y, w, h);
        painter.drawLine
        (
            current_x() + c_keyboard_padding_x,
            current_y(), drop_x() + c_keyboard_padding_x, drop_y()
        );
    }
}

/**
 *
 */

void
qseqdata::mousePressEvent (QMouseEvent * event)
{
    int mouse_x = event->x() - c_keyboard_padding_x + scroll_offset_x();
    int mouse_y = event->y();

    /*
     * If near an event (4px), do relative adjustment.  Compare to
     * seqdata code marked by USE_STAZED_SEQDATA_EXTENSIONS.
     */

    midipulse tick_start, tick_finish;
    convert_x(mouse_x - 2, tick_start);
    convert_x(mouse_x + 2, tick_finish);
    seq().push_undo();
    if                      // check if these ticks would select an event
    (
        seq().select_events
        (
            tick_start, tick_finish, m_status, m_cc, sequence::e_would_select
        )
    )
    {
        m_relative_adjust = true;
    }
    else                    // set new values for seqs under a line
    {
        m_line_adjust = true;
    }

    drop_x(mouse_x);                        /* set values for line      */
    drop_y(mouse_y);
    old_rect().clear();                     /* reset dirty redraw box   */
    m_dragging = true;                      /* may be dragging now      */
}

/**
 *
 */

void
qseqdata::mouseReleaseEvent (QMouseEvent * event)
{
    current_x(int(event->x()) - c_keyboard_padding_x + scroll_offset_x());
    current_y(int(event->y()));
    if (m_line_adjust)
    {
        midipulse tick_s, tick_f;
        if (current_x() < drop_x())
        {
            swap_x();                   // swap(mCurrentX, mDropX);
            swap_y();                   // swap(mCurrentY, mDropY);
        }

        /*
         * convert x,y to ticks, then set events in range
         */

        convert_x(drop_x(), tick_s);
        convert_x(current_x(), tick_f);
        bool ok = seq().change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc,
            c_dataarea_y - drop_y() - 1, c_dataarea_y - current_y() - 1,
            true                        /* allow undo setup to happen */
        );
        m_line_adjust = false;
        if (ok)
            set_dirty();                /* TODO: do this for undo/redo  */
    }
    else if (m_relative_adjust)
        m_relative_adjust = false;

    m_dragging = false;
}

/**
 *
 */

void
qseqdata::mouseMoveEvent (QMouseEvent * event)
{
    if (! m_dragging)
        return;

    current_x(int(event->x()) - c_keyboard_padding_x);
    current_y(int(event->y()));
    midipulse tick_s, tick_f;
    if (m_line_adjust)
    {
        int adj_x_min, adj_x_max, adj_y_min, adj_y_max;
        if (current_x() < drop_x())
        {
            adj_x_min = current_x();
            adj_y_min = current_y();
            adj_x_max = drop_x();
            adj_y_max = drop_y();
        }
        else
        {
            adj_x_max = current_x();
            adj_y_max = current_y();
            adj_x_min = drop_x();
            adj_y_min = drop_y();
        }

        convert_x(adj_x_min, tick_s);
        convert_x(adj_x_max, tick_f);
        bool ok = seq().change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc,
            c_dataarea_y - adj_y_min - 1, c_dataarea_y - adj_y_max - 1
        );
        if (ok)
            set_dirty();
    }
    else if (m_relative_adjust)
    {
        convert_x(drop_x() - 2, tick_s);
        convert_x(drop_x() + 2, tick_f);

        int adjy = drop_y() - current_y();
        bool ok = seq().change_event_data_relative
        (
            tick_s, tick_f, m_status, m_cc, adjy
        );
        if (ok)
            set_dirty();

        /*
         * Move the drop location so we increment properly on next mouse move.
         */

        drop_y(current_y());
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
    set_dirty();
}

/**
 *
 */

void
qseqdata::convert_x (int x, midipulse & tick)
{
    tick = x * zoom();
}

}           // namespace seq64

/*
 * qseqdata.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

