dnl ***************************************************************************
dnl gcc-version.m4 -- Chris Ahlstrom
dnl ---------------------------------------------------------------------------
dnl
dnl \file       	gcc-version.m4
dnl \library    	xpc_suite
dnl \author     	Chris Ahlstrom
dnl \date       	03/04/2008-03/04/2008
dnl \version    	$Revision$
dnl \license    	$XPC_SUITE_GPL_LICENSE$
dnl
dnl Found somewhere, and modified with explanations.  We've added two more
dnl variables that get returned, too:
dnl
dnl      gcc_version_major
dnl      gcc_version_minor
dnl
dnl \references
dnl      -  http://www.gnu.org/software/automake/manual/
dnl            autoconf/Changequote-is-Evil.html
dnl      -  http://www.shlomifish.org/lecture/Autotools/slides/changequote.html
dnl
dnl         We fixed this macro to add the space in each changequote call.
dnl
dnl \usage
dnl      AC_GCC_VERSION(TOPSRCDIR)
dnl
dnl         where TOPSRCDIR is the top-level source directory.
dnl
dnl Set up the variables:
dnl
dnl     gcc_version_trigger: pathname of gcc's version.c, if available
dnl
dnl         This comes out as "/gcc/version.c", which seems to mean that
dnl         version.c is not available.  But, if we pass, to ./configure, the
dnl         --with-gcc-version-trigger argument, then it comes out simply as
dnl         "yes".  Huh?
dnl
dnl     gcc_version_full: full gcc version string
dnl
dnl         This comes out as: 4.1.2 20061115 (prerelease) (Debian 4.1.1-21)
dnl
dnl     gcc_version: the first "word" in $gcc_version_full
dnl
dnl         This comes out as: 4.1.2
dnl
dnl         If we knew the compiler was gcc, we could use the -dumpversion
dnl         argument to obtain this value.
dnl
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_GCC_VERSION],
[
   changequote(, )dnl
   if test "${with_gcc_version_trigger+set}" = set; then
     gcc_version_trigger=$with_gcc_version_trigger
   else
     gcc_version_trigger=$1/gcc/version.c
   fi
   if test -f "${gcc_version_trigger}"; then
     gcc_version_full=`grep version_string "${gcc_version_trigger}" | sed -e 's/.*"\([^"]*\)".*/\1/'`
   else
     gcc_version_full=`$CC -v 2>&1 | sed -n 's/^gcc version //p'`
   fi
   gcc_version=`echo ${gcc_version_full} | sed -e 's/\([^ ]*\) .*/\1/'`
   gcc_version_major=`$CC -v 2>&1 | sed -n 's/^gcc version \(.\).*/\1/p'`
   gcc_version_minor=`gcc -v 2>&1 | sed -n 's/^gcc version [0-9]\.\(.\).*/\1/p'`
   changequote([, ])dnl
   AC_SUBST(gcc_version_trigger)
   AC_SUBST(gcc_version_full)
   AC_SUBST(gcc_version)
])dnl

dnl ***************************************************************************
dnl gcc-version.m4
dnl ---------------------------------------------------------------------------
dnl Local Variables:
dnl End:
dnl ---------------------------------------------------------------------------
dnl vim: ts=3 sw=3 et ft=config
dnl ---------------------------------------------------------------------------
