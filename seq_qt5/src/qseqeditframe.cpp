#include "qseqeditframe.hpp"
#include "forms/qseqeditframe.ui.h"

qseqeditframe::qseqeditframe(QWidget *parent,
                             perform *perf,
                             int seqId):
    QFrame(parent),
    ui(new Ui::qseqeditframe),
    mSeq(perf->get_sequence(seqId)),
    mPerformance(perf),
    mSeqId(seqId)
{
    ui->setupUi(this);

    editMode = mPerformance->getEditMode(seqId);

    setSizePolicy(QSizePolicy::Expanding,
                  QSizePolicy::Expanding);

    /* fill options for grid snap & note length
     * combo box and set their defaults */

    //16th intervals
    for (int i = 0; i < 8; i++)
    {
        QString combo_text = "1/" + QString::number(pow(2, i));
        ui->cmbGridSnap->insertItem(i, combo_text);
        ui->cmbNoteLen->insertItem(i, combo_text);
    }

    ui->cmbGridSnap->insertSeparator(8);
    ui->cmbNoteLen->insertSeparator(8);

    //triplet intervals
    for (int i = 1; i < 8; i++)
    {
        QString combo_text = "1/" +
                             QString::number(pow(2, i) * 1.5);
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

    //fill MIDI bus options
    mastermidibus *masterbus = mPerformance->get_master_midi_bus();
    for (int i = 0; i < masterbus->get_num_out_buses(); i++)
        ui->cmbmidibus->addItem(QString::fromStdString(masterbus->get_midi_out_bus_name(i)));
    ui->cmbmidibus->setCurrentText(QString::fromStdString(masterbus->get_midi_out_bus_name(mSeq->get_midi_bus())));

    /* pull data from sequence object */
    ui->txtSeqName->setPlainText(mSeq->name());
    ui->cmbMidiChan->setCurrentIndex(mSeq->get_midi_channel());

    QString snapText("1/");
    snapText.append(QString::number(c_ppqn * 4 / mSeq->getSnap_tick()));
    ui->cmbGridSnap->setCurrentText(snapText);

    QString seqLenText(QString::number(mSeq->getNumMeasures()));
    ui->cmbSeqLen->setCurrentText(seqLenText);

    /* set out our custom elements */
    mSeq->set_editing(true);

    m_scroll_area = new QScrollArea(this);
    ui->vbox_centre->addWidget(m_scroll_area);

    mContainer = new QWidget(m_scroll_area);
    m_layout_grid = new QGridLayout(mContainer);
    mContainer->setLayout(m_layout_grid);

    m_palette = new QPalette();
    m_palette->setColor(QPalette::Background, Qt::darkGray);
    mContainer->setPalette(*m_palette);

    mKeyboard = new qseqkeys(mSeq, mContainer,
                             mPerformance->getEditorKeyHeight(),
                             mPerformance->getEditorKeyboardHeight());
    mTimeBar = new qseqtime(mSeq, mContainer);
    mNoteGrid = new qseqroll(mPerformance, mSeq, mContainer);
    mNoteGrid->updateEditMode(editMode);
    mEventValues = new qseqdata(mSeq, mContainer);
    mEventTriggers = new qstriggereditor(mSeq, mEventValues, mContainer,
                                         mPerformance->getEditorKeyHeight());

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

    //hide unused GUI elements
    ui->lblBackgroundSeq->hide();
    ui->cmbBackgroundSeq->hide();
    ui->lblEventSelect->hide();
    ui->cmbEventSelect->hide();
    ui->lblKey->hide();
    ui->cmbKey->hide();
    ui->lblScale->hide();
    ui->cmbScale->hide();

    //connect all the UI signals
    connect(ui->txtSeqName,
            SIGNAL(textChanged()),
            this,
            SLOT(updateSeqName()));

    connect(ui->cmbGridSnap,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateGridSnap(int)));

    connect(ui->cmbmidibus,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updatemidibus(int)));

    connect(ui->cmbMidiChan,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateMidiChannel(int)));

    connect(ui->btnUndo,
            SIGNAL(clicked(bool)),
            this,
            SLOT(undo()));

    connect(ui->btnRedo,
            SIGNAL(clicked(bool)),
            this,
            SLOT(redo()));

    connect(ui->btnTools,
            SIGNAL(clicked(bool)),
            this,
            SLOT(showTools()));

    connect(ui->cmbNoteLen,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateNoteLength(int)));

    connect(ui->btnZoomIn,
            SIGNAL(clicked(bool)),
            this,
            SLOT(zoom_in()));

    connect(ui->btnZoomOut,
            SIGNAL(clicked(bool)),
            this,
            SLOT(zoom_out()));

    connect(ui->cmbKey,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateKey(int)));

    connect(ui->cmbSeqLen,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateSeqLength()));

    connect(ui->cmbScale,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateScale(int)));

    connect(ui->cmbBackgroundSeq,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateBackgroundSeq(int)));

    connect(ui->btnDrum,
            SIGNAL(clicked(bool)),
            this,
            SLOT(toggleEditorMode()));

    connect(ui->cmbRecVol,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateRecVol()));

    connect(ui->btnPlay,
            SIGNAL(clicked(bool)),
            this,
            SLOT(toggleMidiPlay(bool)));

    connect(ui->btnQRec,
            SIGNAL(clicked(bool)),
            this,
            SLOT(toggleMidiQRec(bool)));

    connect(ui->btnRec,
            SIGNAL(clicked(bool)),
            this,
            SLOT(toggleMidiRec(bool)));

    connect(ui->btnThru,
            SIGNAL(clicked(bool)),
            this,
            SLOT(toggleMidiThru(bool)));
}

qseqeditframe::~qseqeditframe()
{
    delete ui;
}

void qseqeditframe::updateSeqName()
{
    mSeq->set_name(ui->txtSeqName->document()->toPlainText().toStdString());
}

void qseqeditframe::updateGridSnap(int snapIndex)
{
    int snap;
    switch (snapIndex)
    {
    case 0:
        snap = c_ppqn * 4;
        break;
    case 1:
        snap = c_ppqn * 2;
        break;
    case 2:
        snap = c_ppqn * 1;
        break;
    case 3:
        snap = c_ppqn / 2;
        break;
    case 4:
        snap = c_ppqn / 4;
        break;
    case 5:
        snap = c_ppqn / 8;
        break;
    case 6:
        snap = c_ppqn / 16;
        break;
    case 7:
        snap = c_ppqn / 32;
        break;
    //ignore index 8 as it's a separator
    case 9:
        snap = c_ppqn * 4  / 3;
        break;
    case 10:
        snap = c_ppqn * 2  / 3;
        break;
    case 11:
        snap = c_ppqn * 1 / 3;
        break;
    case 12:
        snap = c_ppqn / 2 / 3;
        break;
    case 13:
        snap = c_ppqn / 4 / 3;
        break;
    case 14:
        snap = c_ppqn / 8 / 3;
        break;
    case 15:
        snap = c_ppqn / 16 / 3;
        break;
    }

    mNoteGrid->set_snap(snap);
    mSeq->set_snap_tick(snap);

}

void qseqeditframe::updatemidibus(int newIndex)
{
    mSeq->set_midi_bus(newIndex);
}

void qseqeditframe::updateMidiChannel(int newIndex)
{
    mSeq->set_midi_channel(newIndex);
}

void qseqeditframe::undo()
{
    mSeq->pop_undo();
}

void qseqeditframe::redo()
{
    mSeq->pop_redo();
}

void qseqeditframe::showTools()
{
    //popup menu over button
    mPopup->exec(ui->btnTools->mapToGlobal(QPoint(ui->btnTools->width() - 2,
                                           ui->btnTools->height() - 2)));
}

void qseqeditframe::updateNoteLength(int newIndex)
{
    int length;
    switch (newIndex)
    {
    case 0:
        length = c_ppqn * 4;
        break;
    case 1:
        length = c_ppqn * 2;
        break;
    case 2:
        length = c_ppqn * 1;
        break;
    case 3:
        length = c_ppqn / 2;
        break;
    case 4:
        length = c_ppqn / 4;
        break;
    case 5:
        length = c_ppqn / 8;
        break;
    case 6:
        length = c_ppqn / 16;
        break;
    case 7:
        length = c_ppqn / 32;
        break;
    //ignore index 8 as it's a separator
    case 9:
        length = c_ppqn * 4  / 3;
        break;
    case 10:
        length = c_ppqn * 2  / 3;
        break;
    case 11:
        length = c_ppqn * 1 / 3;
        break;
    case 12:
        length = c_ppqn / 2 / 3;
        break;
    case 13:
        length = c_ppqn / 4 / 3;
        break;
    case 14:
        length = c_ppqn / 8 / 3;
        break;
    case 15:
        length = c_ppqn / 16 / 3;
        break;
    }

    mNoteGrid->set_note_length(length);
}

void qseqeditframe::zoom_in()
{
    mNoteGrid->zoom_in();
    mTimeBar->zoom_in();
    mEventTriggers->zoom_in();
    mEventValues->zoom_in();
    updateDrawGeometry();
}

void qseqeditframe::zoom_out()
{
    mNoteGrid->zoom_out();
    mTimeBar->zoom_out();
    mEventTriggers->zoom_out();
    mEventValues->zoom_out();
    updateDrawGeometry();
}

void qseqeditframe::updateKey(int newIndex)
{

}

void qseqeditframe::updateSeqLength()
{
    int measures = ui->cmbSeqLen->currentText().toInt();
    mSeq->setNumMeasures(measures);
    mTimeBar->updateGeometry();
    mNoteGrid->updateGeometry();
    mContainer->adjustSize();
}

void qseqeditframe::updateScale(int newIndex)
{

}

void qseqeditframe::updateBackgroundSeq(int newIndex)
{

}

void qseqeditframe::updateDrawGeometry()
{
    QString seqLenText(QString::number(mSeq->getNumMeasures()));
    ui->cmbSeqLen->setCurrentText(seqLenText);
    mTimeBar->updateGeometry();
    mNoteGrid->updateGeometry();
    mContainer->adjustSize();
}

void qseqeditframe::toggleEditorMode()
{
    switch (editMode)
    {
    case NOTE:
        editMode = DRUM;
        ui->cmbNoteLen->hide();
        ui->lblNoteLen->hide();
        break;

    case DRUM:
        editMode = NOTE;
        ui->cmbNoteLen->show();
        ui->lblNoteLen->show();
        break;
    }

    mPerformance->setEditMode(mSeqId, editMode);
    mNoteGrid->updateEditMode(editMode);
}

void qseqeditframe::setEditorMode(edit_mode_e mode)
{
    editMode = mode;
    mPerformance->setEditMode(mSeqId, editMode);
    mNoteGrid->updateEditMode(mode);
}

void qseqeditframe::updateRecVol()
{
    mSeq->set_rec_vol(ui->cmbRecVol->currentData().toInt());
}

void qseqeditframe::toggleMidiPlay(bool newVal)
{
    mSeq->set_playing(newVal);
}

void qseqeditframe::toggleMidiQRec(bool newVal)
{
    mSeq->set_quantized_rec(newVal);
}

void qseqeditframe::toggleMidiRec(bool newVal)
{
    mPerformance->get_master_midi_bus()->set_sequence_input(true, mSeq);
    mSeq->set_recording(newVal);
}

void qseqeditframe::toggleMidiThru(bool newVal)
{
    mPerformance->get_master_midi_bus()->set_sequence_input(true, mSeq);
    mSeq->set_thru(newVal);
}

void qseqeditframe::selectAllNotes()
{
    mSeq->select_events(EVENT_NOTE_ON, 0);
    mSeq->select_events(EVENT_NOTE_OFF, 0);
}

void qseqeditframe::inverseNoteSelection()
{
    mSeq->select_events(EVENT_NOTE_ON, 0, true);
    mSeq->select_events(EVENT_NOTE_OFF, 0, true);
}

void qseqeditframe::quantizeNotes()
{
    mSeq->push_undo();
    mSeq->quantize_events(EVENT_NOTE_ON, 0, mSeq->getSnap_tick(), 1, true);
}

void qseqeditframe::tightenNotes()
{
    mSeq->push_undo();
    mSeq->quantize_events(EVENT_NOTE_ON, 0, mSeq->getSnap_tick(), 2, true);
}

void qseqeditframe::transposeNotes()
{
    QAction *senderAction = (QAction*) sender();
    int transposeVal = senderAction->data().toInt();
    mSeq->push_undo();
    mSeq->transpose_notes(transposeVal, 0);
}
