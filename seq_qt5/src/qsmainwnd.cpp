#include "qsmainwnd.hpp"
#include "forms/qsmainwnd.ui.h"

bool is_pattern_playing = false;

qsmainwnd::qsmainwnd(QWidget *parent, perform *a_p) :
    QMainWindow(parent),
    ui(new Ui::qsmainwnd),
    mRecentMenu(NULL),
    m_main_perf(a_p)
{
    ui->setupUi(this);

    //nullify all recent file action slots
    for (int i = 0; i < 10; i++)
        mRecentFileActions[i] = NULL;

    //center on screen
    QRect screen = QApplication::desktop()->screenGeometry();
    int x = (screen.width() - width()) / 2;
    int y = (screen.height() - height()) / 2;
    move(x, y);

    //maximize by default
    //    setWindowState(Qt::WindowMaximized);

    m_modified = false;

    // fill options for beats per measure combo box and set default
    for (int i = 0; i < 16; i++)
    {
        QString combo_text = QString::number(i + 1);
        ui->cmb_beat_measure->insertItem(i, combo_text);
    }

    // fill options for beat length combo box and set default
    for (int i = 0; i < 5; i++)
    {
        QString combo_text = QString::number(pow(2, i));
        ui->cmb_beat_length->insertItem(i, combo_text);
    }

    m_msg_error = new QErrorMessage(this);

    m_msg_save_changes = new QMessageBox(this);
    m_msg_save_changes->setText(tr("Unsaved changes detected."));
    m_msg_save_changes->setInformativeText(tr("Do you want to save them?"));
    m_msg_save_changes->setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    m_msg_save_changes->setDefaultButton(QMessageBox::Save);

    mImportDialog = new QFileDialog(this,
                                    tr("Import MIDI file"),
                                    last_used_dir,
                                    tr("MIDI files (*.midi *.mid);;"
                                       "All files (*)"));

    m_dialog_prefs = new qseditoptions(m_main_perf, this);
    m_live_frame = new qsliveframe(ui->LiveTab, m_main_perf);
    m_song_frame = new qperfeditframe(m_main_perf, ui->SongTab);
    m_edit_frame = NULL; //set this so we know for sure the edit tab is empty
    m_beat_ind = new qsmaintime(this, m_main_perf, 4, 4);
    mDialogAbout = new qsabout(this);

    ui->lay_bpm->addWidget(m_beat_ind);
    ui->LiveTabLayout->addWidget(m_live_frame);
    ui->SongTabLayout->addWidget(m_song_frame);

    //pull defaults from song frame
    ui->cmb_beat_length->setCurrentText(QString::number(m_song_frame->get_beat_width()));
    ui->cmb_beat_measure->setCurrentText(QString::number(m_song_frame->get_beats_per_measure()));
    m_beat_ind->setbeat_width(m_song_frame->get_beat_width());
    m_beat_ind->set_beats_per_measure(m_song_frame->get_beats_per_measure());

    updateRecentFilesMenu();

    m_live_frame->setFocus();

    //timer to refresh GUI elements every few ms
    m_timer = new QTimer(this);
    m_timer->setInterval(50);
    connect(m_timer,
            SIGNAL(timeout()),
            this,
            SLOT(refresh()));
    m_timer->start();

    //connect GUI elements to handlers
    connect(ui->actionNew,
            SIGNAL(triggered(bool)),
            this,
            SLOT(newFile()));

    connect(ui->actionSave,
            SIGNAL(triggered(bool)),
            this,
            SLOT(saveFile()));

    connect(ui->actionSave_As,
            SIGNAL(triggered(bool)),
            this,
            SLOT(saveFileAs()));

    connect(ui->actionImport_MIDI,
            SIGNAL(triggered(bool)),
            this,
            SLOT(showImportDialog()));

    connect(ui->actionOpen,
            SIGNAL(triggered(bool)),
            this,
            SLOT(showOpenFileDialog()));

    connect(ui->actionQuit,
            SIGNAL(triggered(bool)),
            this,
            SLOT(quit()));

    connect(ui->actionAbout,
            SIGNAL(triggered(bool)),
            this,
            SLOT(showqsabout()));

    connect(ui->actionPreferences,
            SIGNAL(triggered(bool)),
            m_dialog_prefs,
            SLOT(show()));

    connect(ui->btnPlay,
            SIGNAL(clicked(bool)),
            this,
            SLOT(startPlaying()));

    connect(ui->btnSongPlay,
            SIGNAL(clicked(bool)),
            this,
            SLOT(setSongPlayback(bool)));

    connect(ui->btnStop,
            SIGNAL(clicked(bool)),
            this,
            SLOT(stopPlaying()));

    connect(ui->btnRecord,
            SIGNAL(clicked(bool)),
            this,
            SLOT(setRecording(bool)));

    connect(ui->spinBpm,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(updateBpm(int)));

    connect(ui->cmb_beat_length,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateBeatLength(int)));

    connect(ui->cmb_beat_measure,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updatebeats_per_measure(int)));

    connect(ui->tabWidget,
            SIGNAL(currentChanged(int)),
            this,
            SLOT(tabWidgetClicked(int)));

    connect(ui->btnRecSnap,
            SIGNAL(clicked(bool)),
            this,
            SLOT(setRecordingSnap(bool)));

    //connect to the seq edit signal from the live tab
    connect(m_live_frame,
            SIGNAL(callEditor(int)),
            this,
            SLOT(loadEditor(int)));

    connect(ui->btnPanic,
            SIGNAL(clicked(bool)),
            this,
            SLOT(panic()));

    show();
}

qsmainwnd::~qsmainwnd()
{
    delete ui;
}

void qsmainwnd::startPlaying()
{
    m_main_perf->start();
    m_main_perf->start_jack();
    is_pattern_playing = true;
}

void qsmainwnd::stopPlaying()
{
    m_main_perf->stop_jack();
    m_main_perf->stop();
    ui->btnPlay->setChecked(false);
}

void qsmainwnd::setRecording(bool record)
{
    m_main_perf->set_song_recording(record);
}

void qsmainwnd::setSongPlayback(bool playSongData)
{
    m_main_perf->set_playback_mode(playSongData);

    if (playSongData)
    {
        ui->btnRecord->setEnabled(true);
    }
    else
    {
        setRecording(false);
        ui->btnRecord->setChecked(false);
        ui->btnRecord->setEnabled(false);
    }
}

void qsmainwnd::updateBpm(int newBpm)
{
    m_main_perf->set_bpm(newBpm);
    m_modified = true;
}

void qsmainwnd::showOpenFileDialog()
{
    QString file;
    if (saveCheck())
        file = QFileDialog::getOpenFileName(
                   this,
                   tr("Open MIDI file"),
                   last_used_dir,
                   tr("MIDI files (*.midi *.mid);;"
                      "All files (*)")
                   //                    ,0,
                   //                    QFileDialog::DontUseNativeDialog
               );

    //don't bother trying to open if the user cancels
    if (!file.isEmpty())
        openmidifile(file);
}

void qsmainwnd::openmidifile(const QString &path)
{
    bool result;

    m_main_perf->clear_all();

    midifile f(path);
    result = f.parse(m_main_perf, 0);
    m_modified = !result;

    if (!result)
    {
        QString msg_text = "Error reading MIDI data from file: " + path;
        m_msg_error->showMessage(msg_text);
        m_msg_error->exec();
        return;
    }

    //set last used dir to the one we have just loaded from
    int last_slash = path.lastIndexOf("/");
    last_used_dir = path.left(last_slash + 1);
    global_filename = path;

    updateWindowTitle();

    //reinitialize live frame
    ui->LiveTabLayout->removeWidget(m_live_frame);
    if (m_live_frame)
        delete  m_live_frame;
    m_live_frame = new qsliveframe(ui->LiveTab, m_main_perf);
    ui->LiveTabLayout->addWidget(m_live_frame);
    connect(m_live_frame, //reconnect this as we've made a new object
            SIGNAL(callEditor(int)),
            this,
            SLOT(loadEditor(int)));
    m_live_frame->show();

    //add to recent files list
    m_dialog_prefs->addRecentFile(path);

    m_live_frame->setFocus();

    //update recent menu
    updateRecentFilesMenu();

    m_live_frame->redraw();
    ui->spinBpm->setValue(m_main_perf->get_bpm());
    m_song_frame->update_sizes();
}

void qsmainwnd::updateWindowTitle()
{
    QString title;

    if (global_filename == "")
        title = (PACKAGE) + QString(" - [unnamed]");
    else
    {
        //give us a title with just the MIDI filename, after the last slash
        int last_slash = global_filename.lastIndexOf("/");
        title = global_filename.right(
                    global_filename.length() - last_slash - 1);
        title = (PACKAGE)
                + QString(" - [")
                + title
                + QString("]");
    }

    this->setWindowTitle(title);

}

void qsmainwnd::refresh()
{
    m_beat_ind->update();
}

bool qsmainwnd::saveCheck()
{
    bool result = false;

    if (m_modified)
    {
        int choice = m_msg_save_changes->exec();
        switch (choice)
        {
        case QMessageBox::Save:
            if (saveFile())
                result = true;
            break;
        case QMessageBox::Discard:
            result = true;
            break;
        case QMessageBox::Cancel:
        default:
            break;
        }
    }
    else
        result = true;

    return result;
}

void qsmainwnd::newFile()
{
    if (saveCheck())
    {
        m_main_perf->clear_all();

        //TODO ensure proper reset on load

        global_filename = "";
        updateWindowTitle();
        m_modified = false;
    }
}

bool qsmainwnd::saveFile()
{
    bool result = false;

    if (global_filename == "")
    {
        saveFileAs();
        return true;
    }

    midifile file(global_filename);
    result = file.write(m_main_perf);

    if (!result)
    {
        m_msg_error->showMessage("Error writing file.");
        m_msg_error->exec();
    }
    else
    {
        /* add to recent files list */
        m_dialog_prefs->addRecentFile(global_filename);

        /* update recent menu */
        updateRecentFilesMenu();
    }
    m_modified = !result;
    return result;

}

void qsmainwnd::saveFileAs()
{
    QString file;

    file = QFileDialog::getSaveFileName(
               this,
               tr("Save MIDI file as..."),
               last_used_dir,
               tr("MIDI files (*.midi *.mid);;"
                  "All files (*)")
               //                                ,0,
               //                                QFileDialog::DontUseNativeDialog
           );

    if (!file.isEmpty())
    {
        QFileInfo fileInfo(file);
        QString suffix = fileInfo.completeSuffix();

        if ((suffix != "midi") && (suffix != "mid"))
        {
            file += ".midi";
        }

        global_filename = file;
        updateWindowTitle();
        saveFile();
    }
}

void qsmainwnd::showImportDialog()
{
    mImportDialog->exec();

    QStringList filePaths = mImportDialog->selectedFiles();

    for (int i = 0; i < filePaths.length(); i++)
    {
        QString path = mImportDialog->selectedFiles()[i];

        if (!path.isEmpty())
        {
            try
            {
                midifile f(path);
                f.parse(m_main_perf, m_main_perf->getBank());
                m_modified = true;
                ui->spinBpm->setValue(m_main_perf->get_bpm());
                m_live_frame->setBank(m_main_perf->getBank());

            }
            catch (...)
            {
                QString msg_text = "Error reading MIDI data from file: " + path;
                m_msg_error->showMessage(msg_text);
                m_msg_error->exec();
            }
        }
    }
}

void qsmainwnd::showqsabout()
{
    mDialogAbout->show();
}

void qsmainwnd::loadEditor(int seqId)
{
    ui->EditTabLayout->removeWidget(m_edit_frame);
    if (m_edit_frame)
        delete  m_edit_frame;
    m_edit_frame = new qseqeditframe(ui->EditTab, m_main_perf, seqId);
    ui->EditTabLayout->addWidget(m_edit_frame);
    m_edit_frame->show();
    ui->tabWidget->setCurrentIndex(2);
    m_modified = true;
}

void qsmainwnd::updateBeatLength(int blIndex)
{
    int bl;
    switch (blIndex)
    {
    case 0:
        bl = 1;
        break;
    case 1:
        bl = 2;
        break;
    case 2:
        bl = 4;
        break;
    case 3:
        bl = 8;
        break;
    case 4:
        bl = 16;
        break;
    }

    m_song_frame->set_beat_width(bl);
    m_beat_ind->setbeat_width(bl);

    //also set beat length for all sequences
    for (int i = 0; i < c_max_sequence; i++)
    {
        if (m_main_perf->is_active(i))
        {
            sequence *seq =  m_main_perf->get_sequence(i);
            seq->setbeat_width(bl);
            //reset number of measures, causing length to adjust to new b/m
            seq->setNumMeasures(seq->getNumMeasures());
        }
    }
    m_modified = true;

    //update the edit frame if it exists
    if (m_edit_frame)
        m_edit_frame->updateDrawGeometry();
}

void qsmainwnd::updatebeats_per_measure(int bmIndex)
{
    int bm = bmIndex + 1;
    m_song_frame->set_beats_per_measure(bm);
    m_beat_ind->set_beats_per_measure(bm);

    //also set beat length for all sequences
    for (int i = 0; i < c_max_sequence; i++)
    {
        if (m_main_perf->is_active(i))
        {
            sequence *seq =  m_main_perf->get_sequence(i);
            seq->set_beats_per_measure(bmIndex + 1);
            //reset number of measures, causing length to adjust to new b/m
            seq->setNumMeasures(seq->getNumMeasures());

        }
    }
    m_modified = true;
    //update the edit frame if it exists
    if (m_edit_frame)
        m_edit_frame->updateDrawGeometry();
}

void qsmainwnd::tabWidgetClicked(int newIndex)
{
    //if we've selected the edit tab,
    //make sure it has something to edit
    if (newIndex == 2 && !m_edit_frame)
    {
        int seqId = -1;
        for (int i = 0; i < c_max_sequence; i++)
        {
            if (m_main_perf->is_active(i))
            {
                seqId = i;
                break;
            }
        }
        //no sequence found, make a new one
        if (seqId == -1)
        {
            m_main_perf->new_sequence(0);
            seqId = 0;
        }

        sequence *seq = m_main_perf->get_sequence(seqId);
        seq->set_dirty();
        m_edit_frame = new qseqeditframe(ui->EditTab, m_main_perf, seqId);
        ui->EditTabLayout->addWidget(m_edit_frame);
        m_edit_frame->show();
        update();
    }
}

void qsmainwnd::updateRecentFilesMenu()
{
    //if menu already exists, delete it.
    if (mRecentMenu && mRecentMenu->isWidgetType())
        delete mRecentMenu;

    //recent files sub-menu
    mRecentMenu = new QMenu(tr("&Recent..."), this);

    /* only add if a path is actually contained in each slot */
    if (recent_files[0] != "")
    {
        mRecentFileActions[0] = new QAction(recent_files[0], this);
        mRecentFileActions[0]->setShortcut(tr("Ctrl+R"));
        connect(mRecentFileActions[0],
                SIGNAL(triggered(bool)),
                this,
                SLOT(load_recent_1()));
    }
    else
    {
        mRecentMenu->addAction(tr("(No recent files)"));
        ui->menuFile->insertMenu(ui->actionSave, mRecentMenu);
        return;
    }

    if (recent_files[1] != "")
    {
        mRecentFileActions[1] = new QAction(recent_files[1], this);
        connect(mRecentFileActions[1],
                SIGNAL(triggered(bool)),
                this,
                SLOT(load_recent_2()));
    }

    if (recent_files[2] != "")
    {
        mRecentFileActions[2] = new QAction(recent_files[2], this);
        connect(mRecentFileActions[2],
                SIGNAL(triggered(bool)),
                this,
                SLOT(load_recent_3()));
    }

    if (recent_files[3] != "")
    {
        mRecentFileActions[3] = new QAction(recent_files[3], this);
        connect(mRecentFileActions[3],
                SIGNAL(triggered(bool)),
                this,
                SLOT(load_recent_4()));
    }

    if (recent_files[4] != "")
    {
        mRecentFileActions[4] = new QAction(recent_files[4], this);
        connect(mRecentFileActions[4],
                SIGNAL(triggered(bool)),
                this,
                SLOT(load_recent_5()));
    }

    if (recent_files[5] != "")
    {
        mRecentFileActions[5] = new QAction(recent_files[5], this);
        connect(mRecentFileActions[5],
                SIGNAL(triggered(bool)),
                this,
                SLOT(load_recent_6()));
    }

    if (recent_files[6] != "")
    {
        mRecentFileActions[6] = new QAction(recent_files[6], this);
        connect(mRecentFileActions[6],
                SIGNAL(triggered(bool)),
                this,
                SLOT(load_recent_7()));
    }

    if (recent_files[7] != "")
    {
        mRecentFileActions[7] = new QAction(recent_files[7], this);
        connect(mRecentFileActions[7],
                SIGNAL(triggered(bool)),
                this,
                SLOT(load_recent_8()));
    }

    if (recent_files[8] != "")
    {
        mRecentFileActions[8] = new QAction(recent_files[8], this);
        connect(mRecentFileActions[8],
                SIGNAL(triggered(bool)),
                this,
                SLOT(load_recent_9()));
    }

    if (recent_files[9] != "")
    {
        mRecentFileActions[9] = new QAction(recent_files[9], this);
        connect(mRecentFileActions[9],
                SIGNAL(triggered(bool)),
                this,
                SLOT(load_recent_10()));
    }

    for (int i = 0; i < 10; i++)
    {
        if (mRecentFileActions[i])
        {
            mRecentMenu->addAction(mRecentFileActions[i]);
        }
        else
            break;
    }

    ui->menuFile->insertMenu(ui->actionSave, mRecentMenu);
}

void qsmainwnd::quit()
{
    if (saveCheck())
        QCoreApplication::exit();
}

void qsmainwnd::load_recent_1()
{
    if (saveCheck())
        openmidifile(recent_files[0]);
}

void qsmainwnd::load_recent_2()
{
    if (saveCheck())
        openmidifile(recent_files[1]);
}

void qsmainwnd::load_recent_3()
{
    if (saveCheck())
        openmidifile(recent_files[2]);
}

void qsmainwnd::load_recent_4()
{
    if (saveCheck())
        openmidifile(recent_files[3]);
}

void qsmainwnd::load_recent_5()
{
    if (saveCheck())
        openmidifile(recent_files[4]);
}

void qsmainwnd::load_recent_6()
{
    if (saveCheck())
        openmidifile(recent_files[5]);
}

void qsmainwnd::load_recent_7()
{
    if (saveCheck())
        openmidifile(recent_files[6]);
}

void qsmainwnd::load_recent_8()
{
    if (saveCheck())
        openmidifile(recent_files[7]);
}

void qsmainwnd::load_recent_9()
{
    if (saveCheck())
        openmidifile(recent_files[8]);
}

void qsmainwnd::load_recent_10()
{
    if (saveCheck())
        openmidifile(recent_files[9]);
}

void qsmainwnd::setRecordingSnap(bool snap)
{
    m_main_perf->setSongRecordSnap(snap);
}

void qsmainwnd::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Space:
        if (m_main_perf->is_running())
        {
            stopPlaying();
            ui->btnPlay->setChecked(false);
        }
        else
        {
            startPlaying();
            ui->btnPlay->setChecked(true);
        }
        break;

    default:
        event->ignore();
        break;
    }
}

void qsmainwnd::panic()
{
    m_main_perf->panic();
}
