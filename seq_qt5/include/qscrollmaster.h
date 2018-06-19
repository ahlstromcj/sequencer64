#ifndef SEQ64_QSCROLLMASTER_HPP
#define SEQ64_QSCROLLMASTER_HPP

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
 * \file          qscrollmaster.hpp
 *
 *  This module declares/defines a class for controlling other QScrollAreas
 *  from this one.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2018-06-18
 * \updates       2018-06-18
 * \license       GNU GPLv2 or above
 *
 */

#include <list>                         /* std::list container class        */
#include <QScrollArea>

/*
 *  Forward declarations.  The Qt header files are moved into the cpp file.
 */

class QFrame;
class QScrollBar;

/*
 * Note that there is no namespace; the Qt uic specification does not seem to
 * support them.
 */

/**
 *
 */

class qscrollmaster : public QScrollArea
{

private:

    typedef std::list<QScrollBar *> container;
    typedef std::list<QScrollBar *>::iterator iterator;
    typedef std::list<QScrollBar *>::const_iterator const_iterator;

private:

    /**
     *  Holds a list of external vertical scroll bars to me maintained.
     */

    container m_v_scrollbars;

    /**
     *  Holds a list of external horizontal scroll bars to me maintained.
     */

    container m_h_scrollbars;

    /**
     *  Holds a pointer to this scroll-area's vertical scrollbar.
     */

    QScrollBar * m_self_v_scrollbar;

    /**
     *  Holds a pointer to this scroll-area's horizontal scrollbar.
     */

    QScrollBar * m_self_h_scrollbar;

public:

    qscrollmaster (QFrame * qf);
    virtual ~qscrollmaster ();

    void add_v_scroll (QScrollBar * qsb)
    {
        m_v_scrollbars.push_back(qsb);
    }

    void add_h_scroll (QScrollBar * qsb)
    {
        m_h_scrollbars.push_back(qsb);
    }

protected:

    virtual void scrollContentsBy (int dx, int dy);

};          // class qscrollmaster

#endif      // SEQ64_QSCROLLMASTER_HPP

/*
 * qscrollmaster.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

