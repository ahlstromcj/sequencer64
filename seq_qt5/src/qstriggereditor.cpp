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
 * \updates       2018-10-29
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.
 */

#include "Globals.hpp"
#include "perform.hpp"
#include "qseqdata.hpp"
#include "qstriggereditor.hpp"
#include "sequence.hpp"
#include "settings.hpp"                 /* seq64::usr().key_height(), etc.  */

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
    perform & p,
    sequence & seq,
    qseqdata * seqdata_wid,
    int zoom,
    int snap,
    int ppqn,
    int keyheight,
    QWidget * parent
) :
    QWidget             (parent),
    qseqbase
    (
        p, seq, zoom, snap, ppqn, usr().key_height(),
        (usr().key_height() * c_num_keys + 1)
    ),
    m_seqdata_wid       (seqdata_wid),
    m_timer             (nullptr),
    m_font              (),
    m_key_y             (keyheight),
    m_status            (EVENT_NOTE_ON),
    m_cc                (0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_timer = new QTimer(this);                          // redraw timer !!!
    m_timer->setInterval(usr().window_redraw_rate());    // 20
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->start();
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::needs_update().
 */

void
qstriggereditor::conditional_update ()
{
    if (needs_update())
        update();
}

/**
 *
 */

QSize
qstriggereditor::sizeHint () const
{
    return QSize
    (
        seq().get_length() / zoom() + 100 + c_keyboard_padding_x,
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
    m_font.setPointSize(6);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);
    painter.drawRect(c_keyboard_padding_x, 0, width(), height()); /* background */

    int bpbar = seq().get_beats_per_bar();
    int bwidth = seq().get_beat_width();
    midipulse ticks_per_beat = 4 * perf().get_ppqn() / bwidth;
    midipulse ticks_per_bar = bpbar * ticks_per_beat;
    midipulse ticks_per_step = 6 * zoom();
    midipulse starttick = scroll_offset_ticks() -
            (scroll_offset_ticks() % ticks_per_step);
    midipulse endtick = width() * zoom();
    for (midipulse tick = starttick; tick < endtick; tick += ticks_per_step)
    {
        int x_offset = tick / zoom() + c_keyboard_padding_x - scroll_offset_x();
        pen.setWidth(1);
        if (tick % ticks_per_bar == 0)          /* solid line on every beat */
        {
            pen.setColor(Qt::black);
            pen.setStyle(Qt::SolidLine);
            pen.setWidth(2);                    /* two pixels               */
        }
        else if (tick % ticks_per_beat == 0)
        {
            pen.setColor(Qt::black);
            pen.setStyle(Qt::SolidLine);
        }
        else
        {
            pen.setColor(Qt::lightGray);
            pen.setStyle(Qt::DashLine);
            int tick_snap = tick - (tick % snap());

            if (tick == tick_snap)
            {
                pen.setStyle(Qt::SolidLine);    // pen.setColor(Qt::DashLine)
                pen.setColor(Qt::lightGray);    // faint step lines
            }
            else
            {
                pen.setStyle(Qt::DashLine);     // Gdk::LINE_ON_OFF_DASH
                pen.setColor(Qt::lightGray);    // faint step lines
            }
        }
        painter.setPen(pen);
        painter.drawLine(x_offset, 1, x_offset, qc_eventarea_y);
    }

    /* draw boxes from sequence */

    pen.setColor(Qt::black);
    pen.setStyle(Qt::SolidLine);

    event_list::const_iterator cev;
    seq().reset_ex_iterator(cev);                   /* reset_draw_marker()  */
    while (seq().get_next_event_match(m_status, m_cc, cev))
    {
        midipulse tick = cev->get_timestamp();
        if ((tick >= starttick && tick <= endtick))
        {
            bool selected = cev->is_selected();
            int x = tick / zoom() + c_keyboard_padding_x;
            int y = (qc_eventarea_y - qc_eventevent_y) / 2;
            pen.setColor(Qt::black);                /* outer event border   */
            brush.setStyle(Qt::SolidPattern);
            brush.setColor(Qt::black);
            painter.setBrush(brush);
            painter.setPen(pen);
            painter.drawRect(x, y, qc_eventevent_x, qc_eventevent_y);
            brush.setColor(selected ? Qt::red : Qt::white);
            painter.setBrush(brush);                /* draw event highlight */
            painter.drawRect(x, y, qc_eventevent_x - 1, qc_eventevent_y - 1);
        }
        ++cev;
    }

    int y = (qc_eventarea_y - qc_eventevent_y) / 2; /* draw selection       */
    int h =  qc_eventevent_y;
    brush.setStyle(Qt::NoBrush);                    /* painter reset        */
    painter.setBrush(brush);
    if (selecting())
    {
        int x, w;
        x_to_w(drop_x(), current_x(), x, w);
        old_rect().x(x);
        old_rect().width(w);
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, w, h);
    }
    if (drop_action())              // (m_moving || m_paste)
    {
        int delta_x = current_x() - drop_x();
        int x = selection().x() + delta_x;
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, selection().width(), h);
        old_rect().x(x);
        old_rect().width(selection().width());
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

    /*
     * If it was a button press, set values for dragging.
     */

    current_x(int(event->x()) - c_keyboard_padding_x);
    drop_x(current_x());
    old_rect().clear();             /* reset box holding dirty redraw spot */
    if (paste())
    {
        snap_current_x();
        convert_x(current_x(), tick_s);
        paste(false);
        seq().push_undo();
        seq().paste_selected(tick_s, 0);
    }
    else
    {
        if (event->button() == Qt::LeftButton)
        {
            convert_x(drop_x(), tick_s);        /* turn x,y in to tick/note */
            tick_f = tick_s + (zoom());         /* shift back a few ticks */
            tick_s -= (tick_w);
            if (tick_s < 0)
                tick_s = 0;

            if (adding())
            {
                painting(true);
                snap_drop_x();
                convert_x(drop_x(), tick_s);    /* turn x,y in to tick/note */

                /* add note, length = little less than snap */

                if
                (
                    ! seq().select_events
                    (
                        tick_s, tick_f, m_status, m_cc, sequence::e_would_select
                    )
                )
                {
                    seq().push_undo();
                    drop_event(tick_s);
                }
            }
            else /* selecting */
            {
                if
                (
                    ! seq().select_events
                    (
                        tick_s, tick_f, m_status, m_cc, sequence::e_is_selected
                    )
                )
                {
                    if (!(event->modifiers() & Qt::ControlModifier))
                        seq().unselect();

                    numsel = seq().select_events(tick_s, tick_f,
                          m_status, m_cc, sequence::e_select_one);

                    /* if we didnt select anyhing (user clicked empty space)
                       unselect all notes, and start selecting */

                    /* none selected, start selection box */
                    if (numsel == 0)
                    {
                        selecting(true);
                    }
                    else
                    {
                        // needs update
                    }
                }

                if
                (
                    seq().select_events
                    (
                        tick_s, tick_f, m_status, m_cc, sequence::e_is_selected
                    )
                )
                {
                    int note;
                    moving_init(true);

                    /* get the box that selected elements are in */
                    seq().get_selected_box(tick_s, note, tick_f, note);
                    tick_f += tick_w;

                    convert_t(tick_s, x); /* convert box to X,Y values */
                    convert_t(tick_f, w);

                    /* w is actually coorids now, so we have to change */

                    w = w - x;

                    /*
                     * Set selection to hold the x, y, w, h of selected events.
                     */

                    selection().set
                    (
                        x, w, (qc_eventarea_y - qc_eventevent_y) / 2,
                        qc_eventevent_y
                    );

                    /* save offset that we get from the snap above */

                    int adjusted_selected_x = selection().x();
                    snap_x(adjusted_selected_x);
                    move_snap_offset_x(selection().x() - adjusted_selected_x);

                    /* align selection for drawing */
                    //save X as a variable so we can use the snap function
                    int tempSelectedX = selection().x();
                    snap_x(tempSelectedX);
                    selection().x(tempSelectedX);
                    snap_current_x();
                    snap_drop_x();
                }
            }

        } /* end if button == 1 */

        if (event->button() == Qt::RightButton)
        {
            adding(true);
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
    current_x(int(event->x()) - c_keyboard_padding_x);
    if (moving())
        snap_current_x();

    int delta_x = current_x() - drop_x();
    midipulse delta_tick;
    if (event->button() == Qt::LeftButton)
    {
        if (selecting())
        {
            x_to_w(drop_x(), current_x(), x, w);
            convert_x(x, tick_s);
            convert_x(x + w, tick_f);
            seq().select_events(tick_s, tick_f, m_status, m_cc, sequence::e_select);
        }

        if (moving())
        {
            /*
             * Adjust for snap, then convert deltas into screen coordinates.
             */

            delta_x -= move_snap_offset_x();
            convert_x(delta_x, delta_tick);

            /* not really notes, but still moves events */

            seq().push_undo();
            seq().move_selected_notes(delta_tick, 0);
        }
        set_adding(adding());
    }

    if (event->button() == Qt::RightButton)
        set_adding(false);

    clear_action_flags();               /* turn off */
    seq().unpaint_all();
}

/**
 *
 */

void
qstriggereditor::mouseMoveEvent (QMouseEvent * event)
{
    midipulse tick = 0;
    if (moving_init())
    {
        moving_init(false);
        moving(true);
    }
    if (select_action())                // (m_selecting || m_moving || m_paste)
    {
        current_x(int(event->x()) - c_keyboard_padding_x);
        if (drop_action())              // (m_moving || m_paste)
            snap_current_x();
    }
    if (painting())
    {
        current_x(int(event->x()));
        snap_current_x();
        convert_x(current_x(), tick);
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
        seq().remove_selected();
        ret = true;
    }
    if (event->modifiers() & Qt::ControlModifier)
    {
        switch (event->key())
        {
        case Qt::Key_X: /* cut */

            seq().cut_selected();
            ret = true;
            break;

        case Qt::Key_C: /* copy */
            seq().copy_selected();
            ret = true;
            break;

        case Qt::Key_V: /* paste */
            start_paste();
            ret = true;
            break;

        case Qt::Key_Z: /* Undo */
            if (event->modifiers() & Qt::ShiftModifier)
                seq().pop_redo();
            else
                seq().pop_undo();
            ret = true;
            break;
        }
    }
    if (ret == true)
        seq().set_dirty();
    else
        QWidget::keyPressEvent(event);
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

    snap_current_x();
    snap_current_y();
    drop_x(current_x());
    drop_y(current_y());
    paste(true);

    /* get the box that selected elements are in */

    seq().get_clipboard_box(tick_s, note_h, tick_f, note_l);
    convert_t(tick_s, x);           /* convert box to X,Y values */
    convert_t(tick_f, w);

    /* w is actually corrids now, so we have to change */

    w -= x;

    /* set the m_selected rectangle to hold the x,y,w,h of our selected events */

    selection().set
    (
        x, w, (qc_eventarea_y - qc_eventevent_y) / 2, qc_eventevent_y
    );
    selection().x(selection().x() + drop_x()); /* adjust, clipboard shift to tick 0 */
}

/**
 *
 */

void
qstriggereditor::convert_x (int x, midipulse & tick)
{
    tick = x * zoom();
}

/**
 *
 */

void
qstriggereditor::convert_t (midipulse ticks, int & x)
{
    x = ticks / zoom();
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

    seq().add_event(a_tick, status, d0, d1, true);
}

/**
 *
 */

void
qstriggereditor::set_adding (bool a)
{
    adding(a);
    if (a)
        setCursor(Qt::PointingHandCursor);
    else
        setCursor(Qt::ArrowCursor);
}

/**
 *
 */

void
qstriggereditor::set_data_type (midibyte status, midibyte control)
{
    m_status = status;
    m_cc = control;
    set_dirty();
}

}           // namespace seq64

/*
 * qstriggereditor.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

