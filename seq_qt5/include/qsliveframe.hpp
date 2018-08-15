#ifndef SEQ64_QSLIVEFRAME_HPP
#define SEQ64_QSLIVEFRAME_HPP

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
 * \file          qsliveframe.hpp
 *
 *  This module declares/defines the base class for plastering
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-08-14
 * \license       GNU GPLv2 or above
 *
 */

#include <QFrame>

#include "globals.h"
#include "gui_palette_qt5.hpp"
#include "sequence.hpp"

class QMenu;
class QTimer;
class QMessageBox;
class QFont;

/*
 * Do not document namespaces.
 */

namespace Ui
{
    class qsliveframe;
}

namespace seq64
{
    class perform;

/**
 *
 */

class qsliveframe : public QFrame, gui_palette_qt5
{
    Q_OBJECT

public:

    explicit qsliveframe (perform & perf, QWidget * parent = 0 );
    virtual ~qsliveframe ();

    void setBank (int newBank);     // bank (screen-set) of sequences displayed
    void setBank ();                // bank number retrieved from perform

protected:                          // overrides of event handlers

    virtual void paintEvent (QPaintEvent * event);
    virtual void mousePressEvent (QMouseEvent * event);
    virtual void mouseReleaseEvent (QMouseEvent * event);
    virtual void mouseMoveEvent (QMouseEvent * event);
    virtual void mouseDoubleClickEvent (QMouseEvent * event);
    virtual void keyPressEvent (QKeyEvent * event);
    virtual void keyReleaseEvent (QKeyEvent * event);

private:

    const seq64::perform & perf () const
    {
        return mPerf;
    }

    seq64::perform & perf ()
    {
        return mPerf;
    }

private:

    void calculate_base_sizes (int seq, int & basex, int & basey);
    void drawSequence (int seq);
    void drawAllSequences ();
    void updateInternalBankName ();
    bool valid_sequence (int seqnum);
    int seqIDFromClickXY (int click_x, int click_y);

private:

    Ui::qsliveframe * ui;
    seq64::perform & mPerf;
    seq64::sequence m_moving_seq;
    seq64::sequence m_seq_clipboard;
    QMenu * m_popup;
    QTimer * m_timer;
    QMessageBox * m_msg_box;
    QFont m_font;
    int m_bank_id;                  // same as the screen-set number

    /**
     *  These values are assigned to the values given by the constants of
     *  similar names in globals.h, and we will make them parameters or
     *  user-interface configuration items later.  Some of them already have
     *  counterparts in the user_settings class.
     */

    int m_mainwnd_rows;
    int m_mainwnd_cols;
    int m_mainwid_spacing;

    /**
     *  Provides a convenience variable for avoiding multiplications.
     *  It is equal to m_mainwnd_rows * m_mainwnd_cols.
     */

    const int m_screenset_slots;

    /**
     *  Provides a convenience variable for avoiding multiplications.
     *  It is equal to m_screenset_slots * m_screenset.
     */

    int m_screenset_offset;

    int m_slot_w;
    int m_slot_h;
    int m_preview_w;
    int m_preview_h;                    // internal seq MIDI preview dimensions
    int m_last_metro;                   // beat pulsing
    int m_alpha;
    int m_curr_seq;                     // mouse interaction
    int m_old_seq;
    bool m_button_down;
    bool m_moving;                      // are we moving between slots
    bool m_adding_new;                  // new seq here, wait for double click
    midipulse m_last_tick_x[c_max_sequence];
    bool m_last_playing[c_max_sequence];
    bool m_can_paste;

private slots:

    void conditional_update ();
    void updateBank (int newBank);
    void updateBankName ();
    void newSeq ();
    void editSeq ();
    void editSeqEx ();
    void editEvents ();
    void copySeq ();
    void cutSeq ();
    void pasteSeq ();
    void deleteSeq ();

    void color_white ();
    void color_red ();
    void color_green ();
    void color_blue ();
    void color_yellow ();
    void color_purple ();
    void color_pink ();
    void color_orange ();

signals:

    void callEditor (int seqid);    /* call editor tab for the sequence     */
    void callEditorEx (int seqid);  /* call editor window for the sequence  */
    void callEditorEvents (int seqid);  /* call event tab for the sequence  */

};              // class qsliveframe

}               // namespace seq64

#endif          // SEQ64_QSLIVEFRAME_HPP

/*
 * qsliveframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

