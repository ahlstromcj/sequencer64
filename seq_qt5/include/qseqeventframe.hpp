#ifndef SEQ64_QSEQEVENTFRAME_HPP
#define SEQ64_QSEQEVENTFRAME_HPP

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
 *  along with seq24; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

/**
 * \file          qseqeventframe.hpp
 *
 *  This module declares/defines the edit frame for sequences.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-08-13
 * \updates       2018-08-13
 * \license       GNU GPLv2 or above
 *
 */

#include <QFrame>

namespace Ui
{
   class qseqeventframe;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq64
{

class qseqeventframe : public QFrame
{
    Q_OBJECT

public:

    explicit qseqeventframe (QWidget * parent = 0);
    virtual ~qseqeventframe ();

private:

    void setRowHeights (int height);
    void setColumnWidths (int total_width);

private:

    Ui::qseqeventframe * ui;

};

}           // namespace seq64

#endif      // QSEQEVENTFRAME_HPP

/*
 * qseqeventframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


