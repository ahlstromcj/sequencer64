#ifndef SEQ64_GUI_ASSISTANT_HPP
#define SEQ64_GUI_ASSISTANT_HPP

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
 * \file          gui_assistant.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI, without being tied to it.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2015-09-30
 * \license       GNU GPLv2 or above
 *
 */

namespace seq64
{

class jack_assistant;                   // forward reference
class lash;                             // ditto
class keys_perform;                     // ditto

/**
 *  This class provides an interface for some of the GUI support needed
 *  in Sequencer64.  It also contain a number of helper objects that all
 *  kind of go together; only this assistant object will need to be passed
 *  around (by non-GUI code).
 */

class gui_assistant
{

private:

    /**
     *  Provides a reference to the app-specific GUI-specific
     *  keys_perform-derived object that an application is going to use
     *  for handling sequence-control keys.
     */

    keys_perform & m_keys_perform;

public:

    gui_assistant (keys_perform & kp);

    /**
     *  Stock base-class implementation of a virtual destructor.
     */

    virtual ~gui_assistant ()
    {
        // Empty body
    }

    virtual void quit () = 0;
    virtual void jack_idle_connect (jack_assistant & jack) = 0;
    virtual void lash_timeout_connect (lash & lashobject) = 0;

    /**
     * \getter m_keys_perform
     *      The const getter.
     */

    const keys_perform & keys () const
    {
        return m_keys_perform;
    }

    /**
     * \getter m_keys_perform
     *      The un-const getter.
     */

    keys_perform & keys ()
    {
        return m_keys_perform;
    }

};

}           // namespace seq64

#endif      // SEQ64_GUI_ASSISTANT_HPP

/*
 * gui_assistant.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

