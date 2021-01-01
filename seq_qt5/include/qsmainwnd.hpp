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
 * \updates       2018-11-04
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
#include "midibyte.hpp"                 /* typedef midibpm              */

/*
 *  Forward declaration.
 */

class QCloseEvent;
class QErrorMessage;
class QFileDialog;
class QMessageBox;
class QResizeEvent;
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
    class qliveframeex;
    class qperfeditex;
    class qperfeditframe64;
    class qplaylistframe;
    class qseqeditex;
    class qseqeditframe;
    class qseqeventframe;
    class qsliveframe;
    class qsmaintime;
    class qseditoptions;
    class qsabout;
    class qsbuildinfo;

/**
 * The main window of Kepler34.
 */

class qsmainwnd : public QMainWindow
{
    friend class qsliveframe;           /* semantically a "child" class */

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
    void hide_qperfedit (bool hide = false);
    void remove_live_frame (int ssnum);

    /**
     * \getter m_ppqn
     */

    int ppqn () const
    {
        return m_ppqn;
    }

    /**
     *
     */

    void open_playlist ()
    {
        show_open_list_dialog();
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
    virtual void changeEvent (QEvent *);
    virtual void resizeEvent (QResizeEvent *);

    void make_perf_frame_in_tab ();

    /*
     * Check if the file has been modified.  If modified, ask the user whether
     * to save changes.
     */

    bool check ();
    std::string filename_prompt (const std::string & prompt);
    void update_window_title (const std::string & fn = "");
    void update_recent_files_menu ();
    void create_action_connections ();
    void create_action_menu ();
    void remove_all_editors ();
    void remove_all_live_frames ();
    void connect_editor_slots ();
    void set_tap_button (int beats);
    midibpm update_tap_bpm ();

private:

    /**
     *  A typedef for keeping track of externa sequence edits.
     */

    typedef std::map<int, qseqeditex *> edit_container;

    /**
     *  A typedef for keeping track of external live-frames.
     */

    typedef std::map<int, qliveframeex *> live_container;

private:

    Ui::qsmainwnd * ui;
    qsliveframe * m_live_frame;
    qperfeditex * m_perfedit;
    qperfeditframe64 * m_song_frame64;
    qseqeditframe * m_edit_frame;
    qseqeventframe * m_event_frame;
    qplaylistframe * m_playlist_frame;
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
     *  Indicates the number of beats considered in calculating the BPM via
     *  button tapping.  This value is displayed in the button.
     */

    int m_current_beats;

    /**
     *  Indicates the first time the tap button was ... tapped.
     */

    long m_base_time_ms;

    /**
     *  Indicates the last time the tap button was tapped.  If this button
     *  wasn't tapped for awhile, we assume the user has been satisfied with
     *  the tempo he/she tapped out.
     */

    long m_last_time_ms;

    /**
     *  Holds a list of the sequences currently under edit.  We do not want to
     *  open the same sequence in two different editors.  Also, we need to be
     *  able to delete any open qseqeditex windows when exiting the
     *  application.
     */

    edit_container m_open_editors;

    /**
     *  Holds a list of open external qliveframeex objects.
     */

    live_container m_open_live_frames;

    /**
     *  Indicates the visibility of the external performance-edit frame.
     */

    bool m_perf_frame_visible;

private slots:

    void start_playing ();
    void pause_playing ();
    void stop_playing ();
    void set_song_mode (bool song_mode);
    void set_recording (bool record);
    void set_recording_snap (bool snap);
    void panic ();
    void update_bpm (double bpm);
    void edit_bpm ();
    void updatebeats_per_measure (int bmIndex);
    void update_beat_length (int blIndex);
    void open_recent_file ();
    void new_file ();
    bool save_file (const std::string & fname = "");
    bool save_file_as ();
    bool export_file_as_midi (const std::string & fname = "");
    bool export_song (const std::string & fname = "");
    void quit ();
    void show_import_dialog ();           /* import MIDI into current bank    */
    void show_open_file_dialog ();
    void show_open_list_dialog ();
    void showqsabout ();
    void showqsbuildinfo ();
    void tabWidgetClicked (int newindex);
    void refresh ();                    /* redraw certain GUI elements      */
    void load_editor (int seqid);
    void load_event_editor (int seqid);
    void load_qseqedit (int seqid);
    void load_qperfedit (bool on);
    void load_live_frame (int ssnum);
    void toggle_time_format (bool on);
    void open_performance_edit ();
    void apply_song_transpose ();
    void reload_mute_groups ();
    void clear_mute_groups ();
    void set_song_mute_on ();
    void set_song_mute_off ();
    void set_song_mute_toggle ();
    void learn_toggle ();
    void tap ();
    void queue_it ();

};          // class qsmainwnd

}           // namespace seq64

#endif      // SEQ64_QSMAINWND_HPP

/*
 * qsmainwnd.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

