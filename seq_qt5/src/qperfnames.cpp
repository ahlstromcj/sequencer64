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
 * \updates       2018-03-04
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
    mPen                (nullptr),
    mBrush              (nullptr),
    mPainter            (nullptr),
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

qperfnames::~qperfnames()
{
    if (not_nullptr(mPen))
        delete mPen;

    if (not_nullptr(mPainter))
        delete mPainter;

    if (not_nullptr(mBrush))
        delete mBrush;
}

/**
 *
 */

void
qperfnames::paintEvent (QPaintEvent *)
{
    mPainter = new QPainter(this);
    mPen = new QPen(Qt::black);
    mBrush = new QBrush(Qt::lightGray);
    mPen->setStyle(Qt::SolidLine);
    mBrush->setStyle((Qt::SolidPattern));
    mFont.setPointSize(6);
    mFont.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    mPainter->setPen(*mPen);
    mPainter->setBrush(*mBrush);
    mPainter->setFont(mFont);

    int y_s = 0;
    int y_f = height() / c_names_y;
    mPainter->drawRect(0, 0, width(), height() - 1);    // draw border
    for (int y = y_s; y <= y_f; ++y)
    {
        int seqId = y;
        if (seqId < c_max_sequence)
        {
            int i = seqId;
            if (seqId % c_seqs_in_set == 0)     // if first seq in bank
            {
                mPen->setColor(Qt::black);      // black boxes to mark each bank
                mBrush->setColor(Qt::black);
                mBrush->setStyle(Qt::SolidPattern);
                mPainter->setPen(*mPen);
                mPainter->setBrush(*mBrush);
                mPainter->drawRect(1, name_y(i) + 1, 15, c_names_y - 1);

                char ss[4];
                int bankId = seqId / c_seqs_in_set;
                snprintf(ss, sizeof(ss), "%2d", bankId);

                mPen->setColor(Qt::white);  // draw bank number here
                mPainter->setPen(*mPen);
                mPainter->drawText(4, c_names_y * i + 15, ss);

                // offset and draw bank name sideways

                mPen->setColor(Qt::black);
                mPainter->setPen(*mPen);
                mPainter->save();
                QString bankName(perf().get_bank_name(bankId).c_str());
                mPainter->translate
                (
                    12,
                    (c_names_y * i) + (c_names_y * c_seqs_in_set * 0.5)
                        + bankName.length() * 4
                );
                mPainter->rotate(270);
                mFont.setPointSize(9);
                mFont.setBold(true);
                mFont.setLetterSpacing(QFont::AbsoluteSpacing, 2);
                mPainter->setFont(mFont);
                mPainter->drawText(0, 0, bankName);
                mPainter->restore();
            }
            mPen->setStyle(Qt::SolidLine);
            mPen->setColor(Qt::black);
            if (perf().is_active(seqId))
            {
                /*
                 * Commented out until we fix the issues.
                 *
                //get seq's assigned colour and beautify
                QColor colourSpec =
                    QColor(colourMap.value(perf().get_sequence_color(seqId)));
                QColor backColour = QColor(colourSpec);
                if (backColour.value() != 255) //dont do this if we're white
                    backColour.setHsv(colourSpec.hue(),
                                      colourSpec.saturation() * 0.65,
                                      colourSpec.value() * 1.3);
                mBrush->setColor(backColour);


                    New:  we can call get_color(Palettecolor(c), 1.0, 0.65, 1.3);
                 *
                 */
            }
            else
                mBrush->setColor(Qt::lightGray);

            mPainter->setPen(*mPen);    // fill seq label background
            mPainter->setBrush(*mBrush);
            mPainter->drawRect
            (
                6 * 2 + 4, c_names_y * i, c_names_x - 15, c_names_y
            );

            if (perf().is_active(seqId))
            {
                m_sequence_active[seqId] = true;

                // draw seq info on label
                /*
                char name[50];
                snprintf
                (
                    name, sizeof(name), "%-14.14s                        %2d",
                     perf().get_sequence(seqId)->name().c_str(),
                     perf().get_sequence(seqId)->get_midi_channel() + 1
                );
                 */

                std::string name = perf().sequence_label(seqId); // seq name
                mPen->setColor(Qt::black);
                mPainter->setPen(*mPen);
                mPainter->drawText(18, c_names_y * i + 10, name.c_str());

                /*
                char str[20];
                snprintf
                (
                    str, sizeof(str),
                     "%d-%d %d/%d",
                     perf().get_sequence(seqId)->get_midi_bus(),
                     perf().get_sequence(seqId)->get_midi_channel() + 1,
                     perf().get_sequence(seqId)->get_beats_per_bar(),
                     perf().get_sequence(seqId)->get_beat_width()
                );
                mPainter->drawText(18, c_names_y * i + 20, str); // seq info
                 */

                bool muted = perf().get_sequence(seqId)->get_song_mute();
                mPen->setColor(Qt::black);
                mPainter->setPen(*mPen);
                mPainter->drawRect(name_x(2), name_y(i), 10, m_nametext_y);
//              (
//                  6 * 2 + 6 * 20 + 2, (c_names_y * i), 10, c_names_y
//              );

                if (muted) // seq mute state
                {
                    mPainter->drawText(name_x(4), name_y(i) + 14, "M");
//                  (
//                      6 * 2 + 6 * 20 + 4, c_names_y * i + 14, "M"
//                  );
                }
                else
                {
                    mPainter->drawText(name_x(4), name_y(i) + 14, "M");
                }
            }
        }
    }
    if (not_nullptr(mPen))
    {
        delete mPen;
        mPen = nullptr;
    }
    if (not_nullptr(mPainter))
    {
        delete mPainter;
        mPainter = nullptr;
    }
    if (not_nullptr(mBrush))
    {
        delete mBrush;
        mBrush = nullptr;
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

