dnl Version: MPL 1.1 / GPLv3+ / LGPLv3+
dnl
dnl The contents of this file are subject to the Mozilla Public License Version
dnl 1.1 (the "License"); you may not use this file except in compliance with
dnl the License or as specified alternatively below. You may obtain a copy of
dnl the License at http://www.mozilla.org/MPL/
dnl
dnl Software distributed under the License is distributed on an "AS IS" basis,
dnl WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
dnl for the specific language governing rights and limitations under the
dnl License.
dnl
dnl Major Contributor(s):
dnl Copyright (C) 2012 Red Hat, Inc., David Tardon <dtardon@redhat.com>
dnl (initial developer)
dnl
dnl All Rights Reserved.
dnl
dnl For minor contributions see the git repository.
dnl
dnl Alternatively, the contents of this file may be used under the terms of
dnl either the GNU General Public License Version 3 or later (the "GPLv3+"), or
dnl the GNU Lesser General Public License Version 3 or later (the "LGPLv3+"),
dnl in which case the provisions of the GPLv3+ or the LGPLv3+ are applicable
dnl instead of those above.
dnl
dnl SEQ64_MINGW_CHECK_DLL
dnl (
dnl   variable-infix, dll-name-stem, [action-if-found], [action-if-not-found]
dnl )
dnl
dnl Checks for presence of dll dll-name-stem . Sets variable
dnl MINGW_variable-infix_DLL if found, issues an error otherwise.
dnl
dnl It recognizes these dll patterns (x, y match any character, but they
dnl are supposed to be numerals):
dnl
dnl * name-x.dll
dnl * name-xy.dll
dnl * name-x.y.dll
dnl * name.dll
dnl
dnl Example:
dnl
dnl SEQ64_MINGW_CHECK_DLL([EXPAT], [libexpat])
dnl might result in MINGW_EXPAT_DLL=libexpat-1.dll being set.
dnl
dnl uses CC, WITH_MINGW
dnl
dnl Tweaked by Chris Ahlstrom for use in the Sequencer64 project
dnl on 2017-08-30.  Changed libo to SEQ64.
dnl
dnl -------------------------------------------------------------------------

AC_DEFUN([SEQ64_MINGW_CHECK_DLL],
[AC_ARG_VAR([MINGW_][$1][_DLL],[output variable containing the found dll name])dnl

if test -n "$WITH_MINGW"; then
    dnl TODO move this to configure: there is no need to call $CC more than once
    _seq64_mingw_dlldir=`$CC -print-sysroot`/mingw/bin
    _seq64_mingw_dllname=
    AC_MSG_CHECKING([for $2 dll])

    dnl try one- or two-numbered version
    _seq64_mingw_try_dll([$2][-?.dll])
    if test "$_seq64_mingw_dllname" = ""; then
        _seq64_mingw_try_dll([$2][-??.dll])
    fi
    dnl maybe the version contains a dot (e.g., libdb)
    if test "$_seq64_mingw_dllname" = ""; then
        _seq64_mingw_try_dll([$2][-?.?.dll])
    fi
    dnl maybe it is not versioned
    if test "$_seq64_mingw_dllname" = ""; then
        _seq64_mingw_try_dll([$2][.dll])
    fi

    if test "$_seq64_mingw_dllname" = ""; then
        AC_MSG_RESULT([no])
        m4_default([$4],[AC_MSG_ERROR([no dll found for $2])])
    else
        AC_MSG_RESULT([$_seq64_mingw_dllname])
        [MINGW_][$1][_DLL]="$_seq64_mingw_dllname"
        m4_default([$3],[])
    fi
fi[]dnl
]) # seq64_MINGW_CHECK_DLL

# SEQ64_MINGW_TRY_DLL(variable-infix,dll-name-stem)
#
# Checks for presence of dll dll-name-stem . Sets variable
# MINGW_variable-infix_DLL if found, does nothing otherwise.
#
# See SEQ64_MINGW_CHECK_DLL for further info.
#
# uses CC, WITH_MINGW
# ------------------------------------------------
#
AC_DEFUN([SEQ64_MINGW_TRY_DLL],
[dnl shortcut: do not test for already found dlls
if test -z "$[MINGW_][$1][_DLL]"; then
    SEQ64_MINGW_CHECK_DLL([$1],[$2],[[]],[[]])
fi[]dnl
]) # SEQ64_MINGW_TRY_DLL

# _seq64_mingw_try_dll(dll-name,dll-dir)
m4_define([_seq64_mingw_try_dll],
[_seq64_mingw_trying_dll=`ls "[$_seq64_mingw_dlldir]"/[$1] 2>/dev/null`
if test -f "$_seq64_mingw_trying_dll"; then
    _seq64_mingw_dllname=`basename "$_seq64_mingw_trying_dll"`
fi[]dnl
]) # _seq64_mingw_try_dll

dnl vim:set shiftwidth=4 softtabstop=4 expandtab:
