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
 * \updates       2018-04-15
 * \license       GNU GPLv2 or above
 *
 */

#include <QFrame>
#include <QPainter>
#include <QMenu>
#include <QTimer>
#include <QMessageBox>

#include "Globals.hpp"
#include "gui_palette_qt5.hpp"
#include "sequence.hpp"

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
    ~qsliveframe ();

/*
    void redraw ();              // redraw frame contents
    */
    void setBank (int newBank);  // set bank (screen-set) of sequences displayed
    void setBank ();             // bank number retrieved from perform

protected:

    // override painting event to draw on the frame

    void paintEvent (QPaintEvent * event);

    // override mouse events for interaction

    void mousePressEvent (QMouseEvent * event);
    void mouseReleaseEvent (QMouseEvent * event);
    void mouseMoveEvent (QMouseEvent * event);
    void mouseDoubleClickEvent (QMouseEvent * event);

    // override keyboard events for interaction

    void keyPressEvent (QKeyEvent * event);
    void keyReleaseEvent (QKeyEvent * event);

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

    // used to grab std::string bank name and convert it to QString for
    // display

    void updateInternalBankName ();

    // converts the XY coordinates of a click into a seq ID

    int seqIDFromClickXY (int click_x, int click_y);

    Ui::qsliveframe * ui;
    seq64::perform & mPerf;
    seq64::sequence m_moving_seq;
    seq64::sequence m_seq_clipboard;
    QMenu * mPopup;
    QTimer * mRedrawTimer;
    QMessageBox * mMsgBoxNewSeqCheck;
    QFont mFont;
    int m_bank_id;                  // same as the screen-set number
    int thumbW;
    int thumbH;                     // thumbnail dimensions
    int previewW;
    int previewH;                   // internal seq MIDI preview dimensions
    int lastMetro;                  // beat pulsing
    int alpha;
    int m_curr_seq;                 // mouse interaction
    int mOldSeq;
    bool mButtonDown;
    bool mMoving;                   // are we moving bewteen slots
    bool mAddingNew; /*we can add a new seq here, wait for double click*/
    midipulse m_last_tick_x[qc_max_sequence];
    bool m_last_playing[qc_max_sequence];
    bool mCanPaste;

private slots:

    void updateBank (int newBank);
    void updateBankName ();
    void newSeq ();
    void editSeq ();
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

    void callEditor (int seqId); // call the editor tab on the given seq

};              // class qsliveframe

}               // namespace seq64

#endif          // SEQ64_QSLIVEFRAME_HPP

/*
 * qsliveframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

