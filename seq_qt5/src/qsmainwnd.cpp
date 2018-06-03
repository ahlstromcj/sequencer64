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
 * \updates       2018-05-30
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 */

#include "keystroke.hpp"
#include "perform.hpp"
#include "qsmacros.hpp"                 /* QS_TEXT_CHAR() macro             */
#include "qperfeditframe.hpp"
#include "qsabout.hpp"
#include "qsbuildinfo.hpp"
#include "qseditoptions.hpp"
#include "qseqeditframe.hpp"
#include "qskeymaps.hpp"                /* mapping between Gtkmm and Qt     */
#include "qsmaintime.hpp"
#include "qsmainwnd.hpp"
#include "qsliveframe.hpp"
#include "qt5_helpers.hpp"              /* seq64::qt_set_icon()             */
#include "settings.hpp"                 /* seq64::rc() and seq64::usr()     */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qsmainwnd.h"
#else
#include "forms/qsmainwnd.ui.h"         /* generated btnStop, btnPlay, etc. */
#endif

#include "pixmaps/live_mode.xpm"        /* #include "pixmaps/song_mode.xpm" */
#include "pixmaps/panic.xpm"
#include "pixmaps/play2.xpm"
#include "pixmaps/snap.xpm"
#include "pixmaps/song_rec_on.xpm"      /* #include "pixmaps/song_rec.xpm" */
#include "pixmaps/stop.xpm"

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
    QMainWindow         (parent),
    ui                  (new Ui::qsmainwnd),
    m_live_frame        (nullptr),
    m_song_frame        (nullptr),
    m_edit_frame        (nullptr),
    m_msg_error         (nullptr),
    m_msg_save_changes  (nullptr),
    m_timer             (nullptr),
    m_menu_recent       (nullptr),
    m_recent_action_list(),                 // new
    mc_max_recent_files (10),               // new
    mImportDialog       (nullptr),
    m_main_perf         (p),
    m_beat_ind          (nullptr),
    m_dialog_prefs      (nullptr),
    mDialogAbout        (nullptr),
    mDialogBuildInfo    (nullptr)
{
#if __cplusplus < 201103L                               // C++11
    initialize_key_map();
#endif

    ui->setupUi(this);

    QRect screen = QApplication::desktop()->screenGeometry();
    int x = (screen.width() - width()) / 2;             // center on screen
    int y = (screen.height() - height()) / 2;
    move(x, y);

    // fill options for beats per measure combo box and set default

    for (int i = 0; i < 16; ++i)
    {
        QString combo_text = QString::number(i + 1);
        ui->cmb_beat_measure->insertItem(i, combo_text);
    }

    // fill options for beat length combo box and set default

    for (int i = 0; i < 5; ++i)
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
    m_song_frame = new qperfeditframe(m_main_perf, ui->SongTab);
    m_edit_frame = nullptr;                 // set so we know edit tab is empty
    m_beat_ind = new qsmaintime(m_main_perf, this, 4, 4);
    mDialogAbout = new qsabout(this);
    mDialogBuildInfo = new qsbuildinfo(this);

    if (not_nullptr(m_song_frame))
    {
        ui->SongTabLayout->addWidget(m_song_frame);
        ui->cmb_beat_length->setCurrentText // pull defaults from song frame
        (
            QString::number(m_song_frame->get_beat_width())
        );
        ui->cmb_beat_measure->setCurrentText
        (
            QString::number(m_song_frame->get_beats_per_measure())
        );
        if (not_nullptr(m_beat_ind))
        {
            ui->lay_bpm->addWidget(m_beat_ind);
            m_beat_ind->set_beat_width(m_song_frame->get_beat_width());
            m_beat_ind->set_beats_per_measure
            (
                m_song_frame->get_beats_per_measure()
            );
        }
    }

    m_live_frame = new qsliveframe(m_main_perf, ui->LiveTab);
    if (not_nullptr(m_live_frame))
    {
        ui->LiveTabLayout->addWidget(m_live_frame);
        m_live_frame->setFocus();
    }

    m_timer = new QTimer(this);     /* refresh GUI elements every few ms    */
    m_timer->setInterval(50);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
    m_timer->start();

    /*
     * Connect the GUI elements to event handlers.
     */

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
        ui->actionBuildInfo, SIGNAL(triggered(bool)),
        this, SLOT(showqsbuildinfo())
    );
    if (not_nullptr(m_dialog_prefs))
    {
        connect
        (
            ui->actionPreferences, SIGNAL(triggered(bool)),
            m_dialog_prefs, SLOT(show())
        );
    }

    // Play

    connect(ui->btnPlay, SIGNAL(clicked(bool)), this, SLOT(startPlaying()));
    qt_set_icon(play2_xpm, ui->btnPlay);

    // Song Play (live vs song)

    connect
    (
        ui->btnSongPlay, SIGNAL(clicked(bool)),
        this, SLOT(setSongPlayback(bool))
    );
    qt_set_icon(live_mode_xpm, ui->btnSongPlay);

    // Stop

    connect(ui->btnStop, SIGNAL(clicked(bool)), this, SLOT(stopPlaying()));
    qt_set_icon(stop_xpm, ui->btnStop);

    // Record (song)

    connect
    (
        ui->btnRecord, SIGNAL(clicked(bool)), this, SLOT(setRecording(bool))
    );
    qt_set_icon(song_rec_on_xpm, ui->btnRecord);

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

    // Record snap

    connect
    (
        ui->btnRecSnap, SIGNAL(clicked(bool)),
        this, SLOT(setRecordingSnap(bool))
    );
    qt_set_icon(snap_xpm, ui->btnRecSnap);

    // connect to the seq edit signal from the live tab

    if (not_nullptr(m_live_frame))
    {
        connect
        (
            m_live_frame, SIGNAL(callEditor(int)), this, SLOT(loadEditor(int))
        );
    }

    // Panic

    connect(ui->btnPanic, SIGNAL(clicked(bool)), this, SLOT(panic()));
    qt_set_icon(panic_xpm, ui->btnPanic);

    create_action_connections();
    create_action_menu();
    update_recent_files_menu();

    /*
     * This scales the full GUI, cool!
     */

    int width = usr().scale_size(800);
    int height = usr().scale_size(450);
    resize(width, height);
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
 *  Implements the play button, which is also a pause button in Sequencer64,
 *  though we do not yet change the pixmap in the Qt 5 version.
 */

void
qsmainwnd::startPlaying()
{
    perf().pause_key();                 /* not the start_key()              */
}

/**
 *
 */

void
qsmainwnd::stopPlaying()
{
    perf().stop_key();                  /* make sure it's seq64-able        */
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
    if (result)
    {
//      ppqn(f.ppqn());                 /* get and save the actual PPQN     */
        rc().last_used_dir(fn.substr(0, fn.rfind("/") + 1));
        rc().filename(fn);
        rc().add_recent_file(fn);       /* from Oli Kester's Kepler34       */

        /*
         *  Reinitialize the "Live" frame.  Reconnect its signal, as we've
         *  made a new object.
         */

        ui->LiveTabLayout->removeWidget(m_live_frame);
        if (not_nullptr(m_live_frame))
            delete m_live_frame;

        m_live_frame = new qsliveframe(m_main_perf, ui->LiveTab);
        if (not_nullptr(m_live_frame))
        {
            ui->LiveTabLayout->addWidget(m_live_frame);
            connect
            (
                m_live_frame, SIGNAL(callEditor(int)), this, SLOT(loadEditor(int))
            );
            m_live_frame->show();
            m_live_frame->setFocus();

            /*
             * This is not necessary.  And it causes copies painter errors
             * since it bypasses paintEvent().  If we do need to redraw, call
             * m_live_frame->repaint() instead.
             *
             *  m_live_frame->redraw();
             */
        }
        ui->spinBpm->setValue(perf().bpm());
        if (not_nullptr(m_song_frame))
            m_song_frame->update_sizes();

        update_recent_files_menu();
        update_window_title();
    }
    else
    {
        std::string errmsg = f.error_message();
        QString msg_text = errmsg.c_str();      /* msg_text += fn.c_str();  */
        m_msg_error->showMessage(msg_text);
        m_msg_error->exec();
        if (f.error_is_fatal())
            rc().remove_recent_file(fn);
    }
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
    if (not_nullptr(m_beat_ind))
        m_beat_ind->update();
}

/**
 *
 */

bool
qsmainwnd::check ()
{
    bool result = false;
    if (perf().is_modified())
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
            std::string errmsg = f.error_message();
            m_msg_error->showMessage(errmsg.c_str());
            m_msg_error->exec();
        }
    }

    /*
     * The Gtkmm version does not do this.
     */

    return result;
}

/**
 * \todo
 *      Add the export options as done in mainwnd::file_save_as().
 */

void
qsmainwnd::save_file_as ()
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
                ui->spinBpm->setValue(perf().bpm());
                if (not_nullptr(m_live_frame))
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
qsmainwnd::showqsabout ()
{
    if (not_nullptr(mDialogAbout))
        mDialogAbout->show();
}

/**
 *
 */

void
qsmainwnd::showqsbuildinfo ()
{
    if (not_nullptr(mDialogBuildInfo))
        mDialogBuildInfo->show();
}

/**
 * \warning
 *      Somehow, checking for not_nullptr(m_edit_frame) to determine whether
 *      to remove or add that widget causes the edit frame to not get created,
 *      and then the Edit tab is empty.
 */

void
qsmainwnd::loadEditor(int seqId)
{
    ui->EditTabLayout->removeWidget(m_edit_frame);      /* no nullptr check */
    if (not_nullptr(m_edit_frame))
        delete m_edit_frame;

    m_edit_frame = new qseqeditframe(m_main_perf, ui->EditTab, seqId);
    ui->EditTabLayout->addWidget(m_edit_frame);         /* no nullptr check */
    m_edit_frame->show();
    ui->tabWidget->setCurrentIndex(2);
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

    if (not_nullptr(m_song_frame))
        m_song_frame->set_beat_width(bl);

    if (not_nullptr(m_beat_ind))
        m_beat_ind->set_beat_width(bl);

    for (int i = 0; i < c_max_sequence; i++) // set beat length, all sequences
    {
        if (perf().is_active(i))
        {
            sequence *seq =  perf().get_sequence(i);
            seq->set_beat_width(bl);

            // reset number of measures, causing length to adjust to new b/m

            seq->set_num_measures(seq->get_num_measures());
        }
    }

    if (not_nullptr(m_edit_frame))
        m_edit_frame->updateDrawGeometry();
}

/**
 *
 *  Also set beat length for all sequences.
 *  Reset number of measures, causing length to adjust to new b/m.
 */

void
qsmainwnd::updatebeats_per_measure(int bmIndex)
{
    int bm = bmIndex + 1;
    if (not_nullptr(m_song_frame))
        m_song_frame->set_beats_per_measure(bm);

    if (not_nullptr(m_beat_ind))
        m_beat_ind->set_beats_per_measure(bm);

    for (int i = 0; i < c_max_sequence; i++)
    {
        if (perf().is_active(i))
        {
            sequence *seq =  perf().get_sequence(i);
            seq->set_beats_per_bar(bmIndex + 1);
            seq->set_num_measures(seq->get_num_measures());

        }
    }
    if (not_nullptr(m_edit_frame))
        m_edit_frame->updateDrawGeometry();
}

/**
 * \warning
 *      Somehow, checking for not_nullptr(m_edit_frame) to determine whether
 *      to remove or add that widget causes the edit frame to not get created,
 *      and then the Edit tab is empty.
 */

void
qsmainwnd::tabWidgetClicked (int newIndex)
{
    /*
     * If we've selected the edit tab, make sure it has something to edit.
     */

    if (newIndex == 2 && is_nullptr(m_edit_frame))
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
        if (seqId == -1)                /* no sequence found, make a new one */
        {
            perf().new_sequence(0);
            seqId = 0;
        }

        sequence * seq = perf().get_sequence(seqId);
        seq->set_dirty();
        m_edit_frame = new qseqeditframe(m_main_perf, ui->EditTab, seqId);
        ui->EditTabLayout->addWidget(m_edit_frame);     /* no nullptr check */
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
    int count = rc().recent_file_count();
    if (count > 0)
    {
        if (count > mc_max_recent_files)
            count = mc_max_recent_files;

        for (int f = 0; f < count; ++f)
        {
            std::string shortname = rc().recent_file(f);
            std::string longname = rc().recent_file(f, false);
            m_recent_action_list.at(f)->setText(shortname.c_str());
            m_recent_action_list.at(f)->setData(longname.c_str());
            m_recent_action_list.at(f)->setVisible(true);
        }
    }
    for (int fj = count; fj < mc_max_recent_files; ++fj)
        m_recent_action_list.at(fj)->setVisible(false);

    ui->menuFile->insertMenu(ui->actionSave, m_menu_recent);
}

/**
 *
 */

void
qsmainwnd::create_action_connections ()
{
    for (int i = 0; i < mc_max_recent_files; ++i)
    {
        QAction * action = new QAction(this);
        action->setVisible(false);
        QObject::connect
        (
            action, &QAction::triggered, this, &qsmainwnd::open_recent_file
        );
        m_recent_action_list.append(action);
    }
}

/**
 *
 */

void
qsmainwnd::create_action_menu ()
{
    if (not_nullptr(m_menu_recent) && m_menu_recent->isWidgetType())
        delete m_menu_recent;

    m_menu_recent = new QMenu(tr("&Recent MIDI files..."), this);
    for (int i = 0; i < mc_max_recent_files; ++i)
    {
        m_menu_recent->addAction(m_recent_action_list.at(i));
    }
    ui->menuFile->insertMenu(ui->actionSave, m_menu_recent);
}

/**
 *
 */

void
qsmainwnd::open_recent_file ()
{
    QAction * action = qobject_cast<QAction *>(sender());
    if (not_nullptr(action))
    {
        QString fname = QVariant(action->data()).toString();
        std::string actionfile = fname.toStdString();
        open_file(actionfile);
    }
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

/**
 *
 */

void
qsmainwnd::setRecordingSnap (bool snap)
{
    perf().song_record_snap(snap);
}

/**
 *  The Gtkmm 2.4 version calls perform::mainwnd_key_event().  The function
 *  here handles only the Space key to start/stop playback.  See
 *  qsliveframe::keyPressEvent() instead.
 *
 *  Note that there is currently a Qt issue with processing the Escape key
 *  (which is the normal "Stop" key).  For now, we have to set up qseq64.rc to
 *  use the Space key for both Start and Stop.  Also, must debug why the Pause
 *  key is not working.  And why the key appearance is not changing in the
 *  GUI.
 *
 * \todo
 *      Support and indicate the Pause operation.
 *
 * \param event
 *      Provides a pointer to the key event.
 */

void
qsmainwnd::keyPressEvent (QKeyEvent * event)
{
    unsigned ktext = QS_TEXT_CHAR(event->text());
    unsigned kkey = event->key();
    unsigned gdkkey = qt_map_to_gdk(kkey, ktext);

#ifdef PLATFORM_DEBUG_TMI
    std::string kname = qt_key_name(kkey, ktext);
    printf
    (
        "qsmainwnd: name = %s; gdk = 0x%x; key = 0x%x; text = 0x%x\n",
        kname.c_str(), gdkkey, kkey, ktext
    );
#endif

    keystroke k(gdkkey, SEQ64_KEYSTROKE_PRESS);
    if (perf().playback_key_event(k))               // song mode parameter?
    {
        if (perf().is_running())
        {
            ui->btnPlay->setChecked(false);         // stopPlaying();
        }
        else
        {
            ui->btnPlay->setChecked(true);          // startPlaying();
        }
    }
    else
        event->ignore();
}

/**
 *
 */

void
qsmainwnd::panic()
{
    perf().panic();
}

/**
 *
 */

void
qsmainwnd::show_message_box (const std::string & msg_text)
{
    if (not_nullptr(m_msg_error) && ! msg_text.empty())
    {
        QString msg = msg_text.c_str();     /* Qt still needs c_str()!! */
        m_msg_error->showMessage(msg);
        m_msg_error->exec();
    }
}

}               // namespace seq64

/*
 * qsmainwnd.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

