dnl ***************************************************************************
dnl xpc_nullptr.m4
dnl ---------------------------------------------------------------------------
dnl
dnl \file       	xpc_nullptr.m4
dnl \library    	xpc_suite subproject
dnl \author     	Chris Ahlstrom
dnl \date       	03/04/2008-03/04/2008
dnl \version    	$Revision$
dnl \license    	$XPC_SUITE_GPL_LICENSE$
dnl
dnl   Tests whether the user wants null-pointer checking, where the
dnl   is_nullptr() family of macros check the macro argument against
dnl   "nullptr" (a.k.a. "NULL).
dnl
dnl   Set up for inactive null-pointer checking using the switch
dnl   --disable-nullptr, while the default is an implicit
dnl   --enable-nullptr.
dnl
dnl   Disabling it speeds up the code (one would assume).
dnl
dnl   It defines the symbol XPC_NO_NULLPTR, which is added to the CFLAGS
dnl   for the compiler calls.  Also, the "-Wno-xxxxa" flag is set, to nullify
dnl   warning about ....
dnl
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_XPC_NULLPTR],
[
   NONULLPTR=
   if test -n "$GCC"; then

      AC_MSG_CHECKING(whether to enable null-pointer checking)
      AC_ARG_ENABLE(errorlog,
         [  --enable-nullptr=(no/yes) Turn on error logging (default=yes)],
         [
          case "${enableval}" in
           yes) nullptr=yes ;;
            no) nullptr=no  ;;
             *) AC_MSG_ERROR(bad value ${enableval} for --enable-nullptr) ;;
          esac
         ],
         [
            nullptr=yes
         ])

      AM_CONDITIONAL(DONONULLPTR, test x$nullptr = xyes)

      NONULLPTR=""
      if test "x$nullptr" = "xno" ; then
         NONULLPTR="-DXPC_NO_NULLPTR"
         WARNINGS="$WARNINGS -Wno-unused -Wno-extra"
         AC_MSG_RESULT(yes)
      else
         AC_MSG_RESULT(no)
      fi
   fi
   AC_SUBST([NONULLPTR])
   AC_DEFINE_UNQUOTED([NONULLPTR], [$NONULLPTR],
   [Set NONULLPTR=-DXPC_NO_NULLPTR if the user wants to disable null-pointer checking.])
])

dnl ***************************************************************************
dnl xpc_nullptr.m4
dnl ---------------------------------------------------------------------------
dnl Local Variables:
dnl End:
dnl ---------------------------------------------------------------------------
dnl vim: ts=3 sw=3 et ft=config
dnl ---------------------------------------------------------------------------
