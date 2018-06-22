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
 * \file          qseqeditframe64.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-06-15
 * \updates       2018-06-21
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 *
 *  https://stackoverflow.com/questions/1982986/
 *          scrolling-different-widgets-at-the-same-time
 *
 *      ...Try to make it so that each of the items that needs to scroll in
 *      concert is inside its own QScrollArea. I would then put all those
 *      widgets into one widget, with a QScrollBar underneath (and/or to the
 *      side, if needed).
 *
 *      Designate one of the interior scrolled widgets as the "master", probably
 *      the plot widget. Then do the following:
 *
 *      Set every QScrollArea's horizontal scroll bar policy to never show the
 *      scroll bars.  (Set) the master QScrollArea's horizontalScrollBar()'s
 *      rangeChanged(int min, int max) signal to a slot that sets the main
 *      widget's horizontal QScrollBar to the same range. Additionally, it
 *      should set the same range for the other scroll area widget's horizontal
 *      scroll bars.
 *
 *      The horizontal QScrollBar's valueChanged(int value) signal should be
 *      connected to every scroll area widget's horizontal scroll bar's
 *      setValue(int value) slot.  Repeat for vertical scroll bars, if doing
 *      vertical scrolling.
 *
 *  The layout of this frame is depicted here:
 *
\verbatim
                 -----------------------------------------------------------
    QHBoxLayout | seqname : gridsnap : notelength : seqlength : ...         |
                 -----------------------------------------------------------
    QHBoxLayout | undo : redo : tools : zoomin : zoomout : scale : ...      |
QVBoxLayout:     -----------------------------------------------------------
QWidget container?
    QScrollArea |   | qseqtime      (0, 1, 1, 1) Scroll horiz only      | V |
                |-- |---------------------------------------------------| e |
                | q |                                                   | r |
                | s |                                                   | t |
                | e |                                                   |   |
    QScrollArea | q | qseqroll      (1, 1, 1, 1) Scroll h/v both        | s |
                | k |                                                   | ' |
                | e |                                                   | b |
                | y |                                                   | a |
                | s |                                                   | r |
                |---|---------------------------------------------------|   |
    QScrollArea |   | qtriggeredit  (2, 1, 1, 1) Scroll horiz only      |   |
                |   |---------------------------------------------------|   |
                |   |                                                   |   |
    QScrollArea |   | qseqdata      (3, 1, 1, 1) Scroll horiz only      |   |
                |   |                                                   |   |
                 -----------------------------------------------------------
                | Horizontal scroll bar for QWidget container           |   |
                 -----------------------------------------------------------
    QHBoxLayout | Events : ...                                              |
                 -----------------------------------------------------------
\endverbatim
 *
 */

#include <QWidget>
#include <QPalette>
#include <QScrollArea>
#include <QScrollBar>

#include "perform.hpp"                  /* seq64::perform reference         */
#include "qseqdata.hpp"
#include "qseqeditframe64.hpp"
#include "qseqkeys.hpp"
#include "qseqroll.hpp"
#include "qseqtime.hpp"
#include "qstriggereditor.hpp"
#include "qt5_helpers.hpp"              /* seq64::qt_set_icon()             */
#include "settings.hpp"                 /* usr()                            */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qseqeditframe64.h"
#else
#include "forms/qseqeditframe64.ui.h"
#endif

/*
 *  We prefer to load the pixmaps on the fly, rather than deal with those
 *  friggin' resource files.
 */

// #include "pixmaps/drum.xpm"
#include "pixmaps/note_length.xpm"
// #include "pixmaps/play.xpm"
// #include "pixmaps/quantize.xpm"
// #include "pixmaps/rec.xpm"
// #include "pixmaps/redo.xpm"
#include "pixmaps/snap.xpm"
// #include "pixmaps/thru.xpm"
// #include "pixmaps/tools.xpm"
// #include "pixmaps/undo.xpm"
#include "pixmaps/zoom.xpm"     /* zoom_in/zoom_out replaced by combo-box   */

/*
 *  Do not document the name space.
 */

namespace seq64
{

/**
 *  Static data members.  These items apply to all of the instances of seqedit,
 *  and are passed on to the following constructors:
 *
 *  -   seqdata TODO
 *  -   seqevent TODO
 *  -   seqroll TODO
 *  -   seqtime TODO
 *
 *  The snap and note-length defaults would be good to write to the "user"
 *  configuration file.  The scale and key would be nice to write to the
 *  proprietary section of the MIDI song.  Or, even more flexibly, to each
 *  sequence, if that makes sense to do, since all tracks would generally be
 *  in the same key.  Right, Charles Ives?
 *
 *  Note that, currently, that some of these "initial values" are modified, so
 *  that they are "contagious".  That is, the next sequence to be opened in
 *  the sequence editor will adopt these values.  This is a long-standing
 *  feature of Seq24, but strikes us as a bit surprising.
 *
 *  If we just double the PPQN, then the snap divisor becomes 32, and the snap
 *  interval is a 32nd note.  We would like to keep it at a 16th note.  We correct
 *  the snap ticks to the actual PPQN ratio.
 */

int qseqeditframe64::m_initial_snap         = SEQ64_DEFAULT_PPQN / 4;
int qseqeditframe64::m_initial_note_length  = SEQ64_DEFAULT_PPQN / 4;

#ifdef SEQ64_STAZED_CHORD_GENERATOR
int qseqeditframe64::m_initial_chord        = 0;
#endif

/**
 *  These static items are used to fill in and select the proper snap values for
 *  the grids.  Note that they are not members, though they could be.
 */

static const int s_snap_items [] =
{
    1, 2, 4, 8, 16, 32, 64, 128, 0, 3, 6, 12, 24, 48, 96, 192
};
static const int s_snap_count = sizeof(s_snap_items) / sizeof(int);

/**
 *  These static items are used to fill in and select the proper zoom values for
 *  the grids.  Note that they are not members, though they could be.
 *  Also note the features of these zoom numbers:
 *
 *      -#  The lowest zoom value is SEQ64_MINIMUM_ZOOM in app_limits.h.
 *      -#  The highest zoom value is SEQ64_MAXIMUM_ZOOM in app_limits.h.
 *      -#  The zoom values are all powers of 2.
 *      -#  The zoom values are further constrained by the configured values
 *          of usr().min_zoom() and usr().max_zoom().
 *      -#  The default zoom is specified in the user's "usr" file, and
 *          the default value of this default zoom is 2.
 *
 * \todo
 *      We still need to figure out what to do with a zoom of 0, which
 *      is supposed to tell Sequencer64 to auto-adjust to the current PPQN.
 */

static const int s_zoom_items [] =
{
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512
};
static const int s_zoom_count = sizeof(s_zoom_items) / sizeof(int);

/**
 *  Looks up a zoom value.
 */

static int s_lookup_zoom (int zoom)
{
    int result = 0;
    for (int zi = 0; zi < s_zoom_count; ++zi)
    {
        if (s_zoom_items[zi] == zoom)
        {
            result = zi;
            break;
        }
    }
    return result;
}

/**
 * Actions.  These variables represent actions that can be applied to a
 * selection of notes.  One idea would be to add a swing-quantize action.
 * We will reserve the value here, for notes only; not yet used or part of the
 * action menu.
 */

enum edit_action_t
{
    c_select_all_notes         =  1,
    c_select_all_events        =  2,
    c_select_inverse_notes     =  3,
    c_select_inverse_events    =  4,
    c_quantize_notes           =  5,
    c_quantize_events          =  6,
#ifdef USE_STAZED_RANDOMIZE_SUPPORT
    c_randomize_events         =  7,
#endif
    c_tighten_events           =  8,
    c_tighten_notes            =  9,
    c_transpose_notes          = 10,    /* basic transpose      */
    c_reserved                 = 11,
    c_transpose_h              = 12,    /* harmonic transpose   */
    c_expand_pattern           = 13,
    c_compress_pattern         = 14,
    c_select_even_notes        = 15,
    c_select_odd_notes         = 16,
    c_swing_notes              = 17     /* swing quantize       */
};

/**
 *
 * \param p
 *      Provides the perform object to use for interacting with this sequence.
 *      Among other things, this object provides the active PPQN.
 *
 * \param seqid
 *      Provides the sequence number.  The sequence pointer is looked up using
 *      this number.  This number is also the pattern-slot number for this
 *      sequence and for this window.  Ranges from 0 to 1024.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.
 */

qseqeditframe64::qseqeditframe64
(
    perform & p,
    int seqid,
    QWidget * parent
) :
    QFrame              (parent),
    ui                  (new Ui::qseqeditframe64),
    m_performance       (p),                            // a reference
    m_seq               (perf().get_sequence(seqid)),   // a pointer
    m_seqkeys           (nullptr),
    m_seqtime           (nullptr),
    m_seqroll           (nullptr),
    m_seqdata           (nullptr),
    m_seqevent          (nullptr),
    m_initial_zoom      (SEQ64_DEFAULT_ZOOM),           // constant
    m_zoom              (SEQ64_DEFAULT_ZOOM),           // fixed below
    m_snap              (m_initial_snap),
    m_note_length       (m_initial_note_length),
    m_scale             (usr().seqedit_scale()),        // m_initial_scale
#ifdef SEQ64_STAZED_CHORD_GENERATOR
    m_chord             (0),    // (usr().seqedit_chord()),  // m_initial_chord
#endif
    m_key               (usr().seqedit_key()),          // m_initial_key
    m_bgsequence        (usr().seqedit_bgsequence()),   // m_initial_sequence
    m_measures          (0),                            // fixed below
    m_ppqn              (p.ppqn()),
#ifdef USE_STAZED_ODD_EVEN_SELECTION
    m_pp_whole          (0),
    m_pp_eighth         (0),
    m_pp_sixteenth      (0),
#endif
    m_editing_status    (0),
    m_editing_cc        (0),
    m_first_event       (0),
    m_first_event_name  ("(no events)"),
    m_have_focus        (false),
    m_edit_mode         (perf().seq_edit_mode(seqid))
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /*
     * Instantiate the various editable areas of the seqedit user-interface.
     *
     * seqkeys: Not quite working as we'd hope.  The scrollbars still eat up
     * space.  They need to be hidden.
     */

    m_seqkeys = new qseqkeys
    (
        *m_seq,
        ui->keysScrollArea,
        usr().key_height(),
        usr().key_height() * c_num_keys + 1
    );
    ui->keysScrollArea->setWidget(m_seqkeys);
    ui->keysScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->keysScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /*
     * qseqtime
     */

    m_seqtime = new qseqtime
    (
        perf(), *m_seq, SEQ64_DEFAULT_ZOOM, ui->timeScrollArea
    );
    ui->timeScrollArea->setWidget(m_seqtime);
    ui->timeScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->timeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /*
     * qseqroll
     */

    m_seqroll = new qseqroll
    (
        perf(), *m_seq,
        m_seqkeys, m_zoom, m_snap, 0,
        ui->rollScrollArea,
        EDIT_MODE_NOTE
    );
    ui->rollScrollArea->setWidget(m_seqroll);
    ui->rollScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->rollScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_seqroll->update_edit_mode(m_edit_mode);

    /*
     * qseqdata
     */

    m_seqdata = new qseqdata(*m_seq, ui->dataScrollArea);
    ui->dataScrollArea->setWidget(m_seqdata);
    ui->dataScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->dataScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /*
     * qseqevent
     */

    m_seqevent = new qstriggereditor
    (
        *m_seq, *m_seqdata, ui->eventScrollArea, // mContainer,
        usr().key_height()
    );
    ui->eventScrollArea->setWidget(m_seqevent);
    ui->eventScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->eventScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /*
     *  Add the various scrollbar points to the qscrollmaster object,
     *  ui->rollScrollArea.
     */

    ui->rollScrollArea->add_v_scroll(ui->keysScrollArea->verticalScrollBar());
    ui->rollScrollArea->add_h_scroll(ui->timeScrollArea->horizontalScrollBar());
    ui->rollScrollArea->add_h_scroll(ui->dataScrollArea->horizontalScrollBar());
    ui->rollScrollArea->add_h_scroll(ui->eventScrollArea->horizontalScrollBar());

    /*
     *  Sequence Number Label
     */

    char tmp[32];
    snprintf(tmp, sizeof tmp, "%d", seqid);

    QString labeltext = tmp;
    ui->m_label_seqnumber->setText(labeltext);

    /**
     *  Fill "Snap" and "Note" Combo Boxes:
     *
     *      To reduce the amount of written code, we now use a static array to
     *      initialize some of these menu entries.  0 denotes the separator.
     *      This same setup is used to set up both the snap and note menu, since
     *      they are exactly the same.  Saves a *lot* of code.  This code was
     *      copped from the Gtkmm 2.4 seqedit class and adapted to Qt 5.
     */

    for (int si = 0; si < s_snap_count; ++si)
    {
        int item = s_snap_items[si];
        char fmt[8];
        if (item > 1)
            snprintf(fmt, sizeof fmt, "1/%d", item);
        else
            snprintf(fmt, sizeof fmt, "%d", item);

        QString combo_text = fmt;
        if (item == 0)
        {
            ui->m_combo_snap->insertSeparator(8);   // why 8?
            ui->m_combo_note->insertSeparator(8);   // why 8?
            continue;
        }
        else
        {
            ui->m_combo_snap->insertItem(si, combo_text);
            ui->m_combo_note->insertItem(si, combo_text);
        }
    }
    ui->m_combo_snap->setCurrentIndex(4);               /* 16th-note entry  */
    ui->m_combo_note->setCurrentIndex(4);               /* ditto            */
    connect
    (
        ui->m_combo_snap, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_grid_snap(int))
    );
    connect
    (
        ui->m_combo_note, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_note_length(int))
    );

    /*
     * Nullify the existing text and replace it with an icon.
     */

    ui->m_button_snap->setText("");
    qt_set_icon(snap_xpm, ui->m_button_snap);
    ui->m_button_note->setText("");
    qt_set_icon(note_length_xpm, ui->m_button_note);

    /*
     *  Zoom In and Zoom Out:  Rather than two buttons, we use one and
     *  a combo-box.
     */

    ui->m_button_zoom->setText("");
    qt_set_icon(zoom_xpm, ui->m_button_zoom);
    connect
    (
        ui->m_button_zoom, SIGNAL(clicked(bool)),
        this, SLOT(zoom_out())
    );
    for (int zi = 0; zi < s_zoom_count; ++zi)
    {
        int zoom = s_zoom_items[zi];
        if (zoom >= usr().min_zoom() && zoom <= usr().max_zoom())
        {
            char fmt[16];
            snprintf(fmt, sizeof fmt, "1px:%dtx", zoom);

            QString combo_text = fmt;
            ui->m_combo_zoom->insertItem(zi, combo_text);
        }
    }
    ui->m_combo_zoom->setCurrentIndex(0);               /* 16th-note entry  */
    connect
    (
        ui->m_combo_zoom, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_grid_zoom(int))
    );
}

/**
 *  \dtor
 */

qseqeditframe64::~qseqeditframe64 ()
{
    delete ui;
}

/*
 * Play the SLOTS!
 */

/**
 *  Updates the grid-snap values and control based on the index.  The value is
 *  passed to the set_snap() function for processing.
 *
 * \param index
 *      Provides the index selected from the combo-box.
 */

void
qseqeditframe64::update_grid_snap (int index)
{
    int qnfactor = m_ppqn * 4;
    int item = s_snap_items[index];
    int v = qnfactor / item;
    set_snap(v);
}

/**
 *  Selects the given snap value, which is the number of ticks in a snap-sized
 *  interval.  It is passed to the seqroll, seqevent, and sequence objects, as
 *  well.
 *
 *  The default initial snap is the default PPQN divided by 4, or the
 *  equivalent of a 16th note (48 ticks).  The snap divisor is 192 * 4 / 48 or
 *  16.
 *
 * \param s
 *      The prospective snap value to set.  It is checked only to make sure it
 *      is greater than 0, to avoid a numeric exception.
 */

void
qseqeditframe64::set_snap (int s)
{
    if (s > 0)
    {
        m_snap = s;
        m_initial_snap = s;
        m_seqroll->set_snap(s);
        m_seq->set_snap_tick(s);
        // m_seqevent->set_snap(s);     // TODO
    }
}

/**
 *  Updates the note-length values and control based on the index.  It is passed
 *  to the set_note_length() function for processing.
 *
 * \param index
 *      Provides the index selected from the combo-box.
 */

void
qseqeditframe64::update_note_length (int index)
{
    int qnfactor = m_ppqn * 4;
    int item = s_snap_items[index];
    int v = qnfactor / item;
    set_note_length(v);
}

/**
 *  Selects the given note-length value.
 *
 * \warning
 *      Currently, we don't handle changes in the global PPQN after the
 *      creation of the menu.  The creation of the menu hard-wires the values
 *      of note-length.  To adjust for a new global PQN, we will need to store
 *      the original PPQN (m_original_ppqn = m_ppqn), and then adjust the
 *      notelength based on the new PPQN.  For example if the new PPQN is
 *      twice as high as 192, then the notelength should double, though the
 *      text displayed in the "Note length" field should remain the same.
 *      However, we do adjust for a non-default PPQN at startup time.
 *
 * \param notelength
 *      Provides the note length in units of MIDI pulses.
 */

void
qseqeditframe64::set_note_length (int notelength)
{
#ifdef CAN_MODIFY_GLOBAL_PPQN
    if (m_ppqn != m_original_ppqn)
    {
        double factor = double(m_ppqn) / double(m_original);
        notelength = int(notelength * factor + 0.5);
    }
#endif

    m_note_length = notelength;
    m_initial_note_length = notelength;
    m_seqroll->set_note_length(notelength);
}

/**
 *  Updates the grid-zoom values and control based on the index.  The value is
 *  passed to the set_zoom() function for processing.
 *
 * \param index
 *      Provides the index selected from the combo-box.
 */

void
qseqeditframe64::update_grid_zoom (int index)
{
    int v = s_zoom_items[index];
    set_zoom(v);
}

/**
 *
 */

void
qseqeditframe64::zoom_in ()
{
    if (m_zoom > 1 && m_zoom > usr().min_zoom())
        m_zoom /= 2;

    int index = s_lookup_zoom(m_zoom);
    ui->m_combo_zoom->setCurrentIndex(index);
    m_seqroll->zoom_in();
    m_seqtime->zoom_in();
    m_seqevent->zoom_in();
    m_seqdata->zoom_in();
    update_draw_geometry();
}

/**
 *
 */

void
qseqeditframe64::zoom_out ()
{
    if (m_zoom < usr().max_zoom())
    {
        m_zoom *= 2;
        m_seqroll->zoom_out();
        m_seqtime->zoom_out();
        m_seqevent->zoom_out();
        m_seqdata->zoom_out();
    }
    else                                /* wrap around to beginning */
    {
        int v = s_zoom_items[0];
        set_zoom(v);
    }

    int index = s_lookup_zoom(m_zoom);
    ui->m_combo_zoom->setCurrentIndex(index);
    update_draw_geometry();
}

/**
 *
 */

void
qseqeditframe64::set_zoom (int z)
{
    if ((z >= usr().min_zoom()) && (z <= usr().max_zoom()))
    {
        int index = s_lookup_zoom(z);
        ui->m_combo_zoom->setCurrentIndex(index);
        m_zoom = z;
        m_seqroll->set_zoom(z);
        m_seqtime->set_zoom(z);
        m_seqdata->set_zoom(z);
        m_seqevent->set_zoom(z);
    }
}

/**
 *
 */

void
qseqeditframe64::update_draw_geometry()
{
    /*
    QString lentext(QString::number(m_seq->get_num_measures()));
    ui->cmbSeqLen->setCurrentText(lentext);
    mContainer->adjustSize();
    */
    m_seqtime->updateGeometry();
    m_seqroll->updateGeometry();
}

/**
 *
 */

void
qseqeditframe64::set_editor_mode (seq64::edit_mode_t mode)
{
    m_edit_mode = mode;
    perf().seq_edit_mode(*m_seq, mode);
    m_seqroll->update_edit_mode(mode);
}


}           // namespace seq64

/*
 * qseqeditframe64.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

