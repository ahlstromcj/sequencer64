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
 * \file          qstriggereditor.cpp
 *
 *  This module declares/defines the base class for the Performance window
 *  piano roll.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-05-27
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.
 */

#include "Globals.hpp"
#include "qseqdata.hpp"
#include "qstriggereditor.hpp"
#include "sequence.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

static const int qc_eventarea_y     = 16;
static const int qc_eventevent_y    = 10;
static const int qc_eventevent_x    =  5;

/**
 *
 */

qstriggereditor::qstriggereditor
(
    sequence & seq,
    qseqdata & seqdata_wid,
    QWidget * parent,
    int keyHeight
) :
    QWidget             (parent),
    m_seq               (seq),
    m_seqdata_wid       (seqdata_wid),
    m_old               (new QRect()),
    m_selected          (new QRect()),
    mTimer              (new QTimer(this)), // refresh timer for regular redraws
    mFont               (),
    m_zoom              (1),
    m_snap              (1),
    m_window_x          (0),
    m_window_y          (0),
    keyY                (keyHeight),
    m_selecting         (false),
    m_moving_init       (false),
    m_moving            (false),
    m_growing           (false),
    m_painting          (false),
    m_paste             (false),
    m_adding            (false),
    m_drop_x            (0),
    m_drop_y            (0),
    m_current_x         (0),
    m_current_y         (0),
    m_move_snap_offset_x(0),
    m_status            (EVENT_NOTE_ON),
    m_cc                (0)
{
    m_snap = m_seq.get_snap_tick();
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QObject::connect(mTimer, SIGNAL(timeout()), this, SLOT(update()));
    mTimer->setInterval(20);
    mTimer->start();
}

/**
 *
 */

void
qstriggereditor::zoom_in()
{
    if (m_zoom > 1)
        m_zoom *= 0.5;
}

/**
 *
 */

void
qstriggereditor::zoom_out()
{
    if (m_zoom < 32)
        m_zoom *= 2;
}

/**
 *
 */

QSize
qstriggereditor::sizeHint () const
{
    return QSize
    (
        m_seq.get_length() / m_zoom + 100 + c_keyboard_padding_x,
        qc_eventarea_y + 1
    );
}

/**
 *
 */

void
qstriggereditor::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush(Qt::darkGray, Qt::SolidPattern);

    mFont.setPointSize(6);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(mFont);

    /* draw background */

    painter.drawRect(c_keyboard_padding_x, 0, width(), height());

    int measures_per_line = 1;
    int ticks_per_measure =  m_seq.get_beats_per_bar() * (4 * c_ppqn) /
        m_seq.get_beat_width();

    midipulse ticks_per_beat = (4 * c_ppqn) / m_seq.get_beat_width();
    midipulse ticks_per_step = 6 * m_zoom;
    midipulse ticks_per_m_line =  ticks_per_measure * measures_per_line;
    midipulse start_tick = 0;
    midipulse end_tick = width() * m_zoom;
    for (midipulse i = start_tick; i < width(); i += ticks_per_step)
    {
        int base_line = i + c_keyboard_padding_x;
        if (i % ticks_per_m_line == 0)
        {
            /* solid line on every beat */
            pen.setColor(Qt::black);
            pen.setStyle(Qt::SolidLine);
        }
        else if (i % ticks_per_beat == 0)
        {
            pen.setStyle(Qt::SolidLine);
            pen.setColor(Qt::black);
        }
        else
        {
            pen.setColor(Qt::lightGray);
            pen.setStyle(Qt::DashLine);
        }

        painter.setPen(pen);
        painter.drawLine(base_line, 1, base_line, qc_eventarea_y);
    }

    /* draw boxes from sequence */

    pen.setColor(Qt::black);
    pen.setStyle(Qt::SolidLine);

    event_list::const_iterator cev;
    m_seq.reset_ex_iterator(cev);                   /* reset_draw_marker()  */
    while (m_seq.get_next_event_ex(m_status, m_cc, cev))
    {
        midipulse tick = cev->get_timestamp();
        if ((tick >= start_tick && tick <= end_tick))
        {
            bool selected = cev->is_selected();
            int x = tick / m_zoom + c_keyboard_padding_x;
            int y = (qc_eventarea_y - qc_eventevent_y) / 2;
            pen.setColor(Qt::black);                /* outer note border    */
            brush.setStyle(Qt::SolidPattern);
            brush.setColor(Qt::black);
            painter.setBrush(brush);
            painter.setPen(pen);
            painter.drawRect(x, y, qc_eventevent_x, qc_eventevent_y);
            brush.setColor(selected ? Qt::red : Qt::white);
            painter.setBrush(brush);                /* draw note highlight  */
            painter.drawRect(x, y, qc_eventevent_x - 1, qc_eventevent_y - 1);
        }
        ++cev;
    }

    int y = (qc_eventarea_y - qc_eventevent_y) / 2; /* draw selection       */
    int h =  qc_eventevent_y;
    brush.setStyle(Qt::NoBrush);                    /* painter reset        */
    painter.setBrush(brush);
    if (m_selecting)
    {
        int x, w;
        x_to_w(m_drop_x, m_current_x, x, w);
        m_old->setX(x);
        m_old->setWidth(w);
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, w, h);
    }
    if (m_moving || m_paste)
    {
        int delta_x = m_current_x - m_drop_x;
        int x = m_selected->x() + delta_x;
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, m_selected->width(), h);
        m_old->setX(x);
        m_old->setWidth(m_selected->width());
    }
}

/**
 *
 */

void
qstriggereditor::mousePressEvent (QMouseEvent *event)
{
    int x, w, numsel;
    midipulse tick_s;
    midipulse tick_f;
    midipulse tick_w;
    convert_x(qc_eventevent_x, tick_w);

    /* if it was a button press */

    /* set values for dragging */
    m_drop_x = m_current_x = (int) event->x() - c_keyboard_padding_x;

    /* reset box that holds dirty redraw spot */
    m_old->setX(0);
    m_old->setY(0);
    m_old->setWidth(0);
    m_old->setHeight(0);

    if (m_paste)
    {
        snap_x(m_current_x);
        convert_x(m_current_x, tick_s);
        m_paste = false;
        m_seq.push_undo();
        m_seq.paste_selected(tick_s, 0);
    }
    else
    {
        if (event->button() == Qt::LeftButton)
        {
            convert_x(m_drop_x, tick_s);   /* turn x,y in to tick/note */
            tick_f = tick_s + (m_zoom);     /* shift back a few ticks */
            tick_s -= (tick_w);
            if (tick_s < 0)
                tick_s = 0;

            if (m_adding)
            {
                m_painting = true;
                snap_x(m_drop_x);
                convert_x(m_drop_x, tick_s); /* turn x,y in to tick/note */

                /* add note, length = little less than snap */

                if
                (
                    ! m_seq.select_events
                    (
                        tick_s, tick_f, m_status, m_cc, sequence::e_would_select
                    )
                )
                {
                    m_seq.push_undo();
                    drop_event(tick_s);
                }
            }
            else /* selecting */
            {
                if
                (
                    ! m_seq.select_events
                    (
                        tick_s, tick_f, m_status, m_cc, sequence::e_is_selected
                    )
                )
                {
                    if (!(event->modifiers() & Qt::ControlModifier))
                        m_seq.unselect();

                    numsel = m_seq.select_events(tick_s, tick_f,
                          m_status, m_cc, sequence::e_select_one);

                    /* if we didnt select anyhing (user clicked empty space)
                       unselect all notes, and start selecting */

                    /* none selected, start selection box */
                    if (numsel == 0)
                    {
                        m_selecting = true;
                    }
                    else
                    {
                        /// needs update
                    }
                }

                if
                (
                    m_seq.select_events
                    (
                        tick_s, tick_f, m_status, m_cc, sequence::e_is_selected
                    )
                )
                {
                    int note;
                    m_moving_init = true;

                    /* get the box that selected elements are in */
                    m_seq.get_selected_box(tick_s, note, tick_f, note);
                    tick_f += tick_w;

                    convert_t(tick_s, x); /* convert box to X,Y values */
                    convert_t(tick_f, w);

                    /* w is actually coorids now, so we have to change */

                    w = w - x;

                    /* set the m_selected rectangle to hold the
                       x,y,w,h of our selected events */

                    m_selected->setX(x);
                    m_selected->setWidth(w);
                    m_selected->setY((qc_eventarea_y - qc_eventevent_y) / 2);
                    m_selected->setHeight(qc_eventevent_y);

                    /* save offset that we get from the snap above */

                    int adjusted_selected_x = m_selected->x();
                    snap_x(adjusted_selected_x);
                    m_move_snap_offset_x =
                        m_selected->x() - adjusted_selected_x;

                    /* align selection for drawing */
                    //save X as a variable so we can use the snap function
                    int tempSelectedX = m_selected->x();
                    snap_x(tempSelectedX);
                    m_selected->setX(tempSelectedX);
                    snap_x(m_current_x);
                    snap_x(m_drop_x);
                }
            }

        } /* end if button == 1 */

        if (event->button() == Qt::RightButton)
        {
            set_adding(true);
        }
    }
}

/**
 *
 */

void
qstriggereditor::mouseReleaseEvent (QMouseEvent * event)
{
    midipulse tick_s;
    midipulse tick_f;
    int x, w;
    m_current_x = (int) event->x() - c_keyboard_padding_x;
    if (m_moving)
        snap_x(m_current_x);

    int delta_x = m_current_x - m_drop_x;
    midipulse delta_tick;
    if (event->button() == Qt::LeftButton)
    {
        if (m_selecting)
        {
            x_to_w(m_drop_x, m_current_x, x, w);
            convert_x(x, tick_s);
            convert_x(x + w, tick_f);
            m_seq.select_events(tick_s, tick_f, m_status, m_cc, sequence::e_select);
        }

        if (m_moving)
        {
            delta_x -= m_move_snap_offset_x; /* adjust for snap */

            /* convert deltas into screen corridinates */

            convert_x(delta_x, delta_tick);

            /* not really notes, but still moves events */

            m_seq.push_undo();
            m_seq.move_selected_notes(delta_tick, 0);
        }
        set_adding(m_adding);
    }

    if (event->button() == Qt::RightButton)
        set_adding(false);

    /* turn off */
    m_selecting = false;
    m_moving = false;
    m_growing = false;
    m_moving_init = false;
    m_painting = false;
    m_seq.unpaint_all();
}

/**
 *
 */

void
qstriggereditor::mouseMoveEvent (QMouseEvent * event)
{
    midipulse tick = 0;
    if (m_moving_init)
    {
        m_moving_init = false;
        m_moving = true;
    }
    if (m_selecting || m_moving || m_paste)
    {
        m_current_x = (int) event->x() - c_keyboard_padding_x;
        if (m_moving || m_paste)
            snap_x(m_current_x);
    }
    if (m_painting)
    {
        m_current_x = (int) event->x();
        snap_x(m_current_x);
        convert_x(m_current_x, tick);
        drop_event(tick);
    }
}

/**
 *
 */

void
qstriggereditor::keyPressEvent (QKeyEvent * event)
{
    bool ret = false;
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        m_seq.push_undo();
        m_seq.mark_selected();
        m_seq.remove_marked();
        ret = true;
    }

    if (event->modifiers() & Qt::ControlModifier)
    {
        switch (event->key())
        {
        case Qt::Key_X: /* cut */
            m_seq.copy_selected();
            m_seq.mark_selected();
            m_seq.remove_marked();
            ret = true;
            break;

        case Qt::Key_C: /* copy */
            m_seq.copy_selected();
            ret = true;
            break;

        case Qt::Key_V: /* paste */
            start_paste();
            ret = true;
            break;

        case Qt::Key_Z: /* Undo */
            if (event->modifiers() & Qt::ShiftModifier)
                m_seq.pop_redo();
            else
                m_seq.pop_undo();
            ret = true;
            break;
        }
    }
    if (ret == true)
        m_seq.set_dirty();
}

/**
 *
 */

void
qstriggereditor::keyReleaseEvent (QKeyEvent *)
{
    // no code
}

/**
 *
 */

void
qstriggereditor::x_to_w (int  x1, int  x2, int & x, int & w)
{
    if ( x1 <  x2)
    {
        x = x1;
        w = x2 - x1;
    }
    else
    {
        x = x2;
        w = x1 - x2;
    }
}

/**
 *
 */

void
qstriggereditor::start_paste ()
{
    midipulse tick_s;
    midipulse tick_f;
    int note_h;
    int note_l;
    int x, w;

    snap_x(m_current_x);
    snap_y(m_current_x);
    m_drop_x = m_current_x;
    m_drop_y = m_current_y;
    m_paste = true;

    /* get the box that selected elements are in */

    m_seq.get_clipboard_box(tick_s, note_h, tick_f, note_l);

    convert_t(tick_s, x);           /* convert box to X,Y values */
    convert_t(tick_f, w);

    /* w is actually corrids now, so we have to change */

    w = w - x;

    /* set the m_selected rectangle to hold the x,y,w,h of our selected events */

    m_selected->setX(x);
    m_selected->setWidth(w);
    m_selected->setY((qc_eventarea_y - qc_eventevent_y) / 2);
    m_selected->setHeight(qc_eventevent_y);

    /* adjust for clipboard being shifted to tick 0 */

    m_selected->setX(m_selected->x() + m_drop_x);
}

/**
 *
 */

void
qstriggereditor::convert_x (int x, midipulse & tick)
{
    tick = x * m_zoom;
}

/**
 *
 */

void
qstriggereditor::convert_t (midipulse ticks, int & x)
{
    x = ticks / m_zoom;
}

/**
 *
 */

void
qstriggereditor::drop_event (midipulse a_tick)
{
    midibyte status = m_status;
    midibyte d0 = m_cc;
    midibyte d1 = 0x40;
    if (m_status == EVENT_AFTERTOUCH)
        d0 = 0;

    if (m_status == EVENT_PROGRAM_CHANGE)
        d0 = 0; /* d0 == new patch */

    if (m_status == EVENT_CHANNEL_PRESSURE)
        d0 = 0x40; /* d0 == pressure */

    if (m_status == EVENT_PITCH_WHEEL)
        d0 = 0;

    m_seq.add_event(a_tick, status, d0, d1, true);
}

/**
 * performs a 'snap' on y*
 */

void
qstriggereditor::snap_y (int & y)
{
    y -= y % keyY;
}

/**
 * performs a 'snap' on x
    //snap = number pulses to snap to
    //m_zoom = number of pulses per pixel
    //so snap / m_zoom  = number pixels to snap to
 */

void
qstriggereditor::snap_x (int & x)
{
    int mod = (m_snap / m_zoom);
    if (mod <= 0)
        mod = 1;

    x -= (x % mod);
}

/**
 *
 */

void
qstriggereditor::set_adding (bool a_adding)
{
    if (a_adding)
    {
        setCursor(Qt::PointingHandCursor);
        m_adding = true;
    }
    else
    {
        setCursor(Qt::ArrowCursor);
        m_adding = false;
    }
}

/**
 *
 */

void
qstriggereditor::set_data_type (midibyte status, midibyte control)
{
    m_status = status;
    m_cc = control;
}

}           // namespace seq64

/*
 * qstriggereditor.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

