dnl ***************************************************************************
dnl xpc_debug.m4
dnl ---------------------------------------------------------------------------
dnl
dnl \file       	xpc_debug.m4
dnl \library    	xpc_suite subproject
dnl \author     	Chris Ahlstrom
dnl \date       	03/04/2008-07/24/2013
dnl \version    	$Revision$
dnl \license    	$XPC_SUITE_GPL_LICENSE$
dnl
dnl   Tests whether the user wants debugging, test coverage support, or
dnl   profiling.
dnl
dnl      --enable-debug
dnl      --enable-coverage
dnl      --enable-profile
dnl
dnl   Set up for debugging.  This macro is used to get the arguments supplied
dnl   to the configure script (./configure --enable-debug)
dnl   
dnl   It defines the symbol DBGFLAGS, which you should add to the COMMONFLAGS
dnl   for the compiler call.  Optimization is turned off, and the symbols
dnl   DEBUG, _DEBUG, and NWIN32 are defined.
dnl
dnl   In addition, the WARNINGS setting is changed to make sure all warnings
dnl   are shown.
dnl
dnl   In addition, the debugging is turned on via the -ggdb flag, instead
dnl   of the -g flag, to see if there is any advantage.   We're using the gdb
dnl   debugger.  If you don't use it, you may have to remove this flag.
dnl
dnl   Also, for convenience, this macro adds additional debugging symbols to
dnl   supplement DEBUG:  _DEBUG.
dnl
dnl   The main variable that results from this script is DBGFLAGS.
dnl
dnl   Also defined are DOCOVERAGE, COVFLAGS, DOPROFILE, PROFLAGS, and DODEBUG,
dnl   but normally we don't need them.
dnl
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_XPC_DEBUGGING],
[
   COVFLAGS=
   PROFLAGS=
   OPTFLAGS=
   GDBFLAGS=
   DBGFLAGS=
   MORFLAGS=

   if test -n "$GCC"; then

      AC_MSG_CHECKING([whether to enable gcov coverage tests])
      AC_ARG_ENABLE(coverage,
         [  --enable-coverage=(no/yes) Turn on a test-coverage build (default=no)],
         [
          case "${enableval}" in
           yes) coverage=yes ;;
            no) coverage=no  ;;
             *) AC_MSG_ERROR(bad value ${enableval} for --enable-coverage) ;;
          esac
         ],
         [
            coverage=no
         ])

      AM_CONDITIONAL(DOCOVERAGE, test x$coverage = xyes)
      if test "x$coverage" = "xyes" ; then
         COVFLAGS="-fprofile-arcs -ftest-coverage"
         OPTFLAGS="-O0"
         AC_MSG_RESULT(yes)
      else
         AC_MSG_RESULT(no)
      fi
   fi
   AC_SUBST([COVFLAGS])
   AC_DEFINE_UNQUOTED([COVFLAGS], [$COVFLAGS],
   [Define COVFLAGS=-fprofile-arcs -ftest-coverage if coverage support is wanted.])

   if test -n "$GCC"; then
      PROFLAGS=
      AC_MSG_CHECKING([whether to enable gprof profiling])
      AC_ARG_ENABLE(profile,
         [  --enable-profile=(no/yes/gprof/prof) Turn on profiling builds (default=no, yes=gprof)],
         [
          case "${enableval}" in
           yes) profile=yes ;;
            no) profile=no  ;;
          prof) profile=prof  ;;
         gprof) profile=gprof  ;;
             *) AC_MSG_ERROR(bad value ${enableval} for --enable-profile) ;;
          esac
         ],
         [
            profile=no
         ])

      AM_CONDITIONAL(DOPROFILE, test x$profile = xyes)
      if test "x$profile" = "xyes" ; then
         PROFLAGS="-pg"
         OPTFLAGS="-O0"
         GDBFLAGS="-ggdb"
         AC_MSG_RESULT(yes)
      elif test "x$profile" = "xprof" ; then
         PROFLAGS="-p"
         OPTFLAGS="-O0"
         GDBFLAGS="-g"
         AC_MSG_RESULT(prof)
      elif test "x$profile" = "xgprof" ; then
         PROFLAGS="-pg"
         OPTFLAGS="-O0"
         GDBFLAGS="-ggdb"
         AC_MSG_RESULT(prof)
      else
         AC_MSG_RESULT(no)
      fi
   fi
   AC_SUBST([PROFLAGS])
   AC_DEFINE_UNQUOTED([PROFLAGS], [$PROFLAGS],
   [Define PROFLAGS=-pg (gprof) or -p (prof) if profile support is wanted.])

dnl Handle the --enable-debug option.  First set the DBGFLAGS value, in case
dnl the coverage or profile options were processed above.
dnl
dnl DBGFLAGS="$DBGFLAGS -ggdb -O0 -DDEBUG -D_DEBUG -DNWIN32 -fno-inline"

   MORFLAGS="-DDEBUG -D_DEBUG -DNWIN32 -fno-inline"
   if test -n "$GCC"; then
      DBGFLAGS="$COVFLAGS $PROFLAGS"
      AC_MSG_CHECKING([whether to enable gdb debugging])
      AC_ARG_ENABLE(debug,
         [  --enable-debug=(no/yes/db/gdb) Turn on debug builds (default=no,
yes=gdb)],
         [
          case "${enableval}" in
           yes) debug=yes ;;
            no) debug=no  ;;
           gdb) debug=gdb ;;
            db) debug=db  ;;
             *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
          esac
         ],
         [
            debug=no
         ])

      AM_CONDITIONAL(DODEBUG, test x$debug = xyes)
      if test "x$debug" = "xyes" ; then
         OPTFLAGS="-O0"
         GDBFLAGS="-ggdb"
         AC_MSG_RESULT(yes)
      elif test "x$debug" = "xdb" ; then
         OPTFLAGS="-O0"
         GDBFLAGS="-g"
         AC_MSG_RESULT(yes)
      elif test "x$debug" = "xgdb" ; then
         OPTFLAGS="-O0"
         GDBFLAGS="-ggdb"
         AC_MSG_RESULT(yes)
      else
         if test "x$OPTFLAGS" = "x" ; then
            OPTFLAGS="-O3"
         fi
         MORFLAGS="-DNWIN32"
         AC_MSG_RESULT(no)
      fi
      DBGFLAGS="$DBGFLAGS $GDBFLAGS $OPTFLAGS $MORFLAGS"
   fi
   AC_SUBST([DBGFLAGS])
   AC_DEFINE_UNQUOTED([DBGFLAGS], [$DBGFLAGS],
   [Define DBGFLAGS=-g -O0 -DDEBUG -NWIN32 -fno-inline if debug support is wanted.])

])

dnl ***************************************************************************
dnl xpc_debug.m4
dnl ---------------------------------------------------------------------------
dnl Local Variables:
dnl End:
dnl ---------------------------------------------------------------------------
dnl vim: ts=3 sw=3 et ft=config
dnl ---------------------------------------------------------------------------
