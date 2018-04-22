#ifndef SEQ64_DAEMONIZE_HPP
#define SEQ64_DAEMONIZE_HPP

/**
 * \file          daemonize.hpp
 * \author        Chris Ahlstrom
 * \date          2005-07-03 to 2007-08-21 (from xpc-suite project)
 * \updates       2018-04-22
 * \license       GNU GPLv2 or above
 *
 *    Daemonization of POSIX C Wrapper (PSXC) library
 *    Copyright (C) 2005-2017 by Chris Ahlstrom
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *    02110-1301, USA.
 *
 *    This module provides a function to make it easy to run an application
 *    as a daemon.
 */

#include "platform_macros.h"            /* detecting 32-bit builds      */

#include <string>

/*
 * uint32_t typedef for 32-bit code
 */

#ifdef PLATFORM_32_BIT
typedef unsigned int uint32_t;
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/*
 *  Free functions.
 *    These functions do a lot of the work of dealing with UNIX daemons.
 */

extern bool check_daemonize (int argc, char * argv []);
extern uint32_t daemonize
(
    const std::string & appname,
    const std::string & cwd         = ".",
    int mask                        = 0
);
extern void undaemonize (uint32_t previous_umask);
extern bool reroute_stdio
(
    const std::string & logfile = "",
    bool closem = false
);

}        // namespace seq64

#endif   // SEQ64_DAEMONIZE_HPP

/*
 * vim: ts=4 sw=4 et ft=cpp
 */

