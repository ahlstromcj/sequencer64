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
 *  This module declares/defines a class for keeping track of recently-used
 *  files.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-03-29
 * \updates       2018-04-18
 * \license       GNU GPLv2 or above
 *
 *  The seq64::recent class simply keeps track of recently-used files for the
 *  "Recent Files" menu.
 */

#include <algorithm>                    /* std::find()                  */

#include "app_limits.h"                 /* SEQ64_RECENT_FILES_MAX       */
#include "file_functions.hpp"           /* seq64::get_full_path()       */
#include "recent.hpp"                   /* recent-files container       */

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
    m_maximum_size  (SEQ64_RECENT_FILES_MAX)
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

recent::~recent ()
{
    // no code
}

/**
 *  This function is meant to be used when loading the recent-files list from
 *  a configuration file.  Unlike the add() function, this one will not pop of
 *  an existing item to allow the current item to be put into the container.
 *
 * \param item
 *      Provides the file-name to append.  It is converted to the full path to
 *      the file before being added.
 *
 * \return
 *      Returns true if the file-name was appended.
 */

bool
recent::append (const std::string & item)
{
    bool result = count() < maximum();
    if (result)
    {
        std::string fullpath = get_full_path(item);
        result = ! fullpath.empty();
        if (result)
            m_recent_list.push_back(fullpath);
    }
    return result;
}

/**
 *  This function is meant to be used when adding a file that the user
 *  selected.  If the file is already in the list, it is moved to the "top"
 *  (the beginning of the list).
 *
 * \param item
 *      Provides the file-name to add.  It is converted to the full path to
 *      the file before being added.
 *
 * \return
 *      Returns true if the file-name was added.
 */

bool
recent::add (const std::string & item)
{
    std::string fullpath = get_full_path(item);
    bool result = ! fullpath.empty();
    if (result)
    {
        Container::iterator it = std::find
        (
            m_recent_list.begin(), m_recent_list.end(), fullpath
        );
        if (it != m_recent_list.end())
            (void) m_recent_list.erase(it);

        result = count() < maximum();
        if (! result)
        {
            m_recent_list.pop_back();
            result = true;
        }
        if (result)
            m_recent_list.push_front(fullpath);
    }
    return result;
}

/**
 *
 */

bool
recent::remove (const std::string & item)
{
    bool result = ! item.empty();
    if (result)
    {
        Container::iterator it = std::find
        (
            m_recent_list.begin(), m_recent_list.end(), item
        );
        if (it != m_recent_list.end())
            (void) m_recent_list.erase(it);
        else
            result = false;
    }
    return result;
}

/**
 *
 */

std::string
recent::get (int index) const
{
    std::string result;
    if (index >= 0 && index < count())
        result = m_recent_list[Container::size_type(index)];

    return result;
}

}           // namespace seq64

/*
 * recent.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

