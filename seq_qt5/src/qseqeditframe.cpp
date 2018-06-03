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
 * \updates       2018-05-27
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
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
 */

qseqeditframe::qseqeditframe
(
    perform & p, QWidget * parent, int seqId
) :
    QFrame          (parent),
    ui              (new Ui::qseqeditframe),
    mContainer      (nullptr),
    m_layout_grid   (nullptr),
    m_scroll_area   (nullptr),
    m_palette       (new QPalette()),
    mPopup          (nullptr),
    mPerformance    (p),
    mSeq            (perf().get_sequence(seqId)),    // a pointer
    mKeyboard       (nullptr),
    mTimeBar        (nullptr),
    mNoteGrid       (nullptr),
    mEventValues    (nullptr),
    mEventTriggers  (nullptr),
    mSnap           (0),
    mSeqId          (seqId),
    editMode        (perf().seq_edit_mode(seqId))
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /*
     * fill options for grid snap & note length combo box and set their
     * defaults
     */

    for (int i = 0; i < 8; i++) // 16th intervals
    {
        QString combo_text = "1/" + QString::number(pow(2, i));
        ui->cmbGridSnap->insertItem(i, combo_text);
        ui->cmbNoteLen->insertItem(i, combo_text);
    }

    ui->cmbGridSnap->insertSeparator(8);
    ui->cmbNoteLen->insertSeparator(8);

    // triplet intervals

    for (int i = 1; i < 8; i++)
    {
        QString combo_text = "1/" + QString::number(pow(2, i) * 1.5);
        ui->cmbGridSnap->insertItem(i + 9, combo_text);
        ui->cmbNoteLen->insertItem(i + 9, combo_text);
    }
    ui->cmbGridSnap->setCurrentIndex(3);
    ui->cmbNoteLen->setCurrentIndex(3);

    /* fill options for MIDI channel numbers */

    for (int i = 0; i <= 15; i++)
    {
        QString combo_text = QString::number(i + 1);
        ui->cmbMidiChan->insertItem(i, combo_text);
    }

    // fill options for seq length

    for (int i = 0; i <= 15; i++)
    {
        QString combo_text = QString::number(i + 1);
        ui->cmbSeqLen->insertItem(i, combo_text);
    }
    ui->cmbSeqLen->insertItem(16, "32");
    ui->cmbSeqLen->insertItem(17, "64");

    // fill options for scale

    ui->cmbScale->insertItem(0, "Off");
    ui->cmbScale->insertItem(1, "Major");
    ui->cmbScale->insertItem(2, "Minor");

    // fill MIDI buss options

    mastermidibus & masterbus = perf().master_bus();
    for (int i = 0; i < masterbus.get_num_out_buses(); i++)
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
            masterbus.get_midi_out_bus_name(mSeq->get_midi_bus())
        )
    );

    /* pull data from sequence object */

    // ui->txtSeqName->setPlainText(mSeq->name());
    ui->txtSeqName->setPlainText(mSeq->name().c_str());
    ui->cmbMidiChan->setCurrentIndex(mSeq->get_midi_channel());

    QString snapText("1/");
    snapText.append(QString::number(c_ppqn * 4 / mSeq->get_snap_tick()));
    ui->cmbGridSnap->setCurrentText(snapText);

    QString seqLenText(QString::number(mSeq->get_num_measures()));
    ui->cmbSeqLen->setCurrentText(seqLenText);

    mSeq->set_editing(true);

    /*
     * Set out our custom elements.
     */

    m_scroll_area = new QScrollArea(this);
    ui->vbox_centre->addWidget(m_scroll_area);

    mContainer = new QWidget(m_scroll_area);
    m_layout_grid = new QGridLayout(mContainer);
    mContainer->setLayout(m_layout_grid);

    m_palette->setColor(QPalette::Background, Qt::darkGray);
    mContainer->setPalette(*m_palette);

    // The key-height parameters should be a user_settings member.

    mKeyboard = new qseqkeys
    (
        *mSeq,                          // eventually replace ptr with reference
        mContainer,
        usr().key_height(),
        usr().key_height() * c_num_keys + 1
    );
    mTimeBar = new qseqtime(*mSeq, mContainer);
    mNoteGrid = new qseqroll(perf(), *mSeq, mContainer);
    mNoteGrid->updateEditMode(editMode);
    mEventValues = new qseqdata(*mSeq, mContainer);
    mEventTriggers = new qstriggereditor
    (
        *mSeq, *mEventValues, mContainer,
        usr().key_height()
    );

    m_layout_grid->setSpacing(0);
    m_layout_grid->addWidget(mKeyboard, 1, 0, 1, 1);
    m_layout_grid->addWidget(mTimeBar, 0, 1, 1, 1);
    m_layout_grid->addWidget(mNoteGrid, 1, 1, 1, 1);
    m_layout_grid->addWidget(mEventTriggers, 2, 1, 1, 1);
    m_layout_grid->addWidget(mEventValues, 3, 1, 1, 1);
    m_layout_grid->setAlignment(mNoteGrid, Qt::AlignTop);

    m_scroll_area->setWidget(mContainer);

    ui->cmbRecVol->addItem("Free",       0);
    ui->cmbRecVol->addItem("Fixed 127",  127);
    ui->cmbRecVol->addItem("Fixed 111",  111);
    ui->cmbRecVol->addItem("Fixed 95",   95);
    ui->cmbRecVol->addItem("Fixed 79",   79);
    ui->cmbRecVol->addItem("Fixed 63",   63);
    ui->cmbRecVol->addItem("Fixed 47",   47);
    ui->cmbRecVol->addItem("Fixed 31",   31);
    ui->cmbRecVol->addItem("Fixed 15",   15);

    mPopup = new QMenu(this);
    QMenu *menuSelect = new QMenu(tr("Select..."), mPopup);
    QMenu *menuTiming = new QMenu(tr("Timing..."), mPopup);
    QMenu *menuPitch  = new QMenu(tr("Pitch..."), mPopup);

    QAction *actionSelectAll = new QAction(tr("Select all"), mPopup);
    actionSelectAll->setShortcut(tr("Ctrl+A"));
    connect(actionSelectAll,
            SIGNAL(triggered(bool)),
            this,
            SLOT(selectAllNotes()));
    menuSelect->addAction(actionSelectAll);

    QAction *actionSelectInverse = new QAction(tr("Inverse selection"), mPopup);
    actionSelectInverse->setShortcut(tr("Ctrl+Shift+I"));
    connect(actionSelectInverse,
            SIGNAL(triggered(bool)),
            this,
            SLOT(inverseNoteSelection()));
    menuSelect->addAction(actionSelectInverse);

    QAction *actionQuantize = new QAction(tr("Quantize"), mPopup);
    actionQuantize->setShortcut(tr("Ctrl+Q"));
    connect(actionQuantize,
            SIGNAL(triggered(bool)),
            this,
            SLOT(quantizeNotes()));
    menuTiming->addAction(actionQuantize);

    QAction *actionTighten = new QAction(tr("Tighten"), mPopup);
    actionTighten->setShortcut(tr("Ctrl+T"));
    connect(actionTighten,
            SIGNAL(triggered(bool)),
            this,
            SLOT(tightenNotes()));
    menuTiming->addAction(actionTighten);

    //fill out note transpositions
    char num[11];
    QAction *actionsTranspose[24];
    for (int i = -12; i <= 12; i++)
    {
        if (i != 0)
        {
            snprintf(num, sizeof(num), "%+d [%s]", i, c_interval_text[abs(i)]);
            actionsTranspose[i + 12] = new QAction(num, mPopup);
            actionsTranspose[i + 12]->setData(i);
            menuPitch->addAction(actionsTranspose[i + 12]);
            connect(actionsTranspose[i + 12],
                    SIGNAL(triggered(bool)),
                    this,
                    SLOT(transposeNotes()));
        }
        else
            menuPitch->addSeparator();
    }

    mPopup->addMenu(menuSelect);
    mPopup->addMenu(menuTiming);
    mPopup->addMenu(menuPitch);

    // Hide unused GUI elements

#if ! defined PLATFORM_DEBUG
    ui->lblBackgroundSeq->hide();
    ui->cmbBackgroundSeq->hide();
    ui->lblEventSelect->hide();
    ui->cmbEventSelect->hide();
    ui->lblKey->hide();
    ui->cmbKey->hide();
    ui->lblScale->hide();
    ui->cmbScale->hide();
#endif

    // connect all the UI signals

    connect(ui->txtSeqName, SIGNAL(textChanged()), this, SLOT(updateSeqName()));
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
}

/**
 *
 */

qseqeditframe::~qseqeditframe ()
{
    delete ui;
}

/**
 *
 */

void
qseqeditframe::updateSeqName()
{
    mSeq->set_name(ui->txtSeqName->document()->toPlainText().toStdString());
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
    case 0:  snap = c_ppqn * 4; break;
    case 1:  snap = c_ppqn * 2; break;
    case 2:  snap = c_ppqn * 1; break;
    case 3:  snap = c_ppqn / 2; break;
    case 4:  snap = c_ppqn / 4; break;
    case 5:  snap = c_ppqn / 8; break;
    case 6:  snap = c_ppqn / 16; break;
    case 7:  snap = c_ppqn / 32; break;
    case 8: snap = c_ppqn * 4; break; // index 8 is a separator
    case 9:  snap = c_ppqn * 4  / 3; break;
    case 10: snap = c_ppqn * 2  / 3; break;
    case 11: snap = c_ppqn * 1 / 3; break;
    case 12: snap = c_ppqn / 2 / 3; break;
    case 13: snap = c_ppqn / 4 / 3; break;
    case 14: snap = c_ppqn / 8 / 3; break;
    case 15: snap = c_ppqn / 16 / 3; break;
    default: snap = c_ppqn * 4; break;
    }
    mNoteGrid->set_snap(snap);
    mSeq->set_snap_tick(snap);
}

/**
 *
 */

void
qseqeditframe::updatemidibus (int newindex)
{
    mSeq->set_midi_bus(newindex);
}

/**
 *
 */

void
qseqeditframe::updateMidiChannel (int newindex)
{
    mSeq->set_midi_channel(newindex);
}

/**
 *
 */

void
qseqeditframe::undo ()
{
    mSeq->pop_undo();
}

/**
 *
 */

void
qseqeditframe::redo()
{
    mSeq->pop_redo();
}

/**
 *
s   //popup menu over button
 */

void
qseqeditframe::showTools()
{
    mPopup->exec(ui->btnTools->mapToGlobal(QPoint(ui->btnTools->width() - 2,
                                           ui->btnTools->height() - 2)));
}

/**
 *
 */

void
qseqeditframe::updateNoteLength (int newindex)
{
    int length;
    switch (newindex)
    {
    case 0: length = c_ppqn * 4; break;
    case 1: length = c_ppqn * 2; break;
    case 2: length = c_ppqn * 1; break;
    case 3: length = c_ppqn / 2; break;
    case 4: length = c_ppqn / 4; break;
    case 5: length = c_ppqn / 8; break;
    case 6: length = c_ppqn / 16; break;
    case 7: length = c_ppqn / 32; break;

        /*
         * Index 8 is a separator. Treat it as the default case.
         */

    case 8: length = c_ppqn * 4; break;
    case 9: length = c_ppqn * 4  / 3; break;
    case 10: length = c_ppqn * 2  / 3; break;
    case 11: length = c_ppqn * 1 / 3; break;
    case 12: length = c_ppqn / 2 / 3; break;
    case 13: length = c_ppqn / 4 / 3; break;
    case 14: length = c_ppqn / 8 / 3; break;
    case 15: length = c_ppqn / 16 / 3; break;
    default: length = c_ppqn * 4; break;
    }

    mNoteGrid->set_note_length(length);
}

/**
 *
 */

void
qseqeditframe::zoom_in ()
{
    mNoteGrid->zoom_in();
    mTimeBar->zoom_in();
    mEventTriggers->zoom_in();
    mEventValues->zoom_in();
    updateDrawGeometry();
}

/**
 *
 */

void
qseqeditframe::zoom_out ()
{
    mNoteGrid->zoom_out();
    mTimeBar->zoom_out();
    mEventTriggers->zoom_out();
    mEventValues->zoom_out();
    updateDrawGeometry();
}

/**
 *
 */

void
qseqeditframe::updateKey(int newindex)
{
    // no code
}

/**
 *
 */

void
qseqeditframe::updateSeqLength()
{
    int measures = ui->cmbSeqLen->currentText().toInt();
    mSeq->set_num_measures(measures);                       // ????
    mTimeBar->updateGeometry();
    mNoteGrid->updateGeometry();
    mContainer->adjustSize();
}

/**
 *
 */

void
qseqeditframe::updateScale(int newindex)
{

}

void
qseqeditframe::updateBackgroundSeq(int newindex)
{

}

void
qseqeditframe::updateDrawGeometry()
{
    QString seqLenText(QString::number(mSeq->get_num_measures()));
    ui->cmbSeqLen->setCurrentText(seqLenText);
    mTimeBar->updateGeometry();
    mNoteGrid->updateGeometry();
    mContainer->adjustSize();
}

/**
 *
 */

void
qseqeditframe::toggleEditorMode()
{
    switch (editMode)
    {
    case EDIT_MODE_NOTE:
        editMode = EDIT_MODE_DRUM;
        ui->cmbNoteLen->hide();
        ui->lblNoteLen->hide();
        break;

    case EDIT_MODE_DRUM:
        editMode = EDIT_MODE_NOTE;
        ui->cmbNoteLen->show();
        ui->lblNoteLen->show();
        break;
    }
    perf().seq_edit_mode(mSeqId, editMode);
    mNoteGrid->updateEditMode(editMode);
}

/**
 *
 */

void
qseqeditframe::setEditorMode (seq64::edit_mode_t mode)
{
    editMode = mode;
    perf().seq_edit_mode(mSeqId, editMode);
    mNoteGrid->updateEditMode(mode);
}

/**
 *
 */

void
qseqeditframe::updateRecVol()
{
    mSeq->set_rec_vol(ui->cmbRecVol->currentData().toInt());
}

/**
 *
 */

void
qseqeditframe::toggleMidiPlay(bool newVal)
{
    mSeq->set_playing(newVal);
}

/**
 *
 */

void
qseqeditframe::toggleMidiQRec(bool newVal)
{
    mSeq->set_quantized_recording(newVal);
}

/**
 *
 */

void
qseqeditframe::toggleMidiRec (bool newVal)
{
    perf().master_bus().set_sequence_input(true, mSeq);
    mSeq->set_recording(newVal);
}

/**
 *
 */

void
qseqeditframe::toggleMidiThru (bool newVal)
{
    perf().master_bus().set_sequence_input(true, mSeq);
    mSeq->set_thru(newVal);
}

/**
 *
 */

void
qseqeditframe::selectAllNotes()
{
    mSeq->select_events(EVENT_NOTE_ON, 0);
    mSeq->select_events(EVENT_NOTE_OFF, 0);
}

/**
 *
 */

void
qseqeditframe::inverseNoteSelection()
{
    mSeq->select_events(EVENT_NOTE_ON, 0, true);
    mSeq->select_events(EVENT_NOTE_OFF, 0, true);
}

/**
 *
 */

void
qseqeditframe::quantizeNotes()
{
    mSeq->push_undo();
    mSeq->quantize_events(EVENT_NOTE_ON, 0, mSeq->get_snap_tick(), 1, true);
}

/**
 *
 */

void
qseqeditframe::tightenNotes()
{
    mSeq->push_undo();
    mSeq->quantize_events(EVENT_NOTE_ON, 0, mSeq->get_snap_tick(), 2, true);
}

/**
 *
 */

void
qseqeditframe::transposeNotes()
{
    QAction *senderAction = (QAction*) sender();
    int transposeVal = senderAction->data().toInt();
    mSeq->push_undo();
    mSeq->transpose_notes(transposeVal, 0);
}

}           // namespace seq64

/*
 * qseqeditframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

