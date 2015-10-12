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
 * \file          gui_play_base.cpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-09-28
 * \updates       2015-09-30
 * \license       GNU GPLv2 or above
 *
 *  This module was intended to provide a framework for implementing the
 *  actual work of some of the Gtkmm-specific on_xxxx() callback functions.
 *
 *  However, many of those function can be implemented by offloading
 *  functionality to existing objects (such as perform), and so the utility of
 *  this base class is doubtful.  We will leave it around for awhile yet, just
 *  in case.
 */

#include "gui_play_base.hpp"            // seq64::gui_play_base

namespace seq64
{

    // No code!

}           // namespace seq64

/*
 * gui_play_base.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

