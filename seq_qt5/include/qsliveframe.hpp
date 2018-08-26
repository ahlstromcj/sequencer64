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
 * \updates       2018-08-24
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
    class keystroke;
    class perform;
    class qsmainwnd;

/**
 *
 */

class qsliveframe : public QFrame, gui_palette_qt5
{
    friend class qsmainwnd;

    Q_OBJECT

public:

    qsliveframe
    (
        perform & perf,
        qsmainwnd * window,
        QWidget * parent = nullptr
    );
    virtual ~qsliveframe ();

    void set_bank (int newBank);    // bank (screen-set) of sequences displayed
    void set_bank ();               // bank number retrieved from perform

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
        return m_perform;
    }

    seq64::perform & perf ()
    {
        return m_perform;
    }

private:

    void calculate_base_sizes (int seq, int & basex, int & basey);
    void drawSequence (int seq);
    void drawAllSequences ();
    void updateInternalBankName ();
    bool valid_sequence (int seqnum);
    int seqIDFromClickXY (int click_x, int click_y);
    bool handle_key_press (unsigned gdkkey);
    bool handle_group_learn (keystroke & k, std::string & msgout);

private:

    Ui::qsliveframe * ui;
    seq64::perform & m_perform;
    qsmainwnd * m_parent;
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

    /**
     *  Indicates that this object is in a mode where the usual mute/unmute
     *  keystroke will instead bring up the pattern slot for editing.
     *  Currently, the hard-wired key for this function is the equals key.
     */

    bool m_call_seq_edit;

    /**
     *  Indicates that this object is in a mode where the usual mute/unmute
     *  keystroke will instead bring up the pattern slot for event-editing.
     *  Currently, the hard-wired key for this function is the minus key.
     */

    bool m_call_seq_eventedit;

    /**
     *  A new flag to indicate if the next pattern hot-key will reach into the
     *  extended part of the set.  It causes 32 (c_seqs_in_set) to be added to
     *  the hot key.  Actually, let's make it an integer that can range from 0
     *  (off) to 1 to 2 (m_seqs_in_set / c_seqs_in_set).
     *
     *  NOT YET ENABLED.
     */

    int m_call_seq_shift;

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

    void callEditor (int seqid);        /* call editor tab for pattern      */
    void callEditorEx (int seqid);      /* call editor window for pattern   */
    void callEditorEvents (int seqid);  /* call event tab for pattern       */

};              // class qsliveframe

}               // namespace seq64

#endif          // SEQ64_QSLIVEFRAME_HPP

/*
 * qsliveframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

