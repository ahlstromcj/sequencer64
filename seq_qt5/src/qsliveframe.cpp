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
 * \file          qsliveframe.cpp
 *
 *  This module declares/defines the base class for holding pattern slots.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-10-28
 * \license       GNU GPLv2 or above
 *
 *  This class is the Qt counterpart to the mainwid class.
 */

#include <sstream>                      /* std::ostringstream class         */

#include <QPainter>
#include <QMenu>
#include <QTimer>
#include <QMessageBox>

#include "globals.h"
#include "keystroke.hpp"                /* seq64::keystroke class           */
#include "perform.hpp"
#include "qskeymaps.hpp"                /* mapping between Gtkmm and Qt     */
#include "qsliveframe.hpp"
#include "qsmacros.hpp"                 /* QS_TEXT_CHAR() macro             */
#include "qsmainwnd.hpp"                /* the true parent of this class    */
#include "settings.hpp"                 /* usr().window_redraw_rate()       */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qsliveframe.h"
#else
#include "forms/qsliveframe.ui.h"
#endif

/**
 *  Constants to use to fine-tune the MIDI event preview boxes.  The original
 *  values are commented out.  The new values make the event boxes smaller and
 *  nicer looking.  However, we need a less krufty way to change these.
 */

static const int sc_preview_w_factor = 3;   // 2;
static const int sc_preview_h_factor = 8;   // 5;
static const int sc_base_x_offset = 12;     // 7;
static const int sc_base_y_offset = 24;     // 15;

/*
 * Do not document a namespace, it breaks Doxygen.
 */

namespace seq64
{

static const int c_text_x = 6;
static const int c_mainwid_border = 0;

/**
 *  The Qt 5 version of mainwid.
 *
 * \param p
 *      Provides the perform object to use for interacting with this sequence.
 *
 * \param window
 *      Provides the functional parent of this live frame.
 *
 * \param parent
 *      Provides the Qt-parent window/widget for this container window.
 *      Defaults to null.  Normally, this is a pointer to the tab-widget
 *      containing this frame.  If null, there is no parent, and this frame is
 *      in an external window.
 */

qsliveframe::qsliveframe (perform & p, qsmainwnd * window, QWidget * parent)
 :
    QFrame              (parent),
    ui                  (new Ui::qsliveframe),
    m_perform           (p),
    m_parent            (window),
    m_moving_seq        (),
    m_seq_clipboard     (),
    m_popup             (nullptr),
    m_timer             (nullptr),
    m_msg_box           (nullptr),
    m_font              (),
    m_bank_id           (0),
    m_mainwnd_rows      (usr().mainwnd_rows()),
    m_mainwnd_cols      (usr().mainwnd_cols()),
    m_mainwid_spacing   (usr().mainwid_spacing()),
    m_space_rows        (m_mainwid_spacing * m_mainwnd_cols),
    m_space_cols        (m_mainwid_spacing * m_mainwnd_rows),
    m_screenset_slots   (m_mainwnd_rows * m_mainwnd_cols),
    m_screenset_offset  (m_bank_id * m_screenset_slots),
    m_slot_w            (0),
    m_slot_h            (0),
    m_last_metro        (0),
    m_alpha             (0),
    m_gtkstyle_border   (! usr().grid_is_normal()),
    m_curr_seq          (0),            // mouse interaction
    m_old_seq           (0),
    m_button_down       (false),
    m_moving            (false),
    m_adding_new        (false),
    m_call_seq_edit     (false),
    m_call_seq_eventedit(false),
    m_call_seq_shift    (0),            // for future usage
    m_last_tick_x       (),             // array
    m_last_playing      (),             // array
    m_can_paste         (false),
    m_has_focus         (false),
    m_is_external       (is_nullptr(parent))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    ui->setupUi(this);
    m_msg_box = new QMessageBox(this);
    m_msg_box->setText(tr("Sequence already present"));
    m_msg_box->setInformativeText
    (
        tr
        (
            "There is already a sequence stored in this slot. "
            "Overwrite it and create a new blank sequence?"
        )
    );
    m_msg_box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    m_msg_box->setDefaultButton(QMessageBox::No);

    QString bname = m_perform.get_bank_name(m_bank_id).c_str();
    ui->txtBankName->setPlainText(bname);
    ui->spinBank->setRange(0, usr().max_sets() - 1);
    set_bank(0);
    connect(ui->spinBank, SIGNAL(valueChanged(int)), this, SLOT(updateBank(int)));
    connect(ui->txtBankName, SIGNAL(textChanged()), this, SLOT(updateBankName()));

    ui->labelPlaylistSong->setText("");

    m_timer = new QTimer(this);        /* timer for regular redraws    */
    m_timer->setInterval(usr().window_redraw_rate());
    connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->start();
}

/**
 *  Virtual (?) destructor, deletes the user-interface objects and the message
 *  box.
 *
 *  Not needed: delete m_timer;
 */

qsliveframe::~qsliveframe()
{
    delete ui;
    if (not_nullptr(m_msg_box))
        delete m_msg_box;
}

/**
 *
 */

void
qsliveframe::set_playlist_name (const std::string & plname)
{
    QString pln = " ";
    pln += QString::fromStdString(plname);
    ui->labelPlaylistSong->setText(pln);
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::needs_update(). All
 *  sequences are potentially checked.
 */

void
qsliveframe::conditional_update ()
{
    if (perf().needs_update())
        update();
}

/**
 *  This override simply calls drawAllSequences().
 */

void
qsliveframe::paintEvent (QPaintEvent *)
{
    drawAllSequences();
}

/**
 *  Provides a way to calculate the base x and y size values for the
 *  pattern map for a given sequence/pattern/loop.  The values are returned as
 *  side-effects.  Compare it to mainwid::calculate_base_sizes():
 *
 *      -   m_mainwid_border_x and m_mainwid_border_y are
 *          ui->frame->x() and ui->frame->y(), which can be altered by the
 *          user via resizing the main window.
 *      -   m_seqarea_x and m_seqarea_y are the m_slot_w and m_slot_h members.
 *
 * \param seqnum
 *      Provides the number of the sequence to calculate.
 *
 * \param [out] basex
 *      A return parameter for the x coordinate of the base.  This is
 *      basically the x coordinate of the rectangle for a pattern slot.
 *      It is the x location of the frame, plus the slot width offset by its
 *      "x coordinate".
 *
 * \param [out] basey
 *      A return parameter for the y coordinate of the base.  This is
 *      basically the y coordinate of the rectangle for a pattern slot.
 *      It is the y location of the frame, plus the slot height offset by its
 *      "y coordinate".
 */

void
qsliveframe::calculate_base_sizes (int seqnum, int & basex, int & basey)
{
    int i = (seqnum / m_mainwnd_rows) % m_mainwnd_cols;
    int j =  seqnum % m_mainwnd_rows;
    basex = ui->frame->x() + 1 + (m_slot_w + m_mainwid_spacing) * i;
    basey = ui->frame->y() + 1 + (m_slot_h + m_mainwid_spacing) * j;
}

/**
 *  Draws a single pattern slot.  Support for the fading of timed elements is
 *  provided via m_last_metro.
 *
 * \param seq
 *      The number of pattern to be drawn.
 */

void
qsliveframe::drawSequence (int seq)
{
    midipulse tick = perf().get_tick();
    int metro = (tick / perf().get_ppqn()) % 2;

    /*
     * Slot background and font settings.  For background, we want
     * something between black and darkGray at some point.  Here, the pen
     * is black, and the brush is black.
     */

    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush(Qt::black);
    m_font.setPointSize(6);
    m_font.setBold(true);
    m_font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);

    /*
     * Grab frame dimensions for scaled drawing.  Note that the frame size
     * can be modified by the user dragging a corner, in some window
     * managers.
     */

    int fw = ui->frame->width();
    int fh = ui->frame->height();
    m_slot_w = (fw - m_space_cols - 1) / m_mainwnd_cols;
    m_slot_h = (fh - m_space_rows - 1) / m_mainwnd_rows;

    /*
     * IDEA: Subtract 20 from height and add 10 to base y.
     */

    int preview_w = m_slot_w - m_font.pointSize() * sc_preview_w_factor;
    int preview_h = m_slot_h - m_font.pointSize() * sc_preview_h_factor;
    int base_x, base_y;
    calculate_base_sizes(seq, base_x, base_y);  /* side-effects         */
    sequence * s = perf().get_sequence(seq);
    if (not_nullptr(s))
    {
        int c = s->color();
        if (s->get_playing())                   /* playing, no queueing */
        {
            brush.setColor(Qt::black);
            pen.setColor(Qt::white);
        }
        else
        {
            brush.setColor(Qt::white);
            pen.setColor(Qt::black);
        }
        painter.setPen(pen);
        painter.setBrush(brush);

        /*
         * Outer pattern-slot border (Seq64) or whole box (Kepler34).  Do we
         * want to gray-up the border, too, for Seq64 format?
         */

        if (m_gtkstyle_border)                  /* Gtk/seq64 methods    */
        {
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.setFont(m_font);
            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
            {
                // no code
            }
            else if (s->get_playing())          /* playing, no queueing */
            {
                Color backcolor(Qt::black);
                brush.setColor(backcolor);
                pen.setColor(Qt::white);
                painter.setBrush(brush);
                painter.setPen(pen);
            }
            else if (s->get_queued())           /* not playing, queued  */
            {
                // no code
            }
            else if (s->one_shot())             /* one-shot queued      */
            {
                // no code
            }
            else                                /* just not playing     */
            {
                Color backcolor(Qt::white);
                brush.setColor(backcolor);
                pen.setColor(Qt::black);
                painter.setBrush(brush);
                painter.setPen(pen);
            }
            painter.drawRect(base_x, base_y, m_slot_w + 1, m_slot_h + 1);
        }
        else                                    /* Kepler34 methods     */
        {
            /*
             * What color?  Qt::black? Qt::darkCyan? Qt::yellow? Qt::green?
             */

            const int penwidth = 3;             /* 2                    */
            pen.setColor(Qt::black);
            pen.setStyle(Qt::SolidLine);
            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
            {
                Color backcolor = get_color_fix(PaletteColor(c));
                backcolor.setAlpha(210);
                brush.setColor(backcolor);
                pen.setWidth(penwidth);
                pen.setColor(Qt::gray);         /* instead of Qt::black */
                pen.setStyle(Qt::SolidLine);    /* not Qt::DashLine     */
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, m_slot_w + 1, m_slot_h + 1);
            }
            else if (s->get_playing())          /* playing, no queueing */
            {
                Color backcolor = get_color_fix(PaletteColor(c));
                backcolor.setAlpha(210);
                brush.setColor(backcolor);
                pen.setWidth(penwidth);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, m_slot_w + 1, m_slot_h + 1);
            }
            else if (s->get_queued())           /* not playing, queued  */
            {
                Color backcolor = get_color_fix(PaletteColor(c));
                backcolor.setAlpha(180);
                brush.setColor(backcolor);
                pen.setWidth(penwidth);
                pen.setColor(Qt::darkGray);
                pen.setStyle(Qt::SolidLine);    /* not Qt::DashLine     */
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, m_slot_w, m_slot_h);
            }
            else if (s->one_shot())             /* one-shot queued      */
            {
                Color backcolor = get_color_fix(PaletteColor(c));
                backcolor.setAlpha(180);
                brush.setColor(backcolor);
                pen.setWidth(penwidth);
                pen.setColor(Qt::darkGray);
                pen.setStyle(Qt::DotLine);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, m_slot_w, m_slot_h);
            }
            else                                /* just not playing     */
            {
                Color backcolor = get_color_fix(PaletteColor(c));
                backcolor.setAlpha(100);        /* .setAlpha(180)       */
                brush.setColor(backcolor);
                pen.setStyle(Qt::NoPen);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, m_slot_w, m_slot_h);
            }
        }

        std::string st = perf().sequence_title(*s);
        QString title(st.c_str());

        /*
         * Draws the text in the border of the pattern slot.
         */

        if (m_gtkstyle_border)
        {
            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
            {
                pen.setColor(Qt::white);
            }
            else if (s->get_playing())          /* playing, no queueing */
            {
                pen.setColor(Qt::white);
            }
            else if (s->get_queued())           /* not playing, queued  */
            {
                pen.setColor(Qt::black);
            }
            else if (s->one_shot())             /* one-shot queued      */
            {
                pen.setColor(Qt::white);
            }
            else
            {
                pen.setColor(Qt::black);
            }
        }
        else
        {
            pen.setColor(Qt::black);            // or the best contrasting color?
        }
        pen.setWidth(1);
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawText(base_x + c_text_x, base_y + 4, 80, 80, 1, title);

        std::string sl = perf().sequence_label(*s);
        QString label(sl.c_str());
        painter.drawText(base_x + 8, base_y + m_slot_h - 5, label);
        if (perf().show_ui_sequence_key())
        {
            QString key;
            key[0] = (char) perf().lookup_keyevent_key
            (
                seq - perf().screenset() * c_seqs_in_set
            );
            painter.drawText
            (
                base_x + m_slot_w - 10, base_y + m_slot_h - 5, key
            );
        }

        /*
         * Draws the inner box of the pattern slot.
         */

        Color backcolor = get_color_fix(PaletteColor(c));
        Color pencolor = get_pen_color(PaletteColor(c));
        if (m_gtkstyle_border)
        {
#ifdef PLATFORM_DEBUG_TMI
            show_color_rgb(backcolor);
#endif
            brush.setColor(backcolor);
            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
            {
                backcolor = Qt::gray;
            }
            else if (s->get_playing())          /* playing, no queueing */
            {
                if (no_color(c))
                {
                    backcolor = Qt::black;
                    pencolor = Qt::white;
                }
                else
                {
                    // pen color set below
                }
            }
            else if (s->get_queued())           /* not playing, queued  */
            {
                backcolor = Qt::gray;
            }
            else if (s->one_shot())             /* one-shot queued      */
            {
                backcolor = Qt::darkGray;
            }
            else                                /* muted pattern        */
            {
                // pen color set below
            }
        }
        else
        {
            brush.setStyle(Qt::NoBrush);
        }
        brush.setColor(backcolor);
        pen.setColor(pencolor);

        int rectangle_x = base_x + sc_base_x_offset;
        int rectangle_y = base_y + sc_base_y_offset;
        painter.setBrush(brush);
        painter.setPen(pen);                    /* inner box of notes   */
        painter.drawRect
        (
            rectangle_x-2, rectangle_y-1, preview_w, preview_h
        );

        int lowest;
        int highest;
        bool have_notes = s->get_minmax_note_events(lowest, highest);
        if (have_notes)
        {
            int height = highest - lowest + 2;
            int length = s->get_length();
            midipulse tick_s, tick_f;
            int note;
            bool selected;
            int velocity;
            draw_type_t dt;
            Color drawcolor = pencolor;         // fg_color();
            Color eventcolor = pencolor;        // fg_color();
            if (! s->get_transposable())
            {
                eventcolor = red();
                drawcolor = red();
            }
            preview_h -= 6;                     /* padding for box      */
            preview_w -= 6;
            rectangle_x += 2;
            rectangle_y += 2;
            s->reset_draw_marker();             /* reset iterator       */
            while
            (
                (
                    dt = s->get_next_note_event
                    (
                        tick_s, tick_f, note, selected, velocity
                    )
                ) != DRAW_FIN
            )
            {
                int tick_s_x = (tick_s * preview_w) / length;
                int tick_f_x = (tick_f * preview_h) / length;
                int note_y;
                if (dt == DRAW_NOTE_ON || dt == DRAW_NOTE_OFF)
                    tick_f_x = tick_s_x + 1;

                if (tick_f_x <= tick_s_x)
                    tick_f_x = tick_s_x + 1;

                if (dt == DRAW_TEMPO)
                {
                    /*
                     * Do not scale by the note range here.
                     */

                    pen.setWidth(2);
                    drawcolor = tempo_paint();
                    note_y = m_slot_w -
                         m_slot_h * (note + 1) / SEQ64_MAX_DATA_VALUE;
                }
                else
                {
                    pen.setWidth(1);                    /* 2 too thick  */
                    note_y = preview_h -
                         (preview_h * (note+1 - lowest)) / height;
                }

                int sx = rectangle_x + tick_s_x;        /* start x      */
                int fx = rectangle_x + tick_f_x;        /* finish x     */
                int sy = rectangle_y + note_y;          /* start y      */
                int fy = sy;                            /* finish y     */
                pen.setColor(drawcolor);                /* note line    */
                painter.setPen(pen);
                painter.drawLine(sx, sy, fx, fy);
                if (dt == DRAW_TEMPO)
                {
                    pen.setWidth(1);                    /* 2 too thick  */
                    drawcolor = eventcolor;
                }
            }

            int a_tick = perf().get_tick();             /* for playhead */
            a_tick += (length - s->get_trigger_offset());
            a_tick %= length;

            midipulse tick_x = a_tick * preview_w / length;
            if (s->get_playing())
                pen.setColor(Qt::red);
            else
                pen.setColor(Qt::black);

            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
                pen.setColor(Qt::green);
            else if (s->one_shot())
                pen.setColor(Qt::blue);

            pen.setWidth(1);
            painter.setPen(pen);
            painter.drawLine
            (
                rectangle_x + tick_x - 1, rectangle_y - 1,
                rectangle_x + tick_x - 1, rectangle_y + preview_h + 1
            );
        }
    }
    else
    {
        /*
         * This removes the black border around the empty sequence
         * boxes.  We like the border.
         *
         *  pen.setStyle(Qt::NoPen);
         */

        m_font.setPointSize(15);
        pen.setColor(Qt::black);        // or dark gray?
        painter.setPen(pen);
        painter.setFont(m_font);
        painter.drawRect(base_x, base_y, m_slot_w, m_slot_h);   // outline

        /*
         * No sequence present. Insert placeholder.  (Not a big fan of this
         * one, which draws a big ugly plus-sign.)
         *
         *  pen.setStyle(Qt::SolidLine);
         *  painter.setPen(pen);
         *  painter.drawText(base_x + 2, base_y + 17, "+");
         */

        if (perf().show_ui_sequence_number())
        {
            int lx = base_x + (m_slot_w / 2) - 7;
            int ly = base_y + (m_slot_h / 2) + 5;
            char snum[8];
            snprintf(snum, sizeof snum, "%d", seq);
            m_font.setPointSize(8);
            pen.setColor(Qt::white);
            pen.setWidth(1);
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
            painter.setFont(m_font);
            painter.drawText(lx, ly, snum);
        }
    }

    /*
     * Lessen m_alpha on each redraw to have smooth fading.  Done as a factor
     * of the BPM to get useful fades.
     */

    m_alpha *= 0.7 - perf().bpm() / 300.0;
    m_last_metro = metro;
}

/**
 *
 */

void
qsliveframe::drawAllSequences ()
{
#ifdef USE_KEPLER34_REDRAW_ALL
    for (int i = 0; i < m_screenset_slots; ++i)
    {
        drawSequence(i + (m_bank_id * m_screenset_slots));
        m_last_tick_x[i + (m_bank_id * m_screenset_slots)] = 0;
    }
#else
    int send = m_screenset_offset + m_screenset_slots;
    for (int s = m_screenset_offset; s < send; ++s)
    {
        drawSequence(s);
        m_last_tick_x[s] = 0;
    }
#endif
}

/**
 *  Common-code helper function.
 *
 * \param seqnum
 *      Provides the number of the sequence to validate.
 *
 * \return
 *      Returns true if the sequence number is valid for the current
 *      m_bank_id value.
 */

bool
qsliveframe::valid_sequence (int seqnum)
{
    return
    (
        seqnum >= m_screenset_offset &&
        seqnum < (m_screenset_offset + m_screenset_slots)
    );
}

/**
 *
 */

void
qsliveframe::set_bank ()
{
    int bank = perf().screenset();
    set_bank(bank);
}

/**
 *  Roughly similar to mainwid::log_screenset().
 */

void
qsliveframe::set_bank (int bank)
{
    if (bank != m_bank_id && perf().is_screenset_valid(bank))
    {
        QString bname = perf().get_bank_name(bank).c_str();
        ui->txtBankName->setPlainText(bname);
        ui->spinBank->setValue(bank);
        m_bank_id = bank;
        m_screenset_offset = m_screenset_slots * bank;
        if (m_has_focus)
            perf().set_screenset(bank);

        update();
    }
}

/**
 *
 */

void
qsliveframe::updateBank (int bank)
{
    if (perf().is_screenset_valid(bank))
    {
        perf().set_screenset(bank);
        set_bank(bank);
    }
}

/**
 *  Used to grab the std::string bank name and convert it to QString for
 *  display. Let perform set the modify flag, it knows when to do it.
 *  Otherwise, just scrolling to the next screen-set causes a spurious
 *  modification and an annoying prompt to a user exiting the application.
 */

void
qsliveframe::updateBankName ()
{
    updateInternalBankName();

    /*
     * Unnecessary and annoying.
     *
     * if (! m_is_external)
     *    perf().modify();
     */
}

/**
 *  Used to grab the std::string bank name and convert it to QString for
 *  display.
 */

void
qsliveframe::updateInternalBankName ()
{
    std::string name = ui->txtBankName->document()->toPlainText().toStdString();
    perf().set_screenset_notepad(m_bank_id, name, m_is_external);
}

/**
 *  Converts the (x, y) coordinates of a click into a sequence/pattern ID.
 *  Normally, these values can range from 0 to 31, representing one of 32
 *  slots in the live frame.  But sets may be larger or smaller.
 *
 * \param click_x
 *      The x-coordinate of the mouse click.
 *
 * \param click_y
 *      The y-coordinate of the mouse click.
 *
 * \return
 *      Returns the sequence/pattern number.  If not found, then a -1 is
 *      returned.
 */

int
qsliveframe::seq_id_from_xy (int click_x, int click_y)
{
    int x = click_x - c_mainwid_border;         /* adjust for border */
    int y = click_y - c_mainwid_border;
    int w = m_slot_w + m_mainwid_spacing;
    int h = m_slot_h + m_mainwid_spacing;

    /*
     * Is it in the box?
     */

    if (x < 0 || x >= (w * m_mainwnd_cols) || y < 0 || y >= (h * m_mainwnd_rows))
        return -1;

    /*
     * Gives us x, y in box coordinates.  Then we test for the right inactive
     * side of area.
     */

    int box_test_x = x % w;
    int box_test_y = y % h;
    if (box_test_x > m_slot_w || box_test_y > m_slot_h)
        return -1;

    x /= w;
    y /= h;
    int seqid = (x * m_mainwnd_rows + y) + (m_bank_id * m_screenset_slots);
    return seqid;
}

/**
 *  Sets m_curr_seq based on the position of the mouse over the live frame.
 *
 * \param event
 *      Provides the mouse event.
 */

void
qsliveframe::mousePressEvent (QMouseEvent * event)
{
    m_curr_seq = seq_id_from_xy(event->x(), event->y());
    if (m_curr_seq != -1 && event->button() == Qt::LeftButton)
        m_button_down = true;
}

/**
 *
 */

void
qsliveframe::mouseReleaseEvent (QMouseEvent *event)
{
    /* get the sequence number we clicked on */

    m_curr_seq = seq_id_from_xy(event->x(), event->y());
    m_button_down = false;

    /*
     * if we're on a valid sequence, hit the left mouse button, and are not
     * dragging a sequence - toggle playing.
     */

    if (m_curr_seq != -1 && event->button() == Qt::LeftButton && ! m_moving)
    {
        if (perf().is_active(m_curr_seq))
        {
            if (! m_adding_new)
                perf().sequence_playing_toggle(m_curr_seq);

            m_adding_new = false;
            update();
        }
        else
            m_adding_new = true;
    }

    /*
     * If it's the left mouse button and we're moving a pattern between slots,
     * then, if the sequence number is valid, inactive, and not in editing,
     * create a new pattern and copy the data to it.  Otherwise, copy the data
     * to the old sequence.
     */

    if (event->button() == Qt::LeftButton && m_moving)
    {
        m_moving = false;
        if (perf().is_mseq_available(m_curr_seq))
        {
            if (perf().new_sequence(m_curr_seq))
            {
                perf().get_sequence(m_curr_seq)->partial_assign(m_moving_seq);
                update();
            }
        }
        else
        {
            if (perf().new_sequence(m_old_seq))
            {
                perf().get_sequence(m_old_seq)->partial_assign(m_moving_seq);
                update();
            }
        }
    }

    /*
     * Check for right mouse click; this action launches the popup menu for
     * the pattern slot underneath the mouse.
     */

    if (m_curr_seq != -1 && event->button() == Qt::RightButton)
    {
        m_popup = new QMenu(this);

        QAction * newseq = new QAction(tr("&New pattern"), m_popup);
        m_popup->addAction(newseq);
        QObject::connect(newseq, SIGNAL(triggered(bool)), this, SLOT(new_seq()));

        /*
         *  Add an action to bring up an external qsliveframe window based
         *  on the sequence number over which the mouse is resting.  This is
         *  pretty tricky, but might be reasonable.
         */

        if (! m_is_external)
        {
            if (m_curr_seq < usr().max_sets())
            {
                QAction * liveframe = new QAction
                (
                    tr("Extern &live frame"), m_popup
                );
                m_popup->addAction(liveframe);
                QObject::connect
                (
                    liveframe, SIGNAL(triggered(bool)),
                    this, SLOT(new_live_frame())
                );
            }

            if (perf().is_active(m_curr_seq))
            {
                QAction * editseq = new QAction
                (
                    tr("Edit pattern in &tab"), m_popup
                );
                m_popup->addAction(editseq);
                connect(editseq, SIGNAL(triggered(bool)), this, SLOT(edit_seq()));
            }
        }
        if (perf().is_active(m_curr_seq))
        {
            QAction * editseqex = new QAction
            (
                tr("Edit pattern in &window"), m_popup
            );
            m_popup->addAction(editseqex);
            connect(editseqex, SIGNAL(triggered(bool)), this, SLOT(edit_seq_ex()));

            if (! m_is_external)
            {
                QAction * editevents = new QAction
                (
                    tr("Edit e&vents in tab"), m_popup
                );
                m_popup->addAction(editevents);
                connect
                (
                    editevents, SIGNAL(triggered(bool)), this, SLOT(edit_events())
                );
            }

            /*
             * \todo
             *      Use the stored palette colors!
             */

            QMenu * menuColour = new QMenu(tr("Set pattern &color..."));

#ifdef SEQ64_USE_BUILTIN_PALETTE

            int firstcolor = int(SEQ64_COLOR_INT(NONE));
            int lastcolor = int(SEQ64_COLOR_INT(GREY));
            for (int c = firstcolor; c <= lastcolor; ++c)
            {
                if (c != int(SEQ64_COLOR_INT(BLACK)))
                {
                    PaletteColor pc = PaletteColor(c);
                    QString cname = get_color_name(pc).c_str();     // for now
                    QAction * a = new QAction(cname, menuColour);
                    connect
                    (
                        a, &QAction::triggered,
                        [this, c] /*(int i)*/ { color_by_number(c); }
                    );
                    menuColour->addAction(a);
                }
            }

            QMenu * submenuColour = new QMenu(tr("More colors"));
            firstcolor = int(SEQ64_COLOR_INT(DK_RED));
            lastcolor = int(SEQ64_COLOR_INT(DK_GREY));
            for (int c = firstcolor; c <= lastcolor; ++c)
            {
                PaletteColor pc = PaletteColor(c);
                QString cname = get_color_name(pc).c_str();     // for now
                QAction * a = new QAction(cname, submenuColour);
                connect
                (
                    a, &QAction::triggered,
                    [this, c] /*(int i)*/ { color_by_number(c); }
                );
                submenuColour->addAction(a);
            }
            menuColour->addMenu(submenuColour);

#else   // SEQ64_USE_BUILTIN_PALETTE

            QAction * color[12];
            color[0] = new QAction(tr("White"),  menuColour);
            color[1] = new QAction(tr("Red"),    menuColour);
            color[2] = new QAction(tr("Green"),  menuColour);
            color[3] = new QAction(tr("Blue"),   menuColour);
            color[4] = new QAction(tr("Yellow"), menuColour);
            color[5] = new QAction(tr("Magenta"), menuColour);
            color[6] = new QAction(tr("Cyan"),   menuColour);
            color[7] = new QAction(tr("Pink"),   menuColour);
            color[8] = new QAction(tr("Orange"), menuColour);

            connect(color[0], SIGNAL(triggered(bool)), this, SLOT(color_white()));
            connect(color[1], SIGNAL(triggered(bool)), this, SLOT(color_red()));
            connect(color[2], SIGNAL(triggered(bool)), this, SLOT(color_green()));
            connect(color[3], SIGNAL(triggered(bool)), this, SLOT(color_blue()));
            connect(color[4], SIGNAL(triggered(bool)), this, SLOT(color_yellow()));
            connect(color[5], SIGNAL(triggered(bool)), this, SLOT(color_purple()));
            connect(color[6], SIGNAL(triggered(bool)), this, SLOT(color_cyan()));
            connect(color[7], SIGNAL(triggered(bool)), this, SLOT(color_pink()));
            connect(color[8], SIGNAL(triggered(bool)), this, SLOT(color_orange()));

            for (int i = 0; i < 9; ++i)
            {
                menuColour->addAction(color[i]);
            }
#endif  // SEQ64_USE_BUILTIN_PALETTE

            m_popup->addMenu(menuColour);

            QAction * actionCopy = new QAction(tr("Cop&y pattern"), m_popup);
            m_popup->addAction(actionCopy);
            connect(actionCopy, SIGNAL(triggered(bool)), this, SLOT(copy_seq()));

            QAction * actionCut = new QAction(tr("Cu&t pattern"), m_popup);
            m_popup->addAction(actionCut);
            connect(actionCut, SIGNAL(triggered(bool)), this, SLOT(cut_seq()));

            QAction * actionDelete = new QAction(tr("&Delete pattern"), m_popup);
            m_popup->addAction(actionDelete);
            connect
            (
                actionDelete, SIGNAL(triggered(bool)), this, SLOT(delete_seq())
            );
        }
        else if (m_can_paste)
        {
            QAction * actionPaste = new QAction(tr("Paste pattern"), m_popup);
            m_popup->addAction(actionPaste);
            connect(actionPaste, SIGNAL(triggered(bool)), this, SLOT(paste_seq()));
        }
        m_popup->exec(QCursor::pos());
    }

    if                              /* middle button launches seq editor    */
    (   m_curr_seq != -1 && event->button() == Qt::MiddleButton &&
        perf().is_active(m_curr_seq)
    )
    {
        callEditor(m_curr_seq);
    }
}

/**
 *
 */

void
qsliveframe::mouseMoveEvent (QMouseEvent * event)
{
    int seqid = seq_id_from_xy(event->x(), event->y());
    if (m_button_down)
    {
        if
        (
            seqid != m_curr_seq && ! m_moving &&
            ! perf().is_sequence_in_edit(m_curr_seq)
        )
        {
            /*
             * Drag a sequence between slots; save the sequence and clear the
             * old slot.
             */

            if (perf().is_active(m_curr_seq))
            {
                m_old_seq = m_curr_seq;
                m_moving = true;
                m_moving_seq.partial_assign(*(perf().get_sequence(m_curr_seq)));
                perf().delete_sequence(m_curr_seq);
                update();
            }
        }
    }
}

/**
 *
 */

void
qsliveframe::mouseDoubleClickEvent (QMouseEvent * event)
{
#ifdef USE_KEPLER34_LIVE_DOUBLE_CLICK
    if (m_adding_new)
        new_seq();
#else
    int m_curr_seq = seq_id_from_xy(event->x(), event->y());
    if (! perf().is_active(m_curr_seq))
    {
        if (perf().new_sequence(m_curr_seq))
            perf().get_sequence(m_curr_seq)->set_dirty();
    }
    callEditorEx(m_curr_seq);
#endif
}

/**
 *
 */

void
qsliveframe::new_seq ()
{
    if (perf().is_active(m_curr_seq))
    {
        int choice = m_msg_box->exec();
        if (choice == QMessageBox::No)
            return;
    }
    if (perf().new_sequence(m_curr_seq))
        perf().get_sequence(m_curr_seq)->set_dirty();

    /*
     * TODO: reenable - disabled opening the editor for each new seq
     *    callEditor(m_main_perf->get_sequence(m_current_seq));
     */
}

/**
 *  We need to see if there is an external live-frame window already existing
 *  for the current sequence number (which is used as a screen-set number).
 *  If not, we can create a new one and add it to the list.
 */

void
qsliveframe::new_live_frame ()
{
    callLiveFrame(m_curr_seq);
}

/**
 *  Emits the callEditor() signal.  In qsmainwnd, this signal is connected to
 *  the loadEditor() slot.
 */

void
qsliveframe::edit_seq ()
{
    callEditor(m_curr_seq);
}

/**
 *  Emits the callEditorEx() signal.  In qsmainwnd, this signal is connected to
 *  the loadEditorEx() slot.
 */

void
qsliveframe::edit_seq_ex ()
{
    callEditorEx(m_curr_seq);
}

/**
 *
 */

void
qsliveframe::edit_events ()
{
    callEditorEvents(m_curr_seq);
}

/**
 *  Handles associating a group-learn with the give keystroke.
 *
 * \warning
 *      In Gtkmm-2.4, keystrokes that are letters emit the upper-case ASCII
 *      code (e.g. 65 for the letter 'A').  However, Qt 5 emits the actual
 *      ASCII code (e.g. 97 for the letter 'a').  Since we copped out and
 *      translate Qt 5 key-codes to the Gtkmm-2.4 values for use in the "rc"
 *      file, we have to do that here as well.  But only if we are in
 *      group-learn mode.  Note that mainwnd (Gtkmm-2.4) does the same thing.
 *
 * \param k
 *      The (remapped, if necessary) keystroke object that perform expects in
 *      its keystroke handlers.
 *
 * \param [out] msgout
 *      Storage for either an informational string or an error string.
 *      If it comes out empty, then no group-learn action occurred.
 *
 * \return
 *      Returns true if in group-learn mode and a group key was successfully
 *      learned.  If true, then the msgout parameter should contain an
 *      informational message. Returns false otherwise.  In that case, a
 *      non-empty msgout is an error message.
 */

bool
qsliveframe::handle_group_learn (keystroke & k, std::string & msgout)
{
    bool mgl = perf().is_group_learning() && k.key() != PREFKEY(group_learn);
    if (mgl)
        k.shift_lock();                             /* remap to upper-case  */

    int count = perf().get_key_groups().count(k.key());
    msgout.clear();
    if (count != 0)
    {
        int group = perf().lookup_keygroup_group(k.key());
        if (group >= 0)
        {
            perf().select_and_mute_group(group);    /* use mute group key   */
        }
        else
        {
            std::ostringstream os;
            os
                << "Mute group out of range, ignored. "
                << "Due to larger set-size, only " << perf().group_max()
                << " groups available."
                ;
            perf().unset_mode_group_learn();
            msgout = os.str();
            mgl = false;
        }
    }
    if (mgl)                                        /* mute group learn     */
    {
        if (count != 0)
        {
            std::ostringstream os;
            os
                << "MIDI mute group learn success, "
                << "Mute group key '"
                << perf().key_name(k.key())
                << "' (code = " << k.key() << ") successfully mapped."
               ;

            /*
             * Missed the key-up group-learn message, so force it to off.
             */

            perf().unset_mode_group_learn();
            msgout = os.str();
        }
        else
        {
            std::ostringstream os;
            os
                << "Key '" << perf().key_name(k.key()) // keyval_name(k.key())
                << "' (code = " << k.key()
                << ") is not a configured mute-group key. "
                << "To add it, see the 'rc' file, section [mute-group]."
               ;

            /*
             * Missed the key-up message for group-learn, so force it off.
             */

            perf().unset_mode_group_learn();
            msgout = os.str();
            mgl = false;
        }
    }
    return mgl;
}

/**
 *
 */

bool
qsliveframe::handle_key_press (unsigned gdkkey)
{
    bool done = false;
    perform::action_t action = perf().keyboard_group_action(gdkkey);
    if (action == perform::ACTION_NONE)
    {
        /*
         * This call replaces Kepler34's processing of the semi-colon, slash,
         * apostrophe, number sign, and period.
         */

        done = perf().keyboard_group_c_status_press(gdkkey);
        if (! done)
        {
            int seqnum = perf().lookup_keyevent_seq(gdkkey);
            if (m_call_seq_edit)
            {
                m_call_seq_edit = false;
                callEditorEx(seqnum);
                done = true;
            }
            else if (m_call_seq_eventedit)
            {
                m_call_seq_eventedit = false;
                callEditorEvents(seqnum);
                done = true;
            }
            else if (m_call_seq_shift > 0)      /* variset support  */
            {
                /*
                 * NOT READY HERE
                int keynum = seqnum + m_call_seq_shift * c_seqs_in_set;
                sequence_key(keynum);
                 */
                done = true;
            }
        }
        if (! done)
        {
            /*
             * Replaces a call to Kepler34's sequence_key() function.
             */

            done = perf().keyboard_control_press(gdkkey);   // mute toggles
        }
        if (! done)
        {
            keystroke k(gdkkey, SEQ64_KEYSTROKE_PRESS);
            if (k.is(PREFKEY(pattern_edit)))                // equals sign
            {
                m_call_seq_edit = ! m_call_seq_edit;
                done = true;
            }
            else if (k.is(PREFKEY(event_edit)))             // minus sign
            {
                m_call_seq_eventedit = ! m_call_seq_eventedit;
                done = true;
            }
            if (! done)
            {
                if (k.is(PREFKEY(toggle_mutes)))
                {
                    perf().toggle_playing_tracks();
                    done = true;
                }

                /* **********************************
                 * ACTION handled in the switch below.
                 *
                else if (k.is(PREFKEY(tap_bpm)))
                {
                    m_parent->tap();
                    done = true;
                }
                 */

#ifdef SEQ64_SONG_RECORDING
                else if (k.is(PREFKEY(song_record)))
                {
                    bool record = true;             // TODO
                    perf().song_recording(record);
                }
#endif
            }
        }
    }
    else
    {
        done = true;
        switch (action)
        {
        case perform::ACTION_SEQ_TOGGLE:
            break;

        case perform::ACTION_GROUP_MUTE:
            break;

        case perform::ACTION_BPM:
            m_parent->tap();
            break;

        case perform::ACTION_SCREENSET:             // replaces L/R brackets
            set_bank();                             // screenset from perform
            break;

        case perform::ACTION_GROUP_LEARN:
            break;

        case perform::ACTION_C_STATUS:
            break;

        default:
            done = false;                           // seems wrong
            break;
        }
    }
    return done;
}

/**
 *  The Gtkmm 2.4 version calls perform::mainwnd_key_event().  We have broken
 *  that function into pieces (smaller functions) that we can use here.  An
 *  important point is that keys that affect the GUI directly need to be
 *  handled here in the GUI.  Another important point is that other events are
 *  offloaded to the perform object, and we need to let that object handle as
 *  much as possible.  The logic here is an admixture of events that we will
 *  have to sort out.
 *
 *  Note that the QKeyEWvent::key() function does not distinguish between
 *  capital and non-capital letters, so we use the text() function (returning
 *  the Unicode text the key generated) for this purpose and provide a the
 *  QS_TEXT_CHAR() macro to make it obvious.
 *
 *  Weird.  After the first keystroke, for, say 'o' (ascii 111) == kkey, we
 *  get kkey == 0, presumably a terminator character that we have to ignore.
 *  Also, we can't intercept the Esc key.  Qt grabbing it?
 *
 * \param event
 *      Provides a pointer to the key event.
 */

void
qsliveframe::keyPressEvent (QKeyEvent * event)
{
    unsigned ktext = QS_TEXT_CHAR(event->text());
    unsigned kkey = event->key();
    unsigned gdkkey = qt_map_to_gdk(kkey, ktext);   /* remap to "legacy" keys   */

#ifdef PLATFORM_DEBUG_TMI
    std::string kname = qt_key_name(kkey, ktext);
    printf
    (
        "qsliveframe: name = %s; gdk = 0x%x; key = 0x%x; text = 0x%x\n",
        kname.c_str(), gdkkey, kkey, ktext
    );
#endif

    bool done = handle_key_press(gdkkey);
    if (done)
        update();
    else
        QWidget::keyPressEvent(event);  // event->ignore();
}

/**
 *
 */

void
qsliveframe::keyReleaseEvent (QKeyEvent * event)
{
    /*
     * EXPERIMENTAL COMMENTING
     * unsigned kkey = unsigned(event->key());
     * (void) perf().keyboard_group_c_status_press(kkey);
     */
}

/**
 *
 */

void
qsliveframe::color_by_number (int i)
{
    perf().set_sequence_color(m_curr_seq, i);
}

#if ! defined SEQ64_USE_BUILTIN_PALETTE

/**
 *
 */

void
qsliveframe::color_white ()
{
    perf().set_sequence_color(m_curr_seq, int(SEQ64_COLOR(WHITE)));
}

/**
 *
 */

void
qsliveframe::color_red ()
{
    perf().set_sequence_color(m_curr_seq, int(SEQ64_COLOR(RED)));
}

/**
 *
 */

void
qsliveframe::color_green ()
{
    perf().set_sequence_color(m_curr_seq, int(SEQ64_COLOR(GREEN)));
}

/**
 *
 */

void
qsliveframe::color_blue ()
{
    perf().set_sequence_color(m_curr_seq, int(SEQ64_COLOR(BLUE)));
}

/**
 *
 */

void
qsliveframe::color_yellow ()
{
    perf().set_sequence_color(m_curr_seq, int(SEQ64_COLOR(YELLOW)));
}

/**
 *
 */

void
qsliveframe::color_purple ()
{
    perf().set_sequence_color(m_curr_seq, int(SEQ64_COLOR(MAGENTA)));
}

/**
 *
 */

void
qsliveframe::color_pink ()
{
    perf().set_sequence_color(m_curr_seq, int(SEQ64_COLOR(RED))); // Pink);
}

/**
 *
 */

void
qsliveframe::color_orange ()
{
    perf().set_sequence_color(m_curr_seq, int(SEQ64_COLOR(ORANGE)));
}

/**
 *
 */

void
qsliveframe::color_more (int colorcode)
{
    // perf().set_sequence_color(m_curr_seq, int(SEQ64_COLOR(ORANGE)));
    perf().set_sequence_color(m_curr_seq, colorcode);
}

#endif // defined SEQ64_USE_BUILTIN_PALETTE

/**
 *
 */

void
qsliveframe::copy_seq ()
{
    if (perf().is_active(m_curr_seq))
    {
        m_seq_clipboard.partial_assign(*(perf().get_sequence(m_curr_seq)));
        m_can_paste = true;
    }
}

/**
 *
 */

void
qsliveframe::cut_seq ()
{
    // TODO: dialog warning that the editor is the reason
    // this seq cant be cut

    if (perf().is_active(m_curr_seq) && !perf().is_sequence_in_edit(m_curr_seq))
    {
        m_seq_clipboard.partial_assign(*(perf().get_sequence(m_curr_seq)));
        m_can_paste = true;
        perf().delete_sequence(m_curr_seq);
    }
}

/**
 *  If the sequence/pattern is delete-able (valid and not being edited), then
 *  it is deleted via the perform object.
 */

void
qsliveframe::delete_seq ()
{
    bool valid = perf().is_mseq_valid(m_curr_seq);
    bool not_editing = ! perf().is_sequence_in_edit(m_curr_seq);
    if (valid && not_editing)
    {
        perf().delete_sequence(m_curr_seq);
    }
    else
    {
        /*
         * TODO: Dialog warning that the editor is the reason this seq can't be
         * deleted.
         */
    }
}

/**
 *
 */

void
qsliveframe::paste_seq ()
{
    if (! perf().is_active(m_curr_seq))
    {
        if (perf().new_sequence(m_curr_seq))
        {
            perf().get_sequence(m_curr_seq)->partial_assign(m_seq_clipboard);
            perf().get_sequence(m_curr_seq)->set_dirty();
        }
    }
}

/**
 *  This is not called when focus changes.  Instead, we have to call this from
 *  qliveframeex::changeEvent().
 */

void
qsliveframe::changeEvent (QEvent * event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow())
        {
            m_has_focus = true;             // widget is now active
            perf().set_screenset(m_bank_id);
        }
        else
        {
            m_has_focus = false;            // widget is now inactive
        }
    }
}

}           // namespace seq64

/*
 * qsliveframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

