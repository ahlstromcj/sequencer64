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
 * \updates       2018-08-04
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

#include "qseqframe.hpp"                /* seq64::qseqframe                 */
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

class qseqeditframe : public qseqframe
{
    Q_OBJECT

public:

    explicit qseqeditframe
    (
        seq64::perform & perf,
        int seqid,
        QWidget * parent = nullptr
    );
    virtual ~qseqeditframe ();

    void update_draw_geometry ();
    void setEditorMode (seq64::edit_mode_t mode); // set a new editing mode

private:

    Ui::qseqeditframe * ui;
    QWidget * m_container;
    QGridLayout * m_layout_grid;
    QScrollArea * m_scroll_area;
    QPalette * m_palette;
    QMenu * m_popup;

    /**
     *  Update timer for pass-along to the roll, event, and data classes.
     */

    QTimer * m_timer;

    /**
     *  Set the snap-to value in pulses (ticks), off == 1.
     */

    int m_snap;

    /**
     *  Indicates either a drum mode or a note mode of editing the notes.
     */

    edit_mode_t m_edit_mode;

private:

    virtual void update_midi_buttons ();
    virtual void set_dirty ();
    void initialize_panels ();

private slots:

    void conditional_update ();
    void updateSeqName ();
    void updateGridSnap (int snapindex);
    void updatemidibus (int newindex);
    void updateMidiChannel (int newindex);
    void undo ();
    void redo ();
    void showTools ();
    void updateNoteLength (int newindex);
    void zoom_in ();
    void zoom_out ();

    /**
     *  Need a placeholder for this function.
     */

    virtual void reset_zoom ()
    {
        // non-functional, use qseqeditframe64 instead
    }

    void updateKey (int newindex);
    void updateSeqLength ();
    void updateScale (int newindex);
    void updateBackgroundSeq (int newindex);
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

