#ifndef SEQ64_PLATFORM_MACROS_H
#define SEQ64_PLATFORM_MACROS_H

/**
 * \file          platform_macros.h
 *
 *  Provides a rationale and a set of macros to make compile-time
 *  decisions covering Windows versus Linux, GNU versus Microsoft, and
 *  MINGW versus GNU.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2017-08-31
 * \license       GNU GPLv2 or above
 *
 *  Copyright (C) 2013-2015 Chris Ahlstrom <ahlstromcj@gmail.com>
 *
 *  We need a uniform way to specify OS and compiler features without
 *  have to litter our code with many macros.  (Littering this file
 *  with macros is okay, though.)
 *
 * Determining useful macros:
 *
 *    -  GNU:  cpp -dM myheaderfile
 *
 * Settings to distinguish, based on compiler-supplied macros:
 *
 *    -  Platform macros (set in nar-maven-plugin's aol.properties):
 *       -  Windows
 *       -  Linux
 *       -  MacOSX
 *    -  Platform macros (in the absense of Windows, Linux macros):
 *       -  PLATFORM_WINDOWS
 *       -  PLATFORM_LINUX
 *       -  PLATFORM_MACOSX
 *       -  PLATFORM_UNIX
 *    -  Architecture size macros:
 *       -  PLATFORM_32_BIT
 *       -  PLATFORM_64_BIT
 *    -  Debugging macros:
 *       -  PLATFORM_DEBUG
 *       -  PLATFORM_RELEASE
 *    -  Compiler:
 *       -  PLATFORM_MSVC (alternative to _MSC_VER)
 *       -  PLATFORM_GNU
 *       -  PLATFORM_MINGW
 *       -  PLATFORM_CYGWIN
 *    -  API:
 *       -  PLATFORM_WINDOWS_API (Windows and MingW)
 *       -  PLATFORM_POSIX_API (alternative to POSIX)
 *    -  Language:
 *       -  PLATFORM_CPP_11
 */

#undef PLATFORM_WINDOWS
#undef PLATFORM_LINUX
#undef PLATFORM_MACOSX
#undef PLATFORM_UNIX
#undef PLATFORM_32_BIT
#undef PLATFORM_64_BIT
#undef PLATFORM_DEBUG
#undef PLATFORM_RELEASE
#undef PLATFORM_MSVC
#undef PLATFORM_GNU
#undef PLATFORM_MINGW
#undef PLATFORM_CYGWIN
#undef PLATFORM_WINDOWS_API
#undef PLATFORM_POSIX_API
#undef PLATFORM_CPP_11

/**
 *  Provides a "Windows" macro, in case the environment doesn't provide
 *  it.  This macro is defined if not already defined and _WIN32 is
 *  encountered.
 */

#if defined Windows                    /* defined by the nar-maven-plugin     */
#define PLATFORM_WINDOWS
#define PLATFORM_WINDOWS_API
#else
#if defined _WIN32 || defined _WIN64   /* defined by the Microsoft compiler   */
#define PLATFORM_WINDOWS
#define PLATFORM_WINDOWS_API
#define Windows
#endif
#endif

/**
 *  Provides a "Linux" macro, in case the environment doesn't provide it.
 *  This macro is defined if not already defined and XXXXXX is
 *  encountered.
 */

#if defined Linux                      /* defined by the nar-maven-plugin     */
#define PLATFORM_LINUX
#define PLATFORM_POSIX_API
#else
#if defined __linux__                  /* defined by the GNU compiler         */
#define PLATFORM_LINUX
#define PLATFORM_POSIX_API
#define Linux
#endif
#endif

#if defined PLATFORM_LINUX

#if ! defined POSIX
#define POSIX                          /* defined for legacy code purposes  */
#endif

#define PLATFORM_UNIX
#define PLATFORM_POSIX_API

#endif                                  /* PLATFORM_LINUX                   */

/**
 *  Provides a "MacOSX" macro, in case the environment doesn't provide it.
 *  This macro is defined if not already defined and __APPLE__ and
 *  __MACH__ are encountered.
 */

#if defined MacOSX                     /* defined by the nar-maven-plugin   */
#define PLATFORM_MACOSX
#else
#if defined __APPLE__ && defined __MACH__    /* defined by Apple compiler   */
#define PLATFORM_MACOSX
#define MacOSX
#endif
#endif

#if defined PLATFORM_MACOSX
#define PLATFORM_UNIX
#endif

/**
 *  Provides macros that mean 32-bit, and only 32-bit Windows.  For
 *  example, in Windows, _WIN32 is defined for both 32- and 64-bit
 *  systems, because Microsoft didn't want to break people's 32-bit code.
 *  So we need a specific macro.
 *
 *      -  PLATFORM_32_BIT is defined on all platforms.
 *      -  WIN32 is defined on Windows platforms.
 *
 *  Prefer the former macro.  The second is defined only for legacy
 *  purposes for Windows builds, and might eventually disappear.
 */

#if defined PLATFORM_WINDWS
#if defined _WIN32 && ! defined _WIN64

#if ! defined WIN32
#define WIN32                          /* defined for legacy purposes         */
#endif

#if ! defined PLATFORM_32_BIT
#define PLATFORM_32_BIT
#endif

#endif
#endif                                 /* PLATFORM_WINDOWS                    */

/**
 *  Provides macros that mean 64-bit, and only 64-bit.
 *
 *      -  PLATFORM_64_BIT is defined on all platforms.
 *      -  WIN64 is defined on Windows platforms.
 *
 *  Prefer the former macro.  The second is defined only for legacy
 *  purposes for Windows builds, and might eventually disappear.
 *
 */

#if defined PLATFORM_WINDWS
#if defined _WIN64

#if ! defined WIN64
#define WIN64
#endif

#if ! defined  PLATFORM_64_BIT
#define PLATFORM_64_BIT
#endif

#endif
#endif                                 /* PLATFORM_WINDOWS                    */

/**
 *  Provides macros that mean 64-bit versus 32-bit when gcc or g++ are
 *  used. This can occur on Linux and other systems, and with mingw on
 *  Windows.
 *
 *      -  PLATFORM_64_BIT is defined on all platforms.
 *
 *  Prefer the former macro.  The second is defined only for legacy
 *  purposes for Windows builds, and might eventually disappear.
 */

#if defined __GNUC__
#if defined __x86_64__ || __ppc64__

#if ! defined PLATFORM_64_BIT
#define PLATFORM_64_BIT
#endif

#else

#if ! defined PLATFORM_32_BIT
#define PLATFORM_32_BIT
#endif

#endif
#endif

/**
 *  Provides macros that indicate if Microsoft C/C++ versus GNU are being
 *  used.  THe compiler being used normally provides test macros for itself.
 *
 *      -  PLATFORM_MSVC (replaces _MSC_VER)
 *      -  PLATFORM_GNU (replaces __GNUC__)
 *      -  PLATFORM_MINGW (replaces __MINGW32__)
 *      -  PLATFORM_CYGWIN
 */

#if defined _MSC_VER
#define PLATFORM_MSVC
#define PLATFORM_WINDOWS
#define PLATFORM_WINDOWS_API
#endif

#if defined __GNUC__
#define PLATFORM_GNU
// #define _GNU_SOURCE     /* \new ca 2017-09-03       */
#endif

#if defined __MINGW32__ || defined __MINGW64__
#define PLATFORM_MINGW
#define PLATFORM_WINDOWS
#define PLATFORM_WINDOWS_API
#endif

#if defined __CYGWIN32__
#define PLATFORM_CYGWIN
#endif

/**
 *	Provides a way to flag unused parameters at each "usage", without disabling
 *	them globally.  Use it like this:
 *
 *     void foo(int UNUSED(bar)) { ... }
 *     static void UNUSED_FUNCTION(foo)(int bar) { ... }
 *
 *  The UNUSED macro won't work for arguments which contain parenthesis,
 *  so an argument like float (*coords)[3] one cannot do,
 *
 *      float UNUSED((*coords)[3]) or float (*UNUSED(coords))[3].
 *
 *  This is the only downside to the UNUSED macro; in these cases fall back to
 *
 *      (void) coords;
 *
 *  Another possible definition is casting the unused value to void in the
 *  function body.
 */

#ifdef __GNUC__
#define UNUSED(x)               UNUSED_ ## x __attribute__((__unused__))
#else
#define UNUSED(x)               UNUSED_ ## x
#endif

#ifdef __GNUC__
#define UNUSED_FUNCTION(x)      __attribute__((__unused__)) UNUSED_ ## x
#else
#define UNUSED_FUNCTION(x)      UNUSED_ ## x
#endif

#define UNUSED_VOID(x)          (void) (x)

/**
 *  Provides macros to indicate the level standards support for some key
 *  cases.  We may have to play with this a bit to get it right.  The main
 *  use-case right now is in avoiding defining the nullptr macro in C++11.
 *
 *      -  PLATFORM_CPP_11
 */

#ifdef PLATFORM_MSVC
#if _MSC_VER >= 1700                /* __cplusplus value doesn't work, MS!    */
#define PLATFORM_CPP_11
#endif
#else
#if __cplusplus >= 201103L          /* i.e. C++11                             */
#define PLATFORM_CPP_11
#endif
#endif

/**
 *  Provides macros that mean 64-bit, and only 64-bit.
 *
 *      -  PLATFORM_DEBUG or PLATFORM_RELEASE
 *      -  DEBUG or NDEBUG for legacy usage
 *
 * Prefer the former macro.  The second is defined only for legacy
 * purposes for Windows builds, and might eventually disappear.
 */

#if ! defined PLATFORM_DEBUG
#if defined DEBUG || _DEBUG || _DEBUG_ || __DEBUG || __DEBUG__
#define PLATFORM_DEBUG
#if ! defined DEBUG
#define DEBUG
#endif
#endif
#endif

#if ! defined PLATFORM_DEBUG && ! defined PLATFORM_RELEASE
#define PLATFORM_RELEASE
#if ! defined NDEBUG
#define NDEBUG
#endif
#endif

/**
 *  Provides a check for error return codes from applications.  It is a
 *  non-error value for most POSIX-conformant functions.  This macro defines
 *  the integer value returned by many POSIX functions when they succeed --
 *  zero (0).
 *
 * \note
 *      Rather than testing this value directory, the macro functions
 *      is_posix_success() and not_posix_success() should be used.  See the
 *      descriptions of those macros for more information.
 */

#ifndef PLATFORM_POSIX_SUCCESS
#define PLATFORM_POSIX_SUCCESS              0
#endif

/**
 *
 *  PLATFORM_POSIX_ERROR is returned from a string function when it has
 *  processed an error.  It indicates that an error is in force.  Normally,
 *  the caller then uses this indicator to set a class-based error message.
 *  This macro defines the integer value returned by many POSIX functions when
 *  they fail -- minus one (-1).  The EXIT_FAILURE and PLATFORM_POSIX_ERROR
 *  macros also have the same value.
 *
 * \note
 *      Rather than testing this value directory, the macro functions
 *      is_posix_error() and not_posix_error() should be used.  See the
 *      descriptions of those macros for more information.
 */

#ifndef PLATFORM_POSIX_ERROR
#define PLATFORM_POSIX_ERROR              (-1)
#endif

/**
 *    This macro tests the integer value against PLATFORM_POSIX_SUCCESS.
 *    Other related macros are:
 *
 *       -  is_posix_success()
 *       -  is_posix_error()
 *       -  not_posix_success()
 *       -  not_posix_error()
 *       -  set_posix_success()
 *       -  set_posix_error()
 *
 * \note
 *      -   Some functions return values other than PLATFORM_POSIX_ERROR when
 *          an error occurs.
 *      -   Some functions return values other than PLATFORM_POSIX_SUCCESS
 *          when the function succeeds.
 *      -   Please refer to the online documentation for these quixotic
 *          functions, and decide which macro one want to use for the test, if
 *          any.
 *      -   In some case, one might want to use a clearer test.  For example,
 *          the socket functions return a result that is PLATFORM_POSIX_ERROR
 *          (-1) if the function fails, but non-zero integer values are
 *          returned if the function succeeds.  For these functions, the
 *          is_valid_socket() and not_valid_socket() macros are much more
 *          appropriate to use.
 *
 *//*-------------------------------------------------------------------------*/

#define is_posix_success(x)      ((x) == PLATFORM_POSIX_SUCCESS)

/**
 *  This macro tests the integer value against PLATFORM_POSIX_ERROR (-1).
 */

#define is_posix_error(x)        ((x) == PLATFORM_POSIX_ERROR)

/**
 *  This macro tests the integer value against PLATFORM_POSIX_SUCCESS (0).
 */

#define not_posix_success(x)     ((x) != PLATFORM_POSIX_SUCCESS)

/**
 *  This macro tests the integer value against PLATFORM_POSIX_ERROR (-1).
 */

#define not_posix_error(x)       ((x) != PLATFORM_POSIX_ERROR)

/**
 *  This macro set the integer value to PLATFORM_POSIX_SUCCESS (0).  The
 *  parameter must be an lvalue, as the assignment operator is used.
 */

#define set_posix_success(x)     ((x) = PLATFORM_POSIX_SUCCESS)

/**
 *  This macro set the integer value to PLATFORM_POSIX_ERROR (-1).  The
 *  parameter must be an lvalue, as the assignment operator is used.
 */

#define set_posix_error(x)       ((x) = PLATFORM_POSIX_ERROR)

/**
 *  Provides a wrapper for chdir() and _chdir().  Yet another ANSI versus ISO
 *  battle.  (USA versus Europe?  The Fed versus the EU?)
 */

#ifdef PLATFORM_POSIX_API
#define CHDIR       chdir
#else
#define CHDIR       _chdir
#endif

/**
 *  This macro provides a portable name for getcwd() [POSIX] and _getcwd()
 *  [Win32, Microsoft compiler].
 */

#ifdef PLATFORM_POSIX_API
#define GETCWD      getcwd
#else
#define GETCWD      _getcwd
#endif

#endif                  /* SEQ64_PLATFORM_MACROS_H */

/*
 * platform_macros.h
 *
 * vim: ts=4 sw=4 wm=4 et ft=c
 */

