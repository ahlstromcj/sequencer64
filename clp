#!/bin/bash
#
#******************************************************************************
# clp (Sequencer64)
#------------------------------------------------------------------------------
##
# \file       	clp
# \library    	Sequencer64
# \author     	Chris Ahlstrom
# \date       	2018-09-10
# \update     	2018-09-10
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
#     The above is modified by the following to remove even the mild GPL
#     restrictions: Use this script in any manner whatsoever.  You don't
#     even need to give me any credit.  Keep in mind the value of the GPL in
#     keeping software and its descendant modifications available to the
#     community for all time.  runs the configure script by default.
#
#     This file merely overwrites any modified files by checking them out,
#     and moves temporary tar files to ~/tmp.  Short for "clean project", not
#     to be confused with bootstrap --full-clean.
#
#------------------------------------------------------------------------------

git checkout $(git diff --name-only)
mv *.xz ~/tmp

#******************************************************************************
# bootstrap (Sequencer64)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 wm=4 et ft=sh
#------------------------------------------------------------------------------
