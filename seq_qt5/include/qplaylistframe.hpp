#ifndef SEQ64_QPLAYLISTFRAME_HPP
#define SEQ64_QPLAYLISTFRAME_HPP

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
 * \file          qsplaylistframe.hpp
 *
 *  This module declares/defines the base class for a simple playlist editor based
 *  on Qt 5.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-09-04
 * \updates       2018-09-04
 * \license       GNU GPLv2 or above
 *
 */

#include <QFrame>
#include "easy_macros.hpp"              /* nullptr and related macros   */

/*
 * Do not document namespaces.
 */

namespace Ui
{
    class qplaylistframe;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 *
 */

class qplaylistframe : public QFrame
{
    Q_OBJECT

public:

    qplaylistframe (perform & p, QWidget * parent = nullptr);

    virtual ~qplaylistframe ();

private:

    void set_row_heights (int height);

private slots:

private:

    Ui::qplaylistframe * ui;

private:

    /**
     *  The perform object.
     */

    perform & m_perform;

};          // class qplaylistframe

}           // namespace seq64

#endif      // SEQ64_QPLAYLISTFRAME_HPP

/*
 * qplaylistframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

