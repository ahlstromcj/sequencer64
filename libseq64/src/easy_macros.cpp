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
 * \file          easy_macros.cpp
 *
 *  This module defines some informative functions that are actually
 *  better off as functions.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-26
 * \updates       2018-04-12
 * \license       GNU GPLv2 or above
 *
 *  These functions are used in macros such as func_message().
 */

#include "easy_macros.hpp"

#include <assert.h>

/**
 *  Provides a way to still get the benefits of assert() output in release
 *  mode, without aborting the application.
 *
 * \todo
 *      This can slow down client code slightly, and it would be good to
 *      reduce the impact.
 *
 * \param ptr
 *      Provides the pointer to be tested.
 *
 * \param context
 *      Provides context for the message.  Usually the __func__ macro is
 *      the best option for this parameter.
 *
 * \return
 *      Returns true in release mode, if the pointer was not null.  In
 *      debug mode, will always return true, but the assert() will abort
 *      the application anyway.
 */

#ifdef PLATFORM_DEBUG

bool
not_nullptr_assert (void * ptr, const std::string & context)
{
    bool result = true;
    int flag = int(not_nullptr(ptr));
    if (! flag)
    {
        result = false;
        fprintf(stderr, "? null pointer in context %s\n", context.c_str());
    }

#ifdef PLATFORM_GNU_XXX                         /* does not work in Mingw   */
    int errornumber = flag ? 0 : 1 ;
    assert_perror(errornumber);
#else
    assert(flag);
#endif

    return result;
}

#endif  // PLATFORM_DEBUG

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  This function concatenates two C string pointers and returns them as
 *  a string message.  Note that we don't bother with error-checking the
 *  pointers.  You're on your own, Hoss.
 *
 * \param m1
 *      The first message, often a __func__ macro.
 *
 * \param m2
 *      The second message.
 *
 * \return
 *      Returns "m1: m2" as a standard C++ string.
 */

std::string
message_concatenate (const char * m1, const char * m2)
{
    std::string result(m1);
    result += ": ";
    result += m2;
    return result;
}

/**
 *  Common-code for console messages.  Adds markers and a newline.
 *
 * \param msg
 *      The message to print, sans the newline.
 *
 * \return
 *      Returns true.
 */

bool
info_message (const std::string & msg)
{
    printf("[%s]\n", msg.c_str());
    return true;
}

/**
 *  Common-code for error messages.  Adds markers, and returns false.
 *
 * \param msg
 *      The message to print, sans the newline.
 *
 * \return
 *      Returns false for convenience/brevity in setting function return
 *      values.
 */

bool
error_message (const std::string & msg)
{
    (void) info_message(msg);
    return false;
}

}           // namespace seq64

/*
 * easy_macros.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

