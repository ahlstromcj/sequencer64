dnl ***************************************************************************
dnl xpc_errorlog.m4
dnl ---------------------------------------------------------------------------
dnl
dnl \file       	xpc_errorlog.m4
dnl \library    	xpc_suite subproject
dnl \author     	Chris Ahlstrom
dnl \date       	03/04/2008-03/04/2008
dnl \version    	$Revision$
dnl \license    	$XPC_SUITE_GPL_LICENSE$
dnl
dnl   Tests whether the user wants error-logging.
dnl
dnl   Set up for inactive errorlogging functions using the switch
dnl   --disable-errorlog, while the default is an implicit
dnl   --enable-errorlog.
dnl
dnl   Enabling this feature enables any internationalization lookups and
dnl   and error-logging support; disabling it speeds up the code (one would
dnl   assume).
dnl
dnl \todo
dnl   It also disables the is_nullptr() family of macros, so that null
dnl   pointers are not as thoroughly checked for as without this option.
dnl   
dnl   It defines the symbol XPC_NO_ERRORLOG, which is added to the CFLAGS
dnl   for the compiler calls.  Also, the "-Wno-extra" flag is set, to nullify
dnl   the previous occurrence of "-Wextra".  We tried to use "-Wno-empty-body",
dnl   since disabling the error-logging functions causes a lot of these
dnl   warnings, but that flag caused a fatal error, contrary to this document:
dnl
dnl      http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
dnl
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_XPC_ERRORLOG],
[
   NOERRLOG=
   if test -n "$GCC"; then

      AC_MSG_CHECKING(whether to enable error-logging functions)
      AC_ARG_ENABLE(errorlog,
         [  --enable-errorlog=(no/yes) Turn on error logging (default=yes)],
         [
          case "${enableval}" in
           yes) errorlog=yes ;;
            no) errorlog=no  ;;
             *) AC_MSG_ERROR(bad value ${enableval} for --enable-errorlog) ;;
          esac
         ],
         [
            errorlog=yes
         ])

      AM_CONDITIONAL(DONOERRLOG, test x$errorlog = xyes)

      NOERRLOG=""
      if test "x$errorlog" = "xno" ; then
         NOERRLOG="-DXPC_NO_ERRORLOG"
         WARNINGS="$WARNINGS -Wno-unused -Wno-extra"
         AC_MSG_RESULT(yes)
      else
         AC_MSG_RESULT(no)
      fi
   fi
   AC_SUBST([NOERRLOG])
   AC_DEFINE_UNQUOTED([NOERRLOG], [$NOERRLOG],
   [Set NOERRLOG=-DXPC_NO_ERRORLOG if the user wants to disable error-logging.])
])

dnl ***************************************************************************
dnl xpc_errorlog.m4
dnl ---------------------------------------------------------------------------
dnl Local Variables:
dnl End:
dnl ---------------------------------------------------------------------------
dnl vim: ts=3 sw=3 et ft=config
dnl ---------------------------------------------------------------------------
