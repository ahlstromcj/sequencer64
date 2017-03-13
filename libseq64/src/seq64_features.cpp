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
 * \file          seq64_features.cpp
 *
 *  This module adds some functions that reflect the features compiled into
 *  the application.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2017-03-12
 * \updates       2017-03-12
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.
 *
 *  Well, now it is empty.  We will wait a bit before removing this module.
 */

#include "seq64_features.h"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Returns the name of the application.  We could continue to use the macro
 *  SEQ64_APP_NAME, but we might eventually want to make this name
 *  configurable.  Not too likely, but possible.
 *
 * \return
 *      Returns SEQ64_APP_NAME.
 */

const std::string &
seq_app_name ()
{
    static std::string s_app_name = SEQ64_APP_NAME;
    return s_app_name;
}

/**
 *  Returns the name of the client for the application.  We could continue to
 *  use the macro SEQ64_CLIENT_NAME, but we might eventually want to make this
 *  name configurable.  More likely to be a configuration option in the
 *  future.
 *
 * \return
 *      Returns SEQ64_CLIENT_NAME.
 */

const std::string &
seq_client_name ()
{
    static std::string s_client_name = SEQ64_CLIENT_NAME;
    return s_client_name;
}

/**
 *  Returns the version of the application.  We could continue to use the macro
 *  SEQ64_VERSION, but ... let's be consistent.  :-D
 *
 * \return
 *      Returns SEQ64_VERSION.
 */

const std::string &
seq_version ()
{
    static std::string s_version= SEQ64_VERSION;
    return s_version;
}

}           // namespace seq64

/*
 * seq64_features.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

