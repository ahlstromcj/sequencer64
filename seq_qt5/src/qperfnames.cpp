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
 * \file          qperfnames.cpp
 *
 *  This module declares/defines the base class for performance names.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-07-17
 * \license       GNU GPLv2 or above
 *
 *  This module is almost exclusively user-interface code.  There are some
 *  pointers yet that could be replaced by references, and a number of minor
 *  issues that could be fixed.
 *
 *  Adjustments to the performance window can be made with the highlighting
 *  option.  Sequences that don't have events show up as black-on-yellow.
 *  This feature is enabled by default.  To disable this feature, configure
 *  the build with the "--disable-highlight" option.
 *
 * \todo
 *      When bringing up this dialog, and starting play from it, some
 *      extra horizontal lines are drawn for some of the sequences.  This
 *      happens even in seq24, so this is long standing behavior.  Is it
 *      useful, and how?  Where is it done?  In perfroll?
 */

#include <QMouseEvent>

#include "perform.hpp"
#include "qperfnames.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 * Sequence labels for the side of the song editor
 */

qperfnames::qperfnames (perform & p, QWidget * parent)
 :
    QWidget             (parent),
    gui_palette_qt5     (),
    m_perform           (p),
    m_font              ("Monospace"),
    m_sequence_active   (),
    m_sequence_max      (c_max_sequence),
    m_nametext_x        (6 * 2 + 6 * 20),       // not used!
    m_nametext_y        (c_names_y)
{
    m_font.setBold(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    for (int i = 0; i < c_max_sequence; ++i)
        m_sequence_active[i] = false;
}

/**
 *
 */

qperfnames::~qperfnames ()
{
    // no code
}

/**
 *
 */

void
qperfnames::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);
    brush.setStyle((Qt::SolidPattern));
    m_font.setPointSize(7);     // 6
    m_font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    m_font.setStyleHint(QFont::Monospace);               // EXPERIMENT
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);

    int y_s = 0;
    int y_f = height() / m_nametext_y;
    painter.drawRect(0, 0, width(), height() - 1);      // draw border
    for (int y = y_s; y <= y_f; ++y)
    {
        int seqId = y;
        if (seqId < c_max_sequence)
        {
            int i = seqId;
            if (seqId % c_seqs_in_set == 0)     // if first seq in bank
            {
                pen.setColor(Qt::black);        // black boxes to mark each bank
                brush.setColor(Qt::black);
                brush.setStyle(Qt::SolidPattern);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(1, name_y(i) + 1, 15, m_nametext_y - 1);

                char ss[4];
                int bankId = seqId / c_seqs_in_set;
                snprintf(ss, sizeof(ss), "%2d", bankId);

                pen.setColor(Qt::white);        // draw bank number here
                painter.setPen(pen);
                painter.drawText(4, m_nametext_y * i + 15, ss);
                pen.setColor(Qt::black);        // offset, bank name sideways
                painter.setPen(pen);
                painter.save();
                QString bankName(perf().get_bank_name(bankId).c_str());
                painter.translate
                (
                    12, (m_nametext_y * i) + (m_nametext_y * c_seqs_in_set * 0.5)
                        + bankName.length() * 4
                );
                painter.rotate(270);
                m_font.setPointSize(9);
                // m_font.setBold(true);
                m_font.setLetterSpacing(QFont::AbsoluteSpacing, 2);
                painter.setFont(m_font);
                painter.drawText(0, 0, bankName);
                painter.restore();
            }
            int rect_x = 6 * 2 + 4;
            int rect_y = m_nametext_y * i;
            int rect_w = c_names_x - 15;
            if (perf().is_active(seqId))
            {
                std::string sname = perf().sequence_label(seqId); // seq name
                sequence * s = perf().get_sequence(seqId);
                bool muted = s->get_song_mute();
                char name[64];
                m_sequence_active[seqId] = true;
                snprintf
                (
                    name, sizeof name, "%-14.14s   %2d",
                    s->name().c_str(), s->get_midi_channel() + 1
                );

                if (muted)
                {
                    brush.setColor(Qt::black);
                    brush.setStyle(Qt::SolidPattern);
                    painter.setBrush(brush);
                    painter.drawRect(rect_x, rect_y, rect_w, m_nametext_y);
                    pen.setColor(Qt::white);
                }
                else
                {
                    int c = s->color();
                    Color backcolor = get_color_fix(PaletteColor(c));
                    brush.setColor(Qt::white);
                    brush.setStyle(Qt::SolidPattern);
                    painter.setBrush(brush);
                    painter.drawRect(rect_x, rect_y, rect_w, m_nametext_y);
                    brush.setColor(backcolor);
                    pen.setColor(Qt::black);
                }
                painter.setPen(pen);
                painter.drawText(18, rect_y + 10, name);
                painter.drawText(18, rect_y + 20, sname.c_str());
                painter.drawRect(name_x(2), name_y(i), 11, m_nametext_y);
                painter.drawText(name_x(5), name_y(i) + 15, "M");
            }
            else
            {
                pen.setStyle(Qt::SolidLine);
                pen.setColor(Qt::black);
                brush.setColor(Qt::lightGray);
                painter.setPen(pen);        /* fill seq label background    */
                painter.setBrush(brush);
                painter.drawRect(rect_x, rect_y, rect_w, m_nametext_y);
            }
        }
    }
}

/**
 *
 */

QSize
qperfnames::sizeHint () const
{
    return QSize(c_names_x, m_nametext_y * c_max_sequence + 1);
}

/**
 *  Converts a y-value into a sequence number and returns it.  Used in
 *  figuring out which sequence to mute/unmute in the performance editor.
 *
 * \param y
 *      The y value (within the vertical limits of the perfnames column to the
 *      left of the performance editor's piano roll.
 *
 * \return
 *      Returns the sequence number corresponding to the y value.
 */

int
qperfnames::convert_y (int y)
{
    int seq = y / m_nametext_y;            // + m_sequence_offset;
    if (seq >= m_sequence_max)
        seq = m_sequence_max - 1;
    else if (seq < 0)
        seq = 0;

    return seq;
}

/**
 *
 */

void
qperfnames::mousePressEvent (QMouseEvent * ev)
{
    int y = int(ev->y());
    int seqnum = convert_y(y);
    if (ev->button() == Qt::LeftButton)  // (SEQ64_CLICK_LEFT(ev->button))
    {
        bool isshiftkey = (ev->modifiers() & Qt::ShiftModifier) != 0;
        (void) perf().toggle_sequences(seqnum, isshiftkey);
        update();
    }
}

/**
 *
 */

void
qperfnames::mouseReleaseEvent (QMouseEvent * /*ev*/)
{
    // no code; add a call to update() if a change is made
}

/**
 *
 */

void
qperfnames::mouseMoveEvent (QMouseEvent * /*event*/)
{
    // no code; add a call to update() if a change is made
}

}           // namespace seq64

/*
 * qperfnames.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

