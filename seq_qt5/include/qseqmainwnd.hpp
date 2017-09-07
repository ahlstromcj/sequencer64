#ifndef SEQ64_QSEQMAINWND_HPP
#define SEQ64_QSEQMAINWND_HPP

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
 * \file          qseqmainwnd.hpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2017-07-25
 * \license       GNU GPLv2 or above
 *
 */

#include <QtWidgets/QMainWindow>

// #include <QtCore/QDebug>
// #include <QtWidgets/QFileDialog>
// #include <QtWidgets/QErrorMessage>
// #include <QtCore/QTimer>
// #include <QtWidgets/QMessageBox>
// #include <QtWidgets/QDesktopWidget>


namespace Ui
{
    class qseqmainwnd;
}

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

    class perform;

/**
 *  The qseqmainwnd class supports the main window of the Qt version of
 *  Sequencer64.
 */

class qseqmainwnd : public QMainWindow
{
    Q_OBJECT

private:

    // check if the file has been modified.
    // if modified, ask the user whether to save changes
    // bool saveCheck();

    // update window title from the global filename
    // void updateWindowTitle();

    // update the recent files menu
    // void updateRecentFilesMenu();

    Ui::qseqmainwnd * ui;

    // LiveFrame           * m_live_frame;
    // SongFrame           * m_song_frame;
    // EditFrame           * m_edit_frame;

    // QErrorMessage       * m_msg_error;
    // QMessageBox         * m_msg_save_changes;
    // QTimer              * m_timer;
    // QAction             * mRecentFileActions[10];
    // QMenu               * mRecentMenu;
    // QFileDialog         * mImportDialog;

    // MidiPerformance     * m_main_perf;
    // BeatIndicator       * m_beat_ind;
    // PreferencesDialog   * m_dialog_prefs;
    // AboutDialog         * mDialogAbout;

    //TODO fully move this into main performance
    // bool                 m_modified;

private slots:

/*
    void startPlaying();
    void stopPlaying();
    void setSongPlayback(bool playSongData);
    void setRecording(bool record);
    void setRecordingSnap(bool snap);
    void panic();
    void updateBpm(int newBpm);
    void updateBeatsPerMeasure(int bmIndex);
    void updateBeatLength(int blIndex);
    void newFile();
    bool saveFile();
    void saveFileAs();
    void quit();
    void showImportDialog(); //import MIDI data from current bank onwards
    void showOpenFileDialog();
    void showAboutDialog();
    void tabWidgetClicked(int newIndex);
    void load_recent_1();
    void load_recent_2();
    void load_recent_3();
    void load_recent_4();
    void load_recent_5();
    void load_recent_6();
    void load_recent_7();
    void load_recent_8();
    void load_recent_9();
    void load_recent_10();

    //redraw certain GUI elements
    void refresh();

    //set the editor to a specific seq
    //and switch tab to it
    void loadEditor(int seqId);
*/

public:

    explicit qseqmainwnd (QWidget * parent = 0, perform * p = 0);
    ~qseqmainwnd ();

    // open the file at the given path
    // void openMidiFile(const QString& path);

protected:

    // override keyboard events for interaction
    // void keyPressEvent(QKeyEvent * event);

};          // qseqmainwnd

}           // namespace seq64

#endif      // SEQ64_QSEQMAINWND_HPP

/*
 * qseqmainwnd.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

