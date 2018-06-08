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
 * \file          qsabout.cpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-04-08
 * \license       GNU GPLv2 or above
 *
 */

#include "qsabout.hpp"
#include "seq64-config.h"

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qsabout.h"
#else
#include "forms/qsabout.ui.h"
#endif

namespace seq64
{

/**
 *
 */

qsabout::qsabout (QWidget * parent)
 :
    QDialog (parent),
    ui      (new Ui::qsabout)
{
    ui->setupUi(this);
}

/**
 *
 */

qsabout::~qsabout()
{
    delete ui;
}

}               // namespace seq64

/*
 * qsabout.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

