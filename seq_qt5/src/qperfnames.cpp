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
 * \updates       2018-03-26
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
    mPerf               (p),
    mTimer              (nullptr),
    mFont               (),
    m_sequence_active   (),
    m_nametext_x        (6 * 2 + 6 * 20),
    m_nametext_y        (c_names_y)
{
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
    mFont.setPointSize(6);
    mFont.setLetterSpacing(QFont::AbsoluteSpacing, 1);

    mFont.setStyleHint(QFont::Monospace);       // EXPERIMENT

    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(mFont);

    int y_s = 0;
    int y_f = height() / c_names_y;
    painter.drawRect(0, 0, width(), height() - 1);    // draw border
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
                painter.drawRect(1, name_y(i) + 1, 15, c_names_y - 1);

                char ss[4];
                int bankId = seqId / c_seqs_in_set;
                snprintf(ss, sizeof(ss), "%2d", bankId);

                pen.setColor(Qt::white);        // draw bank number here
                painter.setPen(pen);
                painter.drawText(4, c_names_y * i + 15, ss);

                // offset and draw bank name sideways

                pen.setColor(Qt::black);
                painter.setPen(pen);
                painter.save();
                QString bankName(perf().get_bank_name(bankId).c_str());
                painter.translate
                (
                    12, (c_names_y * i) + (c_names_y * c_seqs_in_set * 0.5)
                        + bankName.length() * 4
                );
                painter.rotate(270);
                mFont.setPointSize(9);
                mFont.setBold(true);
                mFont.setLetterSpacing(QFont::AbsoluteSpacing, 2);
                painter.setFont(mFont);
                painter.drawText(0, 0, bankName);
                painter.restore();
            }
            pen.setStyle(Qt::SolidLine);
            pen.setColor(Qt::black);
            sequence * s = mPerf.get_sequence(seqId);
            if (not_nullptr(s))
            {
                int c = s->color();
                Color backcolor = get_color_fix(PaletteColor(c));
                brush.setColor(backcolor);
            }
            else
                brush.setColor(Qt::lightGray);

            painter.setPen(pen);    // fill seq label background
            painter.setBrush(brush);
            painter.drawRect(6 * 2 + 4, c_names_y * i, c_names_x - 15, c_names_y);
            if (perf().is_active(seqId))
            {
                m_sequence_active[seqId] = true;

                char name[64];
                snprintf
                (
                    // name, sizeof name, "%-14.14s   %2d",
                    name, sizeof name, "%-14.14s         %2d",
                    perf().get_sequence(seqId)->name().c_str(),
                    perf().get_sequence(seqId)->get_midi_channel() + 1
                );

                pen.setColor(Qt::black);
                painter.setPen(pen);
                painter.drawText(18, c_names_y * i + 10, name);

                std::string sname = perf().sequence_label(seqId); // seq name
                painter.drawText(18, c_names_y * i + 20, sname.c_str());

                bool muted = perf().get_sequence(seqId)->get_song_mute();
                pen.setColor(Qt::black);
                painter.setPen(pen);
                painter.drawRect(name_x(2), name_y(i), 10, m_nametext_y);
//              (
//                  6 * 2 + 6 * 20 + 2, (c_names_y * i), 10, c_names_y
//              );

                if (muted) // seq mute state
                {
                    painter.drawText(name_x(4), name_y(i) + 14, "M");
//                  (
//                      6 * 2 + 6 * 20 + 4, c_names_y * i + 14, "M"
//                  );
                }
                else
                {
                    painter.drawText(name_x(4), name_y(i) + 14, "M");
                }
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
    return QSize(c_names_x, c_names_y * c_max_sequence + 1);
}

/**
 *
 */

void
qperfnames::mousePressEvent (QMouseEvent * /*event*/)
{
    // no code
}

/**
 *
 */

void
qperfnames::mouseReleaseEvent (QMouseEvent * /*event*/)
{
    // no code
}

/**
 *
 */

void
qperfnames::mouseMoveEvent (QMouseEvent * /*event*/)
{
    // no code
}

}           // namespace seq64

/*
 * qperfnames.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

