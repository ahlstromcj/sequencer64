#ifndef SEQ64_SETTINGS_HPP
#define SEQ64_SETTINGS_HPP

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
 * \file          settings.hpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions used in this application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2016-05-17
 * \updates       2016-08-19
 * \license       GNU GPLv2 or above
 *
 *  A couple of universal helper functions remain as inline functions in the
 *  module.  The rest have been moved to the calculations module.
 *
 *  Also note that this file really is a C++ header file, and should have
 *  the "hpp" file extension.  We will fix that Real Soon Now.
 */

#include <string>

#include "rc_settings.hpp"              /* seq64::rc_settings           */
#include "user_settings.hpp"            /* seq64::user_settings         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 *  Returns a reference to the global rc_settings and user_settings objects.
 *  Why a function instead of direct variable access?  Encapsulation.  We are
 *  then free to change the way "global" settings are accessed, without
 *  changing client code.
 */

extern rc_settings & rc ();
extern user_settings & usr ();
extern int choose_ppqn (int ppqn);

}           // namespace seq64

#endif      // SEQ64_SETTINGS_HPP

/*
 * settings.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

