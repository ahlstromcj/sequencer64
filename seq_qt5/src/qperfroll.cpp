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
 * \updates       2018-03-05
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.
 */

#include "perform.hpp"
#include "qperfroll.hpp"

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
    mPen                (nullptr),
    mBrush              (nullptr),
    mPainter            (nullptr),
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
    zoom                (1),
    tick_s              (0),
    tick_f              (0),
    seq_h               (-1),
    seq_l               (-1),
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
    m_roll_length_ticks = m_roll_length_ticks -
                          (m_roll_length_ticks % (c_ppqn * 16));
    m_roll_length_ticks +=  c_ppqn * 64;

    //start refresh timer to queue regular redraws
    mTimer = new QTimer(this);
    mTimer->setInterval(50);
    QObject::connect(mTimer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(update()));
    mTimer->start();
}

void
qperfroll::paintEvent(QPaintEvent *)
{
    mPainter = new QPainter(this);
    mBrush = new QBrush(Qt::NoBrush);
    mPen = new QPen(Qt::black);
    mPen->setStyle(Qt::SolidLine);
    mFont.setPointSize(6);
    mPainter->setPen(*mPen);
    mPainter->setBrush(*mBrush);
    mPainter->setFont(mFont);

    int beats = m_measure_length / m_beat_length;

    /* draw vert lines */
    for (int i = 0; i < width();)
    {
        /* solid line on every beat */
        if (i % beats == 0)
        {
            mPen->setStyle(Qt::SolidLine);
            mPen->setColor(Qt::black);
        }
        else
        {
            mPen->setColor(Qt::lightGray);
            mPen->setStyle(Qt::DotLine);
        }

        mPainter->setPen(*mPen);
        mPainter->drawLine(i * m_beat_length / (c_perf_scale_x * zoom),
                           1,
                           i * m_beat_length / (c_perf_scale_x * zoom),
                           height() - 1);

        // jump 2 if 16th notes
        if (m_beat_length < c_ppqn / 2)
        {
            i += (c_ppqn / m_beat_length);
        }
        else
        {
            ++i;
        }

    }

    //draw horizontal lines
    for (int i = 0; i < height(); i += c_names_y)
    {
        mPen->setColor(Qt::black);
        mPen->setStyle(Qt::DotLine);
        mPainter->setPen(*mPen);
        mPainter->drawLine(0,
                           i,
                           width(),
                           i);
    }

    //draw background
    int y_s = 0;
    int y_f = height() / c_names_y;

    //draw sequence block
    midipulse tick_on;
    midipulse tick_off;
    midipulse offset;
    bool selected;

    //    long tick_offset = c_ppqn * 16;
    midipulse tick_offset = 0;
    int x_offset = tick_offset / (c_perf_scale_x * zoom);
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
                int length_w = seq_length / (c_perf_scale_x * zoom);

//              while (seq->get_next_trigger(&tick_on, &tick_off, &selected, &offset))
                while (seq->get_next_trigger(tick_on, tick_off, selected, offset))
                {
                    if (tick_off > 0)
                    {
                        int x_on = tick_on  / (c_perf_scale_x * zoom);
                        int x_off = tick_off / (c_perf_scale_x * zoom);
                        int w = x_off - x_on + 1;
                        int x = x_on;
                        int y = c_names_y * seqId + 1;  // + 2
                        int h = c_names_y - 2; // - 4

                        // adjust to screen coordinates

                        x = x - x_offset;
                        if (selected)
                            mPen->setColor(Qt::red);
                        else
                            mPen->setColor(Qt::black);

                        // Get seq's assigned colour and beautify it.

                        Color backcolor;
                        int c = perf().get_sequence_color(seqId);
                        if (c != SEQ64_COLOR_NONE)
                            backcolor = get_color_fix(PaletteColor(c));

                        mPen->setStyle(Qt::SolidLine);  // main seq icon box
                        mBrush->setColor(backcolor);
                        mBrush->setStyle(Qt::SolidPattern);
                        mPainter->setBrush(*mBrush);
                        mPainter->setPen(*mPen);
                        mPainter->drawRect(x, y, w, h);

                        mBrush->setStyle(Qt::NoBrush);  // seq grab handle left
                        mPainter->setBrush(*mBrush);
                        mPen->setColor(Qt::black);
                        mPainter->setPen(*mPen);
                        mPainter->drawRect
                        (
                            x, y, c_perfroll_size_box_w, c_perfroll_size_box_w
                        );

                        mPainter->drawRect              // seq grab handle right
                        (
                            x + w - c_perfroll_size_box_w,
                            y + h - c_perfroll_size_box_w,
                            c_perfroll_size_box_w, c_perfroll_size_box_w
                        );

                        mPen->setColor(Qt::black);
                        mPainter->setPen(*mPen);

                        long length_marker_first_tick =
                        (
                            tick_on - (tick_on % seq_length) +
                            (offset % seq_length) - seq_length
                        );

                        long tick_marker = length_marker_first_tick;
                        while (tick_marker < tick_off)
                        {
                            long tick_marker_x = (tick_marker / (c_perf_scale_x * zoom)) - x_offset;
                            int lowest_note; // = seq->get_lowest_note_event();
                            int highest_note; // = seq->get_highest_note_event();
                            (void) seq->get_minmax_note_events(lowest_note, highest_note);

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
                            mPen->setColor(Qt::black);
                            mPainter->setPen(*mPen);
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
                                              ((c_names_y - 6)  * (note - lowest_note)) / height) + 1;
                                int tick_s_x = ((tick_s * length_w)  / length) + tick_marker_x;
                                int tick_f_x = ((tick_f * length_w)  / length) + tick_marker_x;
                                if (dt == DRAW_NOTE_ON || dt == DRAW_NOTE_OFF)
                                    tick_f_x = tick_s_x + 1;

                                if (tick_f_x <= tick_s_x)
                                    tick_f_x = tick_s_x + 1;

                                if (tick_s_x < x)
                                    tick_s_x = x;

                                if (tick_f_x > x + w)
                                    tick_f_x = x + w;

                                if (tick_f_x >= x && tick_s_x <= x + w)
                                    mPainter->drawLine(tick_s_x,
                                                       y + note_y,
                                                       tick_f_x,
                                                       y + note_y);
                            } while (dt != DRAW_FIN);

                            if (tick_marker > tick_on)
                            {
                                //lines to break up the seq at each tick
                                mPen->setColor(QColor(190, 190, 190, 220));
                                mPainter->setPen(*mPen);
                                mPainter->drawRect
                                (
                                    tick_marker_x, y + 4, 1, h - 8
                                );
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
        //painter reset
        mBrush->setStyle(Qt::NoBrush);
        mPen->setStyle(Qt::SolidLine);
        mPen->setColor(Qt::black);
        mPainter->setBrush(*mBrush);
        mPainter->setPen(*mPen);

        xy_to_rect(m_drop_x, m_drop_y, m_current_x, m_current_y, &x, &y, &w, &h);

        m_old.x(x);
        m_old.y(y);
        m_old.width(w);
        m_old.height(h + c_names_y);

        mPen->setColor(Qt::black);
        mPainter->setPen(*mPen);
        mPainter->drawRect(x, y, w, h + c_names_y);
    }

    //draw border
    mPen->setStyle(Qt::SolidLine);
    mPen->setColor(Qt::black);
    mPainter->setPen(*mPen);
    mPainter->drawRect(0, 0, width(), height() - 1);

    //draw playhead
    midipulse tick = perf().get_tick();
    int progress_x = tick / (c_perf_scale_x * zoom);
    mPen->setColor(Qt::red);
    mPen->setStyle(Qt::SolidLine);
    mPainter->setPen(*mPen);
    mPainter->drawLine(progress_x, 1, progress_x, height() - 2);

    delete mPainter;
    delete mBrush;
    delete mPen;
}

int qperfroll::getSnap() const
{
    return m_snap;
}

void
qperfroll::set_snap(int snap)
{
    m_snap = snap;
}

QSize qperfroll::sizeHint() const
{
    return QSize
    (
        perf().get_max_trigger() / (zoom * c_perf_scale_x) + 2000,
        c_names_y * c_max_sequence + 1
    );
}

void
qperfroll::mousePressEvent(QMouseEvent *event)
{
    //    if ( perf().is_active( m_drop_sequence ))
    //    {
    //        perf().get_sequence( m_drop_sequence )->unselect_triggers( );
    //    }

    m_drop_x = event->x();
    m_drop_y = event->y();

    //convery to get the sequence row we're on
    convert_xy(m_drop_x, m_drop_y, &m_drop_tick, &m_drop_sequence);

    /* left mouse button */
    if (event->button() == Qt::LeftButton)
    {
        long tick = m_drop_tick;

        /* add a new seq instance if we didnt select anything,
         * and are holding the right mouse btn */
        if (m_adding)
        {
            m_adding_pressed = true;

            if (perf().is_active(m_drop_sequence))
            {
                long seq_length = perf().get_sequence(m_drop_sequence)->get_length();
                bool trigger_state = perf().get_sequence(m_drop_sequence)->get_trigger_state(tick);

                if (trigger_state)
                {
                    perf().push_trigger_undo();
                    perf().get_sequence(m_drop_sequence)->delete_trigger(tick);
                }
                else
                {
                    // snap to length of sequence
                    if (perf().song_record_snap())
                        tick = tick - (tick % seq_length);

                    perf().push_trigger_undo();
                    perf().get_sequence(m_drop_sequence)->add_trigger(tick, seq_length);

                }
            }
        }
        /* we aren't holding the right mouse btn */
        else
        {
            bool selected = false;

            //trigger position operations
            if (perf().is_active(m_drop_sequence))
            {
                perf().push_trigger_undo();

                //if the current seq is not in our selection range,
                //bin our selection
                if (m_drop_sequence > seq_h || m_drop_sequence < seq_l ||
                        tick < tick_s || tick > tick_f)
                {
                    perf().unselect_all_triggers();
                    seq_h = seq_l = m_drop_sequence;
                }

                perf().get_sequence(m_drop_sequence)->select_trigger(tick);

                long start_tick =
                perf().get_sequence(m_drop_sequence)->selected_trigger_start();
                long end_tick =
                perf().get_sequence(m_drop_sequence)->selected_trigger_end();

                //check for corner drag to grow sequence start
                if (tick >= start_tick &&
                        tick <= start_tick + (c_perfroll_size_box_click_w * (c_perf_scale_x * zoom)) &&
                        (m_drop_y % c_names_y) <= c_perfroll_size_box_click_w + 1)
                {
                    m_growing = true;
                    m_grow_direction = true;
                    selected = true;
                    m_drop_tick_trigger_offset = m_drop_tick -
                                                 perf().get_sequence(m_drop_sequence)->
                                                 selected_trigger_start();
                }
                //check for corner drag to grow sequence end
                else if (tick >= end_tick -
                         (c_perfroll_size_box_click_w * (c_perf_scale_x * zoom))
                         && tick <= end_tick &&
                         (m_drop_y % c_names_y) >= c_names_y - c_perfroll_size_box_click_w - 1)
                {
                    m_growing = true;
                    selected = true;
                    m_grow_direction = false;
                    m_drop_tick_trigger_offset =
                        m_drop_tick -
                        perf().get_sequence(m_drop_sequence)->selected_trigger_end();
                }
                //else we're moving the seq
                else if (tick <= end_tick && tick >= start_tick)
                {
                    m_moving = true;
                    selected = true;
                    m_drop_tick_trigger_offset = m_drop_tick -
                                                 perf().get_sequence(m_drop_sequence)->
                                                 selected_trigger_start();

                }

            }

            if (!selected) //let's select with a box
            {
                perf().unselect_all_triggers();

                //y is always snapped to rows
                snap_y(&m_drop_y);

                m_current_x = m_drop_x;
                m_current_y = m_drop_y;

                mBoxSelect = true;
            }
        }
    }

    /* right mouse button */
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
            bool state = perf().get_sequence(m_drop_sequence)->get_trigger_state(m_drop_tick);

            if (state)
            {
                half_split_trigger(m_drop_sequence, m_drop_tick);
            }
        }
    }

}

void
qperfroll::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_adding)
        {
            m_adding_pressed = false;
        }

        if (mBoxSelect) //calculate selected seqs in box
        {
            int x, y, w, h; //window dimensions

            m_current_x = event->x();
            m_current_y = event->y();

            snap_y(&m_current_y);

            xy_to_rect(m_drop_x,
                       m_drop_y,
                       m_current_x,
                       m_current_y,
                       &x, &y,
                       &w, &h);

            convert_xy(x,     y, &tick_s, &seq_l);
            convert_xy(x + w, y + h, &tick_f, &seq_h);
            perf().select_triggers_in_range(seq_l, seq_h, tick_s, tick_f);
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

void
qperfroll::mouseMoveEvent(QMouseEvent *event)
{
    long tick;
    int x = event->x();

    if (m_adding && m_adding_pressed)
    {
        convert_x(x, &tick);

        if (perf().is_active(m_drop_sequence))
        {
            long seq_length = perf().get_sequence(m_drop_sequence)->get_length();

            // snap to length of sequence
            if (perf().song_record_snap())
                tick = tick - (tick % seq_length);

            long length = seq_length;

            perf().get_sequence(m_drop_sequence)
            ->grow_trigger(m_drop_tick, tick, length);
        }
    }
    else if (m_moving || m_growing)
    {

        if (perf().is_active(m_drop_sequence))
        {

            convert_x(x, &tick);
            tick -= m_drop_tick_trigger_offset;

            // snap to length of sequence
            if (perf().song_record_snap())
                tick = tick - tick % m_snap;

            if (m_moving)
            {
                //move all selected triggers
                for (int seqId = seq_l; seqId <= seq_h; seqId++)
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
                    //grow start of all selected triggers
                    for (int seqId = seq_l; seqId <= seq_h; seqId++)
                    {
                        if (perf().is_active(seqId))
                        {
                            if (mLastTick != 0)
                                perf().get_sequence(seqId)->
                                    offset_triggers(-(mLastTick - tick), triggers::GROW_START);
                        }
                    }
                }
                else
                {
                    //grow end of all selected triggers
                    for (int seqId = seq_l; seqId <= seq_h; seqId++)
                    {
                        if (perf().is_active(seqId))
                        {
                            if (mLastTick != 0)
                                perf().get_sequence(seqId)->
                                    offset_triggers(-(mLastTick - tick) - 1, triggers::GROW_END);
                        }
                    }
                }
            }
        }
    }
    else if (mBoxSelect) //box selection
    {
        m_current_x = event->x();
        m_current_y = event->y();
        snap_y(&m_current_y);
        convert_xy(0, m_current_y, &tick, &m_drop_sequence);
    }

    mLastTick = tick;
}

void
qperfroll::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete ||
            event->key() == Qt::Key_Backspace)
    {
        //delete selected notes
        perf().push_trigger_undo();
        for (int seqId = seq_l; seqId <= seq_h; seqId++)
            if (perf().is_active(seqId))
                perf().get_sequence(seqId)->delete_selected_triggers();
    }

    //Ctrl + ... events
    if (event->modifiers() & Qt::ControlModifier)
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
void
qperfroll::keyReleaseEvent(QKeyEvent *event)
{

}

/* performs a 'snap' on x */
void
qperfroll::snap_x(int *a_x)
{
    // snap = number pulses to snap to
    // m_scale = number of pulses per pixel
    //  so snap / m_scale  = number pixels to snap to

    int mod = (m_snap / (c_perf_scale_x * zoom));

    if (mod <= 0)
        mod = 1;

    *a_x = *a_x - (*a_x % mod);
}


void
qperfroll::snap_y(int *a_y)
{
    *a_y = *a_y - (*a_y % c_names_y);
}


void
qperfroll::convert_x(int a_x, long *a_tick)
{
    long tick_offset = 0;
    *a_tick = a_x * (c_perf_scale_x * zoom);
    *a_tick += tick_offset;
}


void
qperfroll::convert_xy(int a_x, int a_y, long *a_tick, int *a_seq)
{
    long tick_offset =  0;

    *a_tick = a_x * (c_perf_scale_x * zoom);
    *a_seq = a_y / c_names_y;

    *a_tick += tick_offset;

    if (*a_seq >= c_max_sequence)
        *a_seq = c_max_sequence - 1;

    if (*a_seq < 0)
        *a_seq = 0;
}

void
qperfroll::half_split_trigger(int a_sequence, long a_tick)
{
    perf().push_trigger_undo();
    perf().get_sequence(a_sequence)->half_split_trigger(a_tick);
}

/* simply sets the snap member */
void
qperfroll::set_guides(int a_snap, int a_measure, int a_beat)
{
    m_snap = a_snap;
    m_measure_length = a_measure;
    m_beat_length = a_beat;
}

void
qperfroll::set_adding(bool a_adding)
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

void
qperfroll::undo()
{
    perf().pop_trigger_undo();
}

void
qperfroll::redo()
{
    perf().pop_trigger_redo();
}

void
qperfroll::zoom_in()
{
    if (zoom > 1)
        zoom *= 0.5;
}

void
qperfroll::zoom_out()
{
    zoom *= 2;
}

void
qperfroll::xy_to_rect(int a_x1, int a_y1, int a_x2, int a_y2,
                           int *a_x, int *a_y, int *a_w, int *a_h)
{
    if (a_x1 < a_x2)
    {
        *a_x = a_x1;
        *a_w = a_x2 - a_x1;
    }
    else
    {
        *a_x = a_x2;
        *a_w = a_x1 - a_x2;
    }

    if (a_y1 < a_y2)
    {
        *a_y = a_y1;
        *a_h = a_y2 - a_y1;
    }
    else
    {
        *a_y = a_y2;
        *a_h = a_y1 - a_y2;
    }
}

}           // namespace seq64

/*
 * qperfroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

