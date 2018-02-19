#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
// #include <QDebug>
#include <QFileDialog>
#include <QErrorMessage>
#include <QTimer>
#include <QMessageBox>
#include <QDesktopWidget>

#include "qsliveframe.hpp"
#include "qperfeditframe.hpp"
#include "qseqeditframe.hpp"
#include "qseditoptions.hpp"
#include "perform.hpp"
#include "midifile.hpp"
#include "qsmaintime.hpp"
#include "qseqstyle.hpp"
#include "qsabout.hpp"

namespace Ui
{
    class qsmainwnd;
}

/**
 * The main window of kepler34
 */

class qsmainwnd : public QMainWindow
{
    Q_OBJECT

public:

    explicit qsmainwnd(QWidget *parent = 0,
                       perform *a_p = 0);
    ~qsmainwnd();

    //open the file at the given path
    void openmidifile(const QString& path);

protected:
    //override keyboard events for interaction
    void keyPressEvent(QKeyEvent * event);

private:

    //check if the file has been modified.
    //if modified, ask the user whether to save changes
    bool saveCheck();

    //update window title from the global filename
    void updateWindowTitle();

    //update the recent files menu
    void updateRecentFilesMenu();

    Ui::qsmainwnd      *ui;

    qsliveframe           *m_live_frame;
    qperfeditframe           *m_song_frame;
    qseqeditframe           *m_edit_frame;

    QErrorMessage       *m_msg_error;
    QMessageBox         *m_msg_save_changes;
    QTimer              *m_timer;
    QAction             *mRecentFileActions[10];
    QMenu               *mRecentMenu;
    QFileDialog         *mImportDialog;

    perform     *m_main_perf;
    qsmaintime       *m_beat_ind;
    qseditoptions   *m_dialog_prefs;
    qsabout         *mDialogAbout;

    //TODO fully move this into main performance
    bool                 m_modified;

private slots:
    void startPlaying();
    void stopPlaying();
    void setSongPlayback(bool playSongData);
    void setRecording(bool record);
    void setRecordingSnap(bool snap);
    void panic();
    void updateBpm(int newBpm);
    void updatebeats_per_measure(int bmIndex);
    void updateBeatLength(int blIndex);
    void newFile();
    bool saveFile();
    void saveFileAs();
    void quit();
    void showImportDialog(); //import MIDI data from current bank onwards
    void showOpenFileDialog();
    void showqsabout();
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
};

#endif // MAINWINDOW_HPP
