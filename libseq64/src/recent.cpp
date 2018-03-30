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
 * \file          recent.cpp
 *
 *  This module declares/defines ...
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-03-29
 * \updates       2018-03-30
 * \license       GNU GPLv2 or above
 *
 *
 */

#include "recent.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *
 */

recent::recent ()
 :
    m_recent_list   (),
    m_maximum_size  (0)
{
    // no code
}

/**
 *
 */

recent::recent (const recent & source)
 :
    m_recent_list   (source.m_recent_list),
    m_maximum_size  (source.m_maximum_size)
{
    // no code
}

/**
 *
 */

recent &
recent::operator = (const recent & source)
{
    if (this != &source)
    {
        m_recent_list   = source.m_recent_list;
        m_maximum_size  = source.m_maximum_size;
    }
    return *this;
}

/**
 *
 */

bool
recent::append (const std::string & item)
{
    bool result = count() < maximum();
    if (result)
        m_recent_list.push_back(item);

    return result;
}


}           // namespace seq64

/*
 * recent.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

