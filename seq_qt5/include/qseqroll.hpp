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
 * \updates       2018-06-27
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

#include "qseqbase.hpp"                 /* seq64::qseqbase mixin class      */
#include "sequence.hpp"                 /* seq64::edit_mode_t mode          */

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

class qseqroll : public QWidget, protected qseqbase
{
    Q_OBJECT

    friend class qseqeditframe;
    friend class qseqeditframe64;

public:

    qseqroll
    (
        perform & perf,
        sequence & seq,
        qseqkeys * seqkeys_wid  = nullptr,
        int zoom                = SEQ64_DEFAULT_ZOOM,
        int snap                = SEQ64_DEFAULT_SNAP,
        int pos                 =  0,
        seq64::edit_mode_t mode = EDIT_MODE_NOTE,
        QWidget * parent        = nullptr
    );

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

    void set_snap (int snap)
    {
        m_snap = snap;
    }
     */

protected:      // overrides for painting, mouse/keyboard events, & size hints

    void paintEvent (QPaintEvent *);
    void mousePressEvent (QMouseEvent *);
    void mouseReleaseEvent (QMouseEvent *);
    void mouseMoveEvent (QMouseEvent *);
    void keyPressEvent (QKeyEvent *);
    void keyReleaseEvent (QKeyEvent *);
    QSize sizeHint () const;

private:

    void snap_y (int & y);
    void set_adding (bool a_adding);
    void start_paste();

private:

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
     *  The current key selected?
     */

    int m_key;

    /**
     *  Holds the note length in force for this sequence.  Used in the
     *  seq24seqroll module only.
     */

    int m_note_length;

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

    int keyY;               // dimensions of height
    int keyAreaY;

signals:

public slots:

    void conditional_update ();
    void update_edit_mode (seq64::edit_mode_t mode);

};          // class qseqroll

}           // namespace seq64

#endif      // SEQ64_QSEQROLL_HPP

/*
 * qseqroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

