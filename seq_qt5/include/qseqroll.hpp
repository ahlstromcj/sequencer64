#ifndef SEQ64_QSEQROLL_HPP
#define SEQ64_QSEQROLL_HPP

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
 * \file          qseqroll.hpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-06-19
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 */

#include <QWidget>
#include <QPainter>
#include <QPen>
#include <QTimer>
#include <QMouseEvent>

#include "rect.hpp"
#include "sequence.hpp"                 /* seq64::edit_mode_t mode      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class qseqkeys;

/**
 * The MIDI note grid in the sequence editor
 */

class qseqroll : public QWidget
{
    Q_OBJECT

    friend class qseqeditframe;
    friend class qseqeditframe64;

public:

    explicit qseqroll
    (
        perform & perf,
        sequence & seq,
        qseqkeys * seqkeys_wid  = nullptr,
        int zoom                =  1,
        int snap                = 16,
        int pos                 =  0,
        QWidget * parent        = nullptr,
        seq64::edit_mode_t mode = EDIT_MODE_NOTE
    );

    void zoom_in();
    void zoom_out();

protected:

    /**
     * \getter m_note_length
     */

    int get_note_length () const
    {
        return m_note_length;
    }

    /**
     * \setter m_note_length
     */

    void set_note_length (int len)
    {
        m_note_length = len;
    }

    /**
     * \setter m_snap
     */

    void set_snap (int snap)
    {
        m_snap = snap;
    }

protected:

    // override painting event to draw on the frame

    void paintEvent (QPaintEvent *);

    // override mouse events for interaction

    void mousePressEvent (QMouseEvent * event);
    void mouseReleaseEvent (QMouseEvent * event);
    void mouseMoveEvent (QMouseEvent * event);

    // override keyboard events for interaction

    void keyPressEvent (QKeyEvent * event);
    void keyReleaseEvent (QKeyEvent * event);

    // override the sizehint to set our own defaults

    QSize sizeHint () const;

private:

    void snap_y (int & y);
    void snap_x (int & x);

    /* takes screen corrdinates, give us notes and ticks */

    void convert_xy (int x, int y, midipulse & ticks, int & note);
    void convert_tn (midipulse ticks, int note, int & x, int & a_y);
    void convert_tn_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int note_h, int note_l,
        seq64::rect & r
    );

    void set_adding (bool a_adding);
    void start_paste();

    perform & perf ()
    {
        return m_perform;
    }

private:

    /**
     *  Provides a reference to the performance object.
     */

    perform & m_perform;

    /**
     *  Provides a reference to the sequence represented by piano roll.
     */

    sequence & m_seq;

    /**
     *  The previous selection rectangle, used for undrawing it.
     */

    seq64::rect m_old;

    /**
     *  Used in moving and pasting notes.
     */

    seq64::rect m_selected;

    /**
     *  Holds a pointer to the qseqkeys pane that is associated with the
     *  qseqroll piano roll.
     */

    qseqkeys * m_seqkeys_wid;

    /**
     *  Screen update timer.
     */

    QTimer * mTimer;

    /**
     *  Main font for the piano roll.
     */

    QFont mFont;

    /**
     *  Indicates the musical scale in force for this sequence.
     */

    int m_scale;

    /**
     *  A position value.  Need to clarify what exactly this member is used
     *  for.
     */

    int m_pos;

    /**
     *
     */

    int m_key;

    /**
     *  Zoom setting, means that one pixel == m_zoom ticks.
     */

    int m_zoom;

    /**
     *  The grid-snap setting for the piano roll grid.  Same meaning as for the
     *  event-bar grid.  This value is the denominator of the note size used
     *  for the snap.
     */

    int m_snap;

    /**
     *  Holds the note length in force for this sequence.  Used in the
     *  seq24seqroll module only.
     */

    int m_note_length;

    /**
     *  Set when highlighting a bunch of events.
     */

    bool m_selecting;

    /**
     *  Set when in note-adding mode.  This flag was moved from both
     *  the fruity and the seq24 seqroll classes.
     */

    bool m_adding;

    /**
     *  Set when moving a bunch of events.
     */

    bool m_moving;

    /**
     *  Indicates the beginning of moving some events.  Used in the fruity and
     *  seq24 mouse-handling modules.
     */

    bool m_moving_init;

    /**
     *  Indicates that the notes are to be extended or reduced in length.
     */

    bool m_growing;

    /**
     *  Indicates the painting of events.  Used in the fruity and seq24
     *  mouse-handling modules.
     */

    bool m_painting;

    /**
     *  Indicates that we are in the process of pasting notes.
     */

    bool m_paste;

    /**
     *  Indicates the drag-pasting of events.  Used in the fruity
     *  mouse-handling module.
     */

    bool m_is_drag_pasting;

    /**
     *  Indicates the drag-pasting of events.  Used in the fruity
     *  mouse-handling module.
     */

    bool m_is_drag_pasting_start;

    /**
     *  Indicates the selection of one event.  Used in the fruity
     *  mouse-handling module.
     */

    bool m_justselected_one;

    /**
     *  The x size of the window.  Would be good to allocate this
     *  to a base class for all grid panels.  In Qt 5, this is the width().
     */

    int m_window_x;

    /**
     *  The y size of the window.  Would be good to allocate this
     *  to a base class for all grid panels.  In Qt 5, this is the height().
     */

    int m_window_y;

    /**
     *  The x location of the mouse when dropped.  Would be good to allocate this
     *  to a base class for all grid panels.
     */

    int m_drop_x;           // mouse tracking

    /**
     *  The x location of the mouse when dropped.  Would be good to allocate this
     *  to a base class for all grid panels.
     */

    int m_drop_y;

    /**
     *  Tells where the dragging started, the x value.
     */

    int m_move_delta_x;

    /**
     *  Tells where the dragging started, the y value.
     */

    int m_move_delta_y;

    /**
     *  Current x coordinate of pointer. Could move it to a base class.
     */

    int m_current_x;

    /**
     *  Current y coordinate of pointer. Could move it to a base class.
     */

    int m_current_y;

    /**
     *  This item is used in the fruityseqroll module.
     */

    int m_move_snap_offset_x;

    /**
     *  Provides the location of the progress bar.
     */

    int m_progress_x;

    /**
     *  Provides the old location of the progress bar, for "playhead" tracking.
     */

    int m_old_progress_x;

    /**
     *  The horizontal value of the scroll window in units of
     *  ticks/pulses/divisions.
     */

    int m_scroll_offset_ticks;

    /**
     *  The vertical offset of the scroll window in units of MIDI notes/keys.
     */

    int m_scroll_offset_key;

    /**
     *  The horizontal value of the scroll window in units of pixels.
     */

    int m_scroll_offset_x;

    /**
     *  The vertical value of the scroll window in units of pixels.
     */

    int m_scroll_offset_y;

#ifdef SEQ64_FOLLOW_PROGRESS_BAR

    /**
     *  Provides the current scroll page in which the progress bar resides.
     */

    int m_scroll_page;

    /**
     *  Progress bar follow state.
     */

    bool m_progress_follow;

#endif

    /**
     *  Indicates if we are going to follow the transport in the GUI.
     *  Progress follow?
     */

    bool m_transport_follow;

    /**
     *  TBD.
     */

    bool m_trans_button_press;

    /**
     *  Holds the value of the musical background sequence that is shown in
     *  cyan (formerly grey) on the background of the piano roll.
     */

    int m_background_sequence;

    /**
     *  Set to true if the drawing of the background sequence is to be done.
     */

    bool m_drawing_background_seq;

    /**
     *  Provides an option for expanding the number of measures while
     *  recording.  In essence, the "infinite" track we've wanted, thanks
     *  to Stazed and his Seq32 project.  Defaults to false.
     */

    bool m_expanded_recording;

    /**
     *  The current status/event selected in the seqedit.  Not used in seqroll
     *  at present.
     */

    midibyte m_status;

    /**
     *  The current MIDI control value selected in the seqedit.  Not used in
     *  seqroll at present.
     */

    midibyte m_cc;

    /**
     *  Indicates the edit mode, note versus drum.
     */

    seq64::edit_mode_t m_edit_mode;

    int note_x;             // note drawing variables
    int note_width;
    int note_y;
    int note_height;

    int keyY;               // dimensions
    int keyAreaY;

signals:

public slots:

    void updateEditMode (seq64::edit_mode_t mode);

};          // class qseqroll

}           // namespace seq64

#endif      // SEQ64_QSEQROLL_HPP

/*
 * qseqroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

