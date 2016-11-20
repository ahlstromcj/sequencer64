dnl ***************************************************************************
dnl configure.ac (Sequencer64 rtmidi standalone)
dnl ---------------------------------------------------------------------------
dnl
dnl \file          configure.ac
dnl \library       rtmidi
dnl \author        Chris Ahlstrom
dnl \date          2016-11-19
dnl \update        2016-11-19
dnl \version       $Revision$
dnl \license       $XPC_SUITE_GPL_LICENSE$
dnl
dnl   This script is for the Sequencer64 project, a repacking and fixing
dnl   of seq24 v0.9.2 and a direct fork from sequencer24.
dnl
dnl   Process this file with autoconf to produce a configure script.
dnl
dnl ---------------------------------------------------------------------------

AC_PREREQ(2.60)
AC_REVISION($Revision: 2.1.1$)dnl
AC_INIT(rtmidi, [2.1.1], gary@music.mcgill.ca, rtmidi)
AC_CONFIG_AUX_DIR([aux-files])
AC_CONFIG_SRCDIR(rtmidi.cpp)
AM_INIT_AUTOMAKE([1.14 -Wall -Werror foreign subdir-objects])

dnl libtool version: current:revision:age
dnl
dnl If the library source code has changed at all since the last update, then
dnl increment revision (`c:r:a' becomes `c:r+1:a').
dnl
dnl If any interfaces have been added, removed, or changed since the last update,
dnl increment current, and set revision to 0.
dnl
dnl If any interfaces have been added since the last public release, then
dnl increment age.
dnl
dnl If any interfaces have been removed since the last public release, then set
dnl age to 0.

m4_define([lt_current], 3)
m4_define([lt_revision], 0)
m4_define([lt_age], 0)
m4_define([lt_version_info], [lt_current:lt_revision:lt_age])
m4_define([lt_current_minus_age], [m4_eval(lt_current - lt_age)])

SO_VERSION=lt_version_info
AC_SUBST(SO_VERSION)

dnl Check version number coherency between rtmidi.h and configure.ac

AC_MSG_CHECKING([that version numbers are coherent])
AC_RUN_IFELSE(
   [AC_LANG_PROGRAM([#include <string.h>
        `grep "define SEQ64_RTMIDI_VERSION" $srcdir/rtmidi.h`],
        [return strcmp(SEQ64_RTMIDI_VERSION, PACKAGE_VERSION);])],
   [AC_MSG_RESULT([yes])],
   [AC_MSG_FAILURE([testing SEQ64_RTMIDI_VERSION==PACKAGE_VERSION failed, check that rtmidi.h defines SEQ64_RTMIDI_VERSION as "$PACKAGE_VERSION" or that the first line of configure.ac has been updated.])])

dnl Enable some nice automake features if they are available

m4_ifdef([AM_MAINTAINER_MODE], [AM_MAINTAINER_MODE])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Fill GXX with something before test.

AC_SUBST( GXX, ["no"] )
AC_SUBST(noinst_LIBRARIES)

dnl Checks for programs.

AC_PROG_CXX(g++ CC c++ cxx)
AM_PROG_AR
AC_PATH_PROG(AR, ar, no)
if [[ $AR = "no" ]] ; then
    AC_MSG_ERROR("Could not find ar - needed to create a library");
fi

LT_INIT([win32-dll])
AC_CONFIG_MACRO_DIR([m4])

dnl Checks for header files.

AC_HEADER_STDC

dnl AC_CHECK_HEADERS(sys/ioctl.h unistd.h)

dnl Check for debug

AC_MSG_CHECKING(whether to compile debug version)
AC_ARG_ENABLE(debug,
  [  --enable-debug = enable various debug output],
  [AC_SUBST( cppflag, [-D__RTMIDI_DEBUG__] ) AC_SUBST( cxxflag, [-g] ) AC_SUBST( object_path, [Debug] ) AC_MSG_RESULT(yes)],
  [AC_SUBST( cppflag, [] ) AC_SUBST( cxxflag, [-O3] ) AC_SUBST( object_path, [Release] ) AC_MSG_RESULT(no)])

dnl Set paths if prefix is defined

if test "x$prefix" != "x" && test "x$prefix" != "xNONE"; then
  LIBS="$LIBS -L$prefix/lib"
  CPPFLAGS="$CPPFLAGS -I$prefix/include/rtmidi"
fi

dnl For -I and -D flags

CPPFLAGS="$CPPFLAGS $cppflag"

dnl For debugging and optimization ... overwrite default because it has both -g
dnl and -O2
dnl CXXFLAGS="$CXXFLAGS $cxxflag"

CXXFLAGS="$cxxflag"

dnl Check compiler and use -Wall if gnu.

if [test $GXX = "yes" ;] then
  AC_SUBST( cxxflag, ["-Wall -Wextra"] )
fi

CXXFLAGS="$CXXFLAGS $cxxflag"

dnl Checks for doxygen

AC_CHECK_PROG( DOXYGEN, [doxygen], [doxygen] )
AM_CONDITIONAL( MAKE_DOC, [test "x${DOXYGEN}" != x] )

dnl Copy doc files to build dir if necessary

AC_CONFIG_LINKS( [doc/doxygen/footer.html:doc/doxygen/footer.html] )
AC_CONFIG_LINKS( [doc/doxygen/header.html:doc/doxygen/header.html] )
AC_CONFIG_LINKS( [doc/doxygen/tutorial.txt:doc/doxygen/tutorial.txt] )
AC_CONFIG_LINKS( [doc/images/ccrma.gif:doc/images/ccrma.gif] )
AC_CONFIG_LINKS( [doc/images/mcgill.gif:doc/images/mcgill.gif] )

dnl Checks for package options and external software

AC_CANONICAL_HOST

AC_SUBST( api, [""] )
AC_SUBST( req, [""] )
AC_MSG_CHECKING(for MIDI API)
case $host in
  *-*-linux*)
  AC_ARG_WITH(jack,
      [  --with-jack = choose JACK server support (Mac and Linux)], [
  api="$api -DSEQ64_BUILD_UNIX_JACK"
  AC_MSG_RESULT(using JACK)
  AC_CHECK_LIB(jack, jack_client_open, ,
   AC_MSG_ERROR(JACK support requires the jack library))], )

dnl Look for ALSA flag

  AC_ARG_WITH(alsa, [  --with-alsa = choose native ALSA sequencer API support (Linux only)], [
    api="$api -DSEQ64_BUILD_LINUX_ALSA"
    req="$req alsa"
    AC_MSG_RESULT(using ALSA)
    AC_CHECK_LIB(asound, snd_seq_open, ,
      AC_MSG_ERROR(ALSA support requires the asound library))], )

  if [test "$api" == "";] then
    AC_MSG_RESULT(using ALSA)
    AC_SUBST( api, [-DSEQ64_BUILD_LINUX_ALSA] )
    req="$req alsa"
    AC_CHECK_LIB(asound, snd_seq_open, ,
      AC_MSG_ERROR(ALSA sequencer support requires the asound library))
  fi

dnl Checks for pthread library.

  AC_CHECK_LIB(pthread, pthread_create, ,
   AC_MSG_ERROR(rtmidi requires the pthread library))
  ;;

  *-apple*)
  AC_ARG_WITH(jack, [  --with-jack = choose JACK server support (mac and linux only)], [
  api="$api -DSEQ64_BUILD_UNIX_JACK"
  AC_MSG_RESULT(using JACK)
  AC_CHECK_LIB(jack, jack_client_open, ,
   AC_MSG_ERROR(JACK support requires the jack library!))], )

  # Look for Core flag
  AC_ARG_WITH(core, [  --with-core = choose CoreMidi API support (mac only)], [
    api="$api -DSEQ64_BUILD_MACOSX_CORE"
    AC_MSG_RESULT(using CoreMidi)
    AC_CHECK_HEADER(CoreMIDI/CoreMIDI.h, [], [AC_MSG_ERROR(CoreMIDI header files not found!)] )
    LIBS="$LIBS -framework CoreMIDI -framework CoreFoundation -framework CoreAudio" ], )

  # If no api flags specified, use CoreMidi
  if [test "$api" == ""; ] then
    AC_SUBST( api, [-DSEQ64_BUILD_MACOSX_CORE] )
    AC_MSG_RESULT(using CoreMidi)
    AC_CHECK_HEADER(CoreMIDI/CoreMIDI.h,
      [],
      [AC_MSG_ERROR(CoreMIDI header files not found!)] )
    AC_SUBST( LIBS, ["-framework CoreMIDI -framework CoreFoundation -framework CoreAudio"] )
  fi
  ;;

  *-mingw32*)
  # Look for WinMM flag
  AC_ARG_WITH(winmm, [  --with-winmm = choose Windows MultiMedia (MM) API support (windoze only)], [
    api="$api -DSEQ64_BUILD_WINDOWS_MM"
    AC_MSG_RESULT(using WinMM)
    AC_SUBST( LIBS, [-lwinmm] )], )

  AC_ARG_WITH(winks, [  --with-winks = choose kernel streaming support (windoze only)], [
    api="$api -D__WINDOWS_KS__"
    AC_SUBST( LIBS, ["-lsetupapi -lksuser"] )
    AC_MSG_RESULT(using kernel streaming) ], )

# I can't get the following check to work so just manually add the library
# or could try the following?  AC_LIB_WINMM([midiOutGetNumDevs])
# AC_CHECK_LIB(winmm, midiInGetNumDevs, , AC_MSG_ERROR(Windows MIDI support requires the winmm library!) )],)

  # If no api flags specified, use WinMM
  if [test "$api" == "";] then
    AC_SUBST( api, [-DSEQ64_BUILD_WINDOWS_MM] )
    AC_MSG_RESULT(using WinMM)
    AC_SUBST( LIBS, [-lwinmm] )
  fi
  ;;

  *)
  # Default case for unknown realtime systems.
  AC_MSG_ERROR(Unknown system type for MIDI support!)
  ;;
esac

CPPFLAGS="$CPPFLAGS $api"

AC_CONFIG_FILES([
 rtmidi-config
 rtmidi.pc
 Makefile
 tests/Makefile
 doc/Makefile
 doc/doxygen/Doxyfile
])

AC_OUTPUT

chmod oug+x rtmidi-config

cat << E_O_F

Now do 'make' to compile the rtmidi-related projects.
Do 'make install' as root/sudo to install the Sequencer64 application.
Some tests can be run using the separate seq64-tests project.
Do 'make dist', etc. to create gzip and other archives.
To build the PDF documentation, make sure Doxygen and TexLive are installed.

E_O_F

dnl configure.ac (sequencer64 rtmidi)
dnl
dnl vim: ts=4 sw=4 et ft=config
