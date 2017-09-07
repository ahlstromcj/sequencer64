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
 * \file          qseqmainwnd.cpp
 *
 *  This module declares/defines the base class for the main window of the
 *  application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2017-09-06
 * \updates       2017-09-06
 * \license       GNU GPLv2 or above
 *
 */

#include "perform.hpp"                  /* seq64::perform                   */
#include "qseqmainwnd.hpp"              /* Qt5 version of seq64::mainwnd    */

/*
 * Not ready yet.
 *
 * #include "ui_qseqmainwnd.h"
 */

/*
 *  All library code for this project is in the "seq64" namespace.  Do not
 *  attempt to document this namespace; it breaks Doxygen.
 */

namespace seq64
{

bool is_pattern_playing = false;

qseqmainwnd::qseqmainwnd (QWidget * parent, perform * p)
 :
    QMainWindow (parent) // ,
    // ui          (new Ui::qseqmainwnd)
{
    show();
}

qseqmainwnd::~qseqmainwnd()
{
    delete ui;
}

/*
void
qseqmainwnd::panic ()
{
    m_main_perf->panic();
}
*/

}           /* namespace seq64 */

/*
 * qseqmainwnd.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

