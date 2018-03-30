#ifndef SEQ64_RECENT_HPP
#define SEQ64_RECENT_HPP

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
 * \file          recent.hpp
 *
 *  This module declares/defines a container for "recent" entries.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-03-29
 * \updates       2018-03-30
 * \license       GNU GPLv2 or above
 *
    void add_recent_file (const std::string & filename);
    std::string recent_file (int index, bool shorten = true) const;
    int recent_file_count () const

    push_back()
    push_front()
    remove()
    get()
 */

#include <deque>
#include <string>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *
 */

class recent
{

private:

    /**
     *
     */

    typedef std::deque<std::string>  Container;

private:

    /**
     *
     */

    Container m_recent_list;

    /**
     *
     */

    int m_maximum_size;

public:

    recent ();
    recent (const recent & source);
    recent & operator = (const recent & source);

    void clear ()
    {
        m_recent_list.clear();
    }

    int count () const
    {
        return int(m_recent_list.size());
    }

    int maximum () const
    {
        return m_maximum_size;
    }

    bool append (const std::string & item);

};          // class recent

}           // namespace seq64

#endif      // SEQ64_RECENT_HPP

/*
 * recent.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

