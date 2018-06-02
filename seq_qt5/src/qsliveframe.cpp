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
 *  This module declares/defines the base class for plastering
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-05-31
 * \license       GNU GPLv2 or above
 *
 */

#include "globals.h"
#include "keystroke.hpp"
#include "perform.hpp"
#include "qskeymaps.hpp"                /* mapping between Gtkmm and Qt     */
#include "qsliveframe.hpp"
#include "qsmacros.hpp"                 /* QS_TEXT_CHAR() macro             */
#include "settings.hpp"

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qsliveframe.h"
#else
#include "forms/qsliveframe.ui.h"
#endif

/*
 * Do not document a namespace, it breaks Doxygen.
 */

namespace seq64
{

/**
 *
 */

qsliveframe::qsliveframe (perform & perf, QWidget * parent)
 :
    QFrame              (parent),
    ui                  (new Ui::qsliveframe),
    mPerf               (perf),
    m_moving_seq        (),
    m_seq_clipboard     (),
    mPopup              (nullptr),
    mRedrawTimer        (nullptr),
    mMsgBoxNewSeqCheck  (nullptr),
    mFont               (),
    m_bank_id           (0),
    thumbW              (0),
    thumbH              (0),
    previewW            (0),
    previewH            (0),
    lastMetro           (0),
    alpha               (0),
    m_curr_seq          (0),            // mouse interaction
    mOldSeq             (0),
    mButtonDown         (false),
    mMoving             (false),
    mAddingNew          (false),
    m_last_tick_x       (),             // array
    m_last_playing      (),             // array
    mCanPaste           (false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    ui->setupUi(this);
    mMsgBoxNewSeqCheck = new QMessageBox(this);
    mMsgBoxNewSeqCheck->setText(tr("Sequence already present"));
    mMsgBoxNewSeqCheck->setInformativeText
    (
        tr
        (
            "There is already a sequence stored in this slot. "
            "Overwrite it and create a new blank sequence?"
        )
    );
    mMsgBoxNewSeqCheck->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mMsgBoxNewSeqCheck->setDefaultButton(QMessageBox::No);
    setBank(0);

    QString bname = mPerf.get_bank_name(m_bank_id).c_str();
    ui->txtBankName->setPlainText(bname);
    connect(ui->spinBank, SIGNAL(valueChanged(int)), this, SLOT(updateBank(int)));
    connect(ui->txtBankName, SIGNAL(textChanged()), this, SLOT(updateBankName()));

    // Refresh timer to queue regular redraws

    mRedrawTimer = new QTimer(this);
    mRedrawTimer->setInterval(50);
    connect(mRedrawTimer, SIGNAL(timeout()), this, SLOT(update()));
    mRedrawTimer->start();
}

/**
 *
 */

qsliveframe::~qsliveframe()
{
    delete ui;
    delete mMsgBoxNewSeqCheck;
    delete mRedrawTimer;
}

/**
 *
 */

void
qsliveframe::paintEvent (QPaintEvent *)
{
    drawAllSequences();
}

/**
 *  Provides a way to calculate the base x and y size values for the
 *  pattern map.  The values are returned as side-effects.
 *
 *  Compare it to mainwid::calculate_base_sizes():
 *
 *      -   m_mainwnd_rows and m_mainwnd_cols are qc_mainwnd_rows and
 *          qc_mainwnd_cols.
 *      -   m_mainwid_border_x and m_mainwid_border_y are
 *          ui->frame->x() and ui->frame->y(), which can be alter by the
 *          user via resizing the main window.
 *      -   m_seqarea_x and m_seqarea_y are the thumbW and thumbH members
 *          (!).
 *      -   m_mainwid_spacing is qc_mainwid_spacing.
 *
 * \param seqnum
 *      Provides the number of the sequence to calculate.
 *
 * \param [out] basex
 *      A return parameter for the x coordinate of the base size.
 *
 * \param [out] basey
 *      A return parameter for the y coordinate of the base size.
 */

void
qsliveframe::calculate_base_sizes (int seqnum, int & basex, int & basey)
{
    int i = (seqnum / qc_mainwnd_rows) % qc_mainwnd_cols;
    int j =  seqnum % qc_mainwnd_rows;
    basex = ui->frame->x() + 1 + (thumbW + qc_mainwid_spacing) * i;
    basey = ui->frame->y() + 1 + (thumbH + qc_mainwid_spacing) * j;
}

/**
 *
 */

void
qsliveframe::drawSequence (int seq)
{
    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush(Qt::black);            // QBrush brush(Qt::darkGray);
    mFont.setPointSize(6);
    mFont.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(mFont);

    midipulse tick = perf().get_tick();  // timing info for timed elements
    int metro = (tick / c_ppqn) % 2;

    /*
     * Grab frame dimensions for scaled drawing.  Note that the frame
     * size can be modified by the user dragging a corner.
     */

    thumbW = (ui->frame->width() - 1 - qc_mainwid_spacing * 8) / qc_mainwnd_cols;
    thumbH = (ui->frame->height() - 1 - qc_mainwid_spacing * 5) / qc_mainwnd_rows;
    previewW = thumbW - mFont.pointSize() * 2;
    previewH = thumbH - mFont.pointSize() * 5;
    if
    (
        seq >= (m_bank_id * qc_mainwnd_rows * qc_mainwnd_cols) &&
        seq < ((m_bank_id + 1) * qc_mainwnd_rows * qc_mainwnd_cols)
    )
    {
        int base_x, base_y;
        calculate_base_sizes(seq, base_x, base_y);    /* side-effects    */
        sequence * s = perf().get_sequence(seq);
        if (not_nullptr(s))
        {
            int c = s->color();
            Color backcolor = get_color_fix(PaletteColor(c));
            pen.setColor(Qt::black);
            pen.setStyle(Qt::SolidLine);
            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
            {
                /*
                 * Playing but queued to mute, or turning off after snapping.
                 */

                pen.setWidth(2);
                pen.setColor(Qt::black);
                pen.setStyle(Qt::DashLine);
                painter.setPen(pen);
                backcolor.setAlpha(210);
                brush.setColor(backcolor);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, thumbW + 1, thumbH + 1);
            }
            else if (s->get_playing())              // playing, no queueing
            {
                pen.setWidth(2);
                painter.setPen(pen);
                backcolor.setAlpha(210);
                brush.setColor(backcolor);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, thumbW + 1, thumbH + 1);
            }
            else if (s->get_queued())               // not playing but queued
            {
                pen.setWidth(2);
                pen.setColor(Qt::darkGray);
                pen.setStyle(Qt::DashLine);
                backcolor.setAlpha(180);
                brush.setColor(backcolor);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, thumbW, thumbH);
            }
            else if (s->one_shot())                 // queued for one-shot
            {
                pen.setWidth(2);
                pen.setColor(Qt::darkGray);
                pen.setStyle(Qt::DotLine);
                backcolor.setAlpha(180);
                brush.setColor(backcolor);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, thumbW, thumbH);
            }
            else                                    // just not playing
            {
                pen.setStyle(Qt::NoPen);
                backcolor.setAlpha(180);
                brush.setColor(backcolor);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, thumbW, thumbH);
            }

            std::string st = perf().sequence_title(*s);
            QString title(st.c_str());
            pen.setColor(Qt::black);                // write seq strings, name
            pen.setWidth(1);
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
            painter.drawText(base_x + qc_text_x, base_y + 4, 80, 80, 1, title);

            std::string sl = perf().sequence_label(*s);
            QString label(sl.c_str());
            painter.drawText(base_x + 8, base_y + thumbH - 5, label);
            if (perf().show_ui_sequence_key())
            {
                QString key;
                key[0] = (char) perf().lookup_keyevent_key
                (
                    seq - perf().screenset() * c_seqs_in_set
                );
                painter.drawText(base_x + thumbW - 10, base_y + thumbH - 5, key);
            }

            int rectangle_x = base_x + 7;
            int rectangle_y = base_y + 15;
            pen.setColor(Qt::gray);             // pen.setColor(Qt::black);
            brush.setStyle(Qt::NoBrush);
            painter.setBrush(brush);
            painter.setPen(pen);

            // inner box for notes

            painter.drawRect(rectangle_x-2, rectangle_y-1, previewW, previewH);

            int lowest_note;
            int highest_note;
            bool have_notes = s->get_minmax_note_events(lowest_note, highest_note);
            if (have_notes)
            {
                int height = highest_note - lowest_note + 2;
                int length = s->get_length();
                midipulse tick_s;
                midipulse tick_f;
                int note;
                bool selected;
                int velocity;
                draw_type_t dt;
                Color drawcolor = fg_color();
                Color eventcolor = fg_color();

#ifdef SEQ64_STAZED_TRANSPOSE
                if (! s->get_transposable())
                {
                    eventcolor = red();
                    drawcolor = red();
                }
#endif
                previewH -= 6;          // add padding to box measurements
                previewW -= 6;
                rectangle_x += 2;
                rectangle_y += 2;
                s->reset_draw_marker();     /* reset container iterator */
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
                    int tick_s_x = (tick_s * previewW) / length;
                    int tick_f_x = (tick_f * previewH) / length;
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
                        // pen.setColor(tempo_paint());
                        drawcolor = tempo_paint();
                        note_y = thumbW -
                             thumbH * (note + 1) / SEQ64_MAX_DATA_VALUE;
                    }
                    else
                    {
                        pen.setWidth(1);            // 2 is too thick
                        // pen.setColor(Qt::black); // draw line for note
                        // pen.setColor(drawcolor); // draw line for note
                        note_y = previewH -
                             (previewH * (note+1-lowest_note)) / height;
                    }

                    int sx = rectangle_x + tick_s_x;        /* start x  */
                    int fx = rectangle_x + tick_f_x;        /* finish x */
                    int sy = rectangle_y + note_y;          /* start y  */
                    int fy = sy;                            /* finish y */
                    pen.setColor(drawcolor);    /* draw line for note   */
                    painter.setPen(pen);
                    painter.drawLine(sx, sy, fx, fy);
//                      (
//                          rectangle_x + tick_s_x, rectangle_y + note_y,
//                          rectangle_x + tick_f_x, rectangle_y + note_y
//                      );
                    if (dt == DRAW_TEMPO)
                    {
                        pen.setWidth(1);            // 2 is too thick
                        // pen.setColor(Qt::black);    // eventcolor
                        // pen.setColor(eventcolor);
                        drawcolor = eventcolor;
                    }
                }

                int a_tick = perf().get_tick();     // draw playhead
                a_tick += (length - s->get_trigger_offset());
                a_tick %= length;

                midipulse tick_x = a_tick * previewW / length;
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
                    rectangle_x + tick_x - 1, rectangle_y + previewH + 1
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

            mFont.setPointSize(15);
            pen.setColor(Qt::black);        // or dark gray?
            painter.setPen(pen);
            painter.setFont(mFont);
            painter.drawRect(base_x, base_y, thumbW, thumbH);   // outline

            /*
             * No sequence present. Insert placeholder.  (Not a big fan of this
             * one.)
             *
             *  pen.setStyle(Qt::SolidLine);
             *  painter.setPen(pen);
             *  painter.drawText(base_x + 2, base_y + 17, "+");
             */

            if (perf().show_ui_sequence_number())
            {
                int lx = base_x + (thumbW / 2) - 7;
                int ly = base_y + (thumbH / 2) + 5;
                char snum[8];
                snprintf(snum, sizeof snum, "%d", seq);
                mFont.setPointSize(8);
                pen.setColor(Qt::white);    // pen.setColor(Qt::black);
                pen.setWidth(1);
                pen.setStyle(Qt::SolidLine);
                painter.setPen(pen);
                painter.setFont(mFont);
                painter.drawText(lx, ly, snum);
            }
        }
    }

    // lessen alpha on each redraw to have smooth fading
    // done as a factor of the bpm to get useful fades

    alpha *= 0.7 - perf().bpm() / 300.0;
    lastMetro = metro;
}

/**
 *
 */

void
qsliveframe::drawAllSequences ()
{
    for (int i = 0; i < (qc_mainwnd_rows * qc_mainwnd_cols); i++)
    {
        drawSequence(i + (m_bank_id * qc_mainwnd_rows * qc_mainwnd_cols));
        m_last_tick_x[i + (m_bank_id * qc_mainwnd_rows * qc_mainwnd_cols)] = 0;
    }
}

/**
 *
 */

void
qsliveframe::setBank ()
{
    int bank = perf().screenset();
    setBank(bank);
}

/**
 *
 */

void
qsliveframe::setBank (int bank)
{
    QString bname = perf().get_bank_name(bank).c_str();
    ui->txtBankName->setPlainText(bname);
    ui->spinBank->setValue(bank);
    m_bank_id = bank;
    update();
}

/**
 *
 */

void
qsliveframe::updateBank (int bank)
{
    perf().set_screenset(bank);
    setBank(bank);
    perf().modify();
}

/**
 *
 */

void
qsliveframe::updateBankName ()
{
    updateInternalBankName();
    perf().modify();
}

/**
 *
 */

void
qsliveframe::updateInternalBankName ()
{
    std::string name = ui->txtBankName->document()->toPlainText().toStdString();
    perf().set_screenset_notepad(m_bank_id, name);
}

/**
 *
 */

int
qsliveframe::seqIDFromClickXY (int click_x, int click_y)
{
    int x = click_x - qc_mainwid_border;        /* adjust for border */
    int y = click_y - qc_mainwid_border;
    if                                          /* is it in the box ? */
    (
        x < 0 || x >= ((thumbW + qc_mainwid_spacing) * qc_mainwnd_cols) ||
        y < 0 || y >= ((thumbH + qc_mainwid_spacing) * qc_mainwnd_rows))
    {
        return -1;
    }

    /* gives us x, y in box coordinates */

    int box_test_x = x % (thumbW + qc_mainwid_spacing);
    int box_test_y = y % (thumbH + qc_mainwid_spacing);

    /* right inactive side of area */

    if (box_test_x > thumbW || box_test_y > thumbH)
        return -1;

    x /= (thumbW + qc_mainwid_spacing);
    y /= (thumbH + qc_mainwid_spacing);
    int seqId =
    (
        (x * qc_mainwnd_rows + y) +
        (m_bank_id * qc_mainwnd_rows * qc_mainwnd_cols)
    );
    return seqId;
}

/**
 *
 */

void
qsliveframe::mousePressEvent (QMouseEvent * event)
{
    m_curr_seq = seqIDFromClickXY(event->x(), event->y());
    if (m_curr_seq != -1 && event->button() == Qt::LeftButton)
        mButtonDown = true;
}

/**
 *
 */

void
qsliveframe::mouseReleaseEvent (QMouseEvent *event)
{
    /* get the sequence number we clicked on */

    m_curr_seq = seqIDFromClickXY( event->x(), event->y());
    mButtonDown = false;

    /*
     * if we're on a valid sequence, hit the left mouse button, and are not
     * dragging a sequence - toggle playing.
     */

    if (m_curr_seq != -1 && event->button() == Qt::LeftButton && ! mMoving)
    {
        if (perf().is_active(m_curr_seq))
        {
            if (! mAddingNew)
                perf().sequence_playing_toggle(m_curr_seq);

            mAddingNew = false;
            update();
        }
        else
            mAddingNew = true;
    }

    /* if left mouse button & we're moving a seq between slots */

    if (event->button() == Qt::LeftButton && mMoving)
    {
        mMoving = false;
        if
        (
            ! perf().is_active(m_curr_seq) && m_curr_seq != -1 &&
                ! perf().is_sequence_in_edit(m_curr_seq))
        {
            perf().new_sequence(m_curr_seq);
            perf().get_sequence(m_curr_seq)->partial_assign(m_moving_seq);
            update();
        }
        else
        {
            perf().new_sequence(mOldSeq);
            perf().get_sequence(mOldSeq)->partial_assign(m_moving_seq);
            update();
        }
    }

    /* check for right mouse click - this launches the popup menu */

    if (m_curr_seq != -1 && event->button() == Qt::RightButton)
    {
        mPopup = new QMenu(this);

        //new sequence option
        QAction *actionNew = new QAction(tr("New sequence"), mPopup);
        mPopup->addAction(actionNew);
        QObject::connect
        (
            actionNew, SIGNAL(triggered(bool)),
            this, SLOT(newSeq())
        );

        if (perf().is_active(m_curr_seq))
        {
            // edit sequence

            QAction * actionEdit = new QAction(tr("Edit sequence"), mPopup);
            mPopup->addAction(actionEdit);
            connect(actionEdit, SIGNAL(triggered(bool)), this, SLOT(editSeq()));

            // set the colour from the scheme

            QMenu * menuColour = new QMenu(tr("Set colour..."));
            QAction * color[8];
            color[0] = new QAction(tr("White"), menuColour);
            color[1] = new QAction(tr("Red"), menuColour);
            color[2] = new QAction(tr("Green"), menuColour);
            color[3] = new QAction(tr("Blue"), menuColour);
            color[4] = new QAction(tr("Yellow"), menuColour);
            color[5] = new QAction(tr("Purple"), menuColour);
            color[6] = new QAction(tr("Pink"), menuColour);
            color[7] = new QAction(tr("Orange"), menuColour);

            connect(color[0], SIGNAL(triggered(bool)), this, SLOT(color_white()));
            connect(color[1], SIGNAL(triggered(bool)), this, SLOT(color_red()));
            connect(color[2], SIGNAL(triggered(bool)), this, SLOT(color_green()));
            connect(color[3], SIGNAL(triggered(bool)), this, SLOT(color_blue()));
            connect(color[4], SIGNAL(triggered(bool)), this, SLOT(color_yellow()));
            connect(color[5], SIGNAL(triggered(bool)), this, SLOT(color_purple()));
            connect(color[6], SIGNAL(triggered(bool)), this, SLOT(color_pink()));
            connect(color[7], SIGNAL(triggered(bool)), this, SLOT(color_orange()));

            for (int i = 0; i < 8; ++i)
            {
                menuColour->addAction(color[i]);
            }

            mPopup->addMenu(menuColour);

            QAction * actionCopy = new QAction(tr("Copy sequence"), mPopup);
            mPopup->addAction(actionCopy);
            connect(actionCopy, SIGNAL(triggered(bool)), this, SLOT(copySeq()));

            QAction * actionCut = new QAction(tr("Cut sequence"), mPopup);
            mPopup->addAction(actionCut);
            connect(actionCut, SIGNAL(triggered(bool)), this, SLOT(cutSeq()));

            QAction * actionDelete = new QAction(tr("Delete sequence"), mPopup);
            mPopup->addAction(actionDelete);
            connect
            (
                actionDelete, SIGNAL(triggered(bool)), this, SLOT(deleteSeq())
            );
        }
        else if (mCanPaste)
        {
            QAction * actionPaste = new QAction(tr("Paste sequence"), mPopup);
            mPopup->addAction(actionPaste);
            connect(actionPaste, SIGNAL(triggered(bool)), this, SLOT(pasteSeq()));
        }
        mPopup->exec(QCursor::pos());
    }

    if                      // middle button launches seq editor
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
    int seqId = seqIDFromClickXY(event->x(), event->y());
    if (mButtonDown)
    {
        if (seqId != m_curr_seq && ! mMoving &&
            ! perf().is_sequence_in_edit(m_curr_seq))
        {
            /*
             * Drag a sequence between slots; save the sequence and clear the
             * old slot.
             */

            if (perf().is_active(m_curr_seq))
            {
                mOldSeq = m_curr_seq;
                mMoving = true;
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
qsliveframe::mouseDoubleClickEvent (QMouseEvent *)
{
    if (mAddingNew)
        newSeq();
}

/**
 *
 */

void
qsliveframe::newSeq()
{
    if (perf().is_active(m_curr_seq))
    {
        int choice = mMsgBoxNewSeqCheck->exec();
        if (choice == QMessageBox::No)
            return;
    }
    perf().new_sequence(m_curr_seq);
    perf().get_sequence(m_curr_seq)->set_dirty();

    //TODO reenable - disabled opening the editor for each new seq
    //    callEditor(m_main_perf->get_sequence(m_current_seq));

}

/**
 *
 */

void
qsliveframe::editSeq ()
{
    callEditor(m_curr_seq);
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
    unsigned gdkkey = qt_map_to_gdk(kkey, ktext);

#ifdef PLATFORM_DEBUG_TMI
    std::string kname = qt_key_name(kkey, ktext);
    printf
    (
        "qsliveframe: name = %s; gdk = 0x%x; key = 0x%x; text = 0x%x\n",
        kname.c_str(), gdkkey, kkey, ktext
    );
#endif

    perform::action_t action = perf().keyboard_group_action(gdkkey);
    bool done = false;
    if (action == perform::ACTION_NONE)
    {
        /*
         * This call replaces Kepler34's processing of the semi-colon, slash,
         * apostrophe, number sign, and period.
         */

        done = perf().keyboard_group_c_status_press(gdkkey);
        if (! done)
        {
            /*
             * Replaces a call to Kepler34's sequence_key() function.
             */

            done = perf().keyboard_control_press(gdkkey);   // mute toggles
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
            break;

        case perform::ACTION_SCREENSET:             // replaces L/R brackets
            setBank();                              // screenset from perform
            break;

        case perform::ACTION_GROUP_LEARN:
            break;

        case perform::ACTION_C_STATUS:
            break;

        default:
            done = false;
            break;
        }
    }
    if (! done)
        event->ignore();
}

/**
 *
 */

void
qsliveframe::keyReleaseEvent (QKeyEvent * event)
{
    unsigned kkey = unsigned(event->key());
    (void) perf().keyboard_group_c_status_press(kkey);
}

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
qsliveframe::copySeq ()
{
    if (perf().is_active(m_curr_seq))
    {
        m_seq_clipboard.partial_assign(*(perf().get_sequence(m_curr_seq)));
        mCanPaste = true;
    }
}

/**
 *
 */

void
qsliveframe::cutSeq ()
{
    // TODO: dialog warning that the editor is the reason
    // this seq cant be cut

    if (perf().is_active(m_curr_seq) && !perf().is_sequence_in_edit(m_curr_seq))
    {
        m_seq_clipboard.partial_assign(*(perf().get_sequence(m_curr_seq)));
        mCanPaste = true;
        perf().delete_sequence(m_curr_seq);
    }
}

/**
 *
 */

void
qsliveframe::deleteSeq ()
{
    if
    (
        perf().is_active(m_curr_seq) && !perf().is_sequence_in_edit(m_curr_seq)
    )
    {
        // TODO
        //dialog warning that the editor is the reason this seq cant be deleted
        perf().delete_sequence(m_curr_seq);
    }
}

/**
 *
 */

void
qsliveframe::pasteSeq ()
{
    if (! perf().is_active(m_curr_seq))
    {
        perf().new_sequence(m_curr_seq);
        perf().get_sequence(m_curr_seq)->partial_assign(m_seq_clipboard);
        perf().get_sequence(m_curr_seq)->set_dirty();
    }
}

}           // namespace seq64

/*
 * qsliveframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

