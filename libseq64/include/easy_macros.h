#ifndef SEQ64_EASY_MACROS_H
#define SEQ64_EASY_MACROS_H

/**
 * \file          easy_macros.h
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
 *
 * \library       sequencer64
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2013-11-17
 * \updates       2018-04-12
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Copyright (C) 2013-2016 Chris Ahlstrom <ahlstromcj@gmail.com>
 *
 *  The macros in this file cover:
 *
 *       -  Compiler-support macros.
 *       -  Error and information output macros.
 *       -  One or more global debugging functions that are better suited
 *          than using a macro.
 *
 *  Generally, we'll try to hide this file in "globals.h".  Note that it
 *  really should have the "hpp" extension now.  Oh well.
 */

#include <stdio.h>

#include "platform_macros.h"

/*
 * For now, let's see if we can get MingW to create a Windows-appropriate
 * header file.
 *
#ifdef PLATFORM_WINDOWS
#include "win32-seq64-config.h"
#else
#include "seq64-config.h"
#endif
 *
 */

#include "seq64-config.h"

#undef  SEQ64_SHOW_API_CALLS

/**
 *  Language macros:
 *
 *  __cplusplus >= 201103L implies C++11
 *
 *    "nullptr" provides an alternative to NULL in older compilers.
 *
 *    "override" is undefined (defined to nothing) in older compilers.
 *
 *  http://stackoverflow.com/questions/11053960/
 *      how-are-the-cplusplus-directive-defined-in-various-compilers
 *
 *    The 199711L stands for Year=1997, Month = 11 (i.e., November of 1997) --
 *    the date when the committee approved the standard that the rest of the
 *    ISO approved in early 1998.
 *
 *    For the 2003 standard, there were few enough changes that the committee
 *    (apparently) decided to leave that value unchanged.
 *
 *    For the 2011 standard, it's required to be defined as 201103L, (again,
 *    year=2011, month = 03) again meaning that the committee approved the
 *    standard as finalized in March of 2011.
 *
 *    For the 2014 standard, it's required to be defined as 201402L,
 *    interpreted the same way as above (February 2014).
 *
 *    Before the original standard was approved, quite a few compilers
 *    normally defined it to 0 (or just an empty definition like #define
 *    __cplusplus) to signify "not-conforming". When asked for their strictest
 *    conformance, many defined it to 1.
 */

#if ! defined __cplusplus

#define nullptr                 0
#define override
#define noexcept

#else

#if __cplusplus >= 201103L      /* C++11                */

#define nullptr                 nullptr
#define override                override
#define noexcept                noexcept

#else

#define nullptr                 0
#define override
#define noexcept                throw()

#endif                          /* not C++11            */

#endif                          /* defined __cplusplus  */

/**
 *    Provides a way to declare functions as having either a C++ or C
 *    interface.
 */

#if ! defined EXTERN_C_DEC

#ifdef __cplusplus
#define EXTERN_C_DEC extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_DEC
#define EXTERN_C_END
#endif

#endif

/**
 *    Test for being a valid pointer.  The not_NULL() macro is meant for C
 *    code that returns NULL.
 */

#define not_NULL(x)             ((x) != NULL)
#define not_nullptr(x)          ((x) != nullptr)
#define not_nullptr_2(x1, x2)   ((x1) != nullptr && (x2) != nullptr)

/**
 *    Test for being an invalid pointer.  The is_NULL() macro is meant for C
 *    code that returns NULL.
 */

#define is_NULL(x)              ((x) == NULL)
#define is_nullptr(x)           ((x) == nullptr)

/**
 *    A more obvious boolean type.
 */

#if ! defined __cplus_plus
typedef int cbool_t;
#elif __cplusplus <= 199711L
typedef bool cbool_t;
#endif

/**
 *  Provides the "false" value of the wbool_t type definition.
 */

#if ! defined __cplus_plus
#define false    0
#endif

/**
 *  Provides the "true" value of the wbool_t type definition.
 */

#if ! defined __cplus_plus
#define true     1
#endif

/**
 *  Easy conversion from boolean to string.
 */

#define bool_string(x)      ((x) ? "true" : "false")

/**
 *    GCC provides three magic variables which hold the name of the current
 *    function, as a string.  The first of these is `__func__', which is part
 *    of the C99 standard:
 *
 *    The identifier `__func__' is implicitly declared by the translator.  It
 *    is the name of the lexically-enclosing function.  This name is the
 *    unadorned name of the function.
 *
 *    `__FUNCTION__' is another name for `__func__'.  Older versions of GCC
 *    recognize only this name.  However, it is not standardized.  For maximum
 *    portability, use `__func__', but provide a fallback definition with
 *    the preprocessor, as done below.
 *
 *    `__PRETTY_FUNCTION__' is the decorated version of the function name.
 *    It is longer, but more informative.  It is also deprecated.  But, for
 *    now, we'll continue to use it
 *
 *    Visual Studio defines only __FUNCTION__, so a definition is provided
 *    below.
 */

#ifdef PLATFORM_GNU

#if ! defined __func__
#if __STDC_VERSION__ < 199901L
#if __GNUC__ >= 2

/*
 * Alternative:
 *
 * #define __func__        __PRETTY_FUNCTION__          // adorned func names
 */

#define __func__           __FUNCTION__                 // bald func names

#else
#define __func__        "<unknown>"
#endif  // __GNUC__
#endif  // __STDC_VERSION__
#endif  // __func__

#else   // ! PLATFORM_GNU

#if ! defined __func__
#define __func__        __FUNCTION__    // Windows?
#endif

#endif  // PLATFORM_GNU

/**
 *  A macro to prepend a fully qualified function name to a string.
 *  Currently defined in the rtmidi library due to an weird inability
 *  to resolve circular references involving message_concatenate() and
 *  the mastermidibus() class!
 */

#define func_message(x)         seq64::message_concatenate(__func__, x)

/**
 *  Usage:      errprint(cstring);
 *
 *    Provides an error reporting macro (which happens to match Chris's XPC
 *    error function.
 */

#ifdef PLATFORM_DEBUG
#define errprint(x)             fprintf(stderr, "%s\n", x)
#else
#define errprint(x)
#endif

/**
 *  Usage:      errprintfunc(cstring);
 *
 *    Provides an error reporting macro that includes the function name.
 */

#ifdef PLATFORM_DEBUG
#define errprintfunc(x)         fprintf(stderr, "%s: %s\n", __func__, x)
#else
#define errprintfunc(x)
#endif

/**
 *  Usage:      errprintf(format, cstring);
 *
 *    Provides an error reporting macro that requires a sprintf() format
 *    specifier as well.
 */

#ifdef PLATFORM_DEBUG
#define errprintf(fmt, x)       fprintf(stderr, fmt, x)
#else
#define errprintf(fmt, x)
#endif

/**
 *  Usage:      warnprint(cstring);
 *
 *    Provides a warning reporting macro (which happens to match Chris's
 *    XPC error function.
 */

#ifdef PLATFORM_DEBUG
#define warnprint(x)            fprintf(stderr, "%s!\n", x)
#else
#define warnprint(x)
#endif

/**
 *  Usage:      warnprint(format, cstring);
 *
 *    Provides an error reporting macro that requires a sprintf() format
 *    specifier as well.
 */

#ifdef PLATFORM_DEBUG
#define warnprintf(fmt, x)      fprintf(stderr, fmt, x)
#else
#define warnprintf(fmt, x)
#endif

/**
 *  Usage:      infoprint(cstring);
 *
 *    Provides an information reporting macro (which happens to match
 *    Chris's XPC information function.
 */

#ifdef PLATFORM_DEBUG
#define infoprint(x)            fprintf(stderr, "%s\n", x)
#else
#define infoprint(x)
#endif

/**
 *  Usage:      infoprint(format, cstrin);
 *
 *    Provides an error reporting macro that requires a sprintf() format
 *    specifier as well.
 */

#ifdef PLATFORM_DEBUG
#define infoprintf(fmt, x)      fprintf(stderr, fmt, x)
#else
#define infoprintf(fmt, x)
#endif

/**
 *  Usage:      apiprint(function_name, context_tag);
 *
 *  This macro can be enabled in JACK modules in order to see the flow of
 *  calls to the JACK or ALSA API.  It also disables the hiding of JACK/ALSA
 *  information messages.
 */

#ifdef SEQ64_SHOW_API_CALLS
#define apiprint(name, tag)    fprintf(stderr, "= %s(%s)\n", name, tag)
#else
#define apiprint(name, tag)
#endif

#endif          /* SEQ64_EASY_MACROS_H  */

/*
 * easy_macros.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

