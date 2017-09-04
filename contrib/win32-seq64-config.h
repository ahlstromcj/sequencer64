#ifndef _INCLUDE_SEQ___CONFIG_H
#define _INCLUDE_SEQ___CONFIG_H 1

#define WIN32 1
#define _WIN32 1

/* include/seq64-config.h. Generated automatically at end of configure. */
/* include/config.h.  Generated from config.h.in by configure.  */
/* include/config.h.in.  Generated from configure.ac by autoheader.  */
/* manually adapted to MingW builds */

#ifndef SEQ64_VERSION_DATE_SHORT
#define SEQ64_VERSION_DATE_SHORT "2017-09-02"
#endif
#ifndef SEQ64_VERSION
#define SEQ64_VERSION "0.93.4"
#endif

/* Indicates if ALSA MIDI support is enabled */
/* #undef ALSAMIDI_SUPPORT */

/* Names this version of application */
#ifndef SEQ64_APP_NAME
#define SEQ64_APP_NAME "seq64cli"
#endif

/* "The name to display as client/port" */
#ifndef SEQ64_CLIENT_NAME
#define SEQ64_CLIENT_NAME "seq64"
#endif

/* Define COVFLAGS=-fprofile-arcs -ftest-coverage if coverage support is
   wanted. */
#ifndef SEQ64_COVFLAGS
#define SEQ64_COVFLAGS
#endif

/* Define DBGFLAGS=-ggdb -O0 -DDEBUG -fno-inline if debug support is wanted.
   */
#ifndef SEQ64_DBGFLAGS
#define SEQ64_DBGFLAGS -ggdb -O0 -DDEBUG -D_DEBUG -fno-inline
#endif

/* Define to enable the event editor */
#ifndef SEQ64_ENABLE_EVENT_EDITOR
#define SEQ64_ENABLE_EVENT_EDITOR 1
#endif
#undef SEQ64_ENABLE_EVENT_EDITOR

/* Define to 1 if you have the <ctype.h> header file. */
#ifndef SEQ64_HAVE_CTYPE_H
#define SEQ64_HAVE_CTYPE_H 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the <errno.h> header file. */
#ifndef SEQ64_HAVE_ERRNO_H
#define SEQ64_HAVE_ERRNO_H 1
#endif

/* Define to 1 if you have the <fcntl.h> header file. */
// #ifndef SEQ64_HAVE_FCNTL_H
// #define SEQ64_HAVE_FCNTL_H 1
// #endif

/* Define to 1 if you have the <getopt.h> header file. */
#ifndef SEQ64_HAVE_GETOPT_H
#define SEQ64_HAVE_GETOPT_H 1
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef SEQ64_HAVE_INTTYPES_H
#define SEQ64_HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the `asound' library (-lasound). */
/* #undef HAVE_LIBASOUND */

/* Define to 1 if you have the `gtkmm-2.4' library (-lgtkmm-2.4). */
/* #undef HAVE_LIBGTKMM_2_4 */

/* Define to 1 if you have the `sigc-2.0' library (-lsigc-2.0). */
/* #undef HAVE_LIBSIGC_2_0 */

/* Define to 1 if you have the <limits.h> header file. */
#ifndef SEQ64_HAVE_LIMITS_H
#define SEQ64_HAVE_LIMITS_H 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef SEQ64_HAVE_MEMORY_H
#define SEQ64_HAVE_MEMORY_H 1
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
/* #undef HAVE_SYSLOG_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef SEQ64_HAVE_SYS_STAT_H
#define SEQ64_HAVE_SYS_STAT_H 1
#endif

/* Define to 1 if you have the <sys/sysctl.h> header file. */
/* #undef HAVE_SYS_SYSCTL_H */

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
// #ifndef SEQ64_HAVE_UNISTD_H
// #define SEQ64_HAVE_UNISTD_H 1
// #endif

/* Define to enable highlighting empty sequences */
#ifndef SEQ64_HIGHLIGHT_EMPTY_SEQS
#define SEQ64_HIGHLIGHT_EMPTY_SEQS 1
#endif

/* Define to enable JACK session support */
/* #undef JACK_SESSION */

/* Define to enable JACK driver */
/* #undef JACK_SUPPORT */

/* Define to enable main pattern scrollbars */
/* #undef JE_PATTERN_PANEL_SCROLLBARS */

/* Define to enable LASH support */
/* #undef LASH_SUPPORT */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#ifndef SEQ64_LT_OBJDIR
#define SEQ64_LT_OBJDIR ".libs/"
#endif

/* Define to enable multiple main windows */
#ifndef SEQ64_MULTI_MAINWID
#define SEQ64_MULTI_MAINWID 1
#endif

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
#define SEQ64_PACKAGE_STRING "Sequencer64 0.93.4"
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
#define SEQ64_PACKAGE_VERSION "0.93.4"
#endif

/* Define to enable pausing and pause button */
#ifndef SEQ64_PAUSE_SUPPORT
#define SEQ64_PAUSE_SUPPORT 1
#endif

/* Indicates if PortMidi support is enabled */
/* #undef PORTMIDI_SUPPORT */

/* Define PROFLAGS=-pg (gprof) or -p (prof) if profile support is wanted. */
#ifndef SEQ64_PROFLAGS
#define SEQ64_PROFLAGS 
#endif

/* Indicates if rtmidi support is enabled */
#ifndef SEQ64_RTMIDI_SUPPORT
#define SEQ64_RTMIDI_SUPPORT 1
#endif
#undef SEQ64_RTMIDI_SUPPORT

/* Define to enable statistics gathering */
/* #undef STATISTICS_SUPPORT */

/* Define to enable the chord generator */
#ifndef SEQ64_STAZED_CHORD_GENERATOR
#define SEQ64_STAZED_CHORD_GENERATOR 1
#endif

/* Define to enable Seq32 LFO window support */
#ifndef SEQ64_STAZED_LFO_SUPPORT
#define SEQ64_STAZED_LFO_SUPPORT 1
#endif
#undef SEQ64_STAZED_LFO_SUPPORT

/* Define to enable global transpose */
#ifndef SEQ64_STAZED_TRANSPOSE
#define SEQ64_STAZED_TRANSPOSE 1
#endif

/* Define to 1 if you have the ANSI C header files. */
#ifndef SEQ64_STDC_HEADERS
#define SEQ64_STDC_HEADERS 1
#endif

/* Configured target name. */
#ifndef SEQ64_TARGET
#define SEQ64_TARGET ""
#endif

/* Version number of package */
#ifndef SEQ64_VERSION
#define SEQ64_VERSION "0.93.4"
#endif

/* Indicates limited Windows support */
#ifndef SEQ64_WINDOWS_SUPPORT
#define SEQ64_WINDOWS_SUPPORT 1
#endif

/* gnu source */
#ifndef SEQ64__GNU_SOURCE
#define SEQ64__GNU_SOURCE 1
#endif

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

#ifdef SEQ64_ALSAMIDI_SUPPORT
#/**/undef/**/ SEQ64_RTMIDI_SUPPORT
#endif

#ifdef SEQ64_PORTMIDI_SUPPORT
#/**/undef/**/ SEQ64_RTMIDI_SUPPORT
#endif

#ifdef SEQ64_WINDOWS_SUPPORT
#/**/undef/**/ SEQ64_RTMIDI_SUPPORT
#endif


 
/* once: _INCLUDE_SEQ___CONFIG_H */
#endif
