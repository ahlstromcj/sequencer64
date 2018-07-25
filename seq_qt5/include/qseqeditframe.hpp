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
 * \updates       2018-07-24
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 *
\verbatim
    Gtkmm           Qt 5                Kepler34 (Qt 5)
    ---------       ----------------    -----------------
    mainwid         qsliveframe         LiveFrame
    mainwnd         qsmainwnd           MainWindow
    seqedit         qseqeditframe       EditFrame
    seqedit         qseqeditframe64     EditFrame
    seqkeys         qseqkeys            EditKeys
    seqtime         qseqtime            EditTimeBar
    seqroll         qseqroll            EditNoteRoll
    seqdata         qseqdata            EditEventValues
    seqevent        qstriggereditor     EditEventTriggers
\endverbatim
 */

#include <QFrame>
#include <QLayout>

#include "sequence.hpp"                 /* seq64::edit_mode_t enumeration   */

/**
 *  Specifies the base size of the main window. The size in the "ui" file is
 *  864 x 580.  We can control the base size at build time by altering the
 *  qsmainwnd values.
 */

#define SEQ64_QSMAINWND_WIDTH           800
#define SEQ64_QSMAINWND_HEIGHT          480

/*
 *  A bunch of forward declarations.  The Qt header files are moved into the
 *  cpp file.
 */

class QWidget;
class QGridLayout;
class QScrollArea;
class QPalette;
class QMenu;

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace Ui
{
    class qseqeditframe;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq64
{
    class perform;
    class qseqkeys;
    class qseqtime;
    class qseqroll;
    class qseqdata;
    class qstriggereditor;

/**
 *  This frame holds tools for editing an individual MIDI sequence.
 *  Basically the same as Kepler34's EditFrame class, renamed to fit in with
 *  the Gtkmm version's naming conventions.
 */

class qseqeditframe : public QFrame
{
    Q_OBJECT

public:

    explicit qseqeditframe
    (
        seq64::perform & perf, int seqid, QWidget * parent = nullptr
    );
    virtual ~qseqeditframe ();

    void update_draw_geometry ();
    void setEditorMode (seq64::edit_mode_t mode); // set a new editing mode

private:

    /**
     * \getter m_perform
     *      The const version.
     */

    const seq64::perform & perf () const
    {
        return m_perform;
    }

    /**
     * \getter m_perform
     *      The non-const version.
     */

    seq64::perform & perf ()
    {
        return m_perform;
    }

private:

    Ui::qseqeditframe * ui;
    QWidget * mContainer;
    QGridLayout * m_layout_grid;
    QScrollArea * m_scroll_area;
    QPalette * m_palette;
    QMenu * mPopup;
    perform & m_perform;
    sequence * m_seq;
    qseqkeys * m_seqkeys;
    qseqtime * m_seqtime;
    qseqroll * m_seqroll;
    qseqdata * m_seqdata;
    qstriggereditor * m_seqevent;

    /**
     *  Update timer for pass-along to the roll, event, and data classes.
     */

    QTimer * m_timer;

    int mSnap; /* set snap to in pulses, off = 1 */
    int m_seqId;
    edit_mode_t editMode;

private:

    void set_dirty ();

private slots:

    void conditional_update ();
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

