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
 * \updates       2018-09-06
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

    // QTableWidget *tablePlaylistSections;

    QStringList playcolumns;
    playcolumns << "Index" << "List Name";
    ui->tablePlaylistSections->setHorizontalHeaderLabels(playcolumns);

    // QTableWidget *tablePlaylistSongs;

    QStringList songcolumns;
    songcolumns << "Index" << "Song File";
    ui->tablePlaylistSongs->setHorizontalHeaderLabels(songcolumns);

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

#if 0
    QLineEdit *entry_playlist_file;

    QLabel *editPlaylistPath;
    QLabel *editPlaylistNumber;
    QLabel *editPlaylistName;

    QLabel *editSongPath;
    QLabel *editSongNumber;
    QLabel *editSongFilename;

    QTableWidget *tablePlaylistSections;
    QTableWidget *tablePlaylistSongs;

    QPushButton *buttonPlaylistLoad;
    QPushButton *buttonPlaylistAdd;
    QPushButton *buttonPlaylistRemove;
    QPushButton *buttonPlaylistSave;

    QPushButton *buttonSongLoad;
    QPushButton *buttonSongAdd;
    QPushButton *buttonSongRemove;

    QCheckBox *checkBoxPlaylistActive;
#endif  // 0

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
 *  update() only if necessary.  See qseqbase::needs_update().  For now, we
 *  just copped the code from that function, but have to check all sequences
 *  at some point.  LATER.
 */

void
qplaylistframe::conditional_update ()
{
    if (perf().needs_update(0))
    {
        // /* bool */ load_playlist (const std::string & fullfilespec)
        update();
    }
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

#if 0

/**
 *
 */

void
qplaylistframe::set_row_height (int row, int height)
{
    ui->eventTableWidget->setRowHeight(row, height);
}

#endif  // 0

/**
 *  Scales the columns against the provided window width.
 */

void
qplaylistframe::set_column_widths ()
{
    int w = ui->tablePlaylistSections->width();
    ui->tablePlaylistSections->setColumnWidth(0, int(0.15f * w));
    ui->tablePlaylistSections->setColumnWidth(1, int(0.85f * w));

    w = ui->tablePlaylistSongs->width();
    ui->tablePlaylistSongs->setColumnWidth(0, int(0.15f * w));
    ui->tablePlaylistSongs->setColumnWidth(1, int(0.85f * w));
}

/**
 *
 */

#if 0
    QLineEdit *entry_playlist_file;

    QLabel *editPlaylistPath;
    QLabel *editPlaylistNumber;
    QLabel *editPlaylistName;

    QLabel *editSongPath;
    QLabel *editSongNumber;
    QLabel *editSongFilename;

    QTableWidget *tablePlaylistSections;
    QTableWidget *tablePlaylistSongs;
    QCheckBox *checkBoxPlaylistActive;
#endif  // 0

/**
 *
 */

void
qplaylistframe::set_current_playlist ()
{
    std::string filename = perf().playlist_filename();
    ui->entry_playlist_file->setText(QString::fromStdString(filename));
}

/**
 *

void
qplaylistframe::set_current_song ()
{
    std::string filename = perf().playlist_filename();
    ui->entry_playlist_file->setText(QString::fromStdString(filename));
}
 */

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
        set_current_playlist();
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
    // m_eventslots->select_event(row);
    // set_current_row(row);
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
    // m_eventslots->select_event(row);
    // set_current_row(row);
}

}           // namespace seq64

/*
 * qplaylistframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

