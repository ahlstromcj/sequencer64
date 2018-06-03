#ifndef _INCLUDE_SEQ___CONFIG_H
#define _INCLUDE_SEQ___CONFIG_H 1

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
 * \file          seq64-config.h for Qt/PortMidi
 *
 *  This module provides platform/build-specific configuration that is not
 *  modifiable via a "configure" operation.  It is meant for the hardwired
 *  qmake build of the PortMidi Linux and Windows versions.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-04-08
 * \updates       2018-05-31
 * \license       GNU GPLv2 or above
 *
 *  Qt Portmidi Linux version.
 *  Hardwired for use with qtcreator/qmake.
 *
 *  However, it still defines things that are available on GNU/Linux systems.
 */

#ifndef SEQ64_VERSION_DATE_SHORT
#define SEQ64_VERSION_DATE_SHORT "2018-05-31"
#endif

#ifndef SEQ64_VERSION
#define SEQ64_VERSION "0.95.0"
#endif

#ifndef SEQ64_GIT_VERSION
#define SEQ64_GIT_VERSION SEQ64_VERSION
#endif

/**
 *  This macro helps us adapt our "ui" includes to freaking qmake's
 *  conventions.  We used "userinterface.ui.h", while qmake is stuck on
 *  "ui_user_interface.h".
 *
 *  It's almost enough to make you use Cmake.  :-D
 */

#ifndef SEQ64_QMAKE_RULES
#define SEQ64_QMAKE_RULES
#endif

/* Indicates if ALSA MIDI support is enabled */
/* #undef ALSAMIDI_SUPPORT */

/**
 * Names this version of application.
 *
 *  "qpl" means "Qt PortMidi Linux-based".  On Windows, consider "Linux"
 *  to be "MingW32" :-).
 */

#ifndef SEQ64_APP_NAME
#define SEQ64_APP_NAME "qpseq64"
#endif

/**
 * Names the configuration file for this version of application.
 */

#ifndef SEQ64_CONFIG_NAME
#define SEQ64_CONFIG_NAME "qpseq64"
#endif

/* "The name to display as client/port" */

#ifndef SEQ64_CLIENT_NAME
#define SEQ64_CLIENT_NAME "seq64"
#endif

/* Define DBGFLAGS=-ggdb -O0 -DDEBUG -fno-inline if debug support is wanted.  */

#ifndef SEQ64_DBGFLAGS
#define SEQ64_DBGFLAGS -O3 -DDEBUG -D_DEBUG -fno-inline
#endif

/*
 * Define to enable the event editor.
 *
 *      NOT AVAILABLE YET IN QT USER INTERFACE.
 *
#ifndef SEQ64_ENABLE_EVENT_EDITOR
#define SEQ64_ENABLE_EVENT_EDITOR 1
#endif
 */

/* Define to 1 if you have the <ctype.h> header file. */
#ifndef SEQ64_HAVE_CTYPE_H
#define SEQ64_HAVE_CTYPE_H 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#ifndef SEQ64_HAVE_DLFCN_H
#define SEQ64_HAVE_DLFCN_H 1
#endif

/* Define to 1 if you have the <errno.h> header file. */
#ifndef SEQ64_HAVE_ERRNO_H
#define SEQ64_HAVE_ERRNO_H 1
#endif

/* Define to 1 if you have the <fcntl.h> header file. */
#ifndef SEQ64_HAVE_FCNTL_H
#define SEQ64_HAVE_FCNTL_H 1
#endif

/* Define to 1 if you have the <getopt.h> header file. */
#ifndef SEQ64_HAVE_GETOPT_H
#define SEQ64_HAVE_GETOPT_H 1
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef SEQ64_HAVE_INTTYPES_H
#define SEQ64_HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the `asound' library (-lasound). */
#ifndef SEQ64_HAVE_LIBASOUND
#define SEQ64_HAVE_LIBASOUND 1
#endif

/* Define to 1 if you have the `gtkmm-2.4' library (-lgtkmm-2.4). */
/* #undef HAVE_LIBGTKMM_2_4 */

/* Define to 1 if you have the `sigc-2.0' library (-lsigc-2.0). */
#ifndef SEQ64_HAVE_LIBSIGC_2_0
#define SEQ64_HAVE_LIBSIGC_2_0 1
#endif

/* Define to 1 if you have the <limits.h> header file. */
#ifndef SEQ64_HAVE_LIMITS_H
#define SEQ64_HAVE_LIMITS_H 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef SEQ64_HAVE_MEMORY_H
#define SEQ64_HAVE_MEMORY_H 1
#endif

/* Define if you have POSIX threads libraries and header files. */
#ifndef SEQ64_HAVE_PTHREAD
#define SEQ64_HAVE_PTHREAD 1
#endif

/* Have PTHREAD_PRIO_INHERIT. */
#ifndef SEQ64_HAVE_PTHREAD_PRIO_INHERIT
#define SEQ64_HAVE_PTHREAD_PRIO_INHERIT 1
#endif

/* Define to 1 if you have the <stdarg.h> header file. */
#ifndef SEQ64_HAVE_STDARG_H
#define SEQ64_HAVE_STDARG_H 1
#endif

/* Define to 1 if you have the <stddef.h> header file. */
#ifndef SEQ64_HAVE_STDDEF_H
#define SEQ64_HAVE_STDDEF_H 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef SEQ64_HAVE_STDINT_H
#define SEQ64_HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdio.h> header file. */
#ifndef SEQ64_HAVE_STDIO_H
#define SEQ64_HAVE_STDIO_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef SEQ64_HAVE_STDLIB_H
#define SEQ64_HAVE_STDLIB_H 1
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifndef SEQ64_HAVE_STRINGS_H
#define SEQ64_HAVE_STRINGS_H 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#ifndef SEQ64_HAVE_STRING_H
#define SEQ64_HAVE_STRING_H 1
#endif

/* Define to 1 if you have the <syslog.h> header file. */
#ifndef SEQ64_HAVE_SYSLOG_H
#define SEQ64_HAVE_SYSLOG_H 1
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef SEQ64_HAVE_SYS_STAT_H
#define SEQ64_HAVE_SYS_STAT_H 1
#endif

/* Define to 1 if you have the <sys/sysctl.h> header file. */
#ifndef SEQ64_HAVE_SYS_SYSCTL_H
#define SEQ64_HAVE_SYS_SYSCTL_H 1
#endif

/* Define to 1 if you have the <sys/time.h> header file. */
#ifndef SEQ64_HAVE_SYS_TIME_H
#define SEQ64_HAVE_SYS_TIME_H 1
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#ifndef SEQ64_HAVE_SYS_TYPES_H
#define SEQ64_HAVE_SYS_TYPES_H 1
#endif

/* Define to 1 if you have the <time.h> header file. */
#ifndef SEQ64_HAVE_TIME_H
#define SEQ64_HAVE_TIME_H 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef SEQ64_HAVE_UNISTD_H
#define SEQ64_HAVE_UNISTD_H 1
#endif

/*
 * Define to enable highlighting empty sequences
 *
 *      NOT AVAILABLE YET IN QT USER INTERFACE.
 *
#ifndef SEQ64_HIGHLIGHT_EMPTY_SEQS
#define SEQ64_HIGHLIGHT_EMPTY_SEQS 1
#endif
 */

/*
 * Define to enable JACK session.
 *
 * We want to use portmidi for the qtcreater build, as a stepping stone to
 * Windows builds.

#ifndef SEQ64_JACK_SESSION
#define SEQ64_JACK_SESSION 1
#endif
 */

/*
 * Define to enable JACK driver.
 *
 * We want to use portmidi for the qtcreater build, as a stepping stone to
 * Windows builds.

#ifndef SEQ64_JACK_SUPPORT
#define SEQ64_JACK_SUPPORT 1
#endif
 */

/* Define to enable main pattern scrollbars */
/* #undef JE_PATTERN_PANEL_SCROLLBARS */

/* Define to enable LASH */
/* #undef LASH_SUPPORT */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#ifndef SEQ64_LT_OBJDIR
#define SEQ64_LT_OBJDIR ".libs/"
#endif

/*
 * Define to enable multiple main windows.
 *
 *      NOT AVAILABLE YET IN QT USER INTERFACE.
 *
#ifndef SEQ64_MULTI_MAINWID
#define SEQ64_MULTI_MAINWID 1
#endif
 */

/* Name of package */
#ifndef SEQ64_PACKAGE
#define SEQ64_PACKAGE "sequencer64"
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef SEQ64_PACKAGE_BUGREPORT
#define SEQ64_PACKAGE_BUGREPORT "ahlstromcj@gmail.com"
#endif

/* Define to the full name of this package. */
#ifndef SEQ64_PACKAGE_NAME
#define SEQ64_PACKAGE_NAME "Sequencer64"
#endif

/* Define to the full name and version of this package. */
#ifndef SEQ64_PACKAGE_STRING
#define SEQ64_PACKAGE_STRING "Sequencer64 0.95.0"
#endif

/* Define to the one symbol short name of this package. */
#ifndef SEQ64_PACKAGE_TARNAME
#define SEQ64_PACKAGE_TARNAME "sequencer64"
#endif

/* Define to the home page for this package. */
#ifndef SEQ64_PACKAGE_URL
#define SEQ64_PACKAGE_URL ""
#endif

/* Define to the version of this package. */
#ifndef SEQ64_PACKAGE_VERSION
#define SEQ64_PACKAGE_VERSION "0.95.0"
#endif

/*
 * Define to enable pausing and pause button
 *
 *      NOT AVAILABLE YET IN QT USER INTERFACE.
 *
#ifndef SEQ64_PAUSE_SUPPORT
#define SEQ64_PAUSE_SUPPORT 1
#endif
 */

/*
 * Indicates if PortMidi support is enabled
 */

#ifndef SEQ64_PORTMIDI_SUPPORT
#define SEQ64_PORTMIDI_SUPPORT 1
#endif

/* Define PROFLAGS=-pg (gprof) or -p (prof) if profile support is wanted. */
#ifndef SEQ64_PROFLAGS
#define SEQ64_PROFLAGS
#endif

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Indicates that Qt5 is enabled */
#ifndef SEQ64_QTMIDI_SUPPORT
#define SEQ64_QTMIDI_SUPPORT 1
#endif

/* Indicates that rtmidi is enabled.
 *
 *      NOT AVAILABLE YET IN QT USER INTERFACE.
 *
#ifndef SEQ64_RTMIDI_SUPPORT
#define SEQ64_RTMIDI_SUPPORT 1
#endif
 */

/* Define to enable statistics gathering */
/* #undef STATISTICS_SUPPORT */

/*
 * Define to enable the chord generator
 *
 *      NOT AVAILABLE YET IN QT USER INTERFACE.
 *
#ifndef SEQ64_STAZED_CHORD_GENERATOR
#define SEQ64_STAZED_CHORD_GENERATOR 1
#endif
 */

/*
 * Define to enable Seq32 LFO window support.
 *
 *      NOT AVAILABLE YET IN QT USER INTERFACE.
 *
#ifndef SEQ64_STAZED_LFO_SUPPORT
#define SEQ64_STAZED_LFO_SUPPORT 1
#endif
 */

/*
 * Define to enable global transpose.
 *
 *      NOT AVAILABLE YET IN QT USER INTERFACE. But it can still be supported
 *      in the internal Sequencer64 library.
 */

#ifndef SEQ64_STAZED_TRANSPOSE
#define SEQ64_STAZED_TRANSPOSE 1
#endif

/* Define to 1 if you have the ANSI C header files. */
#ifndef SEQ64_STDC_HEADERS
#define SEQ64_STDC_HEADERS 1
#endif

/* Version number of package */
#ifndef SEQ64_VERSION
#define SEQ64_VERSION "0.94.8"
#endif

/* Indicates limited Windows support */
/* #undef WINDOWS_SUPPORT */

/* Define to 1 if the X Window System is missing or not being used. */
/* #undef X_DISPLAY_MISSING */

/* gnu source */
#ifndef SEQ64__GNU_SOURCE
#define SEQ64__GNU_SOURCE 1
#endif

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

#ifdef SEQ64_PORTMIDI_SUPPORT
#undef SEQ64_RTMIDI_SUPPORT
#endif

#ifdef SEQ64_WINDOWS_SUPPORT
#undef SEQ64_RTMIDI_SUPPORT
#endif

/* once: _INCLUDE_SEQ___CONFIG_H */

#endif

/*
 * seq64-config.h for Qt/PortMidi
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

