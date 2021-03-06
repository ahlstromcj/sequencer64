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
 * \file          qplaylistframe.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-09-04
 * \updates       2018-11-11
 * \license       GNU GPLv2 or above
 *
 */

#include <QTimer>

#include "perform.hpp"                  /* seq64::perform                   */
#include "qplaylistframe.hpp"           /* seq64::qplaylistframe child      */
#include "qsmainwnd.hpp"                /* seq64::qsmainwnd, a parent       */
#include "settings.hpp"                 /* seq64::rc() and seq64::usr()     */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qplaylistframe.h"
#else
#include "forms/qplaylistframe.ui.h"
#endif

/**
 *  For correcting the width of the play-list tables.  It tries to account for
 *  the width of the vertical scroll-bar, plus a bit more.
 */

#define SEQ64_PLAYLIST_TABLE_FIX        24  // 48

/**
 *  Specifies the current hardwired value for set_row_heights().
 */

#define SEQ64_PLAYLIST_ROW_HEIGHT       18

namespace seq64
{

/**
 *
 */

qplaylistframe::qplaylistframe
(
    perform & p,
    qsmainwnd * window,
    QWidget * parent
) :
    QFrame      (parent),
    ui          (new Ui::qplaylistframe),
    m_timer     (nullptr),
    m_perform   (p),
    m_parent    (window)
{
    ui->setupUi(this);

    QStringList playcolumns;
    playcolumns << "MIDI#" << "List Name";
    ui->tablePlaylistSections->setHorizontalHeaderLabels(playcolumns);
    ui->tablePlaylistSections->setSelectionBehavior(QAbstractItemView::SelectRows);

    QStringList songcolumns;
    songcolumns << "MIDI#" << "Song File";
    ui->tablePlaylistSongs->setHorizontalHeaderLabels(songcolumns);
    ui->tablePlaylistSongs->setSelectionBehavior(QAbstractItemView::SelectRows);
    set_row_heights(SEQ64_PLAYLIST_ROW_HEIGHT);
    set_column_widths();
    connect
    (
        ui->tablePlaylistSections, SIGNAL(currentCellChanged(int, int, int, int)),
        this, SLOT(handle_list_click_ex(int, int, int, int))
    );
    connect
    (
        ui->tablePlaylistSongs, SIGNAL(currentCellChanged(int, int, int, int)),
        this, SLOT(handle_song_click_ex(int, int, int, int))
    );
    connect
    (
        ui->buttonPlaylistLoad, SIGNAL(clicked(bool)),
        this, SLOT(handle_list_load_click())
    );
    connect
    (
        ui->buttonPlaylistAdd, SIGNAL(clicked(bool)),
        this, SLOT(handle_list_add_click())
    );
    /*
    connect
    (
        ui->buttonPlaylistRemove, SIGNAL(clicked(bool)),
        this, SLOT(handle_list_remove_click())
    );
    */
    connect
    (
        ui->buttonPlaylistSave, SIGNAL(clicked(bool)),
        this, SLOT(handle_list_save_click())
    );
    connect
    (
        ui->buttonSongAdd, SIGNAL(clicked(bool)),
        this, SLOT(handle_song_add_click())
    );
    connect
    (
        ui->buttonSongRemove, SIGNAL(clicked(bool)),
        this, SLOT(handle_song_remove_click())
    );
    connect
    (
        ui->checkBoxPlaylistActive, SIGNAL(clicked(bool)),
        this, SLOT(handle_playlist_active_click())
    );
    if (perf().playlist_mode())
        reset_playlist();

    m_timer = new QTimer(this);        /* timer for regular redraws    */
    m_timer->setInterval(usr().window_redraw_rate());
    connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->start();
}

/**
 *
 */

qplaylistframe::~qplaylistframe ()
{
    delete ui;
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::needs_update().  We
 *  must check all sequences.
 */

void
qplaylistframe::conditional_update ()
{
    if (perf().needs_update())          /* potentially checks all sequences */
        update();
}

/**
 *
 */

void
qplaylistframe::set_row_heights (int height)
{
    const int prows = ui->tablePlaylistSections->rowCount();
    for (int pr = 0; pr < prows; ++pr)
        ui->tablePlaylistSections->setRowHeight(pr, height);

    const int rows = ui->tablePlaylistSongs->rowCount();
    for (int sr = 0; sr < rows; ++sr)
        ui->tablePlaylistSongs->setRowHeight(sr, height);
}

/**
 *  Scales the columns against the provided window width.
 */

void
qplaylistframe::set_column_widths ()
{
    int w = ui->tablePlaylistSections->width() - SEQ64_PLAYLIST_TABLE_FIX;
    ui->tablePlaylistSections->setColumnWidth(0, int(0.20f * w));
    ui->tablePlaylistSections->setColumnWidth(1, int(0.80f * w));

    w = ui->tablePlaylistSongs->width() - SEQ64_PLAYLIST_TABLE_FIX;
    ui->tablePlaylistSongs->setColumnWidth(0, int(0.20f * w));
    ui->tablePlaylistSongs->setColumnWidth(1, int(0.80f * w));
}

/**
 *  Resets the play-list.  First, resets to the first (0th) play-list and the first
 *  (0th) song.  Then fills the play-list items and resets again.  Then
 *  fills in the play-list and song items for the current selection.
 */

void
qplaylistframe::reset_playlist ()
{
    if (perf().playlist_reset())
    {
        fill_playlists();
        perf().playlist_reset();        /* back to the 0th play-list    */
        fill_songs();
        set_current_playlist();
        ui->tablePlaylistSections->selectRow(0);
        ui->tablePlaylistSongs->selectRow(0);
    }
}

/**
 *
 */

void
qplaylistframe::set_current_playlist ()
{
    if (perf().playlist_mode())
    {
        ui->checkBoxPlaylistActive->setChecked(true);

        std::string temp = perf().playlist_filename();
        ui->entry_playlist_file->setText(QString::fromStdString(temp));

        temp = perf().file_directory();
        ui->editPlaylistPath->setText(QString::fromStdString(temp));

        int midinumber = perf().playlist_midi_number();
        temp = std::to_string(midinumber);
        ui->editPlaylistNumber->setText(QString::fromStdString(temp));

        temp = perf().playlist_name();
        ui->editPlaylistName->setText(QString::fromStdString(temp));

        set_current_song();
    }
    else
    {
        ui->checkBoxPlaylistActive->setChecked(false);
    }
}

/**
 *
 */

void
qplaylistframe::set_current_song ()
{
    if (perf().playlist_mode())
    {
        std::string temp = std::to_string(perf().song_midi_number());
        ui->editSongNumber->setText(QString::fromStdString(temp));

        temp = perf().song_directory();
        ui->editSongPath->setText(QString::fromStdString(temp));

        bool embedded = perf().is_own_song_directory();
        temp = embedded ? "*" : " " ;
        ui->labelDirEmbedded->setText(QString::fromStdString(temp));

        temp = perf().song_filename();
        ui->editSongFilename->setText(QString::fromStdString(temp));
    }
}

/**
 *  Retrieve the table cell at the given row and column.
 *
 * \param isplaylist
 *      If true, this call affects the play-list table.  Otherwise, it affects
 *      the song-list table.
 *
 * \param row
 *      The row number, which should be in range.
 *
 * \param col
 *      The column enumeration value, which will be in range.
 *
 * \return
 *      Returns a pointer the table widget-item for the given row and column.
 *      If out-of-range, a null pointer is returned.
 */

QTableWidgetItem *
qplaylistframe::cell (bool isplaylist, int row, column_id_t col)
{
    int column = int(col);
    QTableWidget * listptr = isplaylist ?
        ui->tablePlaylistSections : ui->tablePlaylistSongs ;

    QTableWidgetItem * result = listptr->item(row, column);
    if (is_nullptr(result))
    {
        /*
         * Will test row/column and maybe add rows on the fly later.
         */

        result = new QTableWidgetItem;
        listptr->setItem(row, column, result);
    }
    return result;
}

/**
 *  Adds the list of playlists to the tablePlaylistSections table-widget.  It
 *  does not select a play-list or select a song.  To be called only when
 *  loading a new play-list, as in reset_playlist().  Only acts if the list is
 *  non-empty.
 */

void
qplaylistframe::fill_playlists ()
{
    int rows = perf().playlist_count();
    if (rows > 0)
    {
        ui->tablePlaylistSections->clearContents();
        ui->tablePlaylistSections->setRowCount(rows);
        for (int r = 0; r < rows; ++r)
        {
            std::string temp;
            QTableWidgetItem * qtip = cell(true, r, CID_MIDI_NUMBER);
            ui->tablePlaylistSections->setRowHeight(r, SEQ64_PLAYLIST_ROW_HEIGHT);
            if (not_nullptr(qtip))
            {
                int midinumber = perf().playlist_midi_number();
                temp = std::to_string(midinumber);
                qtip->setText(QString::fromStdString(temp));
            }
            qtip = cell(true, r, CID_ITEM_NAME);
            if (not_nullptr(qtip))
            {
                temp = perf().playlist_name();
                qtip->setText(QString::fromStdString(temp));
            }
            if (! perf().open_next_list(false))     /* false = no load song */
                break;
        }
    }
}

/**
 *  Adds the songs of the current playlist to the tablePlaylistSongs
 *  table-widget.  It does not select any song.
 */

void
qplaylistframe::fill_songs ()
{
    int rows = perf().song_count();
    if (rows > 0)
    {
        ui->tablePlaylistSongs->clearContents();
        ui->tablePlaylistSongs->setRowCount(rows);
        for (int r = 0; r < rows; ++r)
        {
            std::string temp;
            if (perf().open_select_song_by_index(r, false))
            {
                QTableWidgetItem * qtip = cell(false, r, CID_MIDI_NUMBER);
                ui->tablePlaylistSongs->setRowHeight
                (
                    r, SEQ64_PLAYLIST_ROW_HEIGHT
                );
                if (not_nullptr(qtip))
                {
                    int midinumber = perf().song_midi_number();
                    temp = std::to_string(midinumber);
                    qtip->setText(QString::fromStdString(temp));
                }
                qtip = cell(false, r, CID_ITEM_NAME);
                if (not_nullptr(qtip))
                {
                    temp = perf().song_filename();
                    qtip->setText(QString::fromStdString(temp));
                }
            }
            else
                break;
        }
    }
}

/**
 *
 */

bool
qplaylistframe::load_playlist (const std::string & fullfilespec)
{
    if (! fullfilespec.empty())
    {
        bool playlistmode = perf().open_playlist(fullfilespec);
        if (playlistmode)
            playlistmode = perf().open_current_song();
    }
    if (perf().playlist_mode())
    {
        reset_playlist();
    }
    else
    {
    }
    return false;
}

/**
 *
 */

void
qplaylistframe::handle_list_click_ex
(
    int row, int /*column*/, int /*prevrow*/, int /*prevcolumn*/
)
{
    if (row < 0)
    {
        warnprintf("List row %d\n", row);
    }
    else
    {
        if (perf().open_select_list_by_index(row, false))
        {
            fill_songs();
            set_current_playlist();
            ui->tablePlaylistSongs->selectRow(0);
        }
    }
}

/**
 *
 */

void
qplaylistframe::handle_song_click_ex
(
    int row, int /*column*/, int /*prevrow*/, int /*prevcolumn*/
)
{
    if (row < 0)
    {
        warnprintf("Song row %d!\n", row);
    }
    else
    {
        if (perf().open_select_song_by_index(row, true))
        {
            set_current_song();
        }
    }
}

/**
 *
 */

void
qplaylistframe::handle_list_load_click ()
{
    if (not_nullptr(m_parent))
        m_parent->open_playlist();
}

/**
 *
 */

void
qplaylistframe::handle_list_add_click ()
{
    // if (not_nullptr(m_parent))
    //    m_parent->open_playlist();
}

/**
 *
 */

void
qplaylistframe::handle_list_remove_click ()
{
    // if (not_nullptr(m_parent))
    //    m_parent->open_playlist();
}

/**
 *
 */

void
qplaylistframe::handle_list_save_click ()
{
    // if (not_nullptr(m_parent))
    //    m_parent->open_playlist();
}

/**
 *
 */

void
qplaylistframe::handle_song_add_click ()
{
    if (not_nullptr(m_parent))
    {
        /*
         * These values depend on correct information edited into the Song
         * text fields.  We should support loading a song from a
         * file-selection dialog.
         */

        std::string name = ui->editSongFilename->text().toStdString();
        std::string directory = ui->editSongPath->text().toStdString();
        std::string nstr = ui->editSongNumber->text().toStdString();
        int midinumber = std::stoi(nstr);
        int index = perf().song_count() + 1;
        if (! perf().add_song(index, midinumber, name, directory))
        {
            // report error
        }
    }


    //  m_parent->xxxxxxxxx();
}

/**
 *
 */

void
qplaylistframe::handle_song_remove_click ()
{
    // if (not_nullptr(m_parent))
    //    m_parent->open_playlist();
}

/**
 *
 */

void
qplaylistframe::handle_playlist_active_click ()
{
    // if (not_nullptr(m_parent))
    //    m_parent->open_playlist();
}

/**
 *
 */

void
qplaylistframe::keyPressEvent (QKeyEvent * event)
{
    QWidget::keyPressEvent(event);      // event->ignore();
}

/**
 *
 */

void
qplaylistframe::keyReleaseEvent (QKeyEvent * event)
{
    QWidget::keyReleaseEvent(event);    // event->ignore();
}

}           // namespace seq64

/*
 * qplaylistframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

