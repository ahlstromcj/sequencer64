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
 * \updates       2018-09-04
 * \license       GNU GPLv2 or above
 *
 */

#include "perform.hpp"                  /* seq64::perform                   */
#include "qplaylistframe.hpp"

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qplaylistframe.h"
#else
#include "forms/qplaylistframe.ui.h"
#endif

/**
 *  Specifies the current hardwired value for set_row_heights().
 */

#define SEQ64_PLAYLIST_ROW_HEIGHT       16

namespace seq64
{

/**
 *
 */

qplaylistframe::qplaylistframe (perform & p, QWidget * parent)
 :
    QFrame      (parent),
    ui          (new Ui::qplaylistframe),
    m_perform   (p)
{
    ui->setupUi(this);

    QStringList playcolumns;
    playcolumns << "Index" << "List Name";
    ui->tablePlaylistSections->setHorizontalHeaderLabels(playcolumns);

    // set_column_widths(ui->tablePlaylistSection->width() - 10);

    QStringList songcolumns;
    songcolumns << "Index" << "Song File";
    ui->tablePlaylistSongs->setHorizontalHeaderLabels(songcolumns);

    // set_column_widths(ui->tablePlaylistSongs->width() - 10);

    set_row_heights(SEQ64_PLAYLIST_ROW_HEIGHT);
}

/**
 *
 */

qplaylistframe::~qplaylistframe ()
{
    delete ui;
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
 *

void
qplaylistframe::set_row_height (int row, int height)
{
    ui->eventTableWidget->setRowHeight(row, height);
}
 */

}           // namespace seq64

/*
 * qplaylistframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

