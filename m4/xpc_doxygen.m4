dnl ***************************************************************************
dnl xpc_doxygen.m4
dnl ---------------------------------------------------------------------------
dnl
dnl \file       	xpc_doxygen.m4
dnl \library    	xpc_suite subproject
dnl \author     	Chris Ahlstrom
dnl \date       	04/09/2009-04/11/2009
dnl \version    	$Revision$
dnl \license    	$XPC_SUITE_GPL_LICENSE$
dnl
dnl   Tests that the system has Doxygen installed.
dnl
dnl   We could have used AC_CHECK_PROG instead.
dnl
dnl   Note that the autoconf-archive package contains a very complex m4
dnl   macro for producing a variety of output documentation formats via
dnl   Doxygen.  The file containing the macro is
dnl
dnl      /usr/share/aclocal/ax_prog_doxygen.m4
dnl
dnl   We may master that macro someday!
dnl
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_PROG_DOXYGEN], [
   AC_CHECK_TOOL(DOXYGEN, doxygen,)
      if test "$DOXYGEN" = ""; then
echo "WARNING: Doxygen (http://www.stack.nl/~dimitri/doxygen) is not";
echo "         installed. You will not be able to create the documentation.";
      fi;
])

dnl ***************************************************************************
dnl xpc_doxygen.m4
dnl ---------------------------------------------------------------------------
dnl Local Variables:
dnl End:
dnl ---------------------------------------------------------------------------
dnl vim: ts=3 sw=3 et ft=config
dnl ---------------------------------------------------------------------------
