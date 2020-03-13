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
 * \updates       2020-03-12
 * \license       GNU GPLv2 or above
 *
 *  Note that, as of version 0.9.11, the z and Z keys, when focus is on the
 *  perfroll (piano roll), will zoom the view horizontally.  Not working!
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
#include "pixmaps/transpose.xpm"
#include "pixmaps/undo.xpm"
#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"

/**
 *  Helps with making the page leaps slightly smaller than the width of the
 *  piano roll scroll area.  Same value as used in qseqeditframe64.
 */

#define SEQ64_PROGRESS_PAGE_OVERLAP_QT      80

namespace seq64
{

/**
 *  Principal constructor, has a reference to a perform object.
 *
 * \param p
 *      Refers to the main performance object.
 *
 * \param parent
 *      The Qt widget that owns this frame.  Either the qperfeditex (the
 *      external window holding this frame) or the "Song" tab object in the
 *      main window.
 */

qperfeditframe64::qperfeditframe64 (seq64::perform & p, QWidget * parent)
 :
    QFrame                  (parent),
    ui                      (new Ui::qperfeditframe64),
    m_mainperf              (p),
    m_palette               (nullptr),
    m_snap                  (8),
    m_beats_per_measure     (4),
    m_beat_width            (4),
    m_ppqn                  (p.get_ppqn()),     /* not choose_ppqn(ppqn)    */
    m_perfroll              (nullptr),
    m_perfnames             (nullptr),
    m_perftime              (nullptr)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /*
     * What about the window title?
     */

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
        m_mainperf, ui->namesScrollArea
    );
    ui->namesScrollArea->setWidget(m_perfnames);
    ui->namesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->namesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_perftime = new seq64::qperftime
    (
        m_mainperf, SEQ64_DEFAULT_ZOOM, SEQ64_DEFAULT_SNAP,
        ui->timeScrollArea
    );
    ui->timeScrollArea->setWidget(m_perftime);
    ui->timeScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->timeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_perfroll = new seq64::qperfroll
    (
        m_mainperf, SEQ64_DEFAULT_ZOOM, SEQ64_DEFAULT_SNAP,
        this, ui->rollScrollArea
    );
    ui->rollScrollArea->setWidget(m_perfroll);
    ui->rollScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->rollScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    /*
     *  Add the various scrollbar points to the qscrollmaster object.
     */

    ui->rollScrollArea->add_v_scroll(ui->namesScrollArea->verticalScrollBar());
    ui->rollScrollArea->add_h_scroll(ui->timeScrollArea->horizontalScrollBar());

    /*
     * Create the color palette for coloring the patterns.
     */

    m_palette = new QPalette();
    m_palette->setColor(QPalette::Background, Qt::darkGray);

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
     * Transpose button and combo-box.
     */

    connect
    (
        ui->btnTranspose, SIGNAL(clicked(bool)),
        this, SLOT(reset_transpose())
    );
    qt_set_icon(transpose_xpm, ui->btnTranspose);

    char num[16];
    for (int t = -SEQ64_OCTAVE_SIZE; t <= SEQ64_OCTAVE_SIZE; ++t)
    {
        int index = t + SEQ64_OCTAVE_SIZE;
        if (t != 0)
            snprintf(num, sizeof num, "%+d [%s]", t, c_interval_text[abs(t)]);
        else
            snprintf(num, sizeof num, "0 [normal]");

        ui->comboTranspose->insertItem(index, num);
    }
    ui->comboTranspose->setCurrentIndex(SEQ64_OCTAVE_SIZE);
    connect
    (
        ui->comboTranspose, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_transpose(int))
    );

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
    int w = ui->rollScrollArea->width() - SEQ64_PROGRESS_PAGE_OVERLAP_QT;
    if (w > 0)
    {
        QScrollBar * hadjust = ui->rollScrollArea->h_scroll();
        midipulse progtick = perf().get_tick();
        if (progtick > 0 && m_perfroll->progress_follow())
        {
            int progx = m_perfroll->length_pixels(progtick);
            int page = progx / w;
            int oldpage = m_perfroll->scroll_page();
            bool newpage = page != oldpage;
            if (newpage)
            {
                m_perfroll->scroll_page(page);  // scrollmaster will update the rest
                hadjust->setValue(progx);       // set_scroll_x() not needed
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
    return m_beat_width;
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
    default: snap = SEQ64_DEFAULT_SNAP; break;
    }
    m_snap = snap;
    set_guides();
}

/**
 *
 */

void
qperfeditframe64::set_snap (int snap)
{
    char b[16];
    if (snap > 0)
        m_snap = snap;
    else
        m_snap = SEQ64_DEFAULT_SNAP;

    snprintf(b, sizeof(b), "1/%d", m_snap);
    ui->cmbGridSnap->setCurrentText(b);
    set_guides();
}

/**
 *
 */

void
qperfeditframe64::set_beats_per_measure (int bpm)
{
    m_beats_per_measure = bpm;
    set_guides();
}

/**
 *
 */

int qperfeditframe64::get_beats_per_measure () const
{
    return m_beats_per_measure;
}

/**
 *
 */

void
qperfeditframe64::set_beat_width (int bw)
{
    m_beat_width = bw;
    set_guides();
}

/**
 *  These values are ticks, but passed as integers.  The guides are set to 4
 *  measures by default.
 *
 *  Issue #171:  Change it back to what Seq24 does!  Done.  Seems to work.
 */

void
qperfeditframe64::set_guides ()
{
    if (m_beat_width > 0 && m_snap > 0)
    {
        midipulse pp = perf().get_ppqn() * 4;   // TODO: allow runtime changes
        midipulse measure_ticks = pp * m_beats_per_measure / m_beat_width;
        midipulse snap_ticks = measure_ticks / m_snap;
        midipulse beat_ticks = pp / m_beat_width;
        m_perfroll->set_guides(snap_ticks, measure_ticks, beat_ticks);
        m_perftime->set_guides(snap_ticks, measure_ticks);

#ifdef PLATFORM_DEBUG_TMI
        printf
        (
            "set_guides(snap = %d, measure = %d, beat = %d ticks\n",
            snap_ticks, measure_ticks, beat_ticks
        );
#endif
    }
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
 *
 */

void
qperfeditframe64::reset_zoom ()
{
    m_perftime->reset_zoom();
    m_perfroll->reset_zoom();
}

/**
 *  The button callback for transposition for this window.  Unlike the
 *  Gtkmm-2.4 version, this version just toggles whether it is used or not,
 *  for now.  We will add a combo-box selector soon.
 */

void
qperfeditframe64::reset_transpose ()
{
    // if (perf().get_transpose() != transpose)
    //     set_transpose(transpose);
}

/**
 *  Handles updates to the tranposition value.
 */

void
qperfeditframe64::update_transpose (int index)
{
    int transpose = index - SEQ64_OCTAVE_SIZE;
    if (transpose >= -SEQ64_OCTAVE_SIZE && transpose <= SEQ64_OCTAVE_SIZE)
    {
        if (perf().get_transpose() != transpose)
            set_transpose(transpose);
    }
}

/**
 *  Sets the value of transposition for this window.
 *
 * \param transpose
 *      The amount to transpose the transposable sequences.
 *      We need to add validation at some point, if the widget does not
 *      enforce that.
 */

void
qperfeditframe64::set_transpose (int transpose)
{
    /*
    char b[12];
    snprintf(b, sizeof b, "%+d", transpose);
    m_entry_xpose->set_text(b);
     */
    perf().all_notes_off();
    perf().set_transpose(transpose);
}

/**
 *  Calls update geometry on elements to react to changes in MIDI file sizes.
 */

void
qperfeditframe64::update_sizes ()
{
    m_perfroll->updateGeometry();
    m_perftime->updateGeometry();
}

/**
 *
 */

void
qperfeditframe64::markerCollapse ()
{
    /*
     * Can't perform do both of these calls?
     */

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

