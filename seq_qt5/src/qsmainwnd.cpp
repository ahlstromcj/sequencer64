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
 * \file          qsmainwnd.cpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-03-07
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 */

#include "qsmainwnd.hpp"
#include "perform.hpp"
#include "qperfeditframe.hpp"
#include "qsabout.hpp"
#include "qseditoptions.hpp"
#include "qsmaintime.hpp"
#include "qseqeditframe.hpp"
#include "qsliveframe.hpp"
#include "settings.hpp"                 /* seq64::rc() and seq64::usr()     */
#include "forms/qsmainwnd.ui.h"

/*
 * Don't document the namespace.
 */

namespace seq64
{

/**
 *
 */

qsmainwnd::qsmainwnd (perform & p, QWidget * parent)
 :
    QMainWindow     (parent),
    ui              (new Ui::qsmainwnd),
    m_menu_recent   (NULL),
    m_main_perf     (p)
{
    ui->setupUi(this);
    for (int i = 0; i < 10; i++)        // nullify all recent-file action slots
        m_action[i] = NULL;

    // center on screen

    QRect screen = QApplication::desktop()->screenGeometry();
    int x = (screen.width() - width()) / 2;
    int y = (screen.height() - height()) / 2;
    move(x, y);

    // maximize by default setWindowState(Qt::WindowMaximized);

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
    m_msg_save_changes->setStandardButtons
    (
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );
    m_msg_save_changes->setDefaultButton(QMessageBox::Save);

    mImportDialog = new QFileDialog
    (
        this, tr("Import MIDI file"),
        rc().last_used_dir().c_str(),
        tr("MIDI files (*.midi *.mid);;"
        "All files (*)")
    );

    m_dialog_prefs = new qseditoptions(m_main_perf, this);
    m_live_frame = new qsliveframe(m_main_perf, ui->LiveTab);
    m_song_frame = new qperfeditframe(m_main_perf, ui->SongTab);
    m_edit_frame = NULL; //set this so we know for sure the edit tab is empty
    m_beat_ind = new qsmaintime(m_main_perf, this, 4, 4);
    mDialogAbout = new qsabout(this);

    ui->lay_bpm->addWidget(m_beat_ind);
    ui->LiveTabLayout->addWidget(m_live_frame);
    ui->SongTabLayout->addWidget(m_song_frame);

    //pull defaults from song frame
    ui->cmb_beat_length->setCurrentText
    (
        QString::number(m_song_frame->get_beat_width())
    );
    ui->cmb_beat_measure->setCurrentText
    (
        QString::number(m_song_frame->get_beats_per_measure())
    );
    m_beat_ind->set_beat_width(m_song_frame->get_beat_width());
    m_beat_ind->set_beats_per_measure(m_song_frame->get_beats_per_measure());
    update_recent_files_menu();
    m_live_frame->setFocus();

    // timer to refresh GUI elements every few ms

    m_timer = new QTimer(this);
    m_timer->setInterval(50);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
    m_timer->start();

    // connect GUI elements to handlers

    connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(new_file()));
    connect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(save_file()));
    connect
    (
        ui->actionSave_As, SIGNAL(triggered(bool)), this, SLOT(save_file_as())
    );
    connect
    (
        ui->actionImport_MIDI, SIGNAL(triggered(bool)),
        this, SLOT(showImportDialog())
    );
    connect
    (
        ui->actionOpen, SIGNAL(triggered(bool)),
        this, SLOT(showOpenFileDialog())
    );
    connect(ui->actionQuit, SIGNAL(triggered(bool)), this, SLOT(quit()));
    connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(showqsabout()));
    connect
    (
        ui->actionPreferences, SIGNAL(triggered(bool)),
        m_dialog_prefs, SLOT(show())
    );
    connect(ui->btnPlay, SIGNAL(clicked(bool)), this, SLOT(startPlaying()));
    connect
    (
        ui->btnSongPlay, SIGNAL(clicked(bool)),
        this, SLOT(setSongPlayback(bool))
    );
    connect(ui->btnStop, SIGNAL(clicked(bool)), this, SLOT(stopPlaying()));
    connect(ui->btnRecord, SIGNAL(clicked(bool)), this, SLOT(setRecording(bool)));
    connect(ui->spinBpm, SIGNAL(valueChanged(int)), this, SLOT(updateBpm(int)));
    connect
    (
        ui->cmb_beat_length, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updateBeatLength(int))
    );
    connect
    (
        ui->cmb_beat_measure, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updatebeats_per_measure(int))
    );
    connect
    (
        ui->tabWidget, SIGNAL(currentChanged(int)),
        this, SLOT(tabWidgetClicked(int))
    );
    connect
    (
        ui->btnRecSnap, SIGNAL(clicked(bool)),
        this, SLOT(setRecordingSnap(bool))
    );

    // connect to the seq edit signal from the live tab

    connect(m_live_frame, SIGNAL(callEditor(int)), this, SLOT(loadEditor(int)));
    connect(ui->btnPanic, SIGNAL(clicked(bool)), this, SLOT(panic()));
    show();
}

/**
 *
 */

qsmainwnd::~qsmainwnd()
{
    delete ui;
}

/**
 *
 */

void
qsmainwnd::startPlaying()
{
    perf().start(false);                // false = live, need song support too
    perf().start_jack();
    perf().is_pattern_playing(true);
}

/**
 *
 */

void
qsmainwnd::stopPlaying()
{
    perf().stop_jack();
    perf().stop();
    ui->btnPlay->setChecked(false);
}

/**
 *
 */

void
qsmainwnd::setRecording (bool record)
{
    perf().song_recording(record);
}

/**
 *
 */

void
qsmainwnd::setSongPlayback (bool playSongData)
{
    perf().playback_mode(playSongData);
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

/**
 *
 */

void
qsmainwnd::updateBpm (int newBpm)
{
    perf().set_beats_per_minute(newBpm);
    m_modified = true;
}

/**
 *
 */

void
qsmainwnd::showOpenFileDialog ()
{
    QString file;
    if (check())
    {
        file = QFileDialog::getOpenFileName
        (
            this, tr("Open MIDI file"), rc().last_used_dir().c_str(),
            tr("MIDI files (*.midi *.mid);; All files (*)")
        );
    }
    if (! file.isEmpty())                   // if the user did not cancel
        open_file(file.toStdString());
}

/**
 *  Set the last-used directory to the one just loaded.
 *
 *  Compare this function to mainwnd::open_file() [the Gtkmm version]/
 */

void
qsmainwnd::open_file (const std::string & fn)
{
    midifile f(fn);
    perf().clear_all();

    bool result = f.parse(m_main_perf, 0);
    m_modified = ! result;
    if (! result)
    {
        QString msg_text = "Error reading MIDI data from file: ";
        msg_text += fn.c_str();
        m_msg_error->showMessage(msg_text);
        m_msg_error->exec();
        return;
    }

//  ppqn(f.ppqn());                     /* get and save the actual PPQN     */
    rc().last_used_dir(fn.substr(0, fn.rfind("/") + 1));
    rc().filename(fn);
    rc().add_recent_file(fn);           /* from Oli Kester's Kepler34       */
    update_recent_files_menu();
    update_window_title();

    /*
     *  Reinitialize the "Live" frame.
     */

    ui->LiveTabLayout->removeWidget(m_live_frame);
    if (m_live_frame)
        delete m_live_frame;

    m_live_frame = new qsliveframe(m_main_perf, ui->LiveTab);
    ui->LiveTabLayout->addWidget(m_live_frame);

    // Reconnect this signal, as we've made a new object.

    connect(m_live_frame, SIGNAL(callEditor(int)), this, SLOT(loadEditor(int)));
    m_live_frame->show();

    m_live_frame->setFocus();
//  update_recent_files_menu();
    m_live_frame->redraw();
    ui->spinBpm->setValue(perf().bpm());
    m_song_frame->update_sizes();
}

/**
 *
 */

void
qsmainwnd::update_window_title ()
{
    std::string title = SEQ64_APP_NAME + std::string(" - [");
    std::string itemname = "unnamed";
    int ppqn = SEQ64_DEFAULT_PPQN;          // choose_ppqn(m_ppqn);
    char temp[16];
    snprintf(temp, sizeof temp, " (%d ppqn) ", ppqn);
    if (! rc().filename().empty())
    {
        std::string name = shorten_file_spec(rc().filename(), 56);
        itemname = name;                    // Glib::filename_to_utf8(name);
    }
    title += itemname + std::string("]") + std::string(temp);

    QString t = title.c_str();
    setWindowTitle(t);
}

/**
 *
 */

void
qsmainwnd::refresh ()
{
    m_beat_ind->update();
}

/**
 *
 */

bool
qsmainwnd::check ()
{
    bool result = false;
    if (m_modified)
    {
        int choice = m_msg_save_changes->exec();
        switch (choice)
        {
        case QMessageBox::Save:
            if (save_file())
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

/**
 *
 * \todo
 *      Ensure proper reset on load.
 */

void
qsmainwnd::new_file()
{
    if (check() && perf().clear_all())
    {
        // m_entry_notes->set_text(perf().current_screenset_notepad());
        rc().filename("");
        update_window_title();
        m_modified = false;
    }
}

/**
 *
 */

bool qsmainwnd::save_file()
{
    bool result = false;
    if (rc().filename().empty())
    {
        save_file_as();
        result = true;
    }
    else
    {
        midifile f
        (
            rc().filename()
            // , ppqn(), rc().legacy_format(), usr().global_seq_feature()
        );
        result = f.write(m_main_perf);
        if (result)
        {
            rc().add_recent_file(rc().filename());
            update_recent_files_menu();
        }
        else
        {
            m_msg_error->showMessage("Error writing file.");
            m_msg_error->exec();
        }
    }
    m_modified = ! result;
    return result;
}

/**
 *
 */

void
qsmainwnd::save_file_as()
{
    QString file;
    file = QFileDialog::getSaveFileName
    (
        this,
        tr("Save MIDI file as..."),
        rc().last_used_dir().c_str(),
        tr("MIDI files (*.midi *.mid);;All files (*)")
    );

    if (! file.isEmpty())
    {
        QFileInfo fileInfo(file);
        QString suffix = fileInfo.completeSuffix();
        if ((suffix != "midi") && (suffix != "mid"))
            file += ".midi";

        rc().filename(file.toStdString());
        update_window_title();
        save_file();
    }
}

/**
 *
 */

void
qsmainwnd::showImportDialog()
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
                midifile f(path.toStdString());
                f.parse(m_main_perf, perf().screenset());
                m_modified = true;
                ui->spinBpm->setValue(perf().bpm());
                m_live_frame->setBank(perf().screenset());

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

/**
 *
 */

void
qsmainwnd::showqsabout()
{
    mDialogAbout->show();
}

/**
 *
 */

void
qsmainwnd::loadEditor(int seqId)
{
    ui->EditTabLayout->removeWidget(m_edit_frame);
    if (m_edit_frame)
        delete  m_edit_frame;

    m_edit_frame = new qseqeditframe(m_main_perf, ui->EditTab, seqId);
    ui->EditTabLayout->addWidget(m_edit_frame);
    m_edit_frame->show();
    ui->tabWidget->setCurrentIndex(2);
    m_modified = true;
}

/**
 *
 */

void
qsmainwnd::updateBeatLength (int blIndex)
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

    default:
        bl = 4;
        break;
    }

    m_song_frame->set_beat_width(bl);
    m_beat_ind->set_beat_width(bl);

    //also set beat length for all sequences
    for (int i = 0; i < c_max_sequence; i++)
    {
        if (perf().is_active(i))
        {
            sequence *seq =  perf().get_sequence(i);
            seq->set_beat_width(bl);
            //reset number of measures, causing length to adjust to new b/m
            seq->set_num_measures(seq->get_num_measures());
        }
    }
    m_modified = true;

    //update the edit frame if it exists
    if (m_edit_frame)
        m_edit_frame->updateDrawGeometry();
}

/**
 *
 */

void
qsmainwnd::updatebeats_per_measure(int bmIndex)
{
    int bm = bmIndex + 1;
    m_song_frame->set_beats_per_measure(bm);
    m_beat_ind->set_beats_per_measure(bm);

    //also set beat length for all sequences
    for (int i = 0; i < c_max_sequence; i++)
    {
        if (perf().is_active(i))
        {
            sequence *seq =  perf().get_sequence(i);
            seq->set_beats_per_bar(bmIndex + 1);
            //reset number of measures, causing length to adjust to new b/m
            seq->set_num_measures(seq->get_num_measures());

        }
    }
    m_modified = true;
    //update the edit frame if it exists
    if (m_edit_frame)
        m_edit_frame->updateDrawGeometry();
}

/**
 *
 */

void
qsmainwnd::tabWidgetClicked (int newIndex)
{
    /*
     * If we've selected the edit tab, make sure it has something to edit
     */

    if (newIndex == 2 && ! m_edit_frame)
    {
        int seqId = -1;
        for (int i = 0; i < c_max_sequence; ++i)
        {
            if (perf().is_active(i))
            {
                seqId = i;
                break;
            }
        }
        if (seqId == -1)                // no sequence found, make a new one
        {
            perf().new_sequence(0);
            seqId = 0;
        }

        sequence * seq = perf().get_sequence(seqId);
        seq->set_dirty();
        m_edit_frame = new qseqeditframe(m_main_perf, ui->EditTab, seqId);
        ui->EditTabLayout->addWidget(m_edit_frame);
        m_edit_frame->show();
        update();
    }
}

/**
 *
 */

void
qsmainwnd::update_recent_files_menu ()
{
    /*
     *  If the menu already exists, delete it.  This differs from the Gtkmm
     *  implementation, which simply clears it.
     */

    if (m_menu_recent && m_menu_recent->isWidgetType())
        delete m_menu_recent;

    m_menu_recent = new QMenu(tr("&Recent MIDI files..."), this);

    /*
     *  Only add if a path is actually contained in each slot.  This method
     *  of adding paths is pretty clumsy compared to the Gtkmm method, which
     *  uses a simple loop.
     */


    if (rc().recent_file_count() > 0)
    {
        m_action[0]->setShortcut(tr("Ctrl+R"));
    }
    else
    {
        m_menu_recent->addAction(tr("<none>"));
        ui->menuFile->insertMenu(ui->actionSave, m_menu_recent);
        return;
    }

    if (rc().recent_file_count() > 0)
    {
        m_action[0] = new QAction(rc().recent_file(0).c_str(), this);
        connect(m_action[0], SIGNAL(triggered(bool)), this, SLOT(load_recent_1()));
    }
    if (rc().recent_file_count() > 1)
    {
        m_action[1] = new QAction(rc().recent_file(1).c_str(), this);
        connect(m_action[1], SIGNAL(triggered(bool)), this, SLOT(load_recent_2()));
    }
    if (rc().recent_file_count() > 2)
    {
        m_action[2] = new QAction(rc().recent_file(2).c_str(), this);
        connect(m_action[2], SIGNAL(triggered(bool)), this, SLOT(load_recent_3()));
    }
    if (rc().recent_file_count() > 3)
    {
        m_action[3] = new QAction(rc().recent_file(3).c_str(), this);
        connect(m_action[3], SIGNAL(triggered(bool)), this, SLOT(load_recent_4()));
    }
    if (rc().recent_file_count() > 4)
    {
        m_action[4] = new QAction(rc().recent_file(4).c_str(), this);
        connect(m_action[4], SIGNAL(triggered(bool)), this, SLOT(load_recent_5()));
    }
    if (rc().recent_file_count() > 5)
    {
        m_action[5] = new QAction(rc().recent_file(5).c_str(), this);
        connect(m_action[5], SIGNAL(triggered(bool)), this, SLOT(load_recent_6()));
    }
    if (rc().recent_file_count() > 6)
    {
        m_action[6] = new QAction(rc().recent_file(6).c_str(), this);
        connect(m_action[6], SIGNAL(triggered(bool)), this, SLOT(load_recent_7()));
    }
    if (rc().recent_file_count() > 7)
    {
        m_action[7] = new QAction(rc().recent_file(7).c_str(), this);
        connect(m_action[7], SIGNAL(triggered(bool)), this, SLOT(load_recent_8()));
    }
    if (rc().recent_file_count() > 8)
    {
        m_action[8] = new QAction(rc().recent_file(8).c_str(), this);
        connect(m_action[8], SIGNAL(triggered(bool)), this, SLOT(load_recent_9()));
    }
    if (rc().recent_file_count() > 9)
    {
        m_action[9] = new QAction(rc().recent_file(9).c_str(), this);
        connect(m_action[9], SIGNAL(triggered(bool)), this, SLOT(load_recent_10()));
    }

    for (int i = 0; i < 10; ++i)
    {
        if (m_action[i])
            m_menu_recent->addAction(m_action[i]);
        else
            break;
    }
    ui->menuFile->insertMenu(ui->actionSave, m_menu_recent);
}

/**
 *
 */

void
qsmainwnd::quit ()
{
    if (check())
        QCoreApplication::exit();
}

void qsmainwnd::load_recent_1 () { if (check()) open_file(rc().recent_file(0)); }
void qsmainwnd::load_recent_2 () { if (check()) open_file(rc().recent_file(1)); }
void qsmainwnd::load_recent_3 () { if (check()) open_file(rc().recent_file(2)); }
void qsmainwnd::load_recent_4 () { if (check()) open_file(rc().recent_file(3)); }
void qsmainwnd::load_recent_5 () { if (check()) open_file(rc().recent_file(4)); }
void qsmainwnd::load_recent_6 () { if (check()) open_file(rc().recent_file(5)); }
void qsmainwnd::load_recent_7 () { if (check()) open_file(rc().recent_file(6)); }
void qsmainwnd::load_recent_8 () { if (check()) open_file(rc().recent_file(7)); }
void qsmainwnd::load_recent_9 () { if (check()) open_file(rc().recent_file(8)); }
void qsmainwnd::load_recent_10 () { if (check()) open_file(rc().recent_file(9)); }

/**
 *
 */

void
qsmainwnd::setRecordingSnap (bool snap)
{
    perf().song_record_snap(snap);
}

/**
 *
 */

void
qsmainwnd::keyPressEvent (QKeyEvent * event)
{
    switch (event->key())
    {
    case Qt::Key_Space:
        if (perf().is_running())
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

/**
 *
 */

void
qsmainwnd::panic()
{
    perf().panic();
}

}               // namespace seq64

/*
 * qsmainwnd.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

