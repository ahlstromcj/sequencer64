dnl ***************************************************************************
dnl xpc_thisptr.m4
dnl ---------------------------------------------------------------------------
dnl
dnl \file       	xpc_thisptr.m4
dnl \library    	xpc_suite subproject
dnl \author     	Chris Ahlstrom
dnl \date       	03/04/2008-03/04/2008
dnl \version    	$Revision$
dnl \license    	$XPC_SUITE_GPL_LICENSE$
dnl
dnl   Tests whether the user wants to disable checking of C "this" pointers.
dnl
dnl   This macro is used to get the arguments supplied
dnl   to the configure script (./configure --enable-thisptr)
dnl   
dnl   It defines the symbol XPC_NO_THISPTR, which is added to the CFLAGS for
dnl   the compiler call.
dnl
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_XPC_THISPTR],
[
   NOTHISPTR=
   if test -n "$GCC"; then

      AC_MSG_CHECKING(whether to enable this-pointer checks)
      AC_ARG_ENABLE(thisptr,
         [  --enable-thisptr=(no/yes) Turn on this-pointer checks (default=yes)],
         [
          case "${enableval}" in
           yes) thisptr=yes ;;
            no) thisptr=no  ;;
             *) AC_MSG_ERROR(bad value ${enableval} for --enable-thisptr) ;;
          esac
         ],
         [
            thisptr=yes
         ])

      AM_CONDITIONAL(DONOTHISPTR, test x$thisptr = xyes)

      NOTHISPTR=""
      if test "x$thisptr" = "xno" ; then
         NOTHISPTR="-DXPC_NO_THISPTR"
         AC_MSG_RESULT(yes)
      else
         AC_MSG_RESULT(no)
      fi
   fi
   AC_SUBST([NOTHISPTR])
   AC_DEFINE_UNQUOTED([NOTHISPTR], [$NOTHISPTR],
   [Set NOTHISPTR=-DXPC_NO_THISPTR if the user wants to disable this-checking.])
])

dnl ***************************************************************************
dnl xpc_thisptr.m4
dnl ---------------------------------------------------------------------------
dnl Local Variables:
dnl End:
dnl ---------------------------------------------------------------------------
dnl vim: ts=3 sw=3 et ft=config
dnl ---------------------------------------------------------------------------
