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
 * \file          qseqroll.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-05-27
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 */

#include "Globals.hpp"
#include "perform.hpp"
#include "qseqroll.hpp"
#include "sequence.hpp"
#include "settings.hpp"                 /* usr()                            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *
 */

qseqroll::qseqroll
(
    perform & perf,
    sequence & seq,
    QWidget * parent,
    edit_mode_t mode
) :
    QWidget                 (parent),
    m_perform               (perf),
    m_seq                   (seq),
    m_old                   (),
    m_selected              (),
    mTimer                  (nullptr),
    mFont                   (),
    m_scale                 (0),
    m_key                   (0),
    m_zoom                  (1),
    m_snap                  (16),
    m_note_length           (c_ppqn * 4 / 16),
    m_selecting             (false),
    m_adding                (false),
    m_moving                (false),
    m_moving_init           (false),
    m_growing               (false),
    m_painting              (false),
    m_paste                 (false),
    m_is_drag_pasting       (false),
    m_is_drag_pasting_start (false),
    m_justselected_one      (false),
    m_drop_x                (0),
    m_drop_y                (0),
    m_move_delta_x          (0),
    m_move_delta_y          (0),
    m_current_x             (0),
    m_current_y             (0),
    m_move_snap_offset_x    (0),
    m_old_progress_x        (0),
    m_background_sequence   (0),
    m_drawing_background_seq (false),
    editMode                (mode),
    note_x                  (0),
    note_width              (0),
    note_y                  (0),
    note_height             (0),
    keyY                    (usr().key_height()),
    keyAreaY                (keyY * c_num_keys + 1)
{
    set_snap(m_seq.get_snap_tick());
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);

    // start refresh timer to queue regular redraws

    mTimer = new QTimer(this);
    mTimer->setInterval(20);
    QObject::connect(mTimer, SIGNAL(timeout()), this, SLOT(update()));
    mTimer->start();
}

/**
 *
 */

void
qseqroll::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QBrush brush(Qt::NoBrush);
    QPen pen(Qt::black);

    mFont.setPointSize(6);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(mFont);

    // draw border
    //    painter.drawRect(0, 0, width(), height());

    pen.setColor(Qt::lightGray);
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);

    for (int i = 1; i < c_num_keys; ++i)    // for each note row in grid
    {
        //set line colour dependent on the note row we're on
        //        if (0 == (((c_num_keys - i) + ( 12 - m_key )) % 12))
        //        {
        //            m_pen->setColor(Qt::darkGray);
        //            m_pen->setStyle(Qt::SolidLine);
        //            painter.setPen(*m_pen);
        //        }
        //        else if (11 == (((c_num_keys - i) + ( 12 - m_key )) % 12))
        //        {
        //            m_pen->setColor(Qt::darkGray);
        //            m_pen->setStyle(Qt::SolidLine);
        //            painter.setPen(*m_pen);
        //        }

        //draw horizontal grid lines
        //(differently depending on editing mode)
        switch (editMode)
        {
        case EDIT_MODE_NOTE:
            painter.drawLine(0, i * keyY, width(), i * keyY);
            break;

        case EDIT_MODE_DRUM:
            painter.drawLine
            (
                0, i * keyY - (0.5 * keyY), width(), i * keyY - (0.5 * keyY)
            );
            break;
        }

        if (m_scale != c_scale_off)
        {
            if (!c_scales_policy[m_scale][((c_num_keys - i) - 1 + (12 - m_key)) % 12])
            {
                // Commented out in kepler34
                // painter.drawRect(0, i * keyY + 1, m_size_x, keyY - 1);
            }
        }
    }

    int measures_per_line = 1;
    int ticks_per_measure = m_seq.get_beats_per_bar() *
        (4 * c_ppqn) / m_seq.get_beat_width();

    int ticks_per_beat = (4 * c_ppqn) / m_seq.get_beat_width();
    int ticks_per_step = 6 * m_zoom;
    int ticks_per_m_line =  ticks_per_measure * measures_per_line;

    // Draw vertical grid lines

    for (int i = 0; i < width(); i += ticks_per_step)
    {
        int base_line = i + c_keyboard_padding_x;
        if (i % ticks_per_m_line == 0)
        {
            pen.setColor(Qt::black);      // solid line on every beat
            pen.setStyle(Qt::SolidLine);
        }
        else if (i % ticks_per_beat == 0)
        {
            pen.setColor(Qt::DashLine);
        }
        else
        {
            pen.setColor(Qt::lightGray);  // faint step lines
            pen.setStyle(Qt::DotLine);
            int i_snap = i - (i % m_snap);
            if (i == i_snap)
            {
                //                m_pen->setColor(Qt::darkGray);
            }
            else
            {
                //                m_pen->setColor(Qt::gray);
            }
        }

        pen.setStyle(Qt::SolidLine);      // draw vertical grid lines
        painter.setPen(pen);
        painter.drawLine(base_line, 0, base_line, keyAreaY);
    }

    pen.setColor(Qt::red);                // draw the playhead
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.drawLine(m_old_progress_x, 0, m_old_progress_x, height() * 8);
    m_old_progress_x = (m_seq.get_last_tick() / m_zoom + c_keyboard_padding_x);

    midipulse tick_s;                       // draw notes
    midipulse tick_f;
    int note;
    bool selected;
    int velocity;
    draw_type_t dt;
    int start_tick = 0;
    int end_tick = (width() * m_zoom);
    sequence * seq = nullptr;
    for (int method = 0; method < 2; ++method)
    {
        if (method == 0 && m_drawing_background_seq)
        {
            if (perf().is_active(m_background_sequence))
                seq = perf().get_sequence(m_background_sequence);
            else
                ++method;
        }
        else if (method == 0)
            ++method;

        if (method == 1)
            seq = &m_seq;

        pen.setColor(Qt::black);      /* draw boxes from sequence */
        pen.setStyle(Qt::SolidLine);
        seq->reset_draw_marker();
        while
        (
            (
                dt = seq->get_next_note_event
                (
                    tick_s, tick_f, note, selected, velocity
                )
            ) != DRAW_FIN
        )
        {
            if
            (
                (tick_s >= start_tick && tick_s <= end_tick) ||
                (
                    (dt == DRAW_NORMAL_LINKED) &&
                    (tick_f >= start_tick && tick_f <= end_tick)
                )
            )
            {
                note_x = tick_s / m_zoom + c_keyboard_padding_x;
                note_y = keyAreaY - (note * keyY) - keyY - 1 + 2;
                switch (editMode)
                {
                case EDIT_MODE_NOTE:
                    note_height = keyY - 3;
                    break;

                case EDIT_MODE_DRUM:
                    note_height = keyY;
                    break;
                }

                int in_shift = 0;
                int length_add = 0;
                if (dt == DRAW_NORMAL_LINKED)
                {
                    if (tick_f >= tick_s)
                    {
                        note_width = (tick_f - tick_s) / m_zoom;
                        if (note_width < 1)
                            note_width = 1;
                    }
                    else
                        note_width = (m_seq.get_length() - tick_s) / m_zoom;
                }
                else
                    note_width = 16 / m_zoom;

                if (dt == DRAW_NOTE_ON)
                {
                    in_shift = 0;
                    length_add = 2;
                }

                if (dt == DRAW_NOTE_OFF)
                {
                    in_shift = -1;
                    length_add = 1;
                }
                pen.setColor(Qt::black);
                if (method == 0)
                    pen.setColor(Qt::darkGray);

                brush.setStyle(Qt::SolidPattern);
                brush.setColor(Qt::black);
                painter.setBrush(brush);
                painter.setPen(pen);
                switch (editMode)
                {
                case EDIT_MODE_NOTE:

                    // Draw outer note boundary (shadow)

                    painter.drawRect(note_x, note_y, note_width, note_height);
                    if (tick_f < tick_s)    // shadow for notes  before zero
                    {
                        painter.setPen(pen);
                        painter.drawRect
                        (
                            c_keyboard_padding_x, note_y,
                            tick_f / m_zoom, note_height
                        );
                    }
                    break;

                case EDIT_MODE_DRUM:
                    QPointF points[4] =     // polygon for drum hits
                    {
                        QPointF(note_x - note_height * 0.5,
                                note_y + note_height * 0.5),
                        QPointF(note_x, note_y),
                        QPointF(note_x + note_height * 0.5,
                                note_y + note_height * 0.5),
                        QPointF(note_x, note_y + note_height)
                    };
                    painter.drawPolygon(points, 4);
                    break;
                }

                // Draw note highlight if there's room, always draw them in
                // drum mode

                if (note_width > 3 || editMode == EDIT_MODE_DRUM)
                {
                    // red noted selected, otherwise plain white
                    if (selected)
                        brush.setColor(Qt::red);
                    else
                        brush.setColor(Qt::white);

                    painter.setBrush(brush);
                    if (method == 1)
                    {
                        switch (editMode)
                        {
                        case EDIT_MODE_NOTE: // if the note fits in the grid
                            if (tick_f >= tick_s)
                            {
                                // draw inner note (highlight)
                                painter.drawRect
                                (
                                    note_x + in_shift, note_y,
                                    note_width - 1 + length_add, note_height - 1
                                );
                            }
                            else
                            {
                                painter.drawRect
                                (
                                    note_x + in_shift, note_y,
                                    note_width, note_height - 1
                                );
                                painter.drawRect
                                (
                                    c_keyboard_padding_x, note_y,
                                    (tick_f / m_zoom) - 3 + length_add,
                                    note_height - 1
                                );
                            }
                            break;

                        case EDIT_MODE_DRUM: // draw inner note (highlight)
                            QPointF points[4] =
                            {
                                QPointF(note_x - note_height * 0.5,
                                        note_y + note_height * 0.5),
                                QPointF(note_x, note_y),
                                QPointF(note_x + note_height * 0.5 - 1,
                                        note_y + note_height * 0.5),
                                QPointF(note_x, note_y + note_height - 1)
                            };
                            painter.drawPolygon(points, 4);
                            break;
                        }
                    }
                }
            }
        }
    }

    int x, y, w, h;                     /* draw selections  */
    brush.setStyle(Qt::NoBrush);      /* painter reset    */
    painter.setBrush(brush);
    if (m_selecting  ||  m_moving || m_paste ||  m_growing)
        pen.setStyle(Qt::SolidLine);

    if (m_selecting)
    {
        rect::xy_to_rect_get
        (
            m_drop_x, m_drop_y, m_current_x, m_current_y, x, y, w, h
        );

        m_old.set(x, y, w, h + keyY);
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, w, h + keyY);
    }

    if (m_moving || m_paste)
    {
        int delta_x = m_current_x - m_drop_x;
        int delta_y = m_current_y - m_drop_y;
        x = m_selected.x() + delta_x;
        y = m_selected.y() + delta_y;
        pen.setColor(Qt::black);
        painter.setPen(pen);
        switch (editMode)
        {
        case EDIT_MODE_NOTE:
            painter.drawRect
            (
                x + c_keyboard_padding_x, y,
                m_selected.width(), m_selected.height()
            );
            break;

        case EDIT_MODE_DRUM:
            painter.drawRect
            (
                x - note_height * 0.5 + c_keyboard_padding_x,
                y, m_selected.width() + note_height, m_selected.height()
            );
            break;
        }
        m_old.x(x);
        m_old.y(y);
        m_old.width(m_selected.width());
        m_old.height(m_selected.height());
    }

    if (m_growing)
    {
        int delta_x = m_current_x - m_drop_x;
        int width = delta_x + m_selected.width();
        if (width < 1)
            width = 1;

        x = m_selected.x();
        y = m_selected.y();

        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, width, m_selected.height());
        m_old.x(x);
        m_old.y(y);
        m_old.width(width);
        m_old.height(m_selected.height());
    }
}

/**
 *
 */

void
qseqroll::mousePressEvent (QMouseEvent * event)
{
    midipulse tick_s;
    midipulse tick_f;
    int note;
    int note_l;
    bool needs_update = false;
    int norm_x, norm_y, snapped_x, snapped_y;
    snapped_x = norm_x = event->x() - c_keyboard_padding_x;
    snapped_y = norm_y = event->y();
    snap_x(snapped_x);
    snap_y(snapped_y);
    m_current_y = m_drop_y = snapped_y;     /* y is always snapped */
    if (m_paste)
    {
        convert_xy(snapped_x, snapped_y, tick_s, note);
        m_paste = false;
        m_seq.push_undo();
        m_seq.paste_selected(tick_s, note);
        needs_update = true;
    }
    else
    {
        if (event->button() == Qt::LeftButton)
        {
            m_current_x = m_drop_x = norm_x; // for selection, use non-snapped x
            switch (editMode)                // convert screen coords to ticks
            {
            case EDIT_MODE_NOTE:
                convert_xy(m_drop_x, m_drop_y, tick_s, note);
                tick_f = tick_s;
                break;

            case EDIT_MODE_DRUM:            // padding for selecting drum hits
                convert_xy(m_drop_x - note_height * 0.5, m_drop_y, tick_s, note);
                convert_xy(m_drop_x + note_height * 0.5, m_drop_y, tick_f, note);
                break;
            }
            if (m_adding)   //painting new notes
            {
                m_painting = true;                      /* start paint job   */
                m_current_x = m_drop_x = snapped_x;     /* adding, snapped x */
                convert_xy(m_drop_x, m_drop_y, tick_s, note);

                // test if a note is already there, fake select, if so, no add

                if
                (
                    ! m_seq.select_note_events
                    (
                        tick_s, note, tick_s, note, sequence::e_would_select
                    )
                )
                {               /* add note, length = little less than snap */
                    m_seq.push_undo();
                    m_seq.add_note(tick_s, m_note_length - 2, note, true);
                    needs_update = true;
                }
            }
            else                // we're selecting
            {
                // if nothing's already selected in the range

                bool isSelected = false;
                switch (editMode)
                {
                case EDIT_MODE_NOTE:
                    isSelected = m_seq.select_note_events
                    (
                        tick_s, note, tick_f, note, sequence::e_is_selected
                    );
                    break;

                case EDIT_MODE_DRUM:
                    isSelected = m_seq.select_note_events
                    (
                        tick_s, note, tick_f, note, sequence::e_is_selected_onset
                    );
                    break;
                }
                if (! isSelected)
                {
                    int numsel = 0;
                    if (! (event->modifiers() & Qt::ControlModifier))
                        m_seq.unselect();

                    switch (editMode) /* on direct click select only 1 event */
                    {
                    case EDIT_MODE_NOTE:
                        numsel = m_seq.select_note_events
                        (
                            tick_s, note, tick_f, note,
                            sequence::e_select_one  // sequence::e_select_single
                        );
                        break;

                    case EDIT_MODE_DRUM:
                        numsel = m_seq.select_note_events
                        (
                            tick_s, note, tick_f, note,
                            sequence::e_select_one  // sequence::e_select_single
                        );
                        break;
                    }
                    if (numsel == 0)    /* none selected, start selection box */
                    {
                        if (event->button() == Qt::LeftButton)
                            m_selecting = true;
                    }
                    else
                    {
                        needs_update = true;
                    }
                }
                isSelected = false;
                switch (editMode)
                {
                case EDIT_MODE_NOTE:
                    isSelected = m_seq.select_note_events
                    (
                        tick_s, note, tick_f, note, sequence::e_is_selected
                    );
                    break;

                case EDIT_MODE_DRUM:
                    isSelected = m_seq.select_note_events
                    (
                        tick_s, note, tick_f, note, sequence::e_is_selected_onset
                    );
                    break;
                }

                if (isSelected)
                {
                    // moving - left click only
                    if
                    (
                        event->button() == Qt::LeftButton &&
                        ! (event->modifiers() & Qt::ControlModifier)
                    )
                    {
                        m_moving_init = true;
                        needs_update = true;
                        switch (editMode)
                        {
                        case EDIT_MODE_NOTE:           // acount note lengths
                            m_seq.get_selected_box
                            (
                                tick_s, note, tick_f, note_l
                            );
                            break;

                        case EDIT_MODE_DRUM:           // ignore note lengths
                            m_seq.get_onsets_selected_box
                            (
                                tick_s, note, tick_f, note_l
                            );
                            break;
                        }

                        convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note, note_l, m_selected
                        );

                        /* save offset that we get from the snap above */
                        int adjusted_selected_x = m_selected.x();
                        snap_x(adjusted_selected_x);
                        m_move_snap_offset_x =
                            m_selected.x() - adjusted_selected_x;

                        m_current_x = m_drop_x = snapped_x;
                    }

                    /* middle mouse button or left-ctrl click (2 button mice) */

                    if
                    (
                        (
                            event->button() == Qt::MiddleButton ||
                            (
                                event->button() == Qt::LeftButton &&
                                (event->modifiers() & Qt::ControlModifier)
                            )
                        )
                            && editMode == EDIT_MODE_NOTE)
                    {
                        m_growing = true;

                        /* get the box that selected elements are in */
                        m_seq.get_selected_box(tick_s, note, tick_f, note_l);
                        convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note, note_l, m_selected
                        );
                    }
                }
            }
        }
        if (event->button() == Qt::RightButton)
            set_adding(true);
    }
    if (needs_update)       // set seq dirty if something's changed
        m_seq.set_dirty();
}

/**
 *
 */

void
qseqroll::mouseReleaseEvent (QMouseEvent * event)
{
    midipulse tick_s;                   // start of tick window
    midipulse tick_f;                   // end of tick window
    int note_h;                         // highest note in window
    int note_l;                         // lowest note in window
    int x, y, w, h;                     // window dimensions
    bool needs_update = false;
    m_current_x = event->x() - c_keyboard_padding_x;
    m_current_y = event->y();
    snap_y(m_current_y);
    if (m_moving)
        snap_x(m_current_x);

    int delta_x = m_current_x - m_drop_x;
    int delta_y = m_current_y - m_drop_y;
    midipulse delta_tick;
    int delta_note;
    if (event->button() == Qt::LeftButton)
    {
        if (m_selecting)
        {
            rect::xy_to_rect_get
            (
                m_drop_x, m_drop_y, m_current_x, m_current_y, x, y, w, h
            );
            switch (editMode)
            {
            case EDIT_MODE_NOTE:
                convert_xy(x, y, tick_s, note_h);
                convert_xy(x + w, y + h, tick_f, note_l);
                m_seq.select_note_events
                (
                    tick_s, note_h, tick_f, note_l, sequence::e_select
                );
                break;

            case EDIT_MODE_DRUM:
                convert_xy(x, y, tick_s, note_h);
                convert_xy(x + w, y + h, tick_f, note_l);
                m_seq.select_note_events
                (
                    tick_s, note_h, tick_f, note_l, sequence::e_select_onset
                );
                break;
            }
            needs_update = true;
        }

        if (m_moving)
        {
            delta_x -= m_move_snap_offset_x;            /* adjust for snap */

            /* convert deltas into screen corridinates */

            convert_xy(delta_x, delta_y, delta_tick, delta_note);

            /*
             * since delta_note was from delta_y, it will be filpped
             * ( delta_y[0] = note[127], etc.,so we have to adjust
             */

            delta_note = delta_note - (c_num_keys - 1);
            m_seq.push_undo();
            m_seq.move_selected_notes(delta_tick, delta_note);
            needs_update = true;
        }
    }

    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton)
    {
        if (m_growing)
        {
            /* convert deltas into screen corridinates */

            convert_xy(delta_x, delta_y, delta_tick, delta_note);
            m_seq.push_undo();
            if (event->modifiers() & Qt::ShiftModifier)
                m_seq.stretch_selected(delta_tick);
            else
                m_seq.grow_selected(delta_tick);

            needs_update = true;
        }
    }

    if (event->button() == Qt::RightButton)
        set_adding(false);

    /* turn off */

    m_selecting = m_moving = m_growing = m_paste = m_moving_init =
        m_painting = false;

    m_seq.unpaint_all();
    if (needs_update)           /* if they clicked, something changed */
        m_seq.set_dirty();
}

/**
 *
 */

void
qseqroll::mouseMoveEvent (QMouseEvent * event)
{
    m_current_x = event->x() - c_keyboard_padding_x;
    m_current_y = event->y();
    if (m_moving_init)
    {
        m_moving_init = false;
        m_moving = true;
    }
    snap_y(m_current_y);

    int note;
    midipulse tick;
    convert_xy(0, m_current_y, tick, note);
    if (m_selecting || m_moving || m_growing || m_paste)
    {
        if (m_moving || m_paste)
            snap_x(m_current_x);
    }

    if (m_painting)
    {
        snap_x(m_current_x);
        convert_xy(m_current_x, m_current_y, tick, note);
        m_seq.add_note(tick, m_note_length - 2, note, true);
    }
}

/**
 *
 */

void
qseqroll::keyPressEvent (QKeyEvent * event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        // m_seq.push_undo();
        // m_seq.mark_selected();
        // m_seq.remove_marked();          // delete selected notes

        m_seq.remove_selected();
        return;
    }

    if (! perf().is_pattern_playing())
    {
        if (event->key() == Qt::Key_Home)
        {
            m_seq.set_last_tick(0);
            return;
        }
        if (event->key() == Qt::Key_Left)
        {
            m_seq.set_last_tick(m_seq.get_last_tick() - m_snap);
            return;
        }
        if (event->key() == Qt::Key_Right)
        {
            m_seq.set_last_tick(m_seq.get_last_tick() + m_snap);
            return;
        }
    }

    if (event->modifiers() & Qt::ControlModifier)   // Ctrl + ... events
    {
        switch (event->key())
        {
        case Qt::Key_X:
            // m_seq.push_undo();
            // m_seq.copy_selected();
            // m_seq.mark_selected();
            // m_seq.remove_marked();

            m_seq.cut_selected();
            return;
            break;

        case Qt::Key_C:
            m_seq.copy_selected();
            return;
            break;

        case Qt::Key_V:
            start_paste();
            return;
            break;

        case Qt::Key_Z:
            if (event->modifiers() & Qt::ShiftModifier)
            {
                m_seq.pop_redo();
                return;
            }
            else
                m_seq.pop_undo();
            return;
            break;

        case Qt::Key_A:
            m_seq.select_all();
            return;
            break;
        }
    }

    /*
     * If we reach this point, the key isn't relevant to us; ignore it so the
     * event is passed to the parent.
     */

    event->ignore();
}

/**
 *
 */

void
qseqroll::keyReleaseEvent (QKeyEvent *)
{
    // no code
}

/**
 *
 */

QSize
qseqroll::sizeHint () const
{
    return QSize
    (
        m_seq.get_length() / m_zoom + 100 + c_keyboard_padding_x, keyAreaY + 1
    );
}

/**
 *
 */

void
qseqroll::snap_y (int & y)
{
    y -= y % keyY;
}

/**
 *
 *  snap = number pulses to snap to
 *
 *  m_zoom = number of pulses per pixel
 *
 *  so snap / m_zoom  = number pixels to snap to
 */

void
qseqroll::snap_x (int & x)
{
    int mod = (m_snap / m_zoom);
    if (mod <= 0)
        mod = 1;

    x -= x % mod;
}

/**
 *
 */

void
qseqroll::convert_xy (int x, int y, midipulse & tick, int & note)
{
    tick = x * m_zoom;
    note = (keyAreaY - y - 2) / keyY;
}

/**
 *
 */

void
qseqroll::convert_tn (midipulse ticks, int note, int & x, int & y)
{
    x = ticks /  m_zoom;
    y = keyAreaY - ((note + 1) * keyY) - 1;
}

/**
 *  See seqroll::convert_sel_box_to_rect() for a potential upgrade.
 *
 * \param tick_s
 *      The starting tick of the rectangle.
 *
 * \param tick_f
 *      The finishing tick of the rectangle.
 *
 * \param note_h
 *      The high note of the rectangle.
 *
 * \param note_l
 *      The low note of the rectangle.
 *
 * \param [out] r
 *      The destination rectangle for the calculations.
 */

void
qseqroll::convert_tn_box_to_rect
(
    midipulse tick_s, midipulse tick_f, int note_h, int note_l,
    seq64::rect & r
)
{
    int x1, y1, x2, y2;
    convert_tn(tick_s, note_h, x1, y1);         /* convert box to X,Y values */
    convert_tn(tick_f, note_l, x2, y2);
    rect::xy_to_rect(x1, y1, x2, y2, r);
    r.height_incr(keyY);
}

/**
 *
 */

void
qseqroll::set_adding(bool adding)
{
    if (adding)
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

int
qseqroll::length () const
{
    return m_note_length;
}

/**
 *
 */

void
qseqroll::set_note_length (int length)
{
    m_note_length = length;
}

/**
 *
 */

void
qseqroll::set_snap (int snap)
{
    m_snap = snap;
}

/**
 *
 */

void
qseqroll::start_paste ()
{
    snap_x(m_current_x);
    snap_y(m_current_x);
    m_drop_x = m_current_x;
    m_drop_y = m_current_y;
    m_paste = true;

    midipulse tick_s;
    midipulse tick_f;
    int note_h;
    int note_l;

    /*
     *  Get the box that selected elements are in, then adjust for clipboard
     *  being shifted to tick 0.
     */

    m_seq.get_clipboard_box(tick_s, note_h, tick_f, note_l);
    convert_tn_box_to_rect(tick_s, tick_f, note_h, note_l, m_selected);
    m_selected.xy_incr(m_drop_x, m_drop_y - m_selected.y());
}

/**
 *
 */

void
qseqroll::zoom_in ()
{
    if (m_zoom > 1)
        m_zoom *= 0.5;
}

/**
 *
 */

void
qseqroll::zoom_out ()
{
    if (m_zoom < 32)
        m_zoom *= 2;
}

/**
 *
 */

void
qseqroll::updateEditMode (edit_mode_t mode)
{
    editMode = mode;
}

}           // namespace seq64

/*
 * qseqroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

