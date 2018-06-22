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
 * \updates       2018-06-22
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
#include "settings.hpp"                 /* seq64::usr().key_height()        */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *
 */

qseqroll::qseqroll
(
    perform & perf,
    sequence & seq,
    qseqkeys * seqkeys_wid,
    int zoom,
    int snap,
    int pos,
    QWidget * parent,
    edit_mode_t mode
) :
    QWidget                 (parent),
    qseqbase
    (
        perf, seq, zoom, snap,
        usr().key_height(),                         // keyY
        (usr().key_height() * c_num_keys + 1)       // keyAreaY
    ),
    m_seqkeys_wid           (seqkeys_wid),
    mTimer                  (nullptr),
    mFont                   (),
    m_scale                 (0),
    m_pos                   (0),
    m_key                   (0),
    m_note_length           (perf.ppqn() * 4 / 16),
    m_background_sequence   (0),
    m_drawing_background_seq (false),
    m_expanded_recording    (false),
    m_status                (0),
    m_cc                    (0),
    m_edit_mode             (mode),
    note_x                  (0),
    note_width              (0),
    note_y                  (0),
    note_height             (0),
    keyY                    (usr().key_height()),
    keyAreaY                (keyY * c_num_keys + 1)
{
    set_snap(seq.get_snap_tick());
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    mTimer = new QTimer(this);  // refresh timer to queue regular redraws !!!
    mTimer->setInterval(20);
    QObject::connect(mTimer, SIGNAL(timeout()), this, SLOT(update()));
    mTimer->start();
}

/**
 *  Draws the piano roll.
 */

void
qseqroll::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QBrush brush(Qt::NoBrush);
    mFont.setPointSize(6);

#ifdef SEQ64_SOLID_PIANOROLL_GRID
    bool fruity_lines = true;
    QPen pen(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);
#else
    bool fruity_lines = rc().interaction_method() == e_fruity_interaction;
    QPen pen(Qt::gray);
    pen.setStyle(Qt::DashLine);
#endif

    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(mFont);

    /*
     * Draw the border
     */

    painter.drawRect(0, 0, width(), height());

    pen.setColor(Qt::lightGray);
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);

    int octkey = SEQ64_OCTAVE_SIZE - m_key;         /* used three times     */
    for (int key = 1; key < c_num_keys; ++key)      /* for each note row    */
    {
        int remkeys = c_num_keys - key;             /* remaining keys?      */
        int modkey = remkeys - scroll_offset_key() + octkey;

        /*
         * Set line colour dependent on the note row we're on.
         */

        if (fruity_lines)
        {
            if ((modkey % SEQ64_OCTAVE_SIZE) == 0)
            {
                pen.setColor(Qt::darkGray);
                pen.setStyle(Qt::SolidLine);
                painter.setPen(pen);
            }
            else if ((modkey % SEQ64_OCTAVE_SIZE) == (SEQ64_OCTAVE_SIZE-1))
            {
#ifdef SEQ64_SOLID_PIANOROLL_GRID
                pen.setColor(Qt::lightGray);
                pen.setStyle(Qt::SolidLine);
                painter.setPen(pen);
#else
                pen.setColor(Qt::gray);
                pen.setStyle(Qt::DashLine);
                painter.setPen(pen);
#endif
            }
        }

        /*
         * Draw horizontal grid lines differently depending on editing mode.
         */

        switch (m_edit_mode)
        {
        case EDIT_MODE_NOTE:
            painter.drawLine(0, key * keyY, width(), key * keyY);
            break;

        case EDIT_MODE_DRUM:
            painter.drawLine
            (
                0, key * keyY - (0.5 * keyY), width(), key * keyY - (0.5 * keyY)
            );
            break;
        }

        int y = key * keyY; // c_key_y
        painter.drawLine(0, y, width(), y);
        if (m_scale != c_scale_off)
        {
            if (! c_scales_policy[m_scale][(modkey - 1) % SEQ64_OCTAVE_SIZE])
            {
                // painter.drawRect(0, key * keyY + 1, m_size_x, keyY - 1);
            }
        }
    }

    /*
     * The ticks_per_step value needs to be figured out.  Why 6 * m_zoom?  6
     * is the number of pixels in the smallest divisions in the default
     * seqroll background.
     *
     * This code needs to be put into a function.
     */

    int bpbar = seq().get_beats_per_bar();
    int bwidth = seq().get_beat_width();
    int ticks_per_beat = (4 * perf().ppqn()) / bwidth;
    int ticks_per_bar = bpbar * ticks_per_beat;
    int measures_per_line = zoom() * bwidth * bpbar * 2;
    if (measures_per_line <= 0)
        measures_per_line = 1;

    int ticks_per_step = 6 * zoom();
    int starttick = scroll_offset_ticks() -
            (scroll_offset_ticks() % ticks_per_step);
    int endtick = width() * zoom() + scroll_offset_ticks();

    pen.setColor(Qt::darkGray);                 // can we use Palette?
    painter.setPen(pen);

    /*
     * Draw vertical grid lines.  Incrementing by ticks_per_step only works for
     * PPQN of certain multiples.
     */

    for (int tick = starttick; tick < endtick; tick += ticks_per_step)
    {
        int x_offset = tick / zoom() + c_keyboard_padding_x - scroll_offset_x();
        pen.setWidth(1);
        if (tick % ticks_per_bar == 0)
        {
            pen.setColor(Qt::black);            // solid line on every beat
            pen.setStyle(Qt::SolidLine);
#ifdef SEQ64_SOLID_PIANOROLL_GRID
            pen.setWidth(2);                    // two pixels
#endif
        }
        else if (tick % ticks_per_beat == 0)
        {
            pen.setColor(Qt::darkGray);         // can we use Palette?
            pen.setStyle(Qt::SolidLine);        // pen.setColor(Qt::DashLine)
        }
        else
        {
            pen.setColor(Qt::lightGray);        // faint step lines
            pen.setStyle(Qt::DotLine);
            int tick_snap = tick - (tick % snap());

#ifdef SEQ64_SOLID_PIANOROLL_GRID
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
#else
            pen.setStyle(Qt::DashLine);         // Gdk::LINE_ON_OFF_DASH
            if (tick == tick_snap)
                pen.setColor(Qt::darkGray);
            else
                pen.setColor(Qt::lightGray);
#endif
        }
        painter.setPen(pen);
        painter.drawLine(x_offset, 0, x_offset, keyAreaY);
    }

#if ! defined SEQ64_SOLID_PIANOROLL_GRID
    pen.setStyle(Qt::SolidLine);
#endif

    pen.setColor(Qt::red);                      // draw the playhead
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.drawLine(old_progress_x(), 0, old_progress_x(), height() * 8);
    old_progress_x(seq().get_last_tick() / zoom() + c_keyboard_padding_x);

    midipulse tick_s;                               // draw notes
    midipulse tick_f;
    int note;
    bool selected;
    int velocity;
    draw_type_t dt;
    int start_tick = 0;
    int end_tick = width() * zoom();
    sequence * s = nullptr;
    for (int method = 0; method < 2; ++method)
    {
        if (method == 0 && m_drawing_background_seq)
        {
            if (perf().is_active(m_background_sequence))
                s = perf().get_sequence(m_background_sequence);
            else
                ++method;
        }
        else if (method == 0)
            ++method;

        if (method == 1)
            s = &seq();

        pen.setColor(Qt::black);      /* draw boxes from sequence */
        pen.setStyle(Qt::SolidLine);
        s->reset_draw_marker();
        while
        (
            (
                dt = s->get_next_note_event
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
                note_x = tick_s / zoom() + c_keyboard_padding_x;
                note_y = keyAreaY - (note * keyY) - keyY - 1 + 2;
                switch (m_edit_mode)
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
                        note_width = (tick_f - tick_s) / zoom();
                        if (note_width < 1)
                            note_width = 1;
                    }
                    else
                        note_width = (seq().get_length() - tick_s) / zoom();
                }
                else
                    note_width = 16 / zoom();

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
                pen.setColor(Qt::black);
                if (method == 0)
                {
                    length_add = 1;
                    pen.setColor(Qt::darkGray);
                }

                brush.setStyle(Qt::SolidPattern);
                brush.setColor(Qt::black);
                painter.setBrush(brush);
                painter.setPen(pen);
                switch (m_edit_mode)
                {
                case EDIT_MODE_NOTE:        // Draw outer note boundary (shadow)

                    painter.drawRect(note_x, note_y, note_width, note_height);
                    if (tick_f < tick_s)    // shadow for notes  before zero
                    {
                        painter.setPen(pen);
                        painter.drawRect
                        (
                            c_keyboard_padding_x, note_y,
                            tick_f / zoom(), note_height
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

                if (note_width > 3 || m_edit_mode == EDIT_MODE_DRUM)
                {
                    // red noted selected, otherwise plain white
                    if (selected)
                        brush.setColor(Qt::red);
                    else
                        brush.setColor(Qt::white);

                    painter.setBrush(brush);
                    if (method == 1)
                    {
                        switch (m_edit_mode)
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
                                    (tick_f / zoom()) - 3 + length_add,
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

    int x, y, w, h;                     /* draw selections              */
    brush.setStyle(Qt::NoBrush);        /* painter reset                */
    painter.setBrush(brush);
    if (select_action())                /* select/move/paste/grow       */
        pen.setStyle(Qt::SolidLine);

    if (selecting())
    {
        rect::xy_to_rect_get
        (
            drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
        );

        old_rect().set(x, y, w, h + keyY);
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, w, h + keyY);
    }

    if (drop_action())
    {
        int delta_x = current_x() - drop_x();
        int delta_y = current_y() - drop_y();
        x = selection().x() + delta_x;
        y = selection().y() + delta_y;
        pen.setColor(Qt::black);
        painter.setPen(pen);
        switch (m_edit_mode)
        {
        case EDIT_MODE_NOTE:
            painter.drawRect
            (
                x + c_keyboard_padding_x, y,
                selection().width(), selection().height()
            );
            break;

        case EDIT_MODE_DRUM:
            painter.drawRect
            (
                x - note_height * 0.5 + c_keyboard_padding_x,
                y, selection().width() + note_height, selection().height()
            );
            break;
        }
        old_rect().x(x);
        old_rect().y(y);
        old_rect().width(selection().width());
        old_rect().height(selection().height());
    }

    if (growing())
    {
        int delta_x = current_x() - drop_x();
        int width = delta_x + selection().width();
        if (width < 1)
            width = 1;

        x = selection().x();
        y = selection().y();

        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, width, selection().height());
        old_rect().x(x);
        old_rect().y(y);
        old_rect().width(width);
        old_rect().height(selection().height());
    }
}

#ifdef USE_SCROLLING_CODE    // not ready for this class

/**
 *  Sets the horizontal scroll value according to the current value of the
 *  horizontal scroll-bar.
 */

void
qseqroll::set_scroll_x ()
{
    scroll_offset_ticks(int(m_hadjust.get_value()));
    scroll_offset_x(scroll_offset_ticks() / zoom());
}

/**
 *  Sets the vertical scroll value according to the current value of the
 *  vertical scroll-bar.
 */

void
qseqroll::set_scroll_y ()
{
    scroll_offset_key(int(m_vadjust.get_value()));
    scroll_offset_y(scroll_offset_key() * c_key_y);
}

/**
 *  Change the horizontal scrolling offset and redraw.  Roughly similar to
 *  seqevent::change_horz().
 */

void
qseqroll::change_horz ()
{
    set_scroll_x();
    update_and_draw(true);
}

/**
 *  Change the vertical scrolling offset and redraw.
 */

void
qseqroll::change_vert ()
{
    set_scroll_y();
    update_and_draw(true);
}

#endif  // USE_SCROLLING_CODE

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
    current_y(snapped_y);
    drop_y(snapped_y);                  /* y is always snapped */
    if (paste())
    {
        convert_xy(snapped_x, snapped_y, tick_s, note);
        paste(false);
        seq().push_undo();
        seq().paste_selected(tick_s, note);
        needs_update = true;
    }
    else
    {
        if (event->button() == Qt::LeftButton)
        {
            current_x(norm_x);
            drop_x(norm_x);             // for selection, use non-snapped x
            switch (m_edit_mode)        // convert screen coords to ticks
            {
            case EDIT_MODE_NOTE:
                convert_xy(drop_x(), drop_y(), tick_s, note);
                tick_f = tick_s;
                break;

            case EDIT_MODE_DRUM:        // padding for selecting drum hits
                convert_xy(drop_x() - note_height * 0.5, drop_y(), tick_s, note);
                convert_xy(drop_x() + note_height * 0.5, drop_y(), tick_f, note);
                break;
            }
            if (adding())               // painting new notes
            {
                painting(true);         /* start paint job   */
                current_x(snapped_x);
                drop_x(snapped_x);      /* adding, snapped x */
                convert_xy(drop_x(), drop_y(), tick_s, note);

                // test if a note is already there, fake select, if so, no add

                if
                (
                    ! seq().select_note_events
                    (
                        tick_s, note, tick_s, note, sequence::e_would_select
                    )
                )
                {               /* add note, length = little less than snap */
                    seq().push_undo();
                    seq().add_note(tick_s, m_note_length - 2, note, true);
                    needs_update = true;
                }
            }
            else                // we're selecting
            {
                // if nothing's already selected in the range

                bool isSelected = false;
                switch (m_edit_mode)
                {
                case EDIT_MODE_NOTE:
                    isSelected = seq().select_note_events
                    (
                        tick_s, note, tick_f, note, sequence::e_is_selected
                    );
                    break;

                case EDIT_MODE_DRUM:
                    isSelected = seq().select_note_events
                    (
                        tick_s, note, tick_f, note, sequence::e_is_selected_onset
                    );
                    break;
                }
                if (! isSelected)
                {
                    int numsel = 0;
                    if (! (event->modifiers() & Qt::ControlModifier))
                        seq().unselect();

                    switch (m_edit_mode) /* on direct click select only 1 event */
                    {
                    case EDIT_MODE_NOTE:
                        numsel = seq().select_note_events
                        (
                            tick_s, note, tick_f, note,
                            sequence::e_select_one  // sequence::e_select_single
                        );
                        break;

                    case EDIT_MODE_DRUM:
                        numsel = seq().select_note_events
                        (
                            tick_s, note, tick_f, note,
                            sequence::e_select_one  // sequence::e_select_single
                        );
                        break;
                    }
                    if (numsel == 0)    /* none selected, start selection box */
                    {
                        if (event->button() == Qt::LeftButton)
                            selecting(true);
                    }
                    else
                    {
                        needs_update = true;
                    }
                }
                isSelected = false;
                switch (m_edit_mode)
                {
                case EDIT_MODE_NOTE:
                    isSelected = seq().select_note_events
                    (
                        tick_s, note, tick_f, note, sequence::e_is_selected
                    );
                    break;

                case EDIT_MODE_DRUM:
                    isSelected = seq().select_note_events
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
                        moving_init(true);
                        needs_update = true;
                        switch (m_edit_mode)
                        {
                        case EDIT_MODE_NOTE:           // acount note lengths
                            seq().get_selected_box
                            (
                                tick_s, note, tick_f, note_l
                            );
                            break;

                        case EDIT_MODE_DRUM:           // ignore note lengths
                            seq().get_onsets_selected_box
                            (
                                tick_s, note, tick_f, note_l
                            );
                            break;
                        }

                        convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note, note_l, selection()
                        );

                        /* save offset that we get from the snap above */
                        int adjusted_selected_x = selection().x();
                        snap_x(adjusted_selected_x);
                        move_snap_offset_x(selection().x() - adjusted_selected_x);
                        current_x(snapped_x);
                        drop_x(snapped_x);
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
                            && m_edit_mode == EDIT_MODE_NOTE)
                    {
                        growing(true);

                        /* get the box that selected elements are in */

                        seq().get_selected_box(tick_s, note, tick_f, note_l);
                        convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note, note_l, selection()
                        );
                    }
                }
            }
        }
        if (event->button() == Qt::RightButton)
            set_adding(true);
    }
    if (needs_update)       // set seq dirty if something's changed
        seq().set_dirty();
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
    current_x(event->x() - c_keyboard_padding_x);
    current_y(event->y());
    snap_current_y();
    if (moving())
        snap_current_x();

    int delta_x = current_x() - drop_x();
    int delta_y = current_y() - drop_y();
    midipulse delta_tick;
    int delta_note;
    if (event->button() == Qt::LeftButton)
    {
        if (selecting())
        {
            rect::xy_to_rect_get
            (
                drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
            );
            switch (m_edit_mode)
            {
            case EDIT_MODE_NOTE:
                convert_xy(x, y, tick_s, note_h);
                convert_xy(x + w, y + h, tick_f, note_l);
                seq().select_note_events
                (
                    tick_s, note_h, tick_f, note_l, sequence::e_select
                );
                break;

            case EDIT_MODE_DRUM:
                convert_xy(x, y, tick_s, note_h);
                convert_xy(x + w, y + h, tick_f, note_l);
                seq().select_note_events
                (
                    tick_s, note_h, tick_f, note_l, sequence::e_select_onset
                );
                break;
            }
            needs_update = true;
        }

        if (moving())
        {
            delta_x -= move_snap_offset_x();            /* adjust for snap */

            /* convert deltas into screen corridinates */

            convert_xy(delta_x, delta_y, delta_tick, delta_note);

            /*
             * since delta_note was from delta_y, it will be filpped
             * ( delta_y[0] = note[127], etc.,so we have to adjust
             */

            delta_note = delta_note - (c_num_keys - 1);
            seq().push_undo();
            seq().move_selected_notes(delta_tick, delta_note);
            needs_update = true;
        }
    }

    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton)
    {
        if (growing())
        {
            /* convert deltas into screen corridinates */

            convert_xy(delta_x, delta_y, delta_tick, delta_note);
            seq().push_undo();
            if (event->modifiers() & Qt::ShiftModifier)
                seq().stretch_selected(delta_tick);
            else
                seq().grow_selected(delta_tick);

            needs_update = true;
        }
    }

    if (event->button() == Qt::RightButton)
        set_adding(false);

    clear_action_flags();               /* turn off */
    seq().unpaint_all();
    if (needs_update)                   /* if clicked, something changed */
        seq().set_dirty();
}

/**
 *
 */

void
qseqroll::mouseMoveEvent (QMouseEvent * event)
{
    current_x(event->x() - c_keyboard_padding_x);
    current_y(event->y());
    if (moving_init())
    {
        moving_init(false);
        moving(true);
    }
    snap_current_y();

    int note;
    midipulse tick;
    convert_xy(0, current_y(), tick, note);
    if (select_action())
    {
        if (drop_action())
            snap_current_x();
    }

    if (painting())
    {
        snap_current_x();
        convert_xy(current_x(), current_y(), tick, note);
        seq().add_note(tick, m_note_length - 2, note, true);
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
        seq().remove_selected();
        return;
    }

    // TODO: get these working and fix the 1:1 zoom in combo-dropdown.

    if (! perf().is_pattern_playing())
    {
        if (event->key() == Qt::Key_Home)
        {
            seq().set_last_tick(0);
            return;
        }
        if (event->key() == Qt::Key_Left)
        {
            seq().set_last_tick(seq().get_last_tick() - snap());
            return;
        }
        if (event->key() == Qt::Key_Right)
        {
            seq().set_last_tick(seq().get_last_tick() + snap());
            return;
        }
        if (event->modifiers() & Qt::ShiftModifier) // Shift + ... events
        {
            if (event->key() == Qt::Key_Z)
                zoom_in();
        }
        else
        {
            if (event->key() == Qt::Key_Z)
                zoom_out();
        }
    }

    if (event->modifiers() & Qt::ControlModifier)   // Ctrl + ... events
    {
        switch (event->key())
        {
        case Qt::Key_X:

            seq().cut_selected();
            return;
            break;

        case Qt::Key_C:
            seq().copy_selected();
            return;
            break;

        case Qt::Key_V:
            start_paste();
            return;
            break;

        case Qt::Key_Z:
            if (event->modifiers() & Qt::ShiftModifier)
            {
                seq().pop_redo();
                return;
            }
            else
                seq().pop_undo();
            return;
            break;

        case Qt::Key_A:
            seq().select_all();
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
        seq().get_length() / zoom() + 100 + c_keyboard_padding_x, keyAreaY + 1
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
 *  Provides an override to change the mouse "cursor".
 *
 * \param a
 *      The value of the status of adding (e.g. a note).
 */

void
qseqroll::set_adding (bool a)
{
    qseqbase::set_adding(a);
    if (a)
        setCursor(Qt::PointingHandCursor);
    else
        setCursor(Qt::ArrowCursor);
}

/**
 *
 */

void
qseqroll::start_paste ()
{
    snap_current_x();
    snap_current_y();
    drop_x(current_x());
    drop_y(current_y());
    paste(true);

    midipulse tick_s;
    midipulse tick_f;
    int note_h;
    int note_l;

    /*
     *  Get the box that selected elements are in, then adjust for clipboard
     *  being shifted to tick 0.
     */

    seq().get_clipboard_box(tick_s, note_h, tick_f, note_l);
    convert_tn_box_to_rect(tick_s, tick_f, note_h, note_l, selection());
    selection().xy_incr(drop_x(), drop_y() - selection().y());
}

/**
 *
 */

void
qseqroll::update_edit_mode (edit_mode_t mode)
{
    m_edit_mode = mode;
}

}           // namespace seq64

/*
 * qseqroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

