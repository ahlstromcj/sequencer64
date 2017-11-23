dnl ***************************************************************************
dnl xpc_mingw.m4
dnl ---------------------------------------------------------------------------
dnl
dnl \file          xpc_mingw.m4
dnl \library       XPC
dnl \author        Chris Ahlstrom
dnl \date          2008-03-04
dnl \update        2017-09-01
dnl \version       $Revision$
dnl \license       $XPC_SUITE_GPL_LICENSE$
dnl
dnl   Sets up for using MingW.
dnl
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_XPC_MINGW],
[
   AC_MSG_CHECKING([whether to enable MingW32/64 for target system])
   AC_ARG_ENABLE(mingw,
      [  --enable-mingw=(no/w32/w64) Turn on MingW32/64 support (default=no)],
      [
       case "${enableval}" in
        w32)
            mingw="yes"
            ;;
        w64)
            mingw="yes"
            ;;
         no)
            mingw="no"
            ;;
          *)
            AC_MSG_ERROR(bad value ${enableval} for --enable-mingw) ;;
       esac
      ],
      [
         mingw=no
      ])

   CYGWIN=
   MINGW32=
   case $host_os in
      *cygwin*)
            CYGWIN=1 ;
            mingw="no"
            ;;
      *mingw*)
            MINGW32=1
            ;;
   esac

   AR="ar"                # why???
   mingwflavor="none"

   if test "x$MINGW" = "xyes" ; then

      # Fix for the debian distribution of mingw.  Note that we can assume
      # the other executables are found as well.  Actually, let's comment this
      # old stuff out.
      #
      # if test -x "/usr/i586-mingw32msvc/bin/ar" ; then
      #    AR="/usr/i586-mingw32msvc/bin/ar"
      #    AS="/usr/i586-mingw32msvc/bin/as"
      #    DLLTOOL="/usr/i586-mingw32msvc/bin/dlltool"
      #    LD="/usr/i586-mingw32msvc/bin/ld"
      #    LDBFD="/usr/i586-mingw32msvc/bin/ld.bfd"
      #    NM="/usr/i586-mingw32msvc/bin/nm"
      #    OBJCOPY="/usr/i586-mingw32msvc/bin/objcopy"
      #    OBJDUMP="/usr/i586-mingw32msvc/bin/objdump"
      #    RANLIB="/usr/i586-mingw32msvc/bin/ranlib"
      #    READELF="/usr/i586-mingw32msvc/bin/readelf"
      #    STRIP="/usr/i586-mingw32msvc/bin/strip"
      #    mingw="yes"
      #    mingflavor="w32"
      # fi
      #
      # Fix for the gentoo distribution of mingw.  Gentoo as of 2012 sets
      # some these up as links to files in /usr/libexec/gcc/i686-pc-mingw32, as
      # in i686-pc-mingw32-ar -> /usr/libexec/gcc/i686-pc-mingw32/ar.
      # 
      # In both platforms, other executables may be available,
      # but we don't set them up here right now:
      #
      #     gprof gcov gfortran cpp pkg-config fix-root emerge windres
      #     windmc strings size readelf ld.bfd gprof
      #     elfedit c++filt addr2line
      #
      # AR="/opt/xmingw/bin/i386-mingw32msvc-ar"

      if test -x "/usr/i686-w64-mingw32/bin/ar" ; then
         AR="/usr/i686-w64-mingw32/bin/ar"
         AS="/usr/i686-w64-mingw32/bin/as"
         DLLTOOL="/usr/i686-w64-mingw32/bin/dlltool"
         LD="/usr/i686-w64-mingw32/bin/ld"
         LDBFD="/usr/i686-w64-mingw32/bin/ld.bfd"
         NM="/usr/i686-w64-mingw32/bin/nm"
         OBJCOPY="/usr/i686-w64-mingw32/bin/objcopy"
         OBJDUMP="/usr/i686-w64-mingw32/bin/objdump"
         RANLIB="/usr/i686-w64-mingw32/bin/ranlib"
         READELF="/usr/i686-w64-mingw32/bin/readelf"
         STRIP="/usr/i686-w64-mingw32/bin/strip"
         mingflavor="w32"
      fi

      if test -x "/usr/x86_64-w64-mingw32/bin/ar" ; then
         AR="/usr/x86_64-w64-mingw32/bin/ar"
         AS="/usr/x86_64-w64-mingw32/bin/as"
         DLLTOOL="/usr/x86_64-w64-mingw32/bin/dlltool"
         LD="/usr/x86_64-w64-mingw32/bin/ld"
         LDBFD="/usr/x86_64-w64-mingw32/bin/ld.bfd"
         NM="/usr/x86_64-w64-mingw32/bin/nm"
         OBJCOPY="/usr/x86_64-w64-mingw32/bin/objcopy"
         OBJDUMP="/usr/x86_64-w64-mingw32/bin/objdump"
         RANLIB="/usr/x86_64-w64-mingw32/bin/ranlib"
         READELF="/usr/x86_64-w64-mingw32/bin/readelf"
         STRIP="/usr/x86_64-w64-mingw32/bin/strip"
         mingflavor="w64"
      fi

dnl What about mingflavor?

   fi

   if test "x$mingw" = "xno" ; then
      AC_MSG_RESULT(no)
   else
      if test "x$mingflavor" != "xnone" ; then
         AC_MSG_RESULT(yes)
      else
         AC_MSG_RESULT(no)
         AC_MSG_ERROR(MingW support not found for --enable-mingw)
      fi
   fi

   AC_SUBST(AR)

   dnl Checks for system services

   if test "x${CYGWIN}" = "xyes" ; then
      AC_DEFINE([CYGWIN], [1], [Define on cygwin])
      AC_MSG_RESULT(cygwin)
   else
      if test "x${MINGW}" = "xyes" ; then
         AC_DEFINE([MINGW], [1], [Define on Mingw])
         WIN32=1
         export WIN32
         AC_DEFINE([WIN32], [1], [Define on windows])
         LIBS="$LIBS -lws2_32 -lgdi32"
         AC_MSG_RESULT([mingw])
      else
         LINUX=1
         export LINUX
         AC_DEFINE([LINUX], [1], [Define if not on cygwin or mingw])
         AC_MSG_RESULT()
      fi
   fi
])

dnl xpc_mingw.m4
dnl
dnl vim: ts=3 sw=3 et ft=config
