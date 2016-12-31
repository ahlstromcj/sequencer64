/**
 * \file          rtmidi_types.cpp
 *
 *    Classes that use to be structs.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-01
 * \updates       2016-12-31
 * \license       See the r3exmidi.lic file.  Too big for a header file.
 *
 *  Provides some basic types for the (heavily-factored) rtmidi library, very
 *  loosely based on Gary Scavone's RtMidi library.
 */

#include "easy_macros.h"                /* errprintfunc() macro, etc.   */
#include "rtmidi_types.hpp"             /* seq64::rtmidi, etc.          */

/*
 * Do not document the namespace; it breaks Doxygen.
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
    std::string temp = "[";
    temp += msg;
    temp += "]\n";
    printf(temp.c_str());
    return true;
}

/**
 *  Common-code for error messages.  Adds markers, and sets m_jack_running to
 *  false.
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
 * rtmidi_types.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

