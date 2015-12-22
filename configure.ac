dnl ***************************************************************************
dnl configure.ac (Sequencer64)
dnl ---------------------------------------------------------------------------
dnl
dnl \file          configure.ac
dnl \library       Sequencer64
dnl \author        Chris Ahlstrom
dnl \date          2015-09-11
dnl \update        2015-11-28
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
AC_REVISION($Revision: 0.9$)dnl
AC_INIT(Sequencer64, [0.9.9], [ahlstromcj@gmail.com])
AC_CONFIG_AUX_DIR([aux-files])

dnl Do we need the following?  No, but it cleans up a lot of warning messages.

AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_HEADER([include/config.h])

dnl Prepends "SEQ64" to a lot of the macro definitions, for a better
dnl guarantee of avoiding naming conflicts.

AX_PREFIX_CONFIG_H([include/seq64-config.h], [seq64])

dnl New for autoconf 2.60, prevents spurious datarootdir warnings.

AC_DEFUN([AC_DATAROOTDIR_CHECKED])

dnl 1. Package version information.

PACKAGE=sequencer64
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_SUBST(PACKAGE)

VERSION="0.9.9.11"
AC_DEFINE_UNQUOTED(VERSION, "$VERSION")
AC_SUBST(VERSION)

SEQUENCER64_SUITE_NAME="SEQUENCER64"
AC_SUBST([SEQUENCER64_SUITE_NAME])

dnl 1.a. Set up version information for this project.
dnl
dnl      This versioning is the libtool version.
dnl      Warning: These values are inserted into libtool's "version-info"
dnl      value, and have to follow libtool rules:
dnl
dnl      Format is 'current:revision:age', where
dnl
dnl      current = Number of the current interface.  If the interface has
dnl      changed, increment this number and set 'revision' to 0.  If the new
dnl      interface has removed elements of the preceding interface, backward
dnl      compatibility has been broken, and you must set 'age' to 0.
dnl
dnl      revision = Implementation number of the most recent interface,
dnl      where 0 is the first.  The runtime loader always uses the library
dnl      with the highest revision.  If you change any source modules,
dnl      increment this number.
dnl
dnl      age = The number of additional interface supported by this library.
dnl      The age must the less than or equal to 'current'.  If the new
dnl      interface is a superset of the preceding interface, 'age' must be
dnl      incremented.

SEQUENCER64_API_MAJOR="0"
AC_SUBST([SEQUENCER64_API_MAJOR])

SEQUENCER64_API_MINOR="9"
AC_SUBST([SEQUENCER64_API_MINOR])

SEQUENCER64_API_PATCH="0"
AC_SUBST([SEQUENCER64_API_PATCH])

SEQUENCER64_OFFICIAL_VERSION="$SEQUENCER64_API_MAJOR.$SEQUENCER64_API_MINOR.$SEQUENCER64_API_PATCH"

dnl For version 1.0.0, our own first version, we reset the libtool version
dnl information.

SEQUENCER64_LT_CURRENT="0"
AC_SUBST([SEQUENCER64_LT_CURRENT])

SEQUENCER64_LT_REVISION="0"
AC_SUBST([SEQUENCER64_LT_REVISION])

SEQUENCER64_LT_AGE="0"
AC_SUBST([SEQUENCER64_LT_AGE])

SEQUENCER64_LIBTOOL_VERSION="$SEQUENCER64_LT_CURRENT.$SEQUENCER64_LT_REVISION.$SEQUENCER64_LT_AGE"

dnl 1.b. Set up the version strings.  The primary version string of interest
dnl      is SEQUENCER64_API_VERSION.

SEQUENCER64_API_VERSION="$SEQUENCER64_API_MAJOR.$SEQUENCER64_API_MINOR"
AC_SUBST([SEQUENCER64_API_VERSION])

dnl 1.c. Report the version information in the progress output.

AC_MSG_CHECKING(major version)
AC_MSG_RESULT($SEQUENCER64_API_MAJOR)
AC_MSG_CHECKING(minor version)
AC_MSG_RESULT($SEQUENCER64_API_MINOR)
AC_MSG_CHECKING(patchlevel)
AC_MSG_RESULT($SEQUENCER64_API_PATCH)

dnl 1.d. Set up the suite name and current-project name.

SEQUENCER64_PROJECT_NAME="SEQUENCER64"
AC_SUBST([SEQUENCER64_PROJECT_NAME])

dnl ---------------------------------------------------------

AC_DEFINE(_GNU_SOURCE, 1, [gnu source])

dnl Checks for programs.

AC_PROG_AWK
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S

dnl 2. Libtool
dnl
dnl Initialize libtool in the default manner, which turns on shared
dnl libraries if available, and enable static libraries if they do not conflict
dnl with shared libraries.  Also sets the shell variable LIBTOOL_DEPS.
dnl
dnl The latest GNU documentation specifies version 2 of libtool, but Debian
dnl testing provides only 1.5 (Debian unstable now provides 2.2).  So we don't
dnl use these new macros, just the older, deprecated macros.
dnl
dnl LT_PREREQ([1.5])
dnl LT_INIT
dnl AC_SUBST([LIBTOOL_DEPS])
dnl
dnl See <http://www.delorie.com/gnu/docs/libtool/libtool_27.html> for the
dnl usage of LIBTOOL_DEPS.
dnl
dnl Uncomment the following if you want more straightforward debugging.  An
dnl alternative is to specify --disable-shared when running 'configure'.
dnl
dnl AC_DISABLE_SHARED

AC_PROG_LIBTOOL

LT_RELEASE="$SEQUENCER64_API_MAJOR.$SEQUENCER64_API_MINOR"
AC_SUBST([LT_RELEASE])

LT_CURRENT="$SEQUENCER64_API_MAJOR"
AC_SUBST([LT_CURRENT])

LT_REVISION="$SEQUENCER64_API_MINOR"
AC_SUBST([LT_REVISION])

LT_AGE="$SEQUENCER64_API_PATCH"
AC_SUBST([LT_AGE])

dnl 3. Set up expandable installation directory(ies).
dnl
dnl These directories are for the sequencer64 library.
dnl
dnl We want all of the header files and library files for the whole suite
dnl to be in the same sub-directory, sequencer64-1.1.  This will avoid
dnl clutter and put all our code in one place.
dnl
dnl      /usr/include/sequencer64-0.9/sequencer64
dnl      /usr/lib/sequencer64-0.9
dnl      /usr/man/man1
dnl      /usr/share/doc/sequencer64-0.9/doc/dox/html

sequencer64includedir="${includedir}/sequencer64.1-${SEQUENCER64_API_VERSION}"
AC_SUBST(sequencer64includedir)

sequencer64libdir="${libdir}/sequencer64.1-${SEQUENCER64_API_VERSION}"
AC_SUBST(sequencer64libdir)

sequencer64docdir="${datadir}/doc/sequencer64.1-${SEQUENCER64_API_VERSION}"
AC_SUBST(sequencer64docdir)

sequencer64doxygendir="${datadir}/doc/sequencer64.1-${SEQUENCER64_API_VERSION}"
AC_SUBST(sequencer64doxygendir)

dnl Do we have -Wl,--as-needed?

AC_MSG_CHECKING(if --as-needed works)
AC_ARG_ENABLE(as_needed,
    [AS_HELP_STRING(--enable-as-needed,
         [Enable linker option -Wl,--as-needed (default=yes)])],
    [
    case "${enableval}" in
         yes) as_needed="1";;
         no) as_needed="";;
         *) AC_MSG_ERROR(bad value ${enableval} for --enable-as_needed);;
    esac
    ],[
        as_needed="unknown"
    ])
if test x"${as_needed}" = x"unknown"; then
    ac_old_ldflags="${LDFLAGS}"
    LDFLAGS="-Wl,--as-needed"
    AC_TRY_LINK(
                [],
                [],
                [as_needed="1"],
                [as_needed=""])
    LDFLAGS="${ac_old_ldflags}" 
fi
if test -n "$as_needed"; then
    AC_MSG_RESULT(yes)
    LDFLAGS="${LDFLAGS} -Wl,--as-needed"
else
    AC_MSG_RESULT(no)
fi

dnl Check for deprecated GTK constructs as per
dnl http://www.murrayc.com/permalink/2011/01/18/avoiding-use-of-deprecated-api/
dnl
dnl Looks too difficult, errors in gtk headers. Try gtkmm-3 directly instead.
dnl That is unworkable at this time, though.
dnl
dnl MM_ARG_ENABLE_WARNINGS([MM_WFLAGS],
dnl  [-Wall],
dnl  [-Wall -Wextra -Wno-missing-field-initializers -DGSEAL_ENABLE],
dnl  [G GDK GDK_PIXBUF PANGO GTK])

dnl 4. Get external flags, if any.

CFLAGS="${CFLAGS}"

dnl Checks for libraries.

AC_CHECK_LIB(rt, main, ,
    AC_MSG_ERROR([POSIX.1b RealTime Library Missing -lrt]))

dnl Convert from gtkmm-2.4 to gtkmm-3.0.  It currently builds either way.
dnl No! I was mistaken, because I had left some 2.4 paths in place below.

AC_CHECK_LIB(gtkmm-2.4, _init,,
    AC_MSG_ERROR(Essential library libgtkmm-2.4 not found))

dnl LATER:
dnl
dnl AC_CHECK_LIB(gtkmm-3.0, _init,,
dnl     AC_MSG_ERROR(Essential library libgtkmm-3.0 not found))

AC_CHECK_LIB(sigc-2.0, main,,
    AC_MSG_ERROR(Essential library libsigc++-2.0 not found))

dnl Checks for header files.

AC_HEADER_STDC
AC_CHECK_HEADERS(getopt.h)

dnl Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST

dnl ALSA

AM_PATH_ALSA(0.9.0)

dnl Check for the presence of Doxygen.  Not an error to be missing, but
dnl we don't want to try to build documentation if it is not present.
dnl See http://www.gnu.org/software/autoconf-archive/ax_prog_doxygen.html
dnl
dnl AC_PROG_DOXYGEN

AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN";
   then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi

dnl 5.b. gcc support.

AC_GCC_VERSION

dnl C++ support.  Run tests in C++.  Note the order; avoids messy warnings.

AC_LANG_C
AC_LANG_CPLUSPLUS dnl or AC_LANG([C++])
AC_PROG_CXX
AC_PROG_CXXCPP

dnl 5.c. Mingw support.
dnl
dnl Checks for the MingW32 compiler environment. If present, sets shell
dnl variable MINGW32 to 'yes'. If not present, sets MINGW32 to the
dnl empty string.
dnl
dnl Also check to see if the user specified the MingW32 installation
dnl location.  On Debian, the installation is in /usr/i586-mingw32msvc/.
dnl The Mingw versions of the executables start with "i586".  For example,
dnl the compiler is "i586-mingw32msvc-gcc-3.4.5".
dnl
dnl http://www.wxwidgets.org/wiki/index.php/Cross-Compiling_Under_Linux
dnl
dnl is a good place to start learning about building Windows applications.

AC_XPC_MINGW32
AC_ARG_WITH(mingw32,[  --with-mingw32[=DIR]    MingW32 installation location])

dnl gtkmm
dnl
dnl PKG_CHECK_MODULES(GTKMM, gtkmm-3.0 >= 3.0.0)

PKG_CHECK_MODULES(GTKMM, gtkmm-2.4 >= 2.4.0)

AC_SUBST(GTKMM_CFLAGS)
AC_SUBST(GTKMM_LIBS)

dnl Checks for the Cygwin environment. If present, sets shell variable
dnl CYGWIN to 'yes'; if not present, sets CYGWIN to the empty string.

AC_CYGWIN

dnl Clear out the X11 flags for the case where we are cross-compiling
dnl for i586-mingw32 targets, under UNIX.

if test "x$CYGWIN" = "xyes" -o "x$MINGW32" = "xyes" ; then
   X_CFLAGS=""
   X_LIBS=""
fi

dnl JACK SUPPORT

AC_ARG_ENABLE(jack,
    [AS_HELP_STRING(--disable-jack, [Disable JACK support])],
    [jack=$enableval],
    [jack=yes])

AC_ARG_ENABLE(jack-session,
    [AS_HELP_STRING(--disable-jack-session, [Disable JACK session support])],
    [jack_session=$enableval],
    [jack_session=yes])

if test "$jack" != "no"; then
    PKG_CHECK_MODULES(JACK, jack >= 0.90.0, jack_found="yes", jack_found="no")
    if test "$jack_found" = "yes"; then
        AC_DEFINE(JACK_SUPPORT, 1, [Define to enable JACK driver])
        AC_SUBST(JACK_CFLAGS)
        AC_SUBST(JACK_LIBS)

        dnl JACK session        
        if test "$jack_session" != "no"; then
            AC_CHECK_HEADER(jack/session.h, jack_session_found="yes",
                            jack_session_found="no")
            if test "$jack_session_found" = "yes"; then
                AC_DEFINE(JACK_SESSION, 1,
                          [Define to enable JACK session support])
            fi
        fi

    else
        AC_MSG_WARN([*** Could not find JACK library, disabling JACK support])
    fi
else
    AC_MSG_WARN([*** JACK support not enabled.]);
fi

dnl LASH support
dnl
dnl WARNING:  LASH support is too entangle in the GUI system at present in
dnl this code, so the default is to DISABLE it!  We need to separate out
dnl some of the perform code and other code from the GUI framework.

AC_ARG_ENABLE(lash,
    [AS_HELP_STRING(--disable-lash, [Disable LASH support])],
    [lash=$enableval],
    [lash=yes])

if test "$lash" != "no"; then
    PKG_CHECK_MODULES(LASH, lash-1.0 >= 0.5.0, lash_found="yes", lash_found="no")
    if test "$lash_found" = "yes"; then
        AC_DEFINE(LASH_SUPPORT, 1, [Define to enable LASH support])
        AC_SUBST(LASH_CFLAGS)
        AC_SUBST(LASH_LIBS)
    else
        AC_MSG_WARN([*** LASH not found, session support will not be built.])
    fi
else
    AC_MSG_WARN([*** LASH support not enabled.]);
fi

dnl PortMidi support

dnl TODO

dnl Support for highlighting empty sequences (in yellow)

AC_ARG_ENABLE(highlight,
    [AS_HELP_STRING(--disable-highlight, [Disable highlighting empty sequences])],
    [highlight=$enableval],
    [highlight=yes])

if test "$highlight" != "no"; then
    AC_DEFINE(HIGHLIGHT_EMPTY_SEQS, 1, [Define to enable highlighting empty sequences])
else
    AC_MSG_WARN([*** Highlighting empty sequences not enabled.]);
fi

dnl 6.0  Top portion of the config.h/sequencer64-config.h header files.
dnl      The sequencer64-config.h header file has "SEQUENCER64_" prepended
dnl      to these automatically for us.  Also, we can't use
dnl      #define SEQUENCER64_API_VERSION "${SEQUENCER64_API_VERSION}",
dnl      since the macro call in quotes does /not/ get expanded.
dnl
dnl Note: The AX_PREFIX_CONFIG_H macro called way above prepends
dnl       "SEQUENCER64_" to these values when it generated sequencer64-config.h.
dnl
dnl #define API_VERSION "0.9"             // already defined on command line

AH_TOP(

#define VERSION_DATE_SHORT "2015-12-22"
#define VERSION "0.9.9.11"

)

dnl 7. Checks for build configuration.
dnl
dnl 7.a. Compiling with debugging, coverage, or profiling options.
dnl      Implemented in m4/xpc_debug.m4.
dnl
dnl      --enable-debug
dnl      --enable-coverage
dnl      --enable-profiling

AC_XPC_DEBUGGING

dnl 8.  Set up other options in the compiler macros.

APIDEF="-DAPI_VERSION=\"$SEQUENCER64_API_VERSION\""
WARNFLAGS="-Wall -Wextra -pedantic $WARNINGS"
SPEEDFLAGS="-ffast-math"
COMMONFLAGS="$WARNFLAGS -D_REENTRANT $APIDEF $DBGFLAGS $STACKCHK $NOERRLOG $NOTHISPTR"

dnl Add $WARNINGS_ENABLED to CXXFLAGS if you want to see all warnings, all
dnl the time.

WARNINGS_ENABLED="-Wall"
WARNINGS_DISABLED="-Wno-unused-parameter -Wno-non-virtual-dtor"

dnl We added -std=c++11 for g++ 4.8, and -Wno-deprecated-declarations to
dnl stop g++ 5.2 from griping about usage of std::auto_ptr<>.
dnl
dnl 2015-10-02:  After a Debian Sid update, the configfile module wouldn't
dnl compile because, we think, the sigc++ now required C++11 support.
dnl So we put that requiremet back, and had to fix some issues with
dnl conditional compilation.

CFLAGS="$CFLAGS $COMMONFLAGS"
CXXFLAGS="$CFLAGS -std=c++11 -Wno-variadic-macros -Wno-deprecated-declarations"

dnl Set up the Makefiles.  First, add Doxygen support conditionally, not using
dnl doc/dox/Makefile.  Then do the rest.

AM_CONDITIONAL([HAVE_DOXYGEN],
    [test -n "$DOXYGEN"])AM_COND_IF([HAVE_DOXYGEN],
    [AC_CONFIG_FILES([doc/dox/Makefile])])

AC_CONFIG_FILES([
 Makefile
 libseq64/Makefile
 libseq64/include/Makefile
 libseq64/src/Makefile
 seq_gtkmm2/Makefile
 seq_gtkmm2/include/Makefile
 seq_gtkmm2/src/Makefile
 seq_portmidi/Makefile
 seq_portmidi/include/Makefile
 seq_portmidi/src/Makefile
 resources/pixmaps/Makefile
 Sequencer64/Makefile
 man/Makefile
 ])

AC_OUTPUT

dnl configure.ac (sequencer64)
dnl
dnl vim: ts=4 sw=4 et ft=config
