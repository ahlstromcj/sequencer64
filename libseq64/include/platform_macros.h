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
 * \updates       2015-08-30
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
#define POSIX                          /* defined for legacy code purposes    */
#endif
#define PLATFORM_UNIX
#define PLATFORM_POSIX_API
#endif

/**
 *  Provides a "MacOSX" macro, in case the environment doesn't provide it.
 *  This macro is defined if not already defined and __APPLE__ and
 *  __MACH__ are encountered.
 */

#if defined MacOSX                     /* defined by the nar-maven-plugin     */
#define PLATFORM_MACOSX
#else
#if defined __APPLE__ && defined __MACH__    /* defined by the Apple compiler */
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
 *  used.
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
#endif

#if defined __MINGW32__
#define PLATFORM_MINGW
#define PLATFORM_WINDOWS
#define PLATFORM_WINDOWS_API
#endif

#if defined __CYGWIN32__
#define PLATFORM_CYGWIN
#endif

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

#endif                  /* SEQ64_PLATFORM_MACROS_H */

/*
 * platform_macros.h
 *
 * vim: ts=4 sw=4 wm=8 et ft=c
 */
