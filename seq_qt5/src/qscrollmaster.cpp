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
 * \file          qscrollmaster.cpp
 *
 *  This module declares/defines a class for controlling other QScrollAreas
 *  from this one.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2018-06-18
 * \updates       2018-08-10
 * \license       GNU GPLv2 or above
 *
 * Other useful QScrollBar functions:
 *
 *  -   setPageStep()
 *  -   setSingleStep()
 */

#include <QFrame>
#include <QScrollBar>

#include "easy_macros.hpp"              /* nullptr and other macros         */
#include "qscrollmaster.h"              /* ::qscrollmaster class            */

/*
 * Note that there is no namespace; the Qt uic specification does not seem to
 * support them.
 */

/**
 *  Constructor.
 *
 * \param qf
 *      Provides the "parent" of this object.
 */

qscrollmaster::qscrollmaster (QWidget * qf)
 :
    QScrollArea         (qf),
    m_v_scrollbars      (),
    m_h_scrollbars      (),
    m_self_v_scrollbar  (nullptr),
    m_self_h_scrollbar  (nullptr)
{
    m_self_v_scrollbar = verticalScrollBar();
    m_self_h_scrollbar = horizontalScrollBar();
}

/**
 *
 */

qscrollmaster::~qscrollmaster ()
{
    // no code
}

/**
 *  This override of a QScrollArea virtual member function
 *  modifies any attached/listed scrollbars and then calls the base-class
 *  version of this function.
 *
 * \param dx
 *      The change in the x position value of the scrollbar.  Simply passed on
 *      to the base-class version of this function.
 *
 * \param dy
 *      The change in the y position value of the scrollbar.  Simply passed on
 *      to the base-class version of this function.
 */

void
qscrollmaster::scrollContentsBy (int dx, int dy)
{
#ifdef PLATFORM_DEBUG_TMI
    printf("scrollContentsBy(%d, %d)\n", dx, dy);
#endif

    if (! m_v_scrollbars.empty())
    {
        int vvalue = m_self_v_scrollbar->value();
        for
        (
            iterator vit = m_v_scrollbars.begin();
            vit != m_v_scrollbars.end();
            ++vit
        )
        {
            (*vit)->setValue(vvalue);
        }
    }

    if (! m_h_scrollbars.empty())
    {
        int hvalue = m_self_h_scrollbar->value();
        for
        (
            iterator hit = m_h_scrollbars.begin();
            hit != m_h_scrollbars.end();
            ++hit
        )
        {
            (*hit)->setValue(hvalue);
        }
    }
    QScrollArea::scrollContentsBy(dx, dy);
}

/*
 * qscrollmaster.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

