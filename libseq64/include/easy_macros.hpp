#ifndef SEQ64_EASY_MACROS_HPP
#define SEQ64_EASY_MACROS_HPP

/**
 * \file          easy_macros.hpp
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
 *
 * \library       sequencer64
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2018-04-12
 * \updates       2018-04-12
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  This file separates out the C++ declarations, which are not needed in the
 *  C code of the seq_portmidi library.
 */

#include <string>

#include "easy_macros.h"                /* stuff suitable for C code    */

/**
 * Global functions.  The not_nullptr_assert() function is a macro in
 * release mode, to speed up release mode.  It cannot do anything at
 * all, since it is used in the conditional part of if-statements.
 */

#ifdef PLATFORM_DEBUG
extern bool not_nullptr_assert (void * ptr, const std::string & context);
#else
#define not_nullptr_assert(ptr, context) (not_nullptr(ptr))
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

extern std::string message_concatenate (const char * m1, const char * m2);
extern bool info_message (const std::string & msg);
extern bool error_message (const std::string & msg);

}               /* namespace seq64          */

#endif          /* SEQ64_EASY_MACROS_HPP    */

/*
 * easy_macros.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

