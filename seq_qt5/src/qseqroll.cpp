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
 *  roll of the patterns editor for the Qt 5 implementation.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-08-05
 * \license       GNU GPLv2 or above
 *
 *  Please see the additional notes for the Gtkmm-2.4 version of this panel,
 *  seqroll.
 */

#include <QFrame>                       /* base class for seqedit frame(s)  */
#include <QApplication>                 /* QApplication keyboardModifiers() */
#include <QScrollBar>                   /* needed by qscrollmaster          */

#include "perform.hpp"
#include "qseqeditframe.hpp"            /* seq64::qseqeditframe legacy      */
#include "qseqeditframe64.hpp"          /* seq64::qseqeditframe64 class     */
#include "qseqframe.hpp"                /* interface class for seqedits     */
#include "qseqroll.hpp"                 /* seq64::qseqroll class            */
#include "settings.hpp"                 /* seq64::usr().key_height(), etc.  */

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
    perform & p,
    sequence & seq,
    qseqkeys * seqkeys_wid,
    int zoom,
    int snap,
    int ppqn,
    int pos,
    edit_mode_t mode,
    qseqframe * frame
) :
    QWidget                 (frame),
    qseqbase
    (
        p, seq, zoom, snap, ppqn,
        usr().key_height(),                         // m_key_y
        (usr().key_height() * c_num_keys + 1)       // m_keyarea_y
    ),
    m_parent_frame          (frame),
    m_is_new_edit_frame
    (
        not_nullptr(dynamic_cast<qseqeditframe64 *>(m_parent_frame))
    ),
    m_seqkeys_wid           (seqkeys_wid),
    m_timer                 (nullptr),
    mFont                   (),
    m_scale                 (0),
    m_pos                   (0),
#ifdef SEQ64_STAZED_CHORD_GENERATOR
    m_chord                 (0),
#endif
    m_key                   (0),
    m_note_length           (p.get_ppqn() * 4 / 16),
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
    m_key_y                 (usr().key_height()),
    m_keyarea_y             (m_key_y * c_num_keys + 1)
{
    set_snap(seq.get_snap_tick());
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    show();
    m_timer = new QTimer(this);                          // redraw timer !!!
    m_timer->setInterval(usr().window_redraw_rate());    // 20
    QObject::connect
    (
        m_timer, SIGNAL(timeout()), this, SLOT(conditional_update())
    );
    m_timer->start();
}

/**
 *  Zooms in, first calling the base-class version of this function, then
 *  passing along the message to the parent edit frame, so that it can change
 *  the zoom on the other panels of the parent edit frame.
 */

void
qseqroll::zoom_in ()
{
    qseqbase::zoom_in();
    m_parent_frame->set_zoom(zoom());      // m_parent_frame->zoom_in();
}

/**
 *  Zooms out, first calling the base-class version of this function, then
 *  passing along the message to the parent edit frame, so that it can change
 *  the zoom on the other panels of the parent edit frame.
 */

void
qseqroll::zoom_out ()
{
    qseqbase::zoom_out();
    m_parent_frame->set_zoom(zoom());      // m_parent_frame->zoom_out();
}

/**
 *  Tells the parent frame to reset our zoom.
 */

void
qseqroll::reset_zoom ()
{
    m_parent_frame->reset_zoom();
}

/**
 *  This function sets the given sequence onto the piano roll of the pattern
 *  editor, so that the musician can have another pattern to play against.
 *  The state parameter sets the boolean m_drawing_background_seq.
 *
 * \param state
 *      If true, the background sequence will be drawn.
 *
 * \param seq
 *      Provides the sequence number, which is checked against the
 *      SEQ64_IS_LEGAL_SEQUENCE() macro before being used.  This macro allows
 *      the value SEQ64_SEQUENCE_LIMIT, which disables the background
 *      sequence.
 */

void
qseqroll::set_background_sequence (bool state, int seq)
{
    m_drawing_background_seq = state;
    if (SEQ64_IS_LEGAL_SEQUENCE(seq))
        m_background_sequence = seq;

    set_dirty();                        // update_and_draw();
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::needs_update().
 */

void
qseqroll::conditional_update ()
{
    if (needs_update())
    {
#ifdef SEQ64_FOLLOW_PROGRESS_BAR
        bool expandrec = seq().expand_recording();
        if (expandrec)
        {
            set_measures(get_measures() + 1);
            follow_progress();
        }
        else if (perf().follow_progress())
            follow_progress();              /* keep up with progress    */
#endif
        update();
    }
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
    pen.setStyle(Qt::DotLine);
#endif

    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(mFont);

    /*
     * Draw the border.  In later usage, the width() function [and height() as
     * well?], returns a humongous value (38800+).  So we store the current
     * values to use, via window_width() and window_height(), in
     * follow_progress().
     */

    int ww = width();
    int wh = height();
    painter.drawRect(0, 0, ww, wh);
    pen.setColor(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);                    // pen.setStyle(Qt::DotLine)
    painter.setPen(pen);

    int octkey = SEQ64_OCTAVE_SIZE - m_key;         /* used three times     */
    for (int key = 1; key <= c_num_keys; ++key)     /* for each note row    */
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
                pen.setStyle(Qt::DotLine);
                painter.setPen(pen);
#endif
            }
        }

        /*
         * Draw horizontal grid lines differently depending on editing mode.
         */

        int y = key * m_key_y;
        if (m_edit_mode == EDIT_MODE_DRUM)
            y -= (0.5 * m_key_y);

        painter.drawLine(0, y, ww, y);
        if (m_scale != c_scale_off)
        {
            if (! c_scales_policy[m_scale][(modkey - 1) % SEQ64_OCTAVE_SIZE])
            {
                pen.setColor(Qt::lightGray);
                brush.setColor(Qt::lightGray);
                brush.setStyle(Qt::SolidPattern);
                painter.setBrush(brush);
                painter.setPen(pen);
                painter.drawRect(0, y + 1, ww, m_key_y - 1);
            }
        }
    }

    /*
     * The ticks_per_step value needs to be figured out.  Why 6 * zoom()?  6
     * is the number of pixels in the smallest divisions in the default
     * seqroll background.
     *
     * This code needs to be put into a function.
     */

    int bpbar = seq().get_beats_per_bar();
    int bwidth = seq().get_beat_width();
    midipulse ticks_per_beat = (4 * perf().get_ppqn()) / bwidth;
    midipulse ticks_per_bar = bpbar * ticks_per_beat;
    midipulse ticks_per_step = 6 * zoom();
    midipulse starttick = scroll_offset_ticks() -
            (scroll_offset_ticks() % ticks_per_step);
    midipulse endtick = ww * zoom() + scroll_offset_ticks();

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
        if (tick % ticks_per_bar == 0)          /* solid line on every beat */
        {
            pen.setColor(Qt::black);
            pen.setStyle(Qt::SolidLine);
#ifdef SEQ64_SOLID_PIANOROLL_GRID
            pen.setWidth(2);                    /* two pixels               */
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
                pen.setStyle(Qt::DotLine);     // Gdk::LINE_ON_OFF_DASH
                pen.setColor(Qt::lightGray);    // faint step lines
            }
#else
            pen.setStyle(Qt::DotLine);         // Gdk::LINE_ON_OFF_DASH
            if (tick == tick_snap)
                pen.setColor(Qt::darkGray);
            else
                pen.setColor(Qt::lightGray);
#endif
        }
        painter.setPen(pen);
        painter.drawLine(x_offset, 0, x_offset, m_keyarea_y);
    }
    pen.setWidth(1);

#if ! defined SEQ64_SOLID_PIANOROLL_GRID
    pen.setStyle(Qt::SolidLine);
#endif

    /*
     * draw_progress_on_window():
     *
     *  Draw a progress line on the window.  This is done by first blanking out
     *  the line with the background, which contains white space and grey lines,
     *  using the the draw_drawable function.  Remember that we wrap the
     *  draw_drawable() function so it's parameters are xsrc, ysrc, xdest, ydest,
     *  width, and height.
     *
     *  Note that the progress-bar position is based on the
     *  sequence::get_last_tick() value, the current zoom, and the current
     *  scroll-offset x value.
     */

    static bool s_loop_in_progress = false;     /* indicates when to reset  */
    int prog_x = old_progress_x();
    pen.setColor(Qt::red);                      // draw the playhead
    pen.setStyle(Qt::SolidLine);

    /*
     * If this test is used, then when not running, the overwrite
     * functionality of recording will not work: if (perf().is_running())
     */

    if (usr().progress_bar_thick())
        pen.setWidth(2);
    else
        pen.setWidth(1);

    painter.setPen(pen);
    painter.drawLine(prog_x, 0, prog_x, wh * 8);    // why * 8?
    old_progress_x(seq().get_last_tick() / zoom() + c_keyboard_padding_x);

    if (old_progress_x() > c_keyboard_padding_x)
    {
        s_loop_in_progress = true;
    }
    else
    {
        if (s_loop_in_progress)
        {
            seq().loop_reset(true);             /* for overwrite recording  */
            s_loop_in_progress = false;
        }
    }

    /*
     * End of draw_progress_on_window()
     */

    midipulse tick_s;                               // draw notes
    midipulse tick_f;
    int note;
    bool selected;
    int velocity;
    draw_type_t dt;
    int start_tick = 0;
    int end_tick = ww * zoom();
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
        pen.setWidth(1);
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
                note_y = m_keyarea_y - (note * m_key_y) - m_key_y - 1 + 2;
                switch (m_edit_mode)
                {
                case EDIT_MODE_NOTE:
                    note_height = m_key_y - 3;
                    break;

                case EDIT_MODE_DRUM:
                    note_height = m_key_y;
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
                if (method == 0)                    // draw background note
                {
                    length_add = 1;
                    pen.setColor(Qt::darkCyan);     // note border color
                    brush.setColor(Qt::darkCyan);
                }
                else
                {
                    pen.setColor(Qt::black);        // note border color
                    brush.setColor(Qt::black);
                }

                brush.setStyle(Qt::SolidPattern);
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

                /*
                 * Draw note highlight if there's room; always draw them in
                 * drum mode.  Orange noted if selected, red if drum mode,
                 * otherwise plain white.
                 */

                if (note_width > 3 || m_edit_mode == EDIT_MODE_DRUM)
                {
                    if (selected)
                        brush.setColor("orange");         // Qt::red
                    else if (m_edit_mode == EDIT_MODE_DRUM)
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

        old_rect().set(x, y, w, h + m_key_y);
        pen.setColor("orange");         /*  pen.setColor(Qt::black);    */
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, w, h + m_key_y);
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

/**
 *
 */

void
qseqroll::mousePressEvent (QMouseEvent * event)
{
    midipulse tick_s, tick_f;
    int note, note_l, norm_x, norm_y, snapped_x, snapped_y;
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
        set_dirty();
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

                /*
                 * Test if a note is already there, fake select, if so, don't
                 * add, else add a note, length = little less than snap.
                 */

                if
                (
                    ! seq().select_note_events
                    (
                        tick_s, note, tick_s, note, sequence::e_would_select
                    )
                )
                {
                    seq().push_undo();
                    seq().add_note(tick_s, m_note_length - 2, note, true);
                    set_dirty();
                }
            }
            else                            /* we're selecting anew         */
            {
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

                    switch (m_edit_mode)    /* direct click; select 1 event */
                    {
                    case EDIT_MODE_NOTE:
                        numsel = seq().select_note_events
                        (
                            tick_s, note, tick_f, note,
                            sequence::e_select_one
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
                        set_dirty();
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
                    if                          /* moving - left click only */
                    (
                        event->button() == Qt::LeftButton &&
                        ! (event->modifiers() & Qt::ControlModifier)
                    )
                    {
                        moving_init(true);
                        set_dirty();
                        switch (m_edit_mode)
                        {
                        case EDIT_MODE_NOTE:

                            seq().get_selected_box      /* use note lengths */
                            (
                                tick_s, note, tick_f, note_l
                            );
                            break;

                        case EDIT_MODE_DRUM:           /* ignore them       */

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
                    if          /* Middle mouse button or left-ctrl click   */
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
    if (is_dirty())                                 /* something changed?   */
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
            set_dirty();
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
            set_dirty();
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

            set_dirty();
        }
    }

    if (event->button() == Qt::RightButton)
    {
        if (! QApplication::queryKeyboardModifiers().testFlag(Qt::MetaModifier))
        {
            set_adding(false);
            set_dirty();
        }
    }

    clear_action_flags();               /* turn off */
    seq().unpaint_all();
    if (is_dirty())                   /* if clicked, something changed */
        seq().set_dirty();
}

/**
 *  Handles a mouse movement, including selection and note-painting.
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
 *  Handles keystrokes for note movement, zoom, and more.
 */

void
qseqroll::keyPressEvent (QKeyEvent * event)
{
    bool dirty = false;
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        seq().remove_selected();
        dirty = true;
    }
    else
    {
        // TODO: get these working and fix the 1:1 zoom in combo-dropdown.

        if (! perf().is_pattern_playing())
        {
            if (event->key() == Qt::Key_Home)
            {
                seq().set_last_tick(0);
                dirty = true;
            }
            else if (event->key() == Qt::Key_Left)
            {
                seq().set_last_tick(seq().get_last_tick() - snap());
                dirty = true;
            }
            else if (event->key() == Qt::Key_Right)
            {
                seq().set_last_tick(seq().get_last_tick() + snap());
                dirty = true;
            }
            else if (event->modifiers() & Qt::ShiftModifier) // Shift + ...
            {
                if (event->key() == Qt::Key_Z)
                {
                    zoom_in();
                    dirty = true;
                }
            }
            else
            {
                if (event->key() == Qt::Key_Z)
                {
                    zoom_out();
                    dirty = true;
                }
                else if (event->key() == Qt::Key_0)
                {
                    reset_zoom();
                    dirty = true;
                }
            }
        }
        if (! dirty && event->modifiers() & Qt::ControlModifier) // Ctrl + ...
        {
            switch (event->key())
            {
            case Qt::Key_X:

                seq().cut_selected();
                dirty = true;
                break;

            case Qt::Key_C:

                seq().copy_selected();
                dirty = true;
                break;

            case Qt::Key_V:

                start_paste();
                dirty = true;
                break;

            case Qt::Key_Z:

                if (event->modifiers() & Qt::ShiftModifier)
                {
                    seq().pop_redo();
                    dirty = true;
                }
                else
                    seq().pop_undo();

                dirty = true;
                break;

            case Qt::Key_A:

                seq().select_all();
                dirty = true;
                break;
            }
        } else
        {
            if
            (
                (event->modifiers() & Qt::ShiftModifier) == 0 &&
                (event->modifiers() & Qt::MetaModifier) == 0
            )
            {
                switch (event->key())
                {
                case Qt::Key_P:

                    set_adding(true);
                    dirty = true;
                    break;

                case Qt::Key_X:

                    set_adding(false);
                    dirty = true;
                    break;
                }
            }
        }
    }

    /*
     * If we reach this point, the key isn't relevant to us; ignore it so the
     * event is passed to the parent.
     */

    if (dirty)
        set_dirty();
    else
        QWidget::keyPressEvent(event);  // event->ignore();
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
 *  Provides the base sizing of the piano roll.  If less than the width of the
 *  parent frame, it is increased to that, so that the roll covers the whole
 *  scrolling area (in qseqeditframe).
 */

QSize
qseqroll::sizeHint () const
{
    int h = m_keyarea_y + 1;
    int w = m_parent_frame->width();
    int z = zoom();
    int len = int(seq().get_length()) / z;
    if (len < w)
        len = w;

    len += c_keyboard_padding_x;
    return QSize(len, h);
}

/**
 *  Snaps the y pixel to the height of a piano key.
 *
 * \param [in,out] y
 *      The vertical pixel value to be snapped.
 */

void
qseqroll::snap_y (int & y)
{
    y -= y % m_key_y;
}

/**
 *  Provides an override to change the mouse "cursor" based on whether adding
 *  notes is active, or not.
 *
 * \param a
 *      The value of the status of adding (e.g. a note).
 */

void
qseqroll::set_adding (bool a)
{
    qseqbase::set_adding(a);
    if (a)
        setCursor(Qt::PointingHandCursor);  // Qt::CrossCursor ?
    else
        setCursor(Qt::ArrowCursor);

    set_dirty();
}

/**
 *  The current (x, y) drop points are snapped, and the pasting flag is set to
 *  true.  Then this function
 *  Gets the box that selected elements are in, then adjusts for the clipboard
 *  being shifted to tick 0.
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

    midipulse tick_s, tick_f;
    int note_h, note_l;
    seq().get_clipboard_box(tick_s, note_h, tick_f, note_l);
    convert_tn_box_to_rect(tick_s, tick_f, note_h, note_l, selection());
    selection().xy_incr(drop_x(), drop_y() - selection().y());
}

/**
 *  Sets the drum/note mode status.
 *
 * \param mode
 *      The drum or note mode status.
 */

void
qseqroll::update_edit_mode (edit_mode_t mode)
{
    m_edit_mode = mode;
}

#ifdef SEQ64_STAZED_CHORD_GENERATOR

/**
 *  Sets the current chord to the given value.
 *
 * \param chord
 *      The desired chord value.
 */

void
qseqroll::set_chord (int chord)
{
    if (m_chord != chord)
        m_chord = chord;
}

#endif  // SEQ64_STAZED_CHORD_GENERATOR

/**
 *
 */

void
qseqroll::set_key (int key)
{
    if (m_key != key)
        m_key = key;
}

/**
 *
 */

void
qseqroll::set_scale (int scale)
{
    if (m_scale != scale)
        m_scale = scale;
}


/**
 *  Checks the position of the tick, and, if it is in a different piano-roll
 *  "page" than the last page, moves the page to the next page.
 *
 *  We don't want to do any of this if the length of the sequence fits in the
 *  window, but for now it doesn't hurt; the progress bar just never meets the
 *  criterion for moving to the next page.
 *
 *  This feature is not provided by qseqeditframe; it requires
 *  qseqeditframe64.
 *
 * \todo
 *      -   If playback is disabled (such as by a trigger), then do not update
 *          the page;
 *      -   When it comes back, make sure we're on the correct page;
 *      -   When it stops, put the window back to the beginning, even if the
 *          beginning is not defined as "0".
 */

void
qseqroll::follow_progress ()
{
#ifdef SEQ64_FOLLOW_PROGRESS_BAR
    if (not_nullptr(m_parent_frame) && m_is_new_edit_frame)
    {
        reinterpret_cast<qseqeditframe64 *>(m_parent_frame)->follow_progress();
    }
#endif
}

}           // namespace seq64

/*
 * qseqroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

