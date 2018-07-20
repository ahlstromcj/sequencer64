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
 * \file          qperfeditframe64.cpp
 *
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-07-18
 * \updates       2018-07-19
 * \license       GNU GPLv2 or above
 *
 *  Note that, as of version 0.9.11, the z and Z keys, when focus is on the
 *  perfroll (piano roll), will zoom the view horizontally.
 */

#include <QScrollBar>

#include "perform.hpp"
#include "qperfeditframe64.hpp"
#include "qperfroll.hpp"
#include "qperfnames.hpp"
#include "qperftime.hpp"
#include "qt5_helpers.hpp"              /* seq64::qt_set_icon()             */
#include "settings.hpp"                 /* usr()                            */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qperfeditframe64.h"
#else
#include "forms/qperfeditframe64.ui.h"
#endif

/*
 *  We prefer to load the pixmaps on the fly, rather than deal with those
 *  friggin' resource files.
 */

#include "pixmaps/collapse.xpm"
#include "pixmaps/copy.xpm"
#include "pixmaps/expand.xpm"
#include "pixmaps/follow.xpm"
#include "pixmaps/loop.xpm"
#include "pixmaps/redo.xpm"
#include "pixmaps/undo.xpm"
#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"

namespace seq64
{

/**
 *
 */

qperfeditframe64::qperfeditframe64 (seq64::perform & p, QWidget * parent)
 :
    QFrame                  (parent),
    ui                      (new Ui::qperfeditframe64),
    m_mainperf              (p),
    mContainer              (nullptr),
    m_palette               (nullptr),
    m_snap                  (8),
    mbeats_per_measure      (4),
    mbeat_width             (4),
    m_perfroll              (nullptr),
    m_perfnames             (nullptr),
    m_perftime              (nullptr)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /*
     * Snap.  Fill options for grid snap combo box and set the default.
     */

    for (int i = 0; i < 6; ++i)
    {
        QString combo_text = "1/" + QString::number(pow(2, i));
        ui->cmbGridSnap->insertItem(i, combo_text);
    }
    ui->cmbGridSnap->setCurrentIndex(3);
    connect
    (
        ui->cmbGridSnap, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updateGridSnap(int))
    );

    /*
     * Create and add the scroll-area and widget-container for this frame.
     * If we're using the qscrollmaster (a QScrollArea), the scroll-area will
     * contain only the qperfoll.  Otherwise, it will contain the qperfroll,
     * qperfnames, and qperftime.  In either case the widget-container contains
     * all three panels.
     *
     * Create the piano roll panel, the names panel, and time panel of the song
     * editor frame.
     */

    m_perfnames = new seq64::qperfnames
    (
        m_mainperf, ui->namesScrollArea     // this
    );
    ui->namesScrollArea->setWidget(m_perfnames);
    ui->namesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->namesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_perftime = new seq64::qperftime
    (
        m_mainperf, SEQ64_DEFAULT_ZOOM, SEQ64_DEFAULT_SNAP,
        SEQ64_DEFAULT_PPQN, ui->timeScrollArea     // this
    );
    ui->timeScrollArea->setWidget(m_perftime);
    ui->timeScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->timeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_perfroll = new seq64::qperfroll
    (
        m_mainperf, SEQ64_DEFAULT_ZOOM, SEQ64_DEFAULT_SNAP,
        SEQ64_DEFAULT_PPQN, this, ui->rollScrollArea
    );
    ui->rollScrollArea->setWidget(m_perfroll);
    ui->rollScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->rollScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    /*
     *  Add the various scrollbar points to the qscrollmaster object,
     *  m_scroll_area
     */

    ui->rollScrollArea->add_v_scroll(ui->namesScrollArea->verticalScrollBar());
    ui->rollScrollArea->add_h_scroll(ui->timeScrollArea->horizontalScrollBar());

    // mContainer = new QWidget(ui->rollScrollArea);

    /*
     * Create the color palette for coloring the patterns.
     */

    m_palette = new QPalette();
    m_palette->setColor(QPalette::Background, Qt::darkGray);
    // mContainer->setPalette(*m_palette);

    /*
     * Undo and Redo buttons.
     */

    connect(ui->btnUndo, SIGNAL(clicked(bool)), m_perfroll, SLOT(undo()));
    qt_set_icon(undo_xpm, ui->btnUndo);
    connect(ui->btnRedo, SIGNAL(clicked(bool)), m_perfroll, SLOT(redo()));
    qt_set_icon(redo_xpm, ui->btnRedo);

    /*
     * Follow Progress Button.
     */

    qt_set_icon(follow_xpm, ui->m_toggle_follow);
    ui->m_toggle_follow->setEnabled(true);
    ui->m_toggle_follow->setCheckable(true);
    ui->m_toggle_follow->setToolTip
    (
        "If active, the song roll scrolls to "
        "follow the progress bar in playback."
    );

    /*
     * Qt::NoFocus is the default focus policy.
     */

    ui->m_toggle_follow->setAutoDefault(false);
    ui->m_toggle_follow->setChecked(m_perfroll->progress_follow());
    connect(ui->m_toggle_follow, SIGNAL(toggled(bool)), this, SLOT(follow(bool)));

    /*
     * Zoom-In and Zoom-Out buttons.
     */

    connect(ui->btnZoomIn, SIGNAL(clicked(bool)), this, SLOT(zoom_in()));
    qt_set_icon(zoom_in_xpm, ui->btnZoomIn);
    connect(ui->btnZoomOut, SIGNAL(clicked(bool)), this, SLOT(zoom_out()));
    qt_set_icon(zoom_out_xpm, ui->btnZoomOut);

    /*
     * Collapse, Expand, Expand-Copy, and Loop buttons.
     */

    connect(ui->btnCollapse, SIGNAL(clicked(bool)), this, SLOT(markerCollapse()));
    qt_set_icon(collapse_xpm, ui->btnCollapse);
    connect(ui->btnExpand, SIGNAL(clicked(bool)), this, SLOT(markerExpand()));
    qt_set_icon(expand_xpm, ui->btnExpand);
    connect
    (
        ui->btnExpandCopy, SIGNAL(clicked(bool)), this, SLOT(markerExpandCopy())
    );
    qt_set_icon(copy_xpm, ui->btnExpandCopy);
    connect(ui->btnLoop, SIGNAL(clicked(bool)), this, SLOT(markerLoop(bool)));
    qt_set_icon(loop_xpm, ui->btnLoop);

    /*
     * Final settings.
     */

    set_snap(8);
    set_beats_per_measure(4);
    set_beat_width(4);
}

/**
 *
 */

qperfeditframe64::~qperfeditframe64 ()
{
    delete ui;
    if (not_nullptr(m_palette))     // valgrind fix?
        delete m_palette;
}

/**
 *  Passes the Follow status to the qperfroll object.
 */

void
qperfeditframe64::follow (bool ischecked)
{
    m_perfroll->progress_follow(ischecked);
}

/**
 *  Checks the position of the tick, and, if it is in a different piano-roll
 *  "page" than the last page, moves the page to the next page.
 */

void
qperfeditframe64::follow_progress ()
{
    int w = m_perfroll->width();        // m_perfroll->window_width();
    if (w > 0)
    {
        QScrollBar * hadjust = ui->rollScrollArea->horizontalScrollBar();
        int scrollx = hadjust->value();
        midipulse progress_tick = perf().get_tick();
        // printf("tick = %ld\n", progress_tick);
        if (progress_tick > 0 && m_perfroll->progress_follow())
        {
            // int prog_x = progress_tick / m_perfroll->zoom();
            int prog_x = m_perfroll->length_pixels(progress_tick);
            int page = prog_x / w;
            if (page > m_perfroll->scroll_page() || (page == 0 && scrollx != 0))
            {
                m_perfroll->scroll_page(page);
                m_perftime->scroll_page(page);
                hadjust->setValue(prog_x);      // set_scroll_x() not needed
            }
        }
    }
}

/**
 *
 */

int
qperfeditframe64::get_beat_width () const
{
    return mbeat_width;
}

/**
 *
 */

void
qperfeditframe64::updateGridSnap (int snapIndex)
{
    int snap;
    switch (snapIndex)
    {
    case 0: snap = 1; break;
    case 1: snap = 2; break;
    case 2: snap = 4; break;
    case 3: snap = 8; break;
    case 4: snap = 16; break;
    case 5: snap = 32; break;
    default: snap = 16; break;
    }
    m_snap = snap;
    set_guides();
}

/**
 *
 */

void
qperfeditframe64::set_snap (int a_snap)
{
    char b[16];
    snprintf(b, sizeof(b), "1/%d", a_snap);
    ui->cmbGridSnap->setCurrentText(b);
    m_snap = a_snap;
    set_guides();
}

/**
 *
 */

void
qperfeditframe64::set_beats_per_measure (int bpm)
{
    mbeats_per_measure = bpm;
    set_guides();
}

/**
 *
 */

int qperfeditframe64::get_beats_per_measure () const
{
    return mbeats_per_measure;
}

/**
 *
 */

void
qperfeditframe64::set_beat_width (int bw)
{
    mbeat_width = bw;
    set_guides();
}

/**
 *  These values are ticks, but passed as integers.
 */

void
qperfeditframe64::set_guides ()
{
    int ppqn = SEQ64_DEFAULT_PPQN * 4;      // TODO: allow runtime changes
    int measure = (ppqn * 4) * mbeats_per_measure / mbeat_width;
    int snap = m_snap;        // measure / m_snap;
    int beat = (ppqn * 4) / mbeat_width;
    m_perfroll->set_guides(snap, measure, beat);
    m_perftime->set_guides(snap, measure);
#ifdef PLATFORM_DEBUG_TMI
    printf
    (
        "set_guides(snap = %d, measure = %d, beat = %d ticks\n",
        snap, measure, beat
    );
#endif
}

/**
 *
 */

void
qperfeditframe64::zoom_in ()
{
    m_perftime->zoom_in();
    m_perfroll->zoom_in();
}

/**
 *
 */

void
qperfeditframe64::zoom_out ()
{
    m_perftime->zoom_out();
    m_perfroll->zoom_out();
}

/**
 *  Calls update geometry on elements to react to changes in MIDI file sizes.
 */

void
qperfeditframe64::update_sizes ()
{
    m_perfroll->updateGeometry();
    m_perftime->updateGeometry();
    // mContainer->adjustSize();
}

/**
 *
 */

void
qperfeditframe64::markerCollapse ()
{
    perf().push_trigger_undo();
    perf().move_triggers(false);
}

/**
 *
 */

void
qperfeditframe64::markerExpand ()
{
    perf().push_trigger_undo();
    perf().move_triggers(true);
}

/**
 *
 */

void
qperfeditframe64::markerExpandCopy ()
{
    perf().push_trigger_undo();
    perf().copy_triggers();
}

/**
 *
 */

void
qperfeditframe64::markerLoop (bool loop)
{
    perf().set_looping(loop);
}

}           // namespace seq64

/*
 * qperfeditframe64.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

