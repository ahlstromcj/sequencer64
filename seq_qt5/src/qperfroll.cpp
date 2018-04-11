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
 * \file          qperfroll.cpp
 *
 *  This module declares/defines the base class for the Qt 5 version of the
 *  Performance window piano roll.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-04-10
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.
 */

#include "perform.hpp"
#include "qperfroll.hpp"
#include "rect.hpp"                     /* seq64::rect::xy_to_rect_get()    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *
 */

qperfroll::qperfroll (perform & p, QWidget * parent)
 :
    QWidget             (parent),
    gui_palette_qt5     (),
    mPerf               (p),
    mTimer              (nullptr),
    mFont               (),
    m_snap              (0),
    m_measure_length    (0),
    m_beat_length       (0),
    m_roll_length_ticks (0),
    m_drop_x            (0),
    m_drop_y            (0),
    m_current_x         (0),
    m_current_y         (0),
    m_drop_sequence     (0),
    m_zoom              (1),
    m_tick_s            (0),
    m_tick_f            (0),
    m_seq_h             (-1),
    m_seq_l             (-1),
    m_drop_tick         (0),
    m_drop_tick_trigger_offset (0),
    mLastTick           (0),
    m_sequence_active   (),         // array
    m_moving            (false),
    mBoxSelect          (false),
    m_growing           (false),
    m_grow_direction    (false),
    m_adding            (false),
    m_adding_pressed    (false)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    for (int i = 0; i < c_max_sequence; ++i)
        m_sequence_active[i] = false;

    m_roll_length_ticks = perf().get_max_trigger();
    m_roll_length_ticks -= (m_roll_length_ticks % (c_ppqn * 16));
    m_roll_length_ticks +=  c_ppqn * 64;
    mTimer = new QTimer(this);      // refresh timer to queue regular redraws
    mTimer->setInterval(50);
    QObject::connect(mTimer, SIGNAL(timeout()), this, SLOT(update()));
    mTimer->start();
}

/**
 *
 */

void
qperfroll::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush(Qt::NoBrush);
    pen.setStyle(Qt::SolidLine);
    mFont.setPointSize(6);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(mFont);

    int beats = m_measure_length / m_beat_length;
    for (int i = 0; i < width(); )              /* draw vert lines */
    {
        if (i % beats == 0)                     /* solid line on every beat */
        {
            pen.setStyle(Qt::SolidLine);
            pen.setColor(Qt::black);
        }
        else
        {
            pen.setColor(Qt::lightGray);
            pen.setStyle(Qt::DotLine);
        }

        painter.setPen(pen);
        painter.drawLine
        (
            i * m_beat_length / (c_perf_scale_x * m_zoom), 1,
            i * m_beat_length / (c_perf_scale_x * m_zoom), height() - 1
        );
        if (m_beat_length < c_ppqn / 2)             // jump 2 if 16th notes
            i += (c_ppqn / m_beat_length);
        else
            ++i;
    }

    for (int i = 0; i < height(); i += c_names_y)   // draw horizontal lines
    {
        pen.setColor(Qt::black);
        pen.setStyle(Qt::DotLine);
        painter.setPen(pen);
        painter.drawLine(0, i, width(), i);
    }

    int y_s = 0;                                    // draw background
    int y_f = height() / c_names_y;
    midipulse tick_on;                              // draw sequence block
    midipulse tick_off;
    midipulse offset;
    bool selected;

    midipulse tick_offset = 0;              // long tick_offset = c_ppqn * 16;
    int x_offset = tick_offset / (c_perf_scale_x * m_zoom);
    for (int y = y_s; y <= y_f; ++y)
    {
        int seqId = y;
        if (seqId < c_max_sequence)
        {
            if (perf().is_active(seqId))
            {
                m_sequence_active[seqId] = true;
                sequence * seq =  perf().get_sequence(seqId);
                seq->reset_draw_trigger_marker();
                midipulse seq_length = seq->get_length();
                int length_w = seq_length / (c_perf_scale_x * m_zoom);
                while (seq->get_next_trigger(tick_on, tick_off, selected, offset))
                {
                    if (tick_off > 0)
                    {
                        int x_on = tick_on  / (c_perf_scale_x * m_zoom);
                        int x_off = tick_off / (c_perf_scale_x * m_zoom);
                        int w = x_off - x_on + 1;
                        int x = x_on;
                        int y = c_names_y * seqId + 1;  // + 2
                        int h = c_names_y - 2; // - 4
                        x = x - x_offset;   // adjust to screen coordinates
                        if (selected)
                            pen.setColor(Qt::red);
                        else
                            pen.setColor(Qt::black);

                        // Get seq's assigned colour and beautify it.

                        int c = perf().get_sequence_color(seqId);
                        Color backcolor = get_color_fix(PaletteColor(c));

                        pen.setStyle(Qt::SolidLine);  // main seq icon box
                        brush.setColor(backcolor);
                        brush.setStyle(Qt::SolidPattern);
                        painter.setBrush(brush);
                        painter.setPen(pen);
                        painter.drawRect(x, y, w, h);

                        brush.setStyle(Qt::NoBrush);  // seq grab handle left
                        painter.setBrush(brush);
                        pen.setColor(Qt::black);
                        painter.setPen(pen);
                        painter.drawRect
                        (
                            x, y, c_perfroll_size_box_w, c_perfroll_size_box_w
                        );

                        painter.drawRect              // seq grab handle right
                        (
                            x + w - c_perfroll_size_box_w,
                            y + h - c_perfroll_size_box_w,
                            c_perfroll_size_box_w, c_perfroll_size_box_w
                        );

                        pen.setColor(Qt::black);
                        painter.setPen(pen);

                        midipulse length_marker_first_tick =
                        (
                            tick_on - (tick_on % seq_length) +
                            (offset % seq_length) - seq_length
                        );

                        midipulse tick_marker = length_marker_first_tick;
                        while (tick_marker < tick_off)
                        {
                            midipulse tick_marker_x =
                            (
                                tick_marker / (c_perf_scale_x * m_zoom)
                            ) - x_offset;
                            int lowest_note;  // = seq->get_lowest_note_event();
                            int highest_note; // = seq->get_highest_note_event();
                            (void) seq->get_minmax_note_events
                            (
                                lowest_note, highest_note
                            );

                            int height = highest_note - lowest_note;
                            height += 2;

                            int length = seq->get_length();
                            midipulse tick_s;
                            midipulse tick_f;
                            int note;
                            bool selected;
                            int velocity;
                            draw_type_t dt;
                            seq->reset_draw_marker();
                            pen.setColor(Qt::black);
                            painter.setPen(pen);
                            do
                            {
                                dt = seq->get_next_note_event
                                (
                                    tick_s, tick_f, note, selected, velocity
                                );
                                if (dt == DRAW_FIN)
                                    break;

                                /*
                                 * TODO:  handle DRAW_TEMPO
                                 */

                                int note_y = ((c_names_y - 6) -
                                    ((c_names_y - 6)  * (note - lowest_note)) /
                                    height) + 1;

                                int tick_s_x = ((tick_s * length_w) / length) +
                                    tick_marker_x;

                                int tick_f_x = ((tick_f * length_w) / length) +
                                    tick_marker_x;

                                if (dt == DRAW_NOTE_ON || dt == DRAW_NOTE_OFF)
                                    tick_f_x = tick_s_x + 1;

                                if (tick_f_x <= tick_s_x)
                                    tick_f_x = tick_s_x + 1;

                                if (tick_s_x < x)
                                    tick_s_x = x;

                                if (tick_f_x > x + w)
                                    tick_f_x = x + w;

                                if (tick_f_x >= x && tick_s_x <= x + w)
                                {
                                    painter.drawLine
                                    (
                                        tick_s_x, y + note_y,
                                        tick_f_x, y + note_y
                                    );
                                }

                            } while (dt != DRAW_FIN);

                            if (tick_marker > tick_on)
                            {
                                // lines to break up the seq at each tick
                                pen.setColor(QColor(190, 190, 190, 220));
                                painter.setPen(pen);
                                painter.drawRect(tick_marker_x, y + 4, 1, h - 8);
                            }
                            tick_marker += seq_length;
                        }
                    }
                }
            }
        }
    }

    /* draw selections */

    int x, y, w, h;
    if (mBoxSelect)
    {
        brush.setStyle(Qt::NoBrush);            // painter reset
        pen.setStyle(Qt::SolidLine);
        pen.setColor(Qt::black);
        painter.setBrush(brush);
        painter.setPen(pen);
        rect::xy_to_rect_get
        (
            m_drop_x, m_drop_y, m_current_x, m_current_y, x, y, w, h
        );
        m_old.x(x);
        m_old.y(y);
        m_old.width(w);
        m_old.height(h + c_names_y);
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x, y, w, h + c_names_y);
    }

    pen.setStyle(Qt::SolidLine);                // draw border
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.drawRect(0, 0, width(), height() - 1);

    midipulse tick = perf().get_tick();         // draw playhead
    int progress_x = tick / (c_perf_scale_x * m_zoom);
    pen.setColor(Qt::red);
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.drawLine(progress_x, 1, progress_x, height() - 2);
}

/**
 *
 */

int qperfroll::getSnap() const
{
    return m_snap;
}

/**
 *
 */

void
qperfroll::set_snap(int snap)
{
    m_snap = snap;
}

/**
 *
 */

QSize qperfroll::sizeHint() const
{
    return QSize
    (
        perf().get_max_trigger() / (m_zoom * c_perf_scale_x) + 2000,
        c_names_y * c_max_sequence + 1
    );
}

/**
 *
 */

void
qperfroll::mousePressEvent(QMouseEvent *event)
{
    //    if ( perf().is_active( m_drop_sequence ))
    //    {
    //        perf().get_sequence( m_drop_sequence )->unselect_triggers( );
    //    }

    m_drop_x = event->x();
    m_drop_y = event->y();

    // get the sequence row we're on

    convert_xy(m_drop_x, m_drop_y, m_drop_tick, m_drop_sequence);
    if (event->button() == Qt::LeftButton)
    {
        midipulse tick = m_drop_tick;

        /*
         * Add a new seq instance if we didnt select anything, and are holding
         * the right mouse button.
         */

        if (m_adding)
        {
            m_adding_pressed = true;
            if (perf().is_active(m_drop_sequence))
            {
                midipulse seq_length = perf().get_sequence(m_drop_sequence)->
                    get_length();

                bool trigger_state = perf().get_sequence(m_drop_sequence)->
                    get_trigger_state(tick);

                if (trigger_state)
                {
                    // perf().push_trigger_undo();
                    // perf().get_sequence(m_drop_sequence)->delete_trigger(tick);

                    delete_trigger(m_drop_sequence, tick);
                }
                else
                {
                    if (perf().song_record_snap())  // snap to seq length
                        tick = tick - (tick % seq_length);

//                  perf().push_trigger_undo();
//                  perf().get_sequence(m_drop_sequence)->
//                      add_trigger(tick, seq_length);

                    add_trigger(m_drop_sequence, tick);
                }
            }
        }
        else                /* we aren't holding the right mouse btn */
        {
            bool selected = false;

            if (perf().is_active(m_drop_sequence))  // trigger position ops
            {
                perf().push_trigger_undo();

                // if current seq not in selection range, bin it

                if
                (
                    m_drop_sequence > m_seq_h || m_drop_sequence < m_seq_l ||
                    tick < m_tick_s || tick > m_tick_f
                )
                {
                    perf().unselect_all_triggers();
                    m_seq_h = m_seq_l = m_drop_sequence;
                }

                perf().get_sequence(m_drop_sequence)->select_trigger(tick);

                midipulse start_tick = perf().get_sequence(m_drop_sequence)->
                    selected_trigger_start();

                midipulse end_tick = perf().get_sequence(m_drop_sequence)->
                    selected_trigger_end();

                // check for corner drag to grow sequence start

                int clickminus = c_perfroll_size_box_click_w - 1;
                int clickbox = c_perfroll_size_box_click_w *
                    c_perf_scale_x * m_zoom;

                if
                (
                    tick >= start_tick &&
                    tick <= start_tick + clickbox &&
                    (m_drop_y % c_names_y) <= clickminus
                )
                {
                    m_growing = true;
                    m_grow_direction = true;
                    selected = true;
                    m_drop_tick_trigger_offset = m_drop_tick -
                        perf().get_sequence(m_drop_sequence)->
                            selected_trigger_start();
                }
                else if     // check for corner drag to grow sequence end
                (
                    tick >= end_tick - clickbox && tick <= end_tick &&
                    (m_drop_y % c_names_y) >= c_names_y - clickminus
                )
                {
                    m_growing = true;
                    selected = true;
                    m_grow_direction = false;
                    m_drop_tick_trigger_offset = m_drop_tick -
                        perf().get_sequence(m_drop_sequence)->
                            selected_trigger_end();
                }
                else if (tick <= end_tick && tick >= start_tick)
                {
                    m_moving = true;            // we're moving the seq
                    selected = true;
                    m_drop_tick_trigger_offset = m_drop_tick -
                        perf().get_sequence(m_drop_sequence)->
                            selected_trigger_start();
                }
            }

            if (! selected)                     // let's select with a box
            {
                perf().unselect_all_triggers();
                snap_y(m_drop_y);               // y is always snapped to rows
                m_current_x = m_drop_x;
                m_current_y = m_drop_y;
                mBoxSelect = true;
            }
        }
    }
    if (event->button() == Qt::RightButton)
    {
        set_adding(true);
        perf().unselect_all_triggers();
        mBoxSelect = false;
    }

    /* middle mouse button, split seq under cursor */

    if (event->button() == Qt::MiddleButton)
    {
        if (perf().is_active(m_drop_sequence))
        {
            bool state = perf().get_sequence(m_drop_sequence)->
                get_trigger_state(m_drop_tick);

            if (state)
                half_split_trigger(m_drop_sequence, m_drop_tick);
        }
    }
}

/**
 *
 */

void
qperfroll::mouseReleaseEvent (QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_adding)
            m_adding_pressed = false;

        if (mBoxSelect) //calculate selected seqs in box
        {
            int x, y, w, h; //window dimensions
            m_current_x = event->x();
            m_current_y = event->y();
            snap_y(m_current_y);
            rect::xy_to_rect_get
            (
                m_drop_x, m_drop_y, m_current_x, m_current_y, x, y, w, h
            );
            convert_xy(x,     y, m_tick_s, m_seq_l);
            convert_xy(x + w, y + h, m_tick_f, m_seq_h);
            perf().select_triggers_in_range(m_seq_l, m_seq_h, m_tick_s, m_tick_f);
        }
    }

    if (event->button() == Qt::RightButton)
    {
        m_adding_pressed = false;
        set_adding(false);
    }

    m_moving = false;
    m_growing = false;
    m_adding_pressed = false;
    mBoxSelect = false;
    mLastTick = 0;
}

/**
 *
 */

void
qperfroll::mouseMoveEvent (QMouseEvent * event)
{
    midipulse tick = 0;
    int x = event->x();
    if (m_adding && m_adding_pressed)
    {
        convert_x(x, tick);
        if (perf().is_active(m_drop_sequence))
        {
            midipulse seq_length = perf().get_sequence(m_drop_sequence)->
                get_length();

            if (perf().song_record_snap())      // snap to length of sequence
                tick = tick - (tick % seq_length);

            midipulse length = seq_length;
            perf().get_sequence(m_drop_sequence)->
                grow_trigger(m_drop_tick, tick, length);
        }
    }
    else if (m_moving || m_growing)
    {
        if (perf().is_active(m_drop_sequence))
        {
            convert_x(x, tick);
            tick -= m_drop_tick_trigger_offset;
            if (perf().song_record_snap())      // snap to length of sequence
                tick = tick - tick % m_snap;

            if (m_moving)                       // move all selected triggers
            {
                for (int seqId = m_seq_l; seqId <= m_seq_h; seqId++)
                {
                    if (perf().is_active(seqId))
                    {
                        if (mLastTick != 0)
                            perf().get_sequence(seqId)->
                                offset_triggers(-(mLastTick - tick));
                    }
                }
            }
            if (m_growing)
            {
                if (m_grow_direction)
                {
                    // grow start of all selected triggers

                    for (int seqId = m_seq_l; seqId <= m_seq_h; seqId++)
                    {
                        if (perf().is_active(seqId))
                        {
                            if (mLastTick != 0)
                            {
                                perf().get_sequence(seqId)->offset_triggers
                                (
                                    -(mLastTick - tick), triggers::GROW_START
                                );
                            }
                        }
                    }
                }
                else
                {
                    // grow end of all selected triggers

                    for (int seqId = m_seq_l; seqId <= m_seq_h; seqId++)
                    {
                        if (perf().is_active(seqId))
                        {
                            if (mLastTick != 0)
                            {
                                perf().get_sequence(seqId)->offset_triggers
                                (
                                    -(mLastTick - tick) - 1, triggers::GROW_END
                                );
                            }
                        }
                    }
                }
            }
        }
    }
    else if (mBoxSelect)                    // box selection
    {
        m_current_x = event->x();
        m_current_y = event->y();
        snap_y(m_current_y);
        convert_xy(0, m_current_y, tick, m_drop_sequence);
    }
    mLastTick = tick;
}

/**
 *
 */

void
qperfroll::keyPressEvent (QKeyEvent * event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        perf().push_trigger_undo();                 // delete selected notes
        for (int seqId = m_seq_l; seqId <= m_seq_h; seqId++)
            if (perf().is_active(seqId))
                perf().get_sequence(seqId)->delete_selected_triggers();
    }
    if (event->modifiers() & Qt::ControlModifier)   // Ctrl + ... events
    {
        switch (event->key())
        {
        case Qt::Key_X:
            perf().push_trigger_undo();
            perf().get_sequence(m_drop_sequence)->cut_selected_trigger();
            break;

        case Qt::Key_C:
            perf().get_sequence(m_drop_sequence)->copy_selected_trigger();
            break;

        case Qt::Key_V:
            perf().push_trigger_undo();
            perf().get_sequence(m_drop_sequence)->paste_trigger();
            break;

        case Qt::Key_Z:
            if (event->modifiers() & Qt::ShiftModifier)
                perf().pop_trigger_redo();
            else
                perf().pop_trigger_undo();
            break;
        }
    }
}

/**
 *
 */

void
qperfroll::keyReleaseEvent (QKeyEvent * /*event*/)
{
    // no code
}

/**
 *  Performs a 'snap' on x:
 *
 *      -   snap = number pulses to snap to
 *      -   m_scale = number of pulses per pixel
 *
 *  So, snap / m_scale = number pixels to snap to.
 */

void
qperfroll::snap_x (int & x)
{
    int mod = (m_snap / (c_perf_scale_x * m_zoom));
    if (mod <= 0)
        mod = 1;

    x -= x % mod;
}

/**
 *
 */

void
qperfroll::snap_y (int & y)
{
    y -= y % c_names_y;
}

/**
 *
 */

void
qperfroll::convert_x (int x, midipulse & tick)
{
    midipulse tick_offset = 0;                  // it's always this!!!
    tick = x * (c_perf_scale_x * m_zoom);
    tick += tick_offset;
}

/**
 *
 */

void
qperfroll::convert_xy (int x, int y, midipulse & tick, int & seq)
{
    midipulse tick_offset =  0;                 // again, always 0!!!
    tick = x * (c_perf_scale_x * m_zoom);
    seq = y / c_names_y;
    tick += tick_offset;
    if (seq >= c_max_sequence)
        seq = c_max_sequence - 1;

    if (seq < 0)
        seq = 0;
}

/**
 *
 */

void
qperfroll::add_trigger(int seq, midipulse tick)
{
    perf().add_trigger(seq, tick);
}

/**
 *
 */

void
qperfroll::half_split_trigger (int seq, midipulse tick)
{
//  perf().push_trigger_undo();
//  perf().get_sequence(seq)->half_split_trigger(tick);

    perf().split_trigger(seq, tick);
}

/**
 *
 */

void
qperfroll::delete_trigger(int seq, midipulse tick)
{
    perf().delete_trigger(seq, tick);
}

/* simply sets the snap member */

void
qperfroll::set_guides (int snap, int measure, int beat)
{
    m_snap = snap;
    m_measure_length = measure;
    m_beat_length = beat;
}

/**
 *
 */

void
qperfroll::set_adding (bool adding)
{
    m_adding = adding;
    if (adding)
        setCursor(Qt::PointingHandCursor);
    else
        setCursor(Qt::ArrowCursor);
}

/**
 *
 */

void
qperfroll::undo ()
{
    perf().pop_trigger_undo();
}

/**
 *
 */

void
qperfroll::redo ()
{
    perf().pop_trigger_redo();
}

/**
 *
 */

void
qperfroll::zoom_in ()
{
    if (m_zoom > 1)
        m_zoom *= 0.5;
}

/**
 *
 */

void
qperfroll::zoom_out ()
{
    m_zoom *= 2;
}

}           // namespace seq64

/*
 * qperfroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

