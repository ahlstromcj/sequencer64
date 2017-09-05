## aclocal.m4 -*- Autoconf -*- vim: filetype=config
##
## Autoconf macros for MinGW.org Runtime Library Package
##
## $Id$
##
## Written by Keith Marshall <keithmarshall@users.sourceforge.net>
## Copyright (C) 2014, 2016, 2017, MinGW.org Project
##
##
m4_include([VERSION.m4])
m4_define([__BUG_REPORT_URL__],[http://mingw.org/Reporting_Bugs])
##
##
## Permission is hereby granted, free of charge, to any person obtaining a
## copy of this software and associated documentation files (the "Software"),
## to deal in the Software without restriction, including without limitation
## the rights to use, copy, modify, merge, publish, distribute, sublicense,
## and/or sell copies of the Software, and to permit persons to whom the
## Software is furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice (including the next
## paragraph) shall be included in all copies or substantial portions of the
## Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
## OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
## FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
## DEALINGS IN THE SOFTWARE.
##

# MINGW_AC_CONFIG_EXTRA_SRCDIR( PACKAGE, WITNESS_FILE )
# -----------------------------------------------------
# Identify the top source directory for a sibling PACKAGE, which
# provides WITNESS_FILE, and set AC_SUBST variable PACKAGE_srcdir
# to its path relative to the build directory.  Prefers a sibling
# directory of ${srcdir} named exactly PACKAGE, but falls back to
# PACKAGE*, and then to *PACKAGE*, if necessary.
#
AC_DEFUN_ONCE([MINGW_AC_CONFIG_EXTRA_SRCDIR],
[AC_MSG_CHECKING([location of $1 source files])
 for ac_dir in ${srcdir}/../$1 ${srcdir}/../$1* ${srcdir}/../*$1*
   do test -f $ac_dir/$2 && { $1_srcdir=$ac_dir; break; }; done
 AC_MSG_RESULT([$$1_srcdir])
 AC_SUBST([$1_srcdir])dnl
])

# MINGW_AC_NO_EXECUTABLES
# -----------------------
# When building the runtime and W32 API libraries with only
# a partially installed compiler, as we will be required to do
# between the stage-1 and stage-2 phases of building GCC itself,
# autoconf's _AC_COMPILER_EXEEXT may choke because the runtime
# library itself is not yet available; here, we have provided
# a redefined "do-nothing" version, which will avoid this mode
# of failure, while retaining the original test for subsequent
# use, after verifying that it should not fail.
#
AC_DEFUN_ONCE([MINGW_AC_NO_EXECUTABLES],
[AC_BEFORE([$0],[AC_PROG_CC])dnl cannot let this use...
 m4_rename([_AC_COMPILER_EXEEXT],[_MINGW_AC_COMPILER_EXEEXT])dnl so...
 m4_define([_AC_COMPILER_EXEEXT])dnl move it away quickly!
])

# MINGW_AC_PROG_CC_COMPILE_ONLY
# -----------------------------
# A wrapper for AC_PROG_CC, ensuring that it will not succumb to
# the failure mode described above, while still running the checks
# provided by the original _AC_COMPILER_EXEEXT macro, when the
# circumstances of failure do not prevail.
#
AC_DEFUN_ONCE([MINGW_AC_PROG_CC_COMPILE_ONLY],
[AC_REQUIRE([MINGW_AC_NO_EXECUTABLES])dnl no need for linking
 AC_LINK_IFELSE([AC_LANG_PROGRAM],dnl minimal 'int main(){return 0;}'
 [_MINGW_AC_COMPILER_EXEEXT],dnl can create executables anyway!
 [_MINGW_AC_COMPILER_NO_EXECUTABLES])dnl
])

# _MINGW_AC_COMPILER_NO_EXECUTABLES
# ---------------------------------
# Package specific diagnostics for the case where the compiler
# really does succumb to the _AC_COMPILER_EXEEXT failure mode; in
# this case, we allow the build to proceed, but we disallow the
# building of executables and shared libraries by default.
#
AC_DEFUN([_MINGW_AC_COMPILER_NO_EXECUTABLES],
[AC_MSG_CHECKING([whether the C compiler can create executables])
 AC_MSG_RESULT([${may_enable_stage_2=no}])
 AC_MSG_WARN([$CC compiler cannot create executables!])
 AC_MSG_WARN([build will proceed to completion of stage-1 only;])
 AC_MSG_WARN([no executables or shared libraries will be built.])
])

# MINGW_AC_DISABLE_STAGE_2
# ------------------------
# Implement the '--disable-stage-2' configure option, such that
# it activates the non-failing _AC_COMPILER_EXEEXT behaviour, as
# described above; default is to proceed with the stage-2 build,
# provided the compiler is determined to be able to support it.
#
AC_DEFUN_ONCE([MINGW_AC_DISABLE_STAGE_2],
[AC_ARG_ENABLE([stage-2],
 [AS_HELP_STRING([--disable-stage-2],
  [disable building of DLL components which require a fully installed compiler;
   this option may be used during the compiler build process, to permit building
   of the libraries required before commencing stage-2 of the compiler build.
  ])dnl
 ],[],dnl
 [enable_stage_2=auto])dnl let compiler capability govern
])

# MINGW_AC_MAKE_COMMAND_GOALS
# ---------------------------
# Resolve choice of whether stage-2 should be built or not, in
# favour of user's preference, if supported by the compiler; by
# default prefer to build, if possible.  Propagate the resolved
# choice as a default make command goal, by assignment to the
# AC_SUBST variable, DEFAULT_MAKECMDGOALS.
#
AC_DEFUN_ONCE([MINGW_AC_MAKE_COMMAND_GOALS],
[AC_REQUIRE([MINGW_AC_DISABLE_STAGE_2])dnl
 AC_REQUIRE([MINGW_AC_PROG_CC_COMPILE_ONLY])dnl
 AC_MSG_CHECKING([whether to complete stage-2 build])
 ac_val="user's choice"
 AS_CASE([$enable_stage_2],dnl
 [auto],[enable_stage_2=${may_enable_stage_2-yes};dnl
  test x$enable_stage_2 = xyes && ac_val="default choice" dnl
  || ac_val="compiler override"],dnl
 [yes],[enable_stage_2=${may_enable_stage_2-yes};dnl
  test x$enable_stage_2 = xyes || ac_val="compiler override"dnl
 ])
 AC_MSG_RESULT([$enable_stage_2 ($ac_val)])
 test "x$enable_stage_2" = xno dnl
  && DEFAULT_MAKECMDGOALS=all-stage-1-only dnl
  || DEFAULT_MAKECMDGOALS=all-stage-1-and-2
 AC_SUBST([DEFAULT_MAKECMDGOALS])
])

# MINGW_AC_PROG_COMPILE_SX
# ------------------------
# Determine how to invoke GCC to compile *.sx asssembly language
# files, and provide a suitable derivative of GNU make's COMPILE.S
# rule in AC_SUBST variable 'COMPILE_SX'.  Note that GCC itself has
# supported direct compilation of such files from version 4.3 onward,
# (earlier versions require the '-x assembler-with-cpp' hint), but
# GNU make does not provide a complementary built-in rule.
#
AC_DEFUN([MINGW_AC_PROG_COMPILE_SX],
[AC_REQUIRE([AC_PROG_CC])dnl
 AC_MSG_CHECKING([for $CC option to compile .sx files])
 rm -f conftest.sx conftest.$OBJEXT; : > conftest.sx
 ac_compile_sx='$CC -c $ASFLAGS $CPPFLAGS $ac_val conftest.sx >&5'
 for ac_val in "" "-x assembler-with-cpp"; do
   (eval $ac_compile_sx) 2>&5 && test -f conftest.$OBJEXT && break
 done
 AC_SUBST([COMPILE_SX],[`echo '$(COMPILE.S)' $ac_val`])
 test "x$ac_val" = x && ac_val="none needed"
 test -f conftest.$OBJEXT || ac_val="not supported"
 AC_MSG_RESULT([$ac_val])
 rm -f conftest.sx conftest.$OBJEXT
 test "x$ac_val" = "xnot supported" && {
  AC_MSG_FAILURE([$CC cannot compile .sx files])
  }dnl
])

# MINGW_AC_SET_DLLVERSION( TAG, IMPLIB, DLLVERSION )
# --------------------------------------------------
# Create a configuration time substitution for MAP_[TAG]_A_DLLVERSION,
# such that it will define a target specific makefile variable assignment
# for target IMPLIB, with specified value assigned to DLLVERSION.
#
AC_DEFUN([MINGW_AC_SET_DLLVERSION],dnl
[AC_SUBST([MAP_][$1][_A_DLLVERSION],['$2: DLLVERSION = "$3"'])dnl
])

# $RCSfile$: end of file
