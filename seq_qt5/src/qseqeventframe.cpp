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
 * \file          qseqeventframe.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-08-13
 * \updates       2018-08-14
 * \license       GNU GPLv2 or above
 *
 */

#include "qseqeventframe.hpp"
#include "settings.hpp"                 /* SEQ64_QMAKE_RULES indirectly     */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qseqeventframe.h"
#else
#include "forms/qseqeventframe.ui.h"
#endif

/*
 *  Do not document the name space.
 */

namespace seq64
{

/**
 *
 */

qseqeventframe::qseqeventframe (QWidget * parent)
  :
    QFrame     (parent),
    ui         (new Ui::qseqeventframe)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /*
     * Event Table and scroll area.  Some setup is already done in the
     * setupUi() function as configured via Qt Creator.
     *
     * "This" object is the parent of eventScrollArea.  The scroll-area
     * contains scrollAreaWidgetContents, which is the parent of
     * eventTableWidget.
     *
     * Note that setRowHeight() will need to be called for any new rows that
     * get added to the table.  However, there is no function for that!
     *
     * ui->eventTableWidget->setRowHeight(16);
     */

    QStringList columns;
    columns << "Time" << "Event" << "Chan" << "Data 1" << "Data 2";
    ui->eventTableWidget->setHorizontalHeaderLabels(columns);
    setRowHeights(16);
    setColumnWidths(ui->eventTableWidget->width() - 32);

    /*
     * Items to be set up.
     *
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout_3;
    QLabel *label_seq_name;
    QLabel *label_time_sig;
    QLabel *label_ppqn;
    QLabel *label_channel;
    QLabel *label_ev_count;
    QLabel *label_seq_length;
    QLabel *label_modified;
    QLabel *label_category;
    QLineEdit *entry_ev_timestamp;
    QLineEdit *entry_ev_name;
    QLineEdit *entry_ev_data_0;
    QLineEdit *entry_ev_data_1;
    QPushButton *button_del;
    QPushButton *button_ins;
    QPushButton *button_modify;
    QPushButton *button_save;
    QPushButton *button_cancel;
     *
     */
}

/**
 *
 */

qseqeventframe::~qseqeventframe()
{
    delete ui;
}

/**
 *
 */

void
qseqeventframe::setRowHeights (int height)
{
    const int rows = ui->eventTableWidget->rowCount();
    for (int r = 0; r < rows; ++r)
    {
        ui->eventTableWidget->setRowHeight(r, height);
    }
}

/**
 *  Scales the columns against the provided window width.
 *
 *      100 + 164 + 72 + 72 + 72 = 480
 */

void
qseqeventframe::setColumnWidths (int total_width)
{
#ifdef PLATFORM_DEBUG
    printf("table width = %d\n", total_width);
#endif
    ui->eventTableWidget->setColumnWidth(0, int(0.20f * total_width));
    ui->eventTableWidget->setColumnWidth(1, int(0.35f * total_width));
    ui->eventTableWidget->setColumnWidth(2, int(0.15f * total_width));
    ui->eventTableWidget->setColumnWidth(3, int(0.15f * total_width));
    ui->eventTableWidget->setColumnWidth(4, int(0.15f * total_width));
}

}           // namespace seq64

/*
 * qseqeventframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

