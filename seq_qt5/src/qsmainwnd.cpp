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
 * \updates       2020-07-05
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 */

#include <QErrorMessage>
#include <QFileDialog>
#include <QGuiApplication>              /* used for QScreen geometry() call */
#include <QMessageBox>
#include <QResizeEvent>
#include <QScreen>                      /* Qscreen                          */
#include <QTimer>
#include <utility>                      /* std::make_pair()                 */

#include "calculations.hpp"             /* pulse_to_measurestring(), etc.   */
#include "file_functions.hpp"           /* seq64::file_extension_match()    */
#include "keystroke.hpp"
#include "perform.hpp"
#include "qliveframeex.hpp"
#include "qperfeditex.hpp"
#include "qperfeditframe64.hpp"
#include "qplaylistframe.hpp"
#include "qsmacros.hpp"                 /* QS_TEXT_CHAR() macro             */
#include "qsabout.hpp"
#include "qsbuildinfo.hpp"
#include "qseditoptions.hpp"
#include "qseqeditex.hpp"
#include "qseqeditframe.hpp"            /* Kepler34 version                 */
#include "qseqeventframe.hpp"           /* a new event editor for Qt        */
#include "qskeymaps.hpp"                /* mapping between Gtkmm and Qt     */
#include "qsmaintime.hpp"
#include "qsmainwnd.hpp"
#include "qsliveframe.hpp"
#include "qt5_helpers.hpp"              /* seq64::qt_set_icon()             */
#include "settings.hpp"                 /* seq64::rc() and seq64::usr()     */
#include "wrkfile.hpp"                  /* seq64::wrkfile class             */

/*
 *  A signal handler is defined in daemonize.cpp, used for quick & dirty
 *  signal handling.  Thanks due to user falkTX!
 */

#include "daemonize.hpp"

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qsmainwnd.h"
#else
#include "forms/qsmainwnd.ui.h"         /* generated btnStop, btnPlay, etc. */
#endif

#include "pixmaps/learn.xpm"
#include "pixmaps/learn2.xpm"
#include "pixmaps/live_mode.xpm"        /* #include "pixmaps/song_mode.xpm" */
#include "pixmaps/panic.xpm"
#include "pixmaps/pause.xpm"
#include "pixmaps/perfedit.xpm"
#include "pixmaps/play2.xpm"
#include "pixmaps/song_rec_on.xpm"      /* #include "pixmaps/song_rec.xpm" */
#include "pixmaps/stop.xpm"

/*
 * Don't document the namespace.
 */

namespace seq64
{

/**
 *  Given a display coordinate, looks up the screen and returns its geometry.
 *
 *  If no screen was found, return the primary screen's geometry
 */

static QRect
desktop_rectangle (const QPoint & p)
{
    QList<QScreen *> screens = QGuiApplication::screens();
    Q_FOREACH(const QScreen *screen, screens)
    {
        if (screen->geometry().contains(p))
            return screen->geometry();
    }
    return QGuiApplication::primaryScreen()->geometry();
}

/**
 *  Provides the main window for the application.
 *
 * \param p
 *      Provides the perform object to use for interacting with this sequence.
 *
 * \param midifilename
 *      Provides an optional MIDI file-name.  If provided, the file is opened
 *      immediately.
 *
 * \param ppqn
 *      Sets the desired PPQN value.  If 0 (SEQ64_USE_FILE_PPQN), then
 *      the PPQN to use is obtained from the file.  Otherwise, if a legal
 *      PPQN, any file read is scaled temporally to that PPQN, as applicable.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.
 */

qsmainwnd::qsmainwnd
(
    perform & p,
    const std::string & midifilename,
    int ppqn,
    QWidget * parent
) :
    QMainWindow             (parent),
    ui                      (new Ui::qsmainwnd),
    m_live_frame            (nullptr),
    m_perfedit              (nullptr),
    m_song_frame64          (nullptr),
    m_edit_frame            (nullptr),
    m_event_frame           (nullptr),
    m_playlist_frame        (nullptr),
    m_msg_error             (nullptr),
    m_msg_save_changes      (nullptr),
    m_timer                 (nullptr),
    m_menu_recent           (nullptr),
    m_recent_action_list    (),
    mc_max_recent_files     (10),
    mImportDialog           (nullptr),
    m_main_perf             (p),
    m_beat_ind              (nullptr),
    m_dialog_prefs          (nullptr),
    mDialogAbout            (nullptr),
    mDialogBuildInfo        (nullptr),
    m_is_title_dirty        (false),
    m_ppqn                  (ppqn),     /* can specify 0 for file ppqn  */
    m_tick_time_as_bbt      (true),
    m_current_beats         (0),
    m_base_time_ms          (0),
    m_last_time_ms          (0),
    m_open_editors          (),
    m_open_live_frames      (),
    m_perf_frame_visible    (false)
{
#if ! defined PLATFORM_CPP_11
    initialize_key_map();
#endif

    ui->setupUi(this);

    QPoint pt;                                  /* default at (0, 0)        */
    QRect screen = desktop_rectangle(pt);       /* avoids deprecated func   */
    int x = (screen.width() - width()) / 2;     /* center on the screen     */
    int y = (screen.height() - height()) / 2;
    move(x, y);

    /*
     * TODO.
     *  Combo-box for tweaking the PPQN.
     *  Hidden for now.
     */

    ui->cmbPPQN->hide();

    /*
     * Fill options for beats per measure in the combo box, and set the
     * default.
     */

    for (int i = 0; i < 16; ++i)
    {
        QString combo_text = QString::number(i + 1);
        ui->cmb_beat_measure->insertItem(i, combo_text);
    }

    /*
     * Fill options for beat length (beat width) in the combo box, and set the
     * default.
     */

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
        this, tr("Import MIDI file to Current Set..."),
        rc().last_used_dir().c_str(),
        tr("MIDI files (*.midi *.mid);;WRK files (*.wrk);;All files (*)")
    );

    /*
     * New Song editor frame.
     */

    m_dialog_prefs = new qseditoptions(perf(), this);
    m_beat_ind = new qsmaintime(perf(), this, 4, 4);
    mDialogAbout = new qsabout(this);
    mDialogBuildInfo = new qsbuildinfo(this);
    make_perf_frame_in_tab();           /* create m_song_frame64 pointer    */
    m_live_frame = new qsliveframe(perf(), this, ui->LiveTab);
    m_playlist_frame = new qplaylistframe(perf(), this, ui->PlaylistTab);
    if (not_nullptr(m_playlist_frame))
        ui->PlaylistTabLayout->addWidget(m_playlist_frame);

    if (not_nullptr(m_live_frame))
    {
        ui->LiveTabLayout->addWidget(m_live_frame);
        m_live_frame->setFocus();
    }

    /*
     * File menu.  Connect the GUI elements to event handlers.
     */

    connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(new_file()));
    connect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(save_file()));
    connect
    (
        ui->actionSave_As, SIGNAL(triggered(bool)), this, SLOT(save_file_as())
    );
    connect
    (
        ui->actionExportSong, SIGNAL(triggered(bool)), this, SLOT(export_song())
    );
    connect
    (
        ui->actionExportMIDI, SIGNAL(triggered(bool)),
        this, SLOT(export_file_as_midi())
    );
    connect
    (
        ui->actionImport_MIDI, SIGNAL(triggered(bool)),
        this, SLOT(show_import_dialog())
    );
    connect
    (
        ui->actionOpen, SIGNAL(triggered(bool)),
        this, SLOT(show_open_file_dialog())
    );
    connect
    (
        ui->actionOpenPlaylist, SIGNAL(triggered(bool)),
        this, SLOT(show_open_list_dialog())
    );
    connect(ui->actionQuit, SIGNAL(triggered(bool)), this, SLOT(quit()));

    /*
     * Help menu.
     */

    connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(showqsabout()));
    connect
    (
        ui->actionBuildInfo, SIGNAL(triggered(bool)),
        this, SLOT(showqsbuildinfo())
    );

    /*
     * Edit Menu.  First connect the preferences dialog to the main window's
     * Edit / Preferences menu entry.  Then connect all the new Edit menu
     * entries.
     */

    if (not_nullptr(m_dialog_prefs))
    {
        connect
        (
            ui->actionPreferences, SIGNAL(triggered(bool)),
            m_dialog_prefs, SLOT(show())
        );
    }

    connect
    (
        ui->actionSongEditor, SIGNAL(triggered(bool)),
        this, SLOT(open_performance_edit())
    );
    connect
    (
        ui->actionSongTranspose, SIGNAL(triggered(bool)),
        this, SLOT(apply_song_transpose())
    );
    connect
    (
        ui->actionClearMuteGroups, SIGNAL(triggered(bool)),
        this, SLOT(clear_mute_groups())
    );
    connect
    (
        ui->actionReloadMuteGroups, SIGNAL(triggered(bool)),
        this, SLOT(reload_mute_groups())
    );
    connect
    (
        ui->actionMuteAllTracks, SIGNAL(triggered(bool)),
        this, SLOT(set_song_mute_on())
    );
    connect
    (
        ui->actionUnmuteAllTracks, SIGNAL(triggered(bool)),
        this, SLOT(set_song_mute_off())
    );
    connect
    (
        ui->actionToggleAllTracks, SIGNAL(triggered(bool)),
        this, SLOT(set_song_mute_toggle())
    );

    /*
     * Stop button.
     */

    connect(ui->btnStop, SIGNAL(clicked(bool)), this, SLOT(stop_playing()));
    qt_set_icon(stop_xpm, ui->btnStop);

    /*
     * Pause button.
     */

    connect(ui->btnPause, SIGNAL(clicked(bool)), this, SLOT(pause_playing()));
    qt_set_icon(pause_xpm, ui->btnPause);

    /*
     * Play button.
     */

    connect(ui->btnPlay, SIGNAL(clicked(bool)), this, SLOT(start_playing()));
    qt_set_icon(play2_xpm, ui->btnPlay);

    /*
     * Song Play (Live vs Song) button.
     */

    connect
    (
        ui->btnSongPlay, SIGNAL(clicked(bool)), this, SLOT(set_song_mode(bool))
    );
    if (usr().use_more_icons())
        qt_set_icon(live_mode_xpm, ui->btnSongPlay);

    set_song_mode(false);

    /*
     * Record-Song button.
     */

    connect
    (
        ui->btnRecord, SIGNAL(clicked(bool)), this, SLOT(set_recording(bool))
    );
    qt_set_icon(song_rec_on_xpm, ui->btnRecord);

    /*
     * Performance Editor button.
     */

    connect
    (
        ui->btnPerfEdit, SIGNAL(clicked(bool)), this, SLOT(load_qperfedit(bool))
    );
    qt_set_icon(perfedit_xpm, ui->btnPerfEdit);

    /*
     * B:B:T vs H:M:S button.
     */

    connect
    (
        ui->btn_set_HMS, SIGNAL(clicked(bool)),
        this, SLOT(toggle_time_format(bool))
    );

    /*
     * Group-learn ("L") button.
     */

    connect
    (
        ui->button_learn, SIGNAL(clicked(bool)),
        this, SLOT(learn_toggle())
    );
    qt_set_icon(learn_xpm, ui->button_learn);
    // ui->button_learn->setToolTip
    // (
    //     "Mute Group Learn.\n"
    //     "Click the 'L' button, then press a mute-group key to store\n"
    //     "the mute state of the sequences in the Shift of that key.\n"
    // );

    /*
     * Tap BPM button.
     */

    connect
    (
        ui->button_tap_bpm, SIGNAL(clicked(bool)),
        this, SLOT(tap())
    );
    // ui->button_tap_bpm->setToolTip
    // (
    //     "Tap in time to set the beats per minute (BPM) value.\n"
    //     "After 5 seconds of no taps, the tap-counter will reset to 0.\n"
    //     "The default keystroke for tap-BPM is F9."
    // );

    /*
     * Keep Queue button.
     */

    connect
    (
        ui->button_keep_queue, SIGNAL(clicked(bool)),
        this, SLOT(queue_it())
    );
    // ui->button_keep_queue->setToolTip
    // (
    //     "Shows and toggles the keep-queue status.\n"
    //     "Does not show one-shot queue status."
    // );
    ui->button_keep_queue->setCheckable(true);

    /*
     * BPM (beats-per-minute) spin-box.
     */

    ui->spinBpm->setDecimals(usr().bpm_precision());
    ui->spinBpm->setSingleStep(usr().bpm_step_increment());
    ui->spinBpm->setValue(perf().bpm());
    ui->spinBpm->setReadOnly(false);
    connect
    (
        ui->spinBpm, SIGNAL(valueChanged(double)),
        this, SLOT(update_bpm(double))
    );
    connect
    (
        ui->spinBpm, SIGNAL(editingFinished()), this, SLOT(edit_bpm())
    );

    /*
     * Beat Length (Width) combo-box.
     */

    connect
    (
        ui->cmb_beat_length, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_beat_length(int))
    );

    /*
     * Beats-Per-Measure combo-box.
     */

    connect
    (
        ui->cmb_beat_measure, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updatebeats_per_measure(int))
    );

    /*
     * Tab-change callback.
     */

    connect
    (
        ui->tabWidget, SIGNAL(currentChanged(int)),
        this, SLOT(tabWidgetClicked(int))
    );

    /*
     * Record Snap button. Removed.  We always snap.
     *
     *  connect
     *  (
     *      ui->btnRecSnap, SIGNAL(clicked(bool)),
     *      this, SLOT(set_recording_snap(bool))
     *  );
     *  qt_set_icon(snap_xpm, ui->btnRecSnap);
     */

    /*
     * Pattern editor callbacks.  One for editing in the tab, and the other
     * for editing in an external pattern editor window.  Also added is a
     * signal/callback to create an external live-frame window.
     */

    if (not_nullptr(m_live_frame))
    {
        connect_editor_slots();
        connect
        (
            m_live_frame, SIGNAL(callLiveFrame(int)),
            this, SLOT(load_live_frame(int))
        );
    }

    /*
     * Panic button.
     */

    connect(ui->btnPanic, SIGNAL(clicked(bool)), this, SLOT(panic()));
    qt_set_icon(panic_xpm, ui->btnPanic);

    /*
     * Other setups.
     */

    create_action_connections();
    create_action_menu();
    update_recent_files_menu();
    if (! midifilename.empty())
    {
        open_file(midifilename);
        m_is_title_dirty = true;
    }

    if (usr().window_is_scaled())
    {
        /*
         * This scales the full GUI, cool!  However, it can be overridden by the
         * size of the new, larger, qseqeditframe64 frame.  We see the normal-size
         * window come up, and then it jumps to the larger size.
         */

#ifdef PLATFORM_DEBUG_XXX
        int sh = SEQ64_QSMAINWND_WIDTH;
        int sw = SEQ64_QSMAINWND_HEIGHT;
        int width = usr().scale_size(sw);
        int height = usr().scale_size(sh);
#endif

        QSize s = size();
        int h = s.height();
        int w = s.width();
        int width = usr().scale_size(w);
        int height = usr().scale_size(h);
        resize(width, height);
        if (not_nullptr(m_live_frame))
            m_live_frame->repaint();
    }

    show();
    m_timer = new QTimer(this);         /* refresh GUI element every few ms */
    m_timer->setInterval(2 * usr().window_redraw_rate());    // 50
    connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
    m_timer->start();
}

/**
 *  Destroys the user interface and removes any external qperfeditex that
 *  exists.
 */

qsmainwnd::~qsmainwnd ()
{
    remove_qperfedit();     // hmmm, doesn't seem to work; see closeEvent()
    delete ui;
}

/**
 *  Handles closing this window by calling check(), and, if it returns false,
 *  ignoring the close event.
 *
 * \param event
 *      Provides a pointer to the close event to be checked.
 */

void
qsmainwnd::closeEvent (QCloseEvent * event)
{
    if (check())
    {
        remove_all_editors();
        remove_qperfedit();
        remove_all_live_frames();
    }
    else
        event->ignore();
}

/**
 *  Pulls defaults from song frame.
 */

void
qsmainwnd::make_perf_frame_in_tab ()
{
    m_song_frame64 = new qperfeditframe64(perf(), ui->SongTab);
    if (not_nullptr(m_song_frame64))
    {
        int bpmeasure = m_song_frame64->get_beats_per_measure();
        int beatwidth = m_song_frame64->get_beat_width();
        ui->SongTabLayout->addWidget(m_song_frame64);
        ui->cmb_beat_length->setCurrentText(QString::number(beatwidth));
        ui->cmb_beat_measure->setCurrentText(QString::number(bpmeasure));
        if (not_nullptr(m_beat_ind))
        {
            ui->layout_beat_ind->addWidget(m_beat_ind);
            m_beat_ind->set_beat_width(beatwidth);
            m_beat_ind->set_beats_per_measure(bpmeasure);
        }
    }
}

/**
 *  Calls perform::stop_key() and unchecks the Play button.
 */

void
qsmainwnd::stop_playing ()
{
    perf().stop_key();                  /* make sure it's seq64-able        */
    ui->btnPlay->setChecked(false);
}

/**
 *  Implements the pause button.
 */

void
qsmainwnd::pause_playing ()
{
    perf().pause_key();
}

/**
 *  Implements the play button, which is not a pause button in the Qt version
 *  of Sequencer64.
 */

void
qsmainwnd::start_playing ()
{
    perf().start_key();                 /* not the pause_key()              */
}

/**
 *
 */

void
qsmainwnd::set_recording (bool record)
{
    perf().song_recording(record);
}

/**
 *  Sets the song mode, which is actually the JACK start mode.  If true, we
 *  are in playback/song mode.  If false, we are in live mode.  This
 *  function must be in the cpp module, where the button header file is
 *  included.
 */

void
qsmainwnd::set_song_mode (bool song_mode)
{
    if (song_mode)
    {
        ui->btnRecord->setEnabled(true);
        if (! usr().use_more_icons())
            ui->btnSongPlay->setText("Song");
    }
    else
    {
        set_recording(false);
        ui->btnRecord->setChecked(false);
        ui->btnRecord->setEnabled(false);
        if (! usr().use_more_icons())
            ui->btnSongPlay->setText("Live");
    }
    perf().playback_mode(song_mode);        // useful? not used in mainwnd!
    perf().song_start_mode(song_mode);
}

/**
 *  Toggles the song mode.  Note that calling this function will trigger the
 *  button signal callback, set_song_mode().  It only operates if the patterns
 *  are not playing.  This function must be in the cpp module, where the
 *  button header file is included.
 */

void
qsmainwnd::toggle_song_mode ()
{
    if (! perf().is_pattern_playing())
    {
        ui->btnSongPlay->setEnabled(perf().toggle_song_start_mode());
    }
}

/**
 *
 */

void
qsmainwnd::update_bpm (double bpm)
{
    perf().set_beats_per_minute(midibpm(bpm));
}

/**
 *
 */

void
qsmainwnd::edit_bpm ()
{
    double bpm = ui->spinBpm->value();
    perf().set_beats_per_minute(midibpm(bpm));
}

/**
 *
 */

void
qsmainwnd::show_open_file_dialog ()
{
    QString file;
    if (check())
    {
        file = QFileDialog::getOpenFileName
        (
            this, tr("Open MIDI file"), rc().last_used_dir().c_str(),
            tr("MIDI files (*.midi *.mid);;WRK files (*.wrk);;All files (*)")
        );
    }
    if (! file.isEmpty())                   // if the user did not cancel
        open_file(file.toStdString());
}

/**
 *  Opens the dialog to request a playlist.
 */

void
qsmainwnd::show_open_list_dialog ()
{
    QString file;
    if (check())
    {
        file = QFileDialog::getOpenFileName
        (
            this, tr("Open playlist file"), rc().last_used_dir().c_str(),
            tr("Playlist files (*.playlist);;All files (*)")
        );
    }
    if (! file.isEmpty())                   // if the user did not cancel
    {
        bool playlistmode = perf().open_playlist(file.toStdString());
        if (playlistmode)
        {
            playlistmode = perf().open_current_song();

            /*
             * Update Playlist tab.
             */

            m_playlist_frame->load_playlist();
        }
        else
        {
            QString msg_text = tr(perf().playlist_error_message().c_str());
            m_msg_error->showMessage(msg_text);
            m_msg_error->exec();
        }
    }
}

/**
 *  Also sets the current file-name and the last-used directory to the ones
 *  just loaded.
 *
 *  Compare this function to mainwnd::open_file() [the Gtkmm version]/
 */

void
qsmainwnd::open_file (const std::string & fn)
{
    std::string errmsg;
    int ppqn = m_ppqn;                      /* potential side-effect here   */
    bool result = open_midi_file(perf(), fn, ppqn, errmsg);
    if (result)
    {
        /*
         *  Reinitialize the "Live" frame.  Reconnect its signal, as we've
         *  made a new object.
         */

        ui->LiveTabLayout->removeWidget(m_live_frame);
        if (not_nullptr(m_live_frame))
            delete m_live_frame;

        m_live_frame = new qsliveframe(perf(), this, ui->LiveTab);
        if (not_nullptr(m_live_frame))
        {
            ui->LiveTabLayout->addWidget(m_live_frame);
            connect_editor_slots();     /* external pattern editor window   */
            connect                     /* external live frame window       */
            (
                m_live_frame, SIGNAL(callLiveFrame(int)),
                this, SLOT(load_live_frame(int))
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

        /*
         * Update all of the children.  Doesn't seem to work for the edit
         * frames, may have to recreate them, or somehow hook in the new
         * sequence objects (as pointers, not references).  Probably an issue
         * to be ignored; the user will have to close and reopen the pattern
         * editor(s).
         */

        if (not_nullptr(m_song_frame64))
            m_song_frame64->update_sizes();

        if (not_nullptr(m_perfedit))
            m_perfedit->update_sizes();

#ifdef USE_SEQEDIT_REDRAWING
        if (not_nullptr(m_edit_frame))
            m_edit_frame->update_draw_geometry();

        edit_container::iterator ei;
        for (ei = m_open_editors.begin(); ei != m_open_editors.end(); ++ei)
        {
            qseqeditex * qep = ei->second;      /* save the pointer         */
            qep->update_draw_geometry();
        }
#else
        /*
         * The tabbed edit frame is automatically removed if no other seq-edit
         * is open.
         */

        remove_all_editors();
#endif

        update_recent_files_menu();
        m_is_title_dirty = true;
    }
    else
    {
        QString msg_text = tr(errmsg.c_str());
        m_msg_error->showMessage(msg_text);
        m_msg_error->exec();
    }
}

/**
 *
 */

void
qsmainwnd::update_window_title (const std::string & fn)
{
    std::string itemname = "unnamed";
    if (fn.empty())
    {
        itemname = perf().main_window_title(fn);
    }
    else
    {
        int pp = choose_ppqn();                 // choose_ppqn(ppqn());
        char temp[16];
        snprintf(temp, sizeof temp, " (%d ppqn) ", pp);
        itemname = fn;
        itemname += temp;
    }
    itemname += " [*]";                         // required by Qt 5

    QString fname = QString::fromLocal8Bit(itemname.c_str());
    setWindowTitle(fname);
}

/**
 *  Toggles the recording of the live song control done by the musician.
 *  This functionality currently does not have a key devoted to it, nor is it
 *  a saved setting.
 */

void
qsmainwnd::toggle_time_format (bool /*on*/)
{
    m_tick_time_as_bbt = ! m_tick_time_as_bbt; // m_tick_time_as_bbt = on;
    QString label = m_tick_time_as_bbt ? "B:B:T" : "H:M:S" ;
    ui->btn_set_HMS->setText(label);
}

/**
 *  The debug statement shows us that the main-window size starts at
 *  920 x 680, goes to 800 x 480 (unscaled) briefly, and then back to
 *  920 x 680.
 */

void
qsmainwnd::refresh ()
{
    if (session_close())
    {
        m_timer->stop();
        close();
        return;
    }
    if (session_save())
    {
        save_file();
    }
    if (not_nullptr(m_beat_ind))
        m_beat_ind->update();

    if (not_nullptr(m_live_frame))
    {
        int active_screenset = perf().screenset();
        std::string b = std::to_string(active_screenset);
        ui->entry_active_set->setText(b.c_str());
    }

    if (ui->button_keep_queue->isChecked() != perf().is_keep_queue())
        ui->button_keep_queue->setChecked(perf().is_keep_queue());

    /*
     * Calculate the current time, and display it.
     */

    if (perf().is_pattern_playing())
    {
        midipulse tick = perf().get_tick();
        midibpm bpm = perf().bpm();         // perf().get_beats_per_minute();
        int ppqn = perf().get_ppqn();
        if (m_tick_time_as_bbt)
        {
            midi_timing mt
            (
                bpm, perf().get_beats_per_bar(), perf().get_beat_width(), ppqn
            );
            std::string t = pulses_to_measurestring(tick, mt);
            ui->label_HMS->setText(t.c_str());
        }
        else
        {
            std::string t = pulses_to_timestring(tick, bpm, ppqn, false);
            ui->label_HMS->setText(t.c_str());
        }
    }
    else
    {
        qt_set_icon
        (
            perf().is_group_learning() ?
                learn2_xpm : learn_xpm, ui->button_learn
        );
    }
    if (m_current_beats > 0 && m_last_time_ms > 0)
    {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        long ms = long(spec.tv_sec) * 1000;         /* seconds to ms        */
        ms += round(spec.tv_nsec * 1.0e-6);         /* nanoseconds to ms    */
        long difference = ms - m_last_time_ms;
        if (difference > SEQ64_TAP_BUTTON_TIMEOUT)
        {
            m_current_beats = m_base_time_ms = m_last_time_ms = 0;
            set_tap_button(0);
        }
    }
    if (perf().playlist_mode())
    {
        if (not_nullptr(m_live_frame))
        {
            m_is_title_dirty = true;
            m_live_frame->set_playlist_name(perf().playlist_song());
        }
    }
    else
    {
        if (not_nullptr(m_live_frame))
            m_live_frame->set_playlist_name("");
    }
    if (m_is_title_dirty)
    {
        m_is_title_dirty = false;
        update_window_title();
    }
}

/**
 *  Queries the caller to check for allowing modifications, then performs the
 *  Save, Discard, or Cancel actions.
 *
 * \return
 *      Returns true if the Save worked or if the caller chooses Discard.
 */

bool
qsmainwnd::check ()
{
    if (session_close())
        return true;

    bool result = false;
    if (perf().is_modified())
    {
        int choice = m_msg_save_changes->exec();
        switch (choice)
        {
        case QMessageBox::Save:

            result = save_file();
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
 *  Prompts for a file-name, returning it as, what else, a C++ std::string.
 *
 * \param prompt
 *      The prompt to display.
 *
 * \return
 *      Returns the name of the file.  If empty, the user cancelled.
 */

std::string
qsmainwnd::filename_prompt (const std::string & prompt)
{
    std::string result;
    QString file = QFileDialog::getSaveFileName
    (
        this,
        tr(prompt.c_str()),
        rc().last_used_dir().c_str(),
        tr("MIDI files (*.midi *.mid);;All files (*)")
    );

    if (! file.isEmpty())
    {
        QFileInfo fileInfo(file);
        QString suffix = fileInfo.completeSuffix();
        if ((suffix != "midi") && (suffix != "mid"))
            file += ".midi";

        result = file.toStdString();
    }
    return result;
}

/**
 *
 * \todo
 *      Ensure proper reset on load.
 */

void
qsmainwnd::new_file ()
{
    if (check() && perf().remove_playlist_and_clear())
    {
        rc().filename("");
        m_is_title_dirty = true;
    }
}

/**
 *
 */

bool
qsmainwnd::save_file (const std::string & fname)
{
    bool result = false;
    std::string filename = fname.empty() ? rc().filename() : fname ;
    if (filename.empty())
    {
        result = save_file_as();
    }
    else
    {
        std::string errmsg;
        result = save_midi_file(perf(), filename, errmsg);
        if (result)
        {
            rc().add_recent_file(filename);
            update_recent_files_menu();
        }
        else
        {
            m_msg_error->showMessage(errmsg.c_str());
            m_msg_error->exec();
        }
    }
    return result;
}

/**
 *
 */

bool
qsmainwnd::save_file_as ()
{
    bool result = false;
    std::string prompt = "Save MIDI file as...";
    std::string filename = filename_prompt(prompt);
    if (filename.empty())
    {
        // no code
    }
    else
    {
        result = save_file(filename);
        if (result)
        {
            rc().filename(filename);
            m_is_title_dirty = true;
        }
    }
    return result;
}

/**
 *  Prompts for a file-name, then exports the current tune as a standard
 *  MIDI file, stripping out the Sequencer64 SeqSpec information.  Does not
 *  update the the current file-name, but does update the recent-file
 *  information at this time.
 *
 *  This function is ESSENTIALLY EQUIVALENT to export_song()!!!
 *
 * \param fname
 *      The full path-name to the file to be written.  If empty (the default),
 *      then the user is prompted for the file-name.
 *
 * \return
 *      Returns true if the file was successfully written.
 */

bool
qsmainwnd::export_file_as_midi (const std::string & fname)
{
    bool result = false;
    std::string filename;
    if (fname.empty())
    {
        std::string prompt = "Export file as stock MIDI...";
        filename = filename_prompt(prompt);
    }
    else
        filename = fname;

    if (filename.empty())
    {
        /*
         * Maybe later, add some kind of warning dialog.
         */
    }
    else
    {
        midifile f(filename, choose_ppqn());
        bool result = f.write(perf(), false);           /* no SeqSpec   */
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
    return result;
}

/**
 *  Prompts for a file-name, then exports the current tune as a standard
 *  MIDI file, stripping out the Sequencer64 SeqSpec information.  Does not
 *  update the current file-name, but does update the the recent-file
 *  information at this time.
 *
 *  This function is ESSENTIALLY EQUIVALENT to export_file_as_midi()!!!
 *
 * \param fname
 *      The full path-name to the file to be written.  If empty (the default),
 *      then the user is prompted for the file-name.
 *
 * \return
 *      Returns true if the file was successfully written.
 */

bool
qsmainwnd::export_song (const std::string & fname)
{
    bool result = false;
    std::string filename;
    if (fname.empty())
    {
        std::string prompt = "Export Song as MIDI...";
        filename = filename_prompt(prompt);
    }
    else
        filename = fname;

    if (filename.empty())
    {
        /*
         * Maybe later, add some kind of warning dialog.
         */
    }
    else
    {
        midifile f(filename, choose_ppqn());
        bool result = f.write_song(perf());
        if (result)
        {
            rc().add_recent_file(filename);
            update_recent_files_menu();
        }
        else
        {
            std::string errmsg = f.error_message();
            m_msg_error->showMessage(errmsg.c_str());
            m_msg_error->exec();
        }
    }
    return result;
}

/**
 *
 */

void
qsmainwnd::show_import_dialog ()
{
    mImportDialog->exec();
    QStringList filePaths = mImportDialog->selectedFiles();

    /*
     * Rather than rely on the user remembering to set the destination
     * screen-set, prompt for the set/bank number.  Could make this a user
     * option at some point.
     *
     *  #include <QInputDialog>
     *  bool ok;
     *  int sset = QInputDialog::getInt
     *  (
     *      this, tr("Import to Bank"),
     *      tr("Destination screen-set/bank"), 1, 0, usr().max_sets() - 1, 1, &ok
     *  );
     */

    bool ok = filePaths.length() > 0;
    if (ok)
    {
        for (int i = 0; i < filePaths.length(); ++i)
        {
            QString path = mImportDialog->selectedFiles()[i];
            if (! path.isEmpty())
            {
                try
                {
                    std::string fn = path.toStdString();
                    bool is_wrk = file_extension_match(fn, "wrk");
                    midifile * f = is_wrk ?
                        new wrkfile(fn) : new midifile(fn, ppqn()) ;

                    f->parse(perf(), perf().screenset());
                    ui->spinBpm->setValue(perf().bpm());
                    ui->spinBpm->setDecimals(usr().bpm_precision());
                    ui->spinBpm->setSingleStep(usr().bpm_step_increment());
                    if (not_nullptr(m_live_frame))
                        m_live_frame->set_bank(perf().screenset());
                }
                catch (...)
                {
                    QString msg_text =
                        "Error reading MIDI data from file: " + path;

                    m_msg_error->showMessage(msg_text);
                    m_msg_error->exec();
                }
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
 *  Loads the older Kepler34 pattern editor (qseqeditframe) for the selected
 *  sequence into the "Edit" tab.
 *
 *  We wanted to load the newer version, which has more functions, but it
 *  works somewhat differently, and cannot be fit into the current main window
 *  without enlarging it.  Therefore, we use the new version, qsegeditframe64,
 *  only in the external mode.
 *
 * \param seqid
 *      The slot value (0 to 1024) of the sequence to be edited.
 *
 * \warning
 *      Somehow, checking for not_nullptr(m_edit_frame) to determine whether
 *      to remove or add that widget causes the edit frame to not get created,
 *      and then the Edit tab is empty.
 *
 *  There are still some issues here:
 *
 *      -   Making sure that there is only one sequence editor window/tab
 *          per pattern.
 *      -   Removing the current song editor window/tab when another MIDI file
 *          is loaded.
 */

void
qsmainwnd::load_editor (int seqid)
{
    edit_container::iterator ei = m_open_editors.find(seqid);
    if (ei == m_open_editors.end())                         /* 1 editor/seq */
    {
        ui->EditTabLayout->removeWidget(m_edit_frame);      /* no ptr check */
        if (not_nullptr(m_edit_frame))
            delete m_edit_frame;

        m_edit_frame = new qseqeditframe(perf(), seqid, ui->EditTab);
        ui->EditTabLayout->addWidget(m_edit_frame);
        m_edit_frame->show();
        ui->tabWidget->setCurrentIndex(2);
    }
}

/**
 *
 */

void
qsmainwnd::load_event_editor (int seqid)
{
    edit_container::iterator ei = m_open_editors.find(seqid);
    if (ei == m_open_editors.end())                         /* 1 editor/seq */
    {
        ui->EventTabLayout->removeWidget(m_event_frame);    /* no ptr check */
        if (not_nullptr(m_event_frame))
            delete m_event_frame;

        /*
         * First, make sure the sequence exists.  Consider creating it if it
         * does not exist.
         */

        if (perf().is_active(seqid))
        {
            m_event_frame = new qseqeventframe(perf(), seqid, ui->EventTab);
            ui->EventTabLayout->addWidget(m_event_frame);
            m_event_frame->show();
            ui->tabWidget->setCurrentIndex(3);
        }
    }
}

/**
 *  Opens an external window for editing the sequence.  This window
 *  (qseqeditex with an embedded qseqeditframe64) is much more like the Gtkmm
 *  seqedit window, and somewhat more functional.  It has no parent widget,
 *  otherwise the whole big dialog will appear inside that parent.
 *
 * \warning
 *      We have to make sure the pattern ID is valid.  Somehow, we can
 *      double-click on the qsmainwnd's set/bank roller and get this function
 *      called!  For now, we work around that bug.
 *
 * \param seqid
 *      The slot value (0 to 1024) of the sequence to be edited.
 */

void
qsmainwnd::load_qseqedit (int seqid)
{
    if (perf().is_seq_valid(seqid))
    {
        edit_container::iterator ei = m_open_editors.find(seqid);
        if (ei == m_open_editors.end())
        {
            /*
             * First, make sure the sequence exists.  We should consider
             * creating it if it does not exist.  So many features, so little
             * time.
             */

            if (perf().is_active(seqid))
            {
                qseqeditex * ex = new qseqeditex(perf(), seqid, this);
                if (not_nullptr(ex))
                {
                    ex->show();
#ifdef PLATFORM_CPP_11
                    std::pair<int, qseqeditex *> p = std::make_pair(seqid, ex);
#else
                    std::pair<int, qseqeditex *> p =
                        std::make_pair<int, qseqeditex *>(seqid, ex);
#endif
                    m_open_editors.insert(p);
                }
            }
        }
    }
}

/**
 *  Removes the editor window from the list.  This function is called by the
 *  editor window to tell its parent (this) that it is going away.  Note that
 *  this does not delete the editor, it merely removes the pointer to it.
 *
 * \param seqid
 *      The sequence number that the editor represented.
 */

void
qsmainwnd::remove_editor (int seqid)
{
    edit_container::iterator ei = m_open_editors.find(seqid);
    if (ei != m_open_editors.end())
    {
        m_open_editors.erase(ei);

        /*
         * Deleting this pointer makes qseq66 segfault, and valgrind doesn't
         * seem to show any leak.  Commented out. Backported from Seq66.
         *
         * qseqeditex * qep = ei->second;   // save the pointer
         * if (not_nullptr(qep))
         *     delete qep;                  // delete the pointer
         */
    }
}

/**
 *  Uses the standard "associative-container erase-remove idiom".  Otherwise,
 *  the current iterator is invalid, and a segfault results in the top of the
 *  for-loop.  Another option with C++11 is "ci = m_open_editors.erase(ei)".
 */

void
qsmainwnd::remove_all_editors ()
{
    edit_container::iterator ei;
    for (ei = m_open_editors.begin(); ei != m_open_editors.end(); /*++ei*/)
    {
        qseqeditex * qep = ei->second;      /* save the pointer             */
        m_open_editors.erase(ei++);         /* remove pointer, inc iterator */
        if (not_nullptr(qep))
            delete qep;                     /* delete the pointer           */
    }
}

/**
 * \param on
 *      Disabled for now.
 */

void
qsmainwnd::load_qperfedit (bool /*on*/)
{
    if (is_nullptr(m_perfedit))
    {
        qperfeditex * ex = new qperfeditex(perf(), this);
        if (not_nullptr(ex))
        {
            m_perfedit = ex;
            hide_qperfedit(false);

            /*
             * Leave it enabled now to do show versus hide to avoid a weird
             * segfault.
             *
             * ui->btnPerfEdit->setEnabled(false);
             */
        }
    }
    else
    {
        hide_qperfedit();
    }
}

/**
 *  Shows or hides the external performance editor window.  We use to just
 *  delete it, but somehow this started causing a segfault when X-ing
 *  (closing) that window.  So now we just keep it around until the
 *  application is exited.
 *
 * \param hide
 *      If true, the performance editor is unconditionally hidden.  Otherwise,
 *      it is shown if hidden, or hidden if showing.  The default value is
 *      false.
 */

void
qsmainwnd::hide_qperfedit (bool hide)
{
    if (not_nullptr(m_perfedit))
    {
        if (hide)
        {
            m_perfedit->hide();
            m_perf_frame_visible = false;
        }
        else
        {
            if (m_perf_frame_visible)
                m_perfedit->hide();
            else
                m_perfedit->show();

            m_perf_frame_visible = ! m_perf_frame_visible;
        }
    }
}

/**
 *  Removes the single song editor window.  This function is called by the
 *  editor window to tell its parent (this) that it is going away.
 */

void
qsmainwnd::remove_qperfedit ()
{
    if (not_nullptr(m_perfedit))
    {
        qperfeditex * tmp = m_perfedit;
        m_perfedit = nullptr;
        delete tmp;
        ui->btnPerfEdit->setEnabled(true);
    }
}

/**
 *  Opens an external live frame.
 *  It has no parent widget,
 *  otherwise the whole big dialog will appear inside that parent.
 *
 * \param ssnum
 *      The screen-set value (0 to 31) of the live-frame to be displayed.
 */

void
qsmainwnd::load_live_frame (int ssnum)
{
    if (ssnum >= 0 && ssnum < 32)               // FIX LATER
    {
        live_container::iterator ei = m_open_live_frames.find(ssnum);
        if (ei == m_open_live_frames.end())
        {
            qliveframeex * ex = new qliveframeex(perf(), ssnum, this);
            if (not_nullptr(ex))
            {
                ex->show();
#ifdef PLATFORM_CPP_11
                std::pair<int, qliveframeex *> p = std::make_pair(ssnum, ex);
#else
                std::pair<int, qliveframeex *> p =
                    std::make_pair<int, qliveframeex *>(ssnum, ex);
#endif
                m_open_live_frames.insert(p);
            }
        }
    }
}

/**
 *  Removes the live frame window from the list.  This function is called by
 *  the live frame window to tell its parent (this) that it is going away.
 *  Note that this does not delete the editor, it merely removes the pointer
 *  to it.
 *
 * \param ssnum
 *      The screen-set number that the live frame represented.
 */

void
qsmainwnd::remove_live_frame (int ssnum)
{
    live_container::iterator ei = m_open_live_frames.find(ssnum);
    if (ei != m_open_live_frames.end())
        m_open_live_frames.erase(ei);
}

/**
 *  Uses the standard "associative-container erase-remove idiom".  Otherwise,
 *  the current iterator is invalid, and a segfault results in the top of the
 *  for-loop.  Another option with C++11 is "ci = m_open_editors.erase(ei)".
 */

void
qsmainwnd::remove_all_live_frames ()
{
    live_container::iterator ei;
    for
    (
        ei = m_open_live_frames.begin(); ei != m_open_live_frames.end(); /*++ei*/
    )
    {
        qliveframeex * lep = ei->second;    /* save the pointer             */
        m_open_live_frames.erase(ei++);     /* remove pointer, inc iterator */
        if (not_nullptr(lep))
            delete lep;                     /* delete the pointer           */
    }
}

/**
 *
 */

void
qsmainwnd::update_beat_length (int blIndex)
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

    if (not_nullptr(m_song_frame64))
        m_song_frame64->set_beat_width(bl);

    if (not_nullptr(m_beat_ind))
        m_beat_ind->set_beat_width(bl);

    for (int i = 0; i < c_max_sequence; ++i) // set beat length, all sequences
    {
        sequence * seq = perf().get_sequence(i);
        if (not_nullptr(seq))
        {
            /*
             * Set beat width, then reset the number of measures, causing
             * length to adjust to the new beats per measure.
             */

            seq->set_beat_width(bl);
            seq->set_measures(seq->get_measures());
        }
    }
    if (not_nullptr(m_edit_frame))
        m_edit_frame->update_draw_geometry();
}

/**
 *
 *  Also set beat length for all sequences.
 *  Reset number of measures, causing length to adjust to new b/m.
 */

void
qsmainwnd::updatebeats_per_measure(int bmindex)
{
    int bm = bmindex + 1;
    if (not_nullptr(m_beat_ind))
        m_beat_ind->set_beats_per_measure(bm);

    perf().set_beats_per_bar(bmindex + 1);
    for (int i = 0; i < c_max_sequence; ++i)
    {
        sequence * seq = perf().get_sequence(i);
        if (not_nullptr(seq))
        {
            seq->set_beats_per_bar(bmindex + 1);
            seq->set_measures(seq->get_measures());
        }
    }
    if (not_nullptr(m_edit_frame))
        m_edit_frame->update_draw_geometry();
}

/**
 *  If we've selected the edit tab, make sure it has something to edit.
 *
 * \warning
 *      Somehow, checking for not_nullptr(m_edit_frame) to determine whether
 *      to remove or add that widget causes the edit frame to not get created,
 *      and then the Edit tab is empty.
 */

void
qsmainwnd::tabWidgetClicked (int newIndex)
{
    bool isnull = is_nullptr(m_edit_frame);
    if (newIndex == 2 && isnull)
    {
        int seqid = -1;
        for (int i = 0; i < c_max_sequence; ++i)
        {
            if (perf().is_active(i))
            {
                seqid = i;
                break;
            }
        }
        if (seqid == -1)                /* no sequence found, make new one  */
        {
            (void) perf().new_sequence(0);
            seqid = 0;
        }

        sequence * seq = perf().get_sequence(seqid);
        if (not_nullptr(seq))
        {
            seq->set_dirty();
            m_edit_frame = new qseqeditframe(perf(), seqid, ui->EditTab);
            ui->EditTabLayout->addWidget(m_edit_frame); /* no ptr check */
            m_edit_frame->show();
            update();
        }
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
 *  Deletes the recent-files menu and recreates it, insert it into the File
 *  menu.  Not sure why we picked create_action_menu() for the name of this
 *  function.
 */

void
qsmainwnd::create_action_menu ()
{
    if (not_nullptr(m_menu_recent) && m_menu_recent->isWidgetType())
        delete m_menu_recent;

    m_menu_recent = new QMenu(tr("&Recent MIDI Files..."), this);
    for (int i = 0; i < mc_max_recent_files; ++i)
    {
        m_menu_recent->addAction(m_recent_action_list.at(i));
    }
    ui->menuFile->insertMenu(ui->actionSave, m_menu_recent);
}

/**
 *  Opens the selected recent file.
 */

void
qsmainwnd::open_recent_file ()
{
    QAction * action = qobject_cast<QAction *>(sender());
    if (not_nullptr(action) && check())
    {
        QString fname = QVariant(action->data()).toString();
        std::string actionfile = fname.toStdString();
        if (! actionfile.empty())
            open_file(actionfile);
    }
}

/**
 *  Calls check(), and if it checks out (heh heh), remove all of the editor
 *  windows and then calls for an exit of the application.
 */

void
qsmainwnd::quit ()
{
    if (check())
    {
        remove_all_editors();
        QCoreApplication::exit();
    }
}

/**
 *  Sets the perform object's song recording snap value.
 *
 * \param snap
 *      Provides the desired snap value.
 */

void
qsmainwnd::set_recording_snap (bool snap)
{
    /*
     * This will always be in force.
     *
     * perf().song_record_snap(snap);
     */
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

    bool done = false;
    keystroke k(gdkkey, SEQ64_KEYSTROKE_PRESS);
    if (perf().playback_key_event(k))               // song mode parameter?
    {
        if (perf().is_running())
        {
            ui->btnPlay->setChecked(false);         // stop_playing();
        }
        else
        {
            ui->btnPlay->setChecked(true);          // start_playing();
        }
        done = true;
    }
    else
    {
        std::string msgout;
        done = m_live_frame->handle_group_learn(k, msgout);
        if (! msgout.empty())
        {
            if (done)                           // info message
            {
                QMessageBox * msg = new QMessageBox(this);
                msg->setText(tr(msgout.c_str()));
                msg->setInformativeText(tr("Click OK to continue."));
                msg->setStandardButtons(QMessageBox::Ok);
                msg->exec();
            }
            else                                // error message
            {
                QErrorMessage * errmsg = new QErrorMessage(this);
                errmsg->showMessage(tr(msgout.c_str()));
                errmsg->exec();
                done = true;                    // key handled nonetheless
            }
        }
        if (! done)
        {
            if (k.is(SEQ64_Right))
            {
                (void) perf().open_next_song();
                m_is_title_dirty = true;
                done = true;
            }
            else if (k.is(SEQ64_Left))
            {
                (void) perf().open_previous_song();
                m_is_title_dirty = true;
                done = true;
            }
            else if (k.is(SEQ64_Down))
            {
                (void) perf().open_next_list();
                m_is_title_dirty = true;
                done = true;
            }
            else if (k.is(SEQ64_Up))
            {
                (void) perf().open_previous_list();
                m_is_title_dirty = true;
                done = true;
            }
        }

        if (! done && not_nullptr(m_live_frame))
            done = m_live_frame->handle_key_press(gdkkey);

        /*
         * If you reimplement this handler, it is very important that you call
         * the base class implementation if you do not act upon the key.
         */

        if (! done)
            QWidget::keyPressEvent(event);          // event->ignore();
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

/**
 *
 */

void
qsmainwnd::connect_editor_slots ()
{
    connect         // connect to sequence-edit signal from the Live tab
    (
        m_live_frame, SIGNAL(callEditor(int)),
        this, SLOT(load_editor(int))
    );
    connect         // new standalone sequence editor
    (
        m_live_frame, SIGNAL(callEditorEx(int)),
        this, SLOT(load_qseqedit(int))
    );

    /*
     * Event editor callback.  There is only one, for editing in the tab.
     * The event editor is meant for light use only at this time.
     */

    connect         // connect to sequence-edit signal from the Event tab
    (
        m_live_frame, SIGNAL(callEditorEvents(int)),
        this, SLOT(load_event_editor(int))
    );
}


/**
 *  Opens the Performance Editor (Song Editor).
 *
 *  We will let perform keep track of modifications, and not just set an
 *  is-modified flag just because we opened the song editor.  We're going to
 *  centralize the modification flag in the perform object, and see if it can
 *  work.
 */

void
qsmainwnd::open_performance_edit ()
{
    if (not_nullptr(m_perfedit))
    {
        if (m_perfedit->isVisible())
            m_perfedit->hide();
        else
            m_perfedit->show();
    }
    else
        load_qperfedit(true);
}

/**
 *  Apply full song transposition, if enabled.  Then reset the perfedit
 *  transpose setting to 0.
 */

void
qsmainwnd::apply_song_transpose ()
{
    if (perf().get_transpose() != 0)
    {
        perf().apply_song_transpose();
#ifdef USE_THIS_CODE_IT_IS_READY
        m_perfedit->set_transpose(0);
#endif
    }
}

/**
 *  Reload all mute-group settings from the "rc" file.
 */

void
qsmainwnd::reload_mute_groups ()
{
    std::string errmessage;
    bool result = perf().reload_mute_groups(errmessage);
    if (! result)
    {
#ifdef USE_THIS_CODE_IT_IS_READY
        Gtk::MessageDialog dialog           /* display the error-message    */
        (
            *this, "reload of mute groups", false,
            Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
        );
        dialog.set_title("Mute Groups");
        dialog.set_secondary_text("Failed", false);
        dialog.run();
#endif
    }
}

/**
 *  Clear all mute-group settings.  Sets all values to false/zero.  Also,
 *  since the intent might be to clean up the MIDI file, the user is prompted
 *  to save.
 */

void
qsmainwnd::clear_mute_groups ()
{
    if (perf().clear_mute_groups())     /* did any mute statuses change?    */
    {
#ifdef USE_THIS_CODE_IT_IS_READY
        if (is_save())
        {
            if (perf().is_running())    /* \change ca 2016-03-19            */
                stop_playing();
        }
#endif
    }
}

/**
 *  Sets the song-mute mode.
 */

void
qsmainwnd::set_song_mute_on ()
{
    perf().set_song_mute(perform::MUTE_ON);
}

/**
 *  Sets the song-mute mode.
 */

void
qsmainwnd::set_song_mute_off ()
{
    perf().set_song_mute(perform::MUTE_OFF);
}

/**
 *  Sets the song-mute mode.
 */

void
qsmainwnd::set_song_mute_toggle ()
{
    perf().set_song_mute(perform::MUTE_TOGGLE);
}

/**
 *  Toggle the group-learn status.  Simply forwards the call to
 *  perform::learn_toggle().
 */

void
qsmainwnd::learn_toggle ()
{
    perf().learn_toggle();
    qt_set_icon
    (
        perf().is_group_learning() ? learn2_xpm : learn_xpm, ui->button_learn
    );
}

/**
 *  Implements the Tap button or Tap keystroke (currently hard-wired as F9).
 */

void
qsmainwnd::tap ()
{
    midibpm bpm = update_tap_bpm();
    set_tap_button(m_current_beats);
    if (m_current_beats > 1)                    /* first one is useless */
    {
        ui->spinBpm->setValue(double(bpm));
    }
}

/**
 *  Sets the label in the Tap button to the given number of taps.
 *
 * \param beats
 *      The current number of times the user has clicked the Tap button/key.
 */

void
qsmainwnd::set_tap_button (int beats)
{
    char temp[8];
    snprintf(temp, sizeof temp, "%d", beats);
    ui->button_tap_bpm->setText(temp);
}

/**
 *  Calculates the running BPM value from the user's sequences of taps.
 *
 * \return
 *      Returns the current BPM value.
 */

midibpm
qsmainwnd::update_tap_bpm ()
{
    midibpm bpm = 0.0;
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long ms = long(spec.tv_sec) * 1000;     /* seconds to milliseconds      */
    ms += round(spec.tv_nsec * 1.0e-6);     /* nanoseconds to milliseconds  */
    if (m_current_beats == 0)
    {
        m_base_time_ms = ms;
        m_last_time_ms = 0;
    }
    else if (m_current_beats >= 1)
    {
        int diffms = ms - m_base_time_ms;
        bpm = m_current_beats * 60000.0 / diffms;
        m_last_time_ms = ms;
    }
    ++m_current_beats;
    return bpm;
}

/**
 *  Implements the keep-queue button.
 */

void
qsmainwnd::queue_it ()
{
    bool is_active = ui->button_keep_queue->isChecked();
    perf().set_keep_queue(is_active);
}

/**
 *  This is not called when focus changes.  Instead, we have to call this from
 *  qliveframeex::changeEvent().
 */

void
qsmainwnd::changeEvent (QEvent * event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow())
        {
            if (not_nullptr(m_live_frame))
                perf().set_screenset(m_live_frame->bank());
        }
        else
        {
            // widget is now inactive
        }
    }
}

/**
 *
 */

void
qsmainwnd::resizeEvent (QResizeEvent * r)
{
    // useful?
}

}               // namespace seq64

/*
 * qsmainwnd.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

