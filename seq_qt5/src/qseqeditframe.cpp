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
 * \file          qseqeditframe.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-07-26
 * \license       GNU GPLv2 or above
 *
 *  This version of the qseqedit-frame class is basically the Kepler34
 *  version, which scrolls the time, keys, roll, data, and event panes
 *  all at the same time.  This behavior is not ideal, but works better
 *  in the "Edit" tab than the new qseqeditframe64 class, which is too
 *  large vertically, even when shrunk to its minimum.
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 *
 *  The layout of this frame is depicted here:
 *
\verbatim
                 -----------------------------------------------------------
QHBoxLayout     | seqname : gridsnap : notelength : seqlength : ...         |
                 -----------------------------------------------------------
QHBoxLayout     | undo : redo : tools : zoomin : zoomout : scale : ...      |
                 -----------------------------------------------------------
                |   | qseqtime      (0, 1, 1, 1)                        | V |
                |-- |---------------------------------------------------| e |
QVBoxLayout     | q |                                                   | r |
    < >         | s | qseqroll      (1, 1, 1, 1)                        | t |
     |          | e |                                                   |   |
     v          | q |---------------------------------------------------| s |
QScrollArea     | k | qtriggeredit  (2, 1, 1, 1)                        | ' |
QWidget containe| e |---------------------------------------------------| b |
QGridLayout     | y |                                                   | a |
                | s | qseqdata      (3, 1, 1, 1)                        | r |
                |   |                                                   |   |
                 -----------------------------------------------------------
                | Horizontal scroll bar                                 |   |
                 -----------------------------------------------------------

                 qseqkey            (1, 0, 1, 1)
\endverbatim
 *
 *  The horizontal and vertical scroll bars are not shown, but they control
 *  everything in the scroll area.  The disadvantage of having all the "qseq"
 *  objects inside the scroll area is that, unlike the Gtkmm version, the keys
 *  can scroll off horizontally, and the time, trigger edit (events), and data
 *  can scroll off vertically.
 *
 *  The "qseq" widgets are added to the grid layout via the following
 *  function:
 *
 *      addWidget(..., int fromrow, int fromcolumn, int rowspan,
 *          int columnspan, ...)
 */

#include <QMenu>
#include <QPalette>
#include <QScrollArea>
#include <qmath.h>                      /* pow()                            */

#include "Globals.hpp"
#include "perform.hpp"
#include "qseqdata.hpp"
#include "qseqeditframe.hpp"
#include "qseqkeys.hpp"
#include "qseqroll.hpp"
#include "qseqtime.hpp"
#include "qstriggereditor.hpp"
#include "qt5_helpers.hpp"              /* seq64::qt_set_icon()             */
#include "settings.hpp"                 /* usr()                            */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qseqeditframe.h"
#else
#include "forms/qseqeditframe.ui.h"
#endif

/*
 *  We prefer to load the pixmaps on the fly, rather than deal with those
 *  friggin' resource files.
 */

#include "pixmaps/drum.xpm"
#include "pixmaps/play.xpm"
#include "pixmaps/quantize.xpm"
#include "pixmaps/rec.xpm"
#include "pixmaps/redo.xpm"
#include "pixmaps/thru.xpm"
#include "pixmaps/tools.xpm"
#include "pixmaps/undo.xpm"
#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"

namespace seq64
{

/**
 *
 * \param p
 *      Provides the perform object to use for interacting with this sequence.
 *
 * \param seqid
 *      Provides the sequence number.  The sequence pointer is looked up using
 *      this number.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.
 */

qseqeditframe::qseqeditframe
(
    perform & p,
    int seqid,
    QWidget * parent
) :
    QFrame          (parent),
    ui              (new Ui::qseqeditframe),
    m_container     (nullptr),
    m_layout_grid   (nullptr),
    m_scroll_area   (nullptr),
    m_palette       (new QPalette()),
    m_popup         (nullptr),
    m_perform       (p),                            // a reference
    m_seq           (perf().get_sequence(seqid)),   // a pointer
    m_seqkeys       (nullptr),
    m_seqtime       (nullptr),
    m_seqroll       (nullptr),
    m_seqdata       (nullptr),
    m_seqevent      (nullptr),
    m_timer         (nullptr),
    m_snap          (0),
    m_seq_id        (seqid),
    m_ppqn          (perf().ppqn()),
    m_edit_mode     (perf().seq_edit_mode(seqid))
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /*
     * Fill options for grid snap & note length combo box and set their
     * defaults
     */

    for (int i = 0; i < 8; ++i)             /* 16th intervals       */
    {
        QString combo_text = "1/" + QString::number(pow(2, i));
        ui->cmbGridSnap->insertItem(i, combo_text);
        ui->cmbNoteLen->insertItem(i, combo_text);
    }
    ui->cmbGridSnap->insertSeparator(8);
    ui->cmbNoteLen->insertSeparator(8);
    for (int i = 1; i < 8; ++i)             /* triplet intervals    */
    {
        QString combo_text = "1/" + QString::number(pow(2, i) * 1.5);
        ui->cmbGridSnap->insertItem(i + 9, combo_text);
        ui->cmbNoteLen->insertItem(i + 9, combo_text);
    }
    ui->cmbGridSnap->setCurrentIndex(3);
    ui->cmbNoteLen->setCurrentIndex(3);
    for (int i = 0; i <= 15; ++i)           /* MIDI channel numbers */
    {
        QString combo_text = QString::number(i + 1);
        ui->cmbMidiChan->insertItem(i, combo_text);
    }
    for (int i = 0; i <= 15; ++i)           /* seq length options   */
    {
        QString combo_text = QString::number(i + 1);
        ui->cmbSeqLen->insertItem(i, combo_text);
    }
    ui->cmbSeqLen->insertItem(16, "32");
    ui->cmbSeqLen->insertItem(17, "64");

    /*
     * Fill options for scale.  There are many more in the new
     * edit frame.
     */

    ui->cmbScale->insertItem(0, "Off");
    ui->cmbScale->insertItem(1, "Major");
    ui->cmbScale->insertItem(2, "Minor");

    /* MIDI buss options */

    mastermidibus & masterbus = perf().master_bus();
    for (int i = 0; i < masterbus.get_num_out_buses(); ++i)
    {
        ui->cmbMidiBus->addItem
        (
            QString::fromStdString(masterbus.get_midi_out_bus_name(i))
        );
    }
    ui->cmbMidiBus->setCurrentText
    (
        QString::fromStdString
        (
            masterbus.get_midi_out_bus_name(m_seq->get_midi_bus())
        )
    );

    /*
     * Pull data from sequence object.
     */

    ui->txtSeqName->setPlainText(m_seq->name().c_str());
    ui->cmbMidiChan->setCurrentIndex(m_seq->get_midi_channel());
    QString snapText("1/");
    snapText.append(QString::number(m_ppqn * 4 / m_seq->get_snap_tick()));
    ui->cmbGridSnap->setCurrentText(snapText);

    QString seqLenText(QString::number(m_seq->get_num_measures()));
    ui->cmbSeqLen->setCurrentText(seqLenText);
    m_seq->set_editing(true);

    /*
     * Set out our custom elements.
     */

    m_scroll_area = new QScrollArea(this);
    ui->vbox_centre->addWidget(m_scroll_area);
    m_container = new QWidget(m_scroll_area);
    m_layout_grid = new QGridLayout(m_container);
    m_container->setLayout(m_layout_grid);
    m_palette->setColor(QPalette::Background, Qt::white);
    m_container->setPalette(*m_palette);
    m_seqkeys = new qseqkeys
    (
        *m_seq,                          // eventually replace ptr with reference
        m_container,
        usr().key_height(),
        usr().key_height() * c_num_keys + 1
    );
    m_seqtime = new qseqtime
    (
        perf(), *m_seq, SEQ64_DEFAULT_ZOOM, m_ppqn, m_container
    );
    m_seqroll = new qseqroll
    (
        perf(), *m_seq, m_seqkeys, SEQ64_DEFAULT_ZOOM, m_snap, m_ppqn, 0,
        EDIT_MODE_NOTE, this            // m_container
    );
    m_seqroll->update_edit_mode(m_edit_mode);
    m_seqdata = new qseqdata
    (
        perf(), *m_seq, SEQ64_DEFAULT_ZOOM, m_snap,
        m_ppqn, m_container
    );
    m_seqevent = new qstriggereditor
    (
        perf(), *m_seq, m_seqdata, SEQ64_DEFAULT_ZOOM, m_snap,
        m_ppqn, usr().key_height(), m_container
    );
    m_layout_grid->setSpacing(0);
    m_layout_grid->addWidget(m_seqkeys, 1, 0, 1, 1);
    m_layout_grid->addWidget(m_seqtime, 0, 1, 1, 1);
    m_layout_grid->addWidget(m_seqroll, 1, 1, 1, 1);
    m_layout_grid->addWidget(m_seqevent, 2, 1, 1, 1);
    m_layout_grid->addWidget(m_seqdata, 3, 1, 1, 1);
    m_layout_grid->setAlignment(m_seqroll, Qt::AlignTop);
    m_scroll_area->setWidget(m_container);

    ui->cmbRecVol->addItem("Free",       0);
    ui->cmbRecVol->addItem("Fixed 127",  127);
    ui->cmbRecVol->addItem("Fixed 111",  111);
    ui->cmbRecVol->addItem("Fixed 95",   95);
    ui->cmbRecVol->addItem("Fixed 79",   79);
    ui->cmbRecVol->addItem("Fixed 63",   63);
    ui->cmbRecVol->addItem("Fixed 47",   47);
    ui->cmbRecVol->addItem("Fixed 31",   31);
    ui->cmbRecVol->addItem("Fixed 15",   15);

    m_popup = new QMenu(this);
    QMenu * menuSelect = new QMenu(tr("Select..."), m_popup);
    QMenu * menuTiming = new QMenu(tr("Timing..."), m_popup);
    QMenu * menuPitch  = new QMenu(tr("Pitch..."), m_popup);

    QAction * actionSelectAll = new QAction(tr("Select all"), m_popup);
    actionSelectAll->setShortcut(tr("Ctrl+A"));
    connect
    (
        actionSelectAll, SIGNAL(triggered(bool)), this, SLOT(selectAllNotes())
    );
    menuSelect->addAction(actionSelectAll);

    QAction * actionSelectInverse = new QAction(tr("Inverse selection"), m_popup);
    actionSelectInverse->setShortcut(tr("Ctrl+Shift+I"));
    connect(actionSelectInverse, SIGNAL(triggered(bool)),
            this, SLOT(inverseNoteSelection()));
    menuSelect->addAction(actionSelectInverse);

    QAction *actionQuantize = new QAction(tr("Quantize"), m_popup);
    actionQuantize->setShortcut(tr("Ctrl+Q"));
    connect(actionQuantize, SIGNAL(triggered(bool)), this, SLOT(quantizeNotes()));
    menuTiming->addAction(actionQuantize);

    QAction *actionTighten = new QAction(tr("Tighten"), m_popup);
    actionTighten->setShortcut(tr("Ctrl+T"));
    connect(actionTighten, SIGNAL(triggered(bool)), this, SLOT(tightenNotes()));
    menuTiming->addAction(actionTighten);

    char num[12];
    QAction * actionsTranspose[24];
    for (int i = -12; i <= 12; ++i)     /* fill note transpositions     */
    {
        if (i != 0)
        {
            snprintf(num, sizeof(num), "%+d [%s]", i, c_interval_text[abs(i)]);
            actionsTranspose[i + 12] = new QAction(num, m_popup);
            actionsTranspose[i + 12]->setData(i);
            menuPitch->addAction(actionsTranspose[i + 12]);
            connect
            (
                actionsTranspose[i + 12], SIGNAL(triggered(bool)),
                this, SLOT(transposeNotes())
            );
        }
        else
            menuPitch->addSeparator();
    }

    m_popup->addMenu(menuSelect);
    m_popup->addMenu(menuTiming);
    m_popup->addMenu(menuPitch);

    /*
     * Hide unused GUI elements
     */

    ui->lblBackgroundSeq->hide();
    ui->cmbBackgroundSeq->hide();
    ui->lblEventSelect->hide();
    ui->cmbEventSelect->hide();
    ui->lblKey->hide();
    ui->cmbKey->hide();
    ui->lblScale->hide();
    ui->cmbScale->hide();

    connect
    (
        ui->txtSeqName, SIGNAL(textChanged()), this, SLOT(updateSeqName())
    );
    connect
    (
        ui->cmbGridSnap, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updateGridSnap(int))
    );
    connect
    (
        ui->cmbMidiBus, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updatemidibus(int))
    );
    connect
    (
        ui->cmbMidiChan, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updateMidiChannel(int))
    );
    connect(ui->btnUndo, SIGNAL(clicked(bool)), this, SLOT(undo()));
    qt_set_icon(undo_xpm, ui->btnUndo);

    connect(ui->btnRedo, SIGNAL(clicked(bool)), this, SLOT(redo()));
    qt_set_icon(redo_xpm, ui->btnRedo);

    connect(ui->btnTools, SIGNAL(clicked(bool)), this, SLOT(showTools()));
    qt_set_icon(tools_xpm, ui->btnTools);

    connect
    (
        ui->cmbNoteLen, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updateNoteLength(int))
    );
    connect(ui->btnZoomIn, SIGNAL(clicked(bool)), this, SLOT(zoom_in()));
    qt_set_icon(zoom_in_xpm, ui->btnZoomIn);

    connect(ui->btnZoomOut, SIGNAL(clicked(bool)), this, SLOT(zoom_out()));
    qt_set_icon(zoom_out_xpm, ui->btnZoomOut);

    connect
    (
        ui->cmbKey, SIGNAL(currentIndexChanged(int)), this, SLOT(updateKey(int))
    );
    connect
    (
        ui->cmbSeqLen, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updateSeqLength())
    );
    connect
    (
        ui->cmbScale, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updateScale(int))
    );
    connect
    (
        ui->cmbBackgroundSeq, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updateBackgroundSeq(int))
    );
    connect(ui->btnDrum, SIGNAL(clicked(bool)), this, SLOT(toggleEditorMode()));
    qt_set_icon(drum_xpm, ui->btnDrum);

    connect
    (
        ui->cmbRecVol, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updateRecVol())
    );
    connect(ui->btnPlay, SIGNAL(clicked(bool)), this, SLOT(toggleMidiPlay(bool)));
    qt_set_icon(play_xpm, ui->btnPlay);

    connect(ui->btnQRec, SIGNAL(clicked(bool)), this, SLOT(toggleMidiQRec(bool)));
    qt_set_icon(quantize_xpm, ui->btnQRec);

    connect(ui->btnRec, SIGNAL(clicked(bool)), this, SLOT(toggleMidiRec(bool)));
    qt_set_icon(rec_xpm, ui->btnRec);

    connect(ui->btnThru, SIGNAL(clicked(bool)), this, SLOT(toggleMidiThru(bool)));
    qt_set_icon(thru_xpm, ui->btnThru);

    m_timer = new QTimer(this);                             // redraw timer !!!
    m_timer->setInterval(2 * usr().window_redraw_rate());   // 20
    QObject::connect
    (
        m_timer, SIGNAL(timeout()), this, SLOT(conditional_update())
    );
    m_timer->start();
}

/**
 *  Provides stock deletion of the Qt user-interface.
 */

qseqeditframe::~qseqeditframe ()
{
    delete ui;
}

/**
 *  We need to set the dirty state while the sequence has been changed.
 */

void
qseqeditframe::conditional_update ()
{
    if (m_seq->is_dirty_edit())
        set_dirty();
}

/**
 *  Dirties all of the sub-panels to make them reflect any changes made
 *  to the sequence.
 */

void
qseqeditframe::set_dirty ()
{
    m_seqroll->set_dirty();
    m_seqtime->set_dirty();
    m_seqevent->set_dirty();
    m_seqdata->set_dirty();
    update_draw_geometry();
}

/**
 *  Sets the name of the sequence as obtained from the sequence.
 */

void
qseqeditframe::updateSeqName()
{
    m_seq->set_name(ui->txtSeqName->document()->toPlainText().toStdString());
}

/**
 *
 */

void
qseqeditframe::updateGridSnap (int snapindex)
{
    int snap;
    switch (snapindex)
    {
    case  0: snap = m_ppqn * 4; break;
    case  1: snap = m_ppqn * 2; break;
    case  2: snap = m_ppqn * 1; break;
    case  3: snap = m_ppqn / 2; break;
    case  4: snap = m_ppqn / 4; break;
    case  5: snap = m_ppqn / 8; break;
    case  6: snap = m_ppqn / 16; break;
    case  7: snap = m_ppqn / 32; break;
    case  8: snap = m_ppqn * 4; break; // index 8 is a separator
    case  9: snap = m_ppqn * 4  / 3; break;
    case 10: snap = m_ppqn * 2  / 3; break;
    case 11: snap = m_ppqn * 1 / 3; break;
    case 12: snap = m_ppqn / 2 / 3; break;
    case 13: snap = m_ppqn / 4 / 3; break;
    case 14: snap = m_ppqn / 8 / 3; break;
    case 15: snap = m_ppqn / 16 / 3; break;
    default: snap = m_ppqn * 4; break;
    }
    m_seqroll->set_snap(snap);
    m_seq->set_snap_tick(snap);
}

/**
 *
 */

void
qseqeditframe::updatemidibus (int newindex)
{
    m_seq->set_midi_bus(newindex);
}

/**
 *
 */

void
qseqeditframe::updateMidiChannel (int newindex)
{
    m_seq->set_midi_channel(newindex);
}

/**
 *
 */

void
qseqeditframe::undo ()
{
    m_seq->pop_undo();
}

/**
 *
 */

void
qseqeditframe::redo()
{
    m_seq->pop_redo();
}

/**
 *
s   //popup menu over button
 */

void
qseqeditframe::showTools()
{
    m_popup->exec(ui->btnTools->mapToGlobal(QPoint(ui->btnTools->width() - 2,
                                           ui->btnTools->height() - 2)));
}

/**
 *  Updates the selected note length.
 *
 * \param newindex
 *      Provides the index of the combo-box that specifies the note length.
 */

void
qseqeditframe::updateNoteLength (int newindex)
{
    int len;
    switch (newindex)
    {
    case  0: len = m_ppqn * 4; break;
    case  1: len = m_ppqn * 2; break;
    case  2: len = m_ppqn * 1; break;
    case  3: len = m_ppqn / 2; break;
    case  4: len = m_ppqn / 4; break;
    case  5: len = m_ppqn / 8; break;
    case  6: len = m_ppqn / 16; break;
    case  7: len = m_ppqn / 32; break;

    /*
     * Index 8 is a separator. Treat it as the default case.
     */

    case  8: len = m_ppqn * 4; break;
    case  9: len = m_ppqn * 4  / 3; break;
    case 10: len = m_ppqn * 2  / 3; break;
    case 11: len = m_ppqn * 1 / 3; break;
    case 12: len = m_ppqn / 2 / 3; break;
    case 13: len = m_ppqn / 4 / 3; break;
    case 14: len = m_ppqn / 8 / 3; break;
    case 15: len = m_ppqn / 16 / 3; break;
    default: len = m_ppqn * 4; break;
    }
    m_seqroll->set_note_length(len);
}

/**
 *  Zooms in all the panes.
 */

void
qseqeditframe::zoom_in ()
{
    m_seqroll->zoom_in();
    m_seqtime->zoom_in();
    m_seqevent->zoom_in();
    m_seqdata->zoom_in();
    update_draw_geometry();
}

/**
 *  Zooms out all the panes.
 */

void
qseqeditframe::zoom_out ()
{
    m_seqroll->zoom_out();
    m_seqtime->zoom_out();
    m_seqevent->zoom_out();
    m_seqdata->zoom_out();
    update_draw_geometry();
}

/**
 *
 */

void
qseqeditframe::updateKey (int newindex)
{
    // no code, see qseqeditframe64() instead for now.
}

/**
 *  Updates the geometry of all of the sub-panels based on the new pattern
 *  length.
 */

void
qseqeditframe::updateSeqLength ()
{
    int measures = ui->cmbSeqLen->currentText().toInt();
    m_seq->set_num_measures(measures);                       // ????
    m_seqtime->updateGeometry();
    m_seqroll->updateGeometry();
    m_container->adjustSize();
}

/**
 *  Not implemented in this class.  Use qseqeditframe64 for full
 *  functionality.
 */

void
qseqeditframe::updateScale (int newindex)
{
    // Available only in qseqeditframe64.
}

/**
 *  Not implemented in this class.  Use qseqeditframe64 for full
 *  functionality.
 */

void
qseqeditframe::updateBackgroundSeq (int newindex)
{
    // Available only in qseqeditframe64.
}

/**
 *  Updates the geometry of all of the sub-panels.
 */

void
qseqeditframe::update_draw_geometry ()
{
    QString seqLenText(QString::number(m_seq->get_num_measures()));
    ui->cmbSeqLen->setCurrentText(seqLenText);
    m_seqtime->updateGeometry();
    m_seqroll->updateGeometry();
    m_container->adjustSize();
}

/**
 *  Changes the note-editing mode between drum mode and note mode.
 */

void
qseqeditframe::toggleEditorMode ()
{
    switch (m_edit_mode)
    {
    case EDIT_MODE_NOTE:

        m_edit_mode = EDIT_MODE_DRUM;
        ui->cmbNoteLen->hide();
        ui->lblNoteLen->hide();
        break;

    case EDIT_MODE_DRUM:

        m_edit_mode = EDIT_MODE_NOTE;
        ui->cmbNoteLen->show();
        ui->lblNoteLen->show();
        break;
    }
    perf().seq_edit_mode(m_seq_id, m_edit_mode);
    m_seqroll->update_edit_mode(m_edit_mode);
}

/**
 *  Sets the editing mode.
 *
 * \param mode
 *      Provides the DRUM or NOTE mode to be set.
 */

void
qseqeditframe::setEditorMode (seq64::edit_mode_t mode)
{
    m_edit_mode = mode;
    perf().seq_edit_mode(m_seq_id, m_edit_mode);
    m_seqroll->update_edit_mode(mode);
}

/**
 *  Updates the recording volume (incoming volumen [Free] versus wiring
 *  the volume to the current selection.
 */

void
qseqeditframe::updateRecVol ()
{
    m_seq->set_rec_vol(ui->cmbRecVol->currentData().toInt());
}

/**
 *  Toggles the mute status of the sequence.  This also updates,
 *  indirectly, the Live frame view.
 *
 * \param newval
 *      True if the sequence/pattern is to be playing.
 */

void
qseqeditframe::toggleMidiPlay (bool newval)
{
    m_seq->set_playing(newval);
}

/**
 *  Toggles the quantized recording status of the sequence.
 *
 * \param newval
 *      True if the sequence/pattern is to be quantized recorded.
 */

void
qseqeditframe::toggleMidiQRec (bool newval)
{
    m_seq->set_quantized_recording(newval);
}

/**
 *  Toggles the recording status of the sequence.
 *
 * \param newval
 *      True if the sequence/pattern is to be recorded.
 */

void
qseqeditframe::toggleMidiRec (bool newval)
{
    perf().master_bus().set_sequence_input(true, m_seq);
    m_seq->set_recording(newval);
}

/**
 *
 */

void
qseqeditframe::toggleMidiThru (bool newval)
{
    perf().master_bus().set_sequence_input(true, m_seq);
    m_seq->set_thru(newval);
}

/**
 *
 */

void
qseqeditframe::selectAllNotes()
{
    m_seq->select_events(EVENT_NOTE_ON, 0);
    m_seq->select_events(EVENT_NOTE_OFF, 0);
}

/**
 *
 */

void
qseqeditframe::inverseNoteSelection()
{
    m_seq->select_events(EVENT_NOTE_ON, 0, true);
    m_seq->select_events(EVENT_NOTE_OFF, 0, true);
}

/**
 *
 */

void
qseqeditframe::quantizeNotes()
{
    m_seq->push_undo();
    m_seq->quantize_events(EVENT_NOTE_ON, 0, m_seq->get_snap_tick(), 1, true);
}

/**
 *
 */

void
qseqeditframe::tightenNotes()
{
    m_seq->push_undo();
    m_seq->quantize_events(EVENT_NOTE_ON, 0, m_seq->get_snap_tick(), 2, true);
}

/**
 *
 */

void
qseqeditframe::transposeNotes()
{
    QAction * senderAction = (QAction *) sender();
    int transposeVal = senderAction->data().toInt();
    m_seq->push_undo();
    m_seq->transpose_notes(transposeVal, 0);
}

}           // namespace seq64

/*
 * qseqeditframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

