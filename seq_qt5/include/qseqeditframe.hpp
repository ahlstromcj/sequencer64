#ifndef SEQ64_QSEQEDITFRAME_HPP
#define SEQ64_QSEQEDITFRAME_HPP

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
 *  along with seq24; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

/**
 * \file          qseqeditframe.hpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-05-27
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 */

#include <QFrame>
#include <QLayout>

#include "sequence.hpp"                 /* seq64::edit_mode_t enumeration   */

/*
 *  A bunch of forward declarations.  The Qt header files are moved into the
 *  cpp file.
 */

class QWidget;
class QGridLayout;
class QScrollArea;
class QPalette;
class QMenu;

namespace Ui
{
    class qseqeditframe;
}

namespace seq64
{
    class perform;
    class qseqkeys;
    class qseqtime;
    class qseqroll;
    class qseqdata;
    class qstriggereditor;

/**
 * Holds tools for editing an individual MIDI sequence
 */

class qseqeditframe : public QFrame
{
    Q_OBJECT

public:

    explicit qseqeditframe
    (
        seq64::perform & perf, QWidget * parent, int suqnum
    );
    ~qseqeditframe ();

    void updateDrawGeometry ();
    void setEditorMode (seq64::edit_mode_t mode); //set a new editing mode

    const seq64::perform & perf () const
    {
        return mPerformance;
    }

    seq64::perform & perf ()
    {
        return mPerformance;
    }

private:

    Ui::qseqeditframe * ui;
    QWidget * mContainer;
    QGridLayout * m_layout_grid;
    QScrollArea * m_scroll_area;
    QPalette * m_palette;
    QMenu * mPopup;
    seq64::perform & mPerformance;
    seq64::sequence * mSeq;
    seq64::qseqkeys * mKeyboard;
    seq64::qseqtime * mTimeBar;
    seq64::qseqroll * mNoteGrid;
    seq64::qseqdata * mEventValues;
    seq64::qstriggereditor * mEventTriggers;
    int mSnap; /* set snap to in pulses, off = 1 */
    int mSeqId;
    edit_mode_t editMode;

private slots:

    void updateSeqName ();
    void updateGridSnap (int snapIndex);
    void updatemidibus (int newIndex);
    void updateMidiChannel (int newIndex);
    void undo ();
    void redo ();
    void showTools ();
    void updateNoteLength (int newIndex);
    void zoom_in ();
    void zoom_out ();
    void updateKey (int newIndex);
    void updateSeqLength ();
    void updateScale (int newIndex);
    void updateBackgroundSeq (int newIndex);
    void toggleEditorMode ();
    void updateRecVol ();
    void toggleMidiPlay (bool newVal);
    void toggleMidiThru (bool newVal);
    void toggleMidiQRec (bool newVal);
    void toggleMidiRec (bool newVal);
    void selectAllNotes ();
    void inverseNoteSelection ();
    void quantizeNotes ();
    void tightenNotes ();
    void transposeNotes ();

};          // class qseditframe

}           // namespace seq64

#endif      // SEQ64_QSEQEDITFRAME_HPP

/*
 * qseqeditframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

