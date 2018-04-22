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
 * \updates       2018-04-21
 * \license       GNU GPLv2 or above
 *
 *  The seq64::recent class simply keeps track of recently-used files for the
 *  "Recent Files" menu.
 */

#include <algorithm>                    /* std::find()                      */

#include "app_limits.h"                 /* SEQ64_RECENT_FILES_MAX           */
#include "file_functions.hpp"           /* seq64::get_full_path()           */
#include "recent.hpp"                   /* recent-files container           */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  This construction creates an empty recent-files list and sets the maximum
 *  size of the list.
 */

recent::recent ()
 :
    m_recent_list   (),
    m_maximum_size  (SEQ64_RECENT_FILES_MAX)
{
    // no code
}

/**
 *  \copyctor
 */

recent::recent (const recent & source)
 :
    m_recent_list   (source.m_recent_list),
    m_maximum_size  (source.m_maximum_size)
{
    // no code
}

/**
 *  The conventional, rote, principal assignent operator.
 */

recent &
recent::operator = (const recent & source)
{
    if (this != &source)
    {
        m_recent_list   = source.m_recent_list;

        /*
         * A constant, cannot be reassigned:
         *
         * m_maximum_size  = source.m_maximum_size;
         */
    }
    return *this;
}

/**
 * \dtor
 *      Nothing to do in this implementation.
 */

recent::~recent ()
{
    // no code
}

/**
 *  This function is meant to be used when loading the recent-files list from
 *  a configuration file.  Unlike the add() function, this one will not pop
 *  off an existing item to allow the current item to be put into the
 *  container.  If the entry is already in the list, it is ignored.
 *
 * \param item
 *      Provides the file-name to append.  It is converted to the full path to
 *      the file before being added.  Also, it is set to UNIX conventions,
 *      using the forward slash as a path separator.
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
        std::string fullpath = get_full_path(normalize_path(item));
        result = ! fullpath.empty();
        if (result)
        {
            Container::iterator it = std::find
            (
                m_recent_list.begin(), m_recent_list.end(), fullpath
            );
            if (it == m_recent_list.end())              /* not found?   */
                m_recent_list.push_back(fullpath);      /* append it!   */
        }
    }
    return result;
}

/**
 *  This function is meant to be used when adding a file that the user
 *  selected.  If the file is already in the list, it is moved to the "top"
 *  (the beginning of the list); the original entry is removed.  If the list is
 *  full, the last entry is removed, in order to make room for the new entry.
 *
 * \param item
 *      Provides the file-name to add.  It is converted to the full path to
 *      the file before being added.  Also, it is set to UNIX conventions,
 *      using the forward slash as a path separator.
 *
 * \return
 *      Returns true if the file-name was added.
 */

bool
recent::add (const std::string & item)
{
    std::string fullpath = get_full_path(normalize_path(item));
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
            m_recent_list.pop_back();   /* remove last entry to make room   */
            result = true;
        }
        if (result)
            m_recent_list.push_front(fullpath);
    }
    return result;
}

/**
 *  Removes the path from the recent-files list, if it was found in that list.
 *
 * \param item
 *      Provides the path to be removed.
 *
 * \return
 *      Returns true if the item was found and removed.
 */

bool
recent::remove (const std::string & item)
{
    std::string fullpath = get_full_path(normalize_path(item));
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
 *  Gets the file-path at the given position.
 *
 * \param index
 *      Provides the index into the container.  It is checked for validity.
 *
 * \return
 *      Returns the file-path, if available.  Otherwise, an empty string is
 *      returned.  The path is converted to the slash/backslash conventions
 *      appropriate for the operating system on which this code was built.
 */

std::string
recent::get (int index) const
{
    std::string result;
    if (index >= 0 && index < count())
    {
        result = m_recent_list[Container::size_type(index)];

#ifdef PLATFORM_WINDOWS
        result = normalize_path(result, false);
#else
        result = normalize_path(result, true);
#endif
    }
    return result;
}

}           // namespace seq64

/*
 * recent.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

