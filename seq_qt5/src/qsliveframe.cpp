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
 * \updates       2018-03-11
 * \license       GNU GPLv2 or above
 *
 */

#include "globals.h"
#include "qsliveframe.hpp"
#include "perform.hpp"
#include "forms/qsliveframe.ui.h"

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

    QString bankName = mPerf.get_bank_name(m_bank_id).c_str();
    ui->txtBankName->setPlainText(bankName);
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
 *
 */

void
qsliveframe::drawSequence (int seq)
{
    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush(Qt::darkGray);

    mFont.setPointSize(6);
    mFont.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(mFont);

    midipulse tick = mPerf.get_tick();  // timing info for timed draw elements
    int metro = (tick / c_ppqn) % 2;

    // Grab frame dimensions for scaled drawing

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
        int i = (seq / qc_mainwnd_rows) % qc_mainwnd_cols;
        int j =  seq % qc_mainwnd_rows;
        int base_x = (ui->frame->x() + 1 + (thumbW + qc_mainwid_spacing) * i);
        int base_y = (ui->frame->y() + 1 + (thumbH + qc_mainwid_spacing) * j);
        sequence * s = mPerf.get_sequence(seq);
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
            else if (s->get_queued())       // not playing but queued
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
            else if (s->one_shot())         // queued for one-shot
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
            else                            // just not playing
            {
                pen.setStyle(Qt::NoPen);
                backcolor.setAlpha(180);
                brush.setColor(backcolor);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, thumbW, thumbH);
            }

            pen.setColor(Qt::black);      // write seq data strings, name
            pen.setWidth(1);
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);

            // TODO Use perform::sequence_label() to do this
            char name[20];
            snprintf(name, sizeof name, "%.13s", s->name().c_str());
            painter.drawText(base_x + qc_text_x, base_y + 4, 80, 80, 1, name);

            /* midi channel + key + timesig */
            if (mPerf.show_ui_sequence_key())
            {
                /*
                 * When looking up key, ignore bank offset (print keys on
                 * every bank).  A Kepler34 bank is a Sequencer64 screen-set.
                 */

                QString key;
                key[0] = (char) mPerf.lookup_keyevent_key
                (
                    seq - mPerf.screenset() * c_seqs_in_set
                );
                painter.drawText(base_x + thumbW - 10, base_y + thumbH - 5, key);
            }

            QString seqInfo = QString::number(s->get_midi_bus() + 1);
            seqInfo.append("-");
            seqInfo.append(QString::number(s->get_midi_channel() + 1));

            painter.drawText(base_x + 5, base_y + thumbH - 5, seqInfo);

            int rectangle_x = base_x + 7;
            int rectangle_y = base_y + 15;

            pen.setColor(Qt::gray);
            brush.setStyle(Qt::NoBrush);
            painter.setBrush(brush);
            painter.setPen(pen);
            //draw inner box for notes
            painter.drawRect(rectangle_x-2, rectangle_y-1, previewW, previewH);

            int lowest_note;    //  = s->get_lowest_note_event();
            int highest_note;   //  = s->get_highest_note_event();
            (void) s->get_minmax_note_events(lowest_note, highest_note);

            int height = highest_note - lowest_note + 2;
            int length = s->get_length();
            long tick_s;
            long tick_f;
            int note;
            bool selected;
            int velocity;
            draw_type_t dt;
            s->reset_draw_marker();

            previewH -= 6;          // add padding to box measurements
            previewW -= 6;
            rectangle_x += 2;
            rectangle_y += 2;

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
                int note_y = previewH -
                             (previewH  * (note + 1 - lowest_note)) / height ;

                int tick_s_x = (tick_s * previewW)  / length;
                int tick_f_x = (tick_f * previewH)  / length;

                if (dt == DRAW_NOTE_ON || dt == DRAW_NOTE_OFF)
                    tick_f_x = tick_s_x + 1;
                if (tick_f_x <= tick_s_x)
                    tick_f_x = tick_s_x + 1;

                pen.setColor(Qt::black); // draw line representing this note
                pen.setWidth(2);
                painter.setPen(pen);
                painter.drawLine(rectangle_x + tick_s_x,
                                   rectangle_y + note_y,
                                   rectangle_x + tick_f_x,
                                   rectangle_y + note_y);
            }

            int a_tick = mPerf.get_tick();     // draw playhead
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
        else
        {
            pen.setColor(Qt::black);
            pen.setStyle(Qt::NoPen);
            mFont.setPointSize(15);
            painter.setPen(pen);
            painter.setFont(mFont);

            // draw outline of this seq thumbnail

            painter.drawRect(base_x, base_y, thumbW, thumbH);

            // no sequence present. Insert placeholder
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
            painter.drawText(base_x + 2, base_y + 17, "+");
        }
    }

    // lessen alpha on each redraw to have smooth fading
    // done as a factor of the bpm to get useful fades

    alpha *= 0.7 - mPerf.bpm() / 300.0;
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
qsliveframe::setBank (int newBank)
{
    m_bank_id = newBank;
    if (m_bank_id < 0)
        m_bank_id = qc_max_num_banks - 1;

    if (m_bank_id >= qc_max_num_banks)              // 32
        m_bank_id = 0;

    mPerf.set_screenset(m_bank_id);        // set_offset(m_bank_id);

    QString bankName = mPerf.get_bank_name(m_bank_id).c_str();
    ui->txtBankName->setPlainText(bankName);
    ui->spinBank->setValue(m_bank_id);
    update();
}

/**
 *
 */

void
qsliveframe::redraw ()
{
    drawAllSequences();
}

/**
 *
 */

void
qsliveframe::updateBank (int newBank)
{
    mPerf.set_screenset(newBank);
    setBank(newBank);
    mPerf.modify();
}

/**
 *
 */

void
qsliveframe::updateBankName ()
{
    updateInternalBankName();
    mPerf.modify();
}

/**
 *
 */

void
qsliveframe::updateInternalBankName ()
{
    std::string name = ui->txtBankName->document()->toPlainText().toStdString();
    mPerf.set_screenset_notepad(m_bank_id, name);
}

/**
 *
 */

int
qsliveframe::seqIDFromClickXY (int click_x, int click_y)
{
    int x = click_x - qc_mainwid_border; /* adjust for border */
    int y = click_y - qc_mainwid_border;

    /* is it in the box ? */

    if (x < 0
            || x >= ((thumbW + qc_mainwid_spacing) * qc_mainwnd_cols)
            || y < 0
            || y >= ((thumbH + qc_mainwid_spacing) * qc_mainwnd_rows))
    {
        return -1;
    }

    /* gives us in box corrdinates */

    int box_test_x = x % (thumbW + qc_mainwid_spacing);
    int box_test_y = y % (thumbH + qc_mainwid_spacing);

    /* right inactive side of area */

    if (box_test_x > thumbW || box_test_y > thumbH)
        return -1;

    x /= (thumbW + qc_mainwid_spacing);
    y /= (thumbH + qc_mainwid_spacing);
    int seqId = ((x * qc_mainwnd_rows + y)
                 + (m_bank_id * qc_mainwnd_rows * qc_mainwnd_cols));

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
        if (mPerf.is_active(m_curr_seq))
        {
            if (! mAddingNew)
                mPerf.sequence_playing_toggle(m_curr_seq);

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
            ! mPerf.is_active(m_curr_seq) && m_curr_seq != -1 &&
                ! mPerf.is_sequence_in_edit(m_curr_seq))
        {
            mPerf.new_sequence(m_curr_seq);
            mPerf.get_sequence(m_curr_seq)->partial_assign(m_moving_seq);
            update();
        }
        else
        {
            mPerf.new_sequence(mOldSeq);
            mPerf.get_sequence(mOldSeq)->partial_assign(m_moving_seq);
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

        if (mPerf.is_active(m_curr_seq))
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
        mPerf.is_active(m_curr_seq)
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
            ! mPerf.is_sequence_in_edit(m_curr_seq))
        {
            /*
             * Drag a sequence between slots; save the sequence and clear the
             * old slot.
             */

            if (mPerf.is_active(m_curr_seq))
            {
                mOldSeq = m_curr_seq;
                mMoving = true;
                m_moving_seq.partial_assign(*(mPerf.get_sequence(m_curr_seq)));
                mPerf.delete_sequence(m_curr_seq);
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
    if (mPerf.is_active(m_curr_seq))
    {
        int choice = mMsgBoxNewSeqCheck->exec();
        if (choice == QMessageBox::No)
            return;
    }
    mPerf.new_sequence(m_curr_seq);
    mPerf.get_sequence(m_curr_seq)->set_dirty();

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
 *
 */

void
qsliveframe::keyPressEvent (QKeyEvent * event)
{
    switch (event->key())
    {
    case Qt::Key_BracketLeft:
        setBank(m_bank_id - 1);
        break;

    case Qt::Key_BracketRight:
        setBank(m_bank_id + 1);
        break;

    case Qt::Key_Semicolon:
        mPerf.set_sequence_control_status(c_status_replace);
        break;

    case Qt::Key_Slash:
        mPerf.set_sequence_control_status(c_status_queue);
        break;

    case Qt::Key_Apostrophe:
    case Qt::Key_NumberSign:
        mPerf.set_sequence_control_status(c_status_snapshot);
        break;

    case Qt::Key_Period:
        mPerf.set_sequence_control_status(c_status_oneshot);
        break;

    default:                                // sequence mute toggling

        // quint32 keycode = event->key();
        // if (mPerf.get_key_events()->count(event->key()) != 0)

        if (mPerf.get_key_count(event->key()) != 0)
            sequence_key(mPerf.lookup_keyevent_seq(event->key()));
        else
            event->ignore();
        break;
    }
}

/**
 *
 */

void
qsliveframe::keyReleaseEvent (QKeyEvent * event)
{
    // unset the relevant control modifiers

    switch (event->key())
    {
    case Qt::Key_Semicolon:
        mPerf.unset_sequence_control_status(c_status_replace);
        break;

    case Qt::Key_Slash:
        mPerf.unset_sequence_control_status(c_status_queue);
        break;

    case Qt::Key_Apostrophe:
    case Qt::Key_NumberSign:
        mPerf.unset_sequence_control_status(c_status_snapshot);
        break;

    case Qt::Key_Period:
        mPerf.unset_sequence_control_status(c_status_oneshot);
        break;
    }
}

/**
 *
 */

void
qsliveframe::sequence_key (int seq)
{
    /* add bank offset */
    seq += mPerf.screenset() * qc_mainwnd_rows * qc_mainwnd_cols;
    if (mPerf.is_active(seq))
        mPerf.sequence_playing_toggle(seq);
}

/**
 *
 */

void
qsliveframe::color_white ()
{
    mPerf.set_sequence_color(m_curr_seq, int(PaletteColor::WHITE));
}

/**
 *
 */

void
qsliveframe::color_red ()
{
    mPerf.set_sequence_color(m_curr_seq, int(PaletteColor::RED));
}

/**
 *
 */

void
qsliveframe::color_green ()
{
    mPerf.set_sequence_color(m_curr_seq, int(PaletteColor::GREEN));
}

/**
 *
 */

void
qsliveframe::color_blue ()
{
    mPerf.set_sequence_color(m_curr_seq, int(PaletteColor::BLUE));
}

/**
 *
 */

void
qsliveframe::color_yellow ()
{
    mPerf.set_sequence_color(m_curr_seq, int(PaletteColor::YELLOW));
}

/**
 *
 */

void
qsliveframe::color_purple ()
{
    mPerf.set_sequence_color(m_curr_seq, int(PaletteColor::MAGENTA));
}

/**
 *
 */

void
qsliveframe::color_pink ()
{
    mPerf.set_sequence_color(m_curr_seq, int(PaletteColor::RED)); // Pink);
}

/**
 *
 */

void
qsliveframe::color_orange ()
{
    mPerf.set_sequence_color(m_curr_seq, int(PaletteColor::ORANGE));
}

/**
 *
 */

void
qsliveframe::copySeq ()
{
    if (mPerf.is_active(m_curr_seq))
    {
        m_seq_clipboard.partial_assign(*(mPerf.get_sequence(m_curr_seq)));
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

    if (mPerf.is_active(m_curr_seq) && !mPerf.is_sequence_in_edit(m_curr_seq))
    {
        m_seq_clipboard.partial_assign(*(mPerf.get_sequence(m_curr_seq)));
        mCanPaste = true;
        mPerf.delete_sequence(m_curr_seq);
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
        mPerf.is_active(m_curr_seq) && !mPerf.is_sequence_in_edit(m_curr_seq)
    )
    {
        // TODO
        //dialog warning that the editor is the reason this seq cant be deleted
        mPerf.delete_sequence(m_curr_seq);
    }
}

/**
 *
 */

void
qsliveframe::pasteSeq ()
{
    if (! mPerf.is_active(m_curr_seq))
    {
        mPerf.new_sequence(m_curr_seq);
        mPerf.get_sequence(m_curr_seq)->partial_assign(m_seq_clipboard);
        mPerf.get_sequence(m_curr_seq)->set_dirty();
    }
}

}           // namespace seq64

/*
 * qsliveframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

