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
 * \updates       2018-03-05
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 */

#include <QMainWindow>
#include <QFileDialog>
#include <QErrorMessage>
#include <QTimer>
#include <QMessageBox>
#include <QDesktopWidget>

#include "midifile.hpp"

// Not yet useful:
// #include "qseqstyle.hpp"

namespace Ui
{
    class qsmainwnd;
}

namespace seq64
{
    class perform;
    class qsliveframe;
    class qperfeditframe;
    class qseqeditframe;
    class qsmaintime;
    class qseditoptions;
    class qsabout;

/**
 * The main window of kepler34
 */

class qsmainwnd : public QMainWindow
{
    Q_OBJECT

public:

    explicit qsmainwnd (perform & p, QWidget * parent = 0);
    ~qsmainwnd();

    // open the file at the given path

    void openmidifile (const QString & path);

protected:

    // override keyboard events for interaction

    void keyPressEvent (QKeyEvent * event);

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

    // check if the file has been modified.
    // if modified, ask the user whether to save changes

    bool saveCheck ();
    void updateWindowTitle ();
    void updateRecentFilesMenu ();

private:

    Ui::qsmainwnd * ui;
    qsliveframe * m_live_frame;
    qperfeditframe * m_song_frame;
    qseqeditframe  * m_edit_frame;
    QErrorMessage * m_msg_error;
    QMessageBox * m_msg_save_changes;
    QTimer * m_timer;
    QAction * mRecentFileActions[10];
    QMenu * mRecentMenu;
    QFileDialog * mImportDialog;
    perform & m_main_perf;
    qsmaintime * m_beat_ind;
    qseditoptions * m_dialog_prefs;
    qsabout * mDialogAbout;
    bool m_modified; //TODO move this into main performance

private slots:

    void startPlaying ();
    void stopPlaying ();
    void setSongPlayback (bool playSongData);
    void setRecording (bool record);
    void setRecordingSnap (bool snap);
    void panic ();
    void updateBpm (int newBpm);
    void updatebeats_per_measure (int bmIndex);
    void updateBeatLength (int blIndex);
    void newFile ();
    bool saveFile ();
    void saveFileAs ();
    void quit ();
    void showImportDialog (); //import MIDI data from current bank onwards
    void showOpenFileDialog ();
    void showqsabout ();
    void tabWidgetClicked (int newIndex);
    void load_recent_1 ();
    void load_recent_2 ();
    void load_recent_3 ();
    void load_recent_4 ();
    void load_recent_5 ();
    void load_recent_6 ();
    void load_recent_7 ();
    void load_recent_8 ();
    void load_recent_9 ();
    void load_recent_10 ();

    void refresh(); // redraw certain GUI elements

    // set the editor to a specific seq and switch tab to it

    void loadEditor(int seqId);

};          // class qsmainwnd

}           // namespace seq64

#endif      // SEQ64_QSMAINWND_HPP

/*
 * qsmainwnd.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

