#ifndef SEQ64_GUI_ASSISTANT_QT5_HPP
#define SEQ64_GUI_ASSISTANT_QT5_HPP

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
 * \file          gui_assistant_qt5.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI, without being tied to it.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2018-03-16
 * \updates       2018-03-17
 * \license       GNU GPLv2 or above
 *
 *  Note that this module automatically creates a Qt-specific
 *  keys_perform-derived object, which saves the main routine of the
 *  application the trouble of making one and passing it along.
 *
 *  Also, it currently doesn't add lash support or JACK-idle support.
 */

#include "easy_macros.h"                // SEQ64_JACK_SESSION (indirectly)
#include "gui_assistant.hpp"            // seq64::gui_assistant interface
#include "keys_perform_qt5.hpp"         // seq64::keys_perform_qt5

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

    class keys_perform;                 // ditto

/**
 *  This class provides an interface for some of the Qt/Gdk/Glib support
 *  needed in Sequencer64.
 */

class gui_assistant_qt5 : public gui_assistant
{
private:

    static keys_perform_qt5 sm_internal_keys;      // see the cpp file

public:

    gui_assistant_qt5 ();

    /**
     *  Virtual classes require a virtual destructor.
     */

    virtual ~gui_assistant_qt5 ()
    {
        // stock base-class implementation
    }

};

}           // namespace seq64

#endif      // SEQ64_GUI_ASSISTANT_QT5_HPP

/*
 * gui_assistant_qt5.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

