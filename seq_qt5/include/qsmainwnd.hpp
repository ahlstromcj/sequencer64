#ifndef SEQ64_QSMAINWND_HPP
#define SEQ64_QSMAINWND_HPP

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
 * \file          qsmainwnd.hpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-08-14
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 */

#include <QMainWindow>
#include <QList>

#include "app_limits.h"                 /* SEQ64_USE_DEFAULT_PPQN       */

/*
 *  Forward declaration.
 */

class QCloseEvent;
class QErrorMessage;
class QFileDialog;
class QMessageBox;
class QTimer;

/*
 *  The Qt UI namespace.
 */

namespace Ui
{
    class qsmainwnd;
}

/*
 *  Do not document namespaces, it seems to break Doxygen.
 */

namespace seq64
{
    class perform;
    class qsliveframe;
    class qperfeditex;
    class qperfeditframe64;
    class qseqeditex;
    class qseqeditframe;
    class qseqeventframe;
    class qsmaintime;
    class qseditoptions;
    class qsabout;
    class qsbuildinfo;

/**
 * The main window of kepler34
 */

class qsmainwnd : public QMainWindow
{
    Q_OBJECT

public:

    qsmainwnd
    (
        perform & p,
        const std::string & midifilename    = "",
        int ppqn                            = SEQ64_USE_DEFAULT_PPQN,
        QWidget * parent                    = nullptr
    );
    virtual ~qsmainwnd ();

    void open_file (const std::string & path);
    void show_message_box (const std::string & msg_text);
    void remove_editor (int seq);
    void remove_qperfedit ();

    /**
     * \getter m_ppqn
     */

    int ppqn () const
    {
        return m_ppqn;
    }

protected:

    /**
     * \setter m_ppqn
     *      We can't set the PPQN value when the mainwnd is created, we have
     *      to do it later, using this function.
     *
     *      m_ppqn = choose_ppqn(ppqn);
     */

    void ppqn (int ppqn)
    {
        m_ppqn = ppqn;
    }

protected:

    void keyPressEvent (QKeyEvent * event); /* override keyboard events */

protected:

    const perform & perf () const
    {
        return m_main_perf;
    }

    perform & perf ()
    {
        return m_main_perf;
    }

private:

    virtual void closeEvent (QCloseEvent *);

    void make_perf_frame_in_tab ();

    /*
     * Check if the file has been modified.  If modified, ask the user whether
     * to save changes.
     */

    bool check ();
    void update_window_title (const std::string & fn = "");
    void update_recent_files_menu ();

    void create_action_connections ();      // new
    void create_action_menu ();             // new
    void remove_all_editors ();

private:

    /**
     *  A typedef for keeping track of sequence edits.
     */

    typedef std::map<int, qseqeditex *> edit_container;

private:

    Ui::qsmainwnd * ui;
    qsliveframe * m_live_frame;
    qperfeditex * m_perfedit;
    qperfeditframe64 * m_song_frame64;
    qseqeditframe * m_edit_frame;
    qseqeventframe * m_event_frame;
    QErrorMessage * m_msg_error;
    QMessageBox * m_msg_save_changes;
    QTimer * m_timer;
    QMenu * m_menu_recent;
    QList<QAction *> m_recent_action_list;     // new
    const int mc_max_recent_files;
    QFileDialog * mImportDialog;
    perform & m_main_perf;
    qsmaintime * m_beat_ind;
    qseditoptions * m_dialog_prefs;
    qsabout * mDialogAbout;
    qsbuildinfo * mDialogBuildInfo;

    /**
     *  Provides a workaround for a race condition when a MIDI file-name is
     *  provided on the command line.  This would cause the title to be
     *  "unnamed".
     */

    bool m_is_title_dirty;

    /**
     *  Saves the PPQN value obtained from the MIDI file (or the default
     *  value, the global ppqn, if SEQ64_USE_DEFAULT_PPQN was specified in
     *  reading the MIDI file.  We need it early here to be able to pass it
     *  along to child objects.
     */

    int m_ppqn;

    /**
     *  Indicates whether to show the time as bar:beats:ticks or as
     *  hours:minutes:seconds.  The default is true:  bar:beats:ticks.
     */

    bool m_tick_time_as_bbt;

    /**
     *  Holds a list of the sequences currently under edit.  We do not want to
     *  open the same sequence in two different editors.  Also, we need to be
     *  able to delete any open qseqeditex windows when exiting the
     *  application.
     */

    edit_container m_open_editors;

private slots:

    void startPlaying ();
    void pausePlaying ();
    void stopPlaying ();
    void set_song_mode (bool song_mode);
    void toggle_song_mode ();
    void setRecording (bool record);
    void setRecordingSnap (bool snap);
    void panic ();
    void update_bpm (double bpm);
    void edit_bpm ();
    void updatebeats_per_measure (int bmIndex);
    void updateBeatLength (int blIndex);
    void open_recent_file ();   // new
    void new_file ();
    bool save_file ();
    void save_file_as ();
    void quit ();
    void showImportDialog ();           /* import MIDI into current bank    */
    void showOpenFileDialog ();
    void showqsabout ();
    void showqsbuildinfo ();
    void tabWidgetClicked (int newindex);
    void refresh ();                    /* redraw certain GUI elements      */
    void load_editor (int seqid);
    void load_event_editor (int seqid);
    void load_qseqedit (int seqid);
    void load_qperfedit (bool on);
    void toggle_time_format (bool on);

};          // class qsmainwnd

}           // namespace seq64

#endif      // SEQ64_QSMAINWND_HPP

/*
 * qsmainwnd.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

