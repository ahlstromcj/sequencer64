#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_qt5 library
# \author     	Chris Ahlstrom
# \date       	2017-09-06
# \update      2018-07-18
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq_qt5 C++
# 		library.
#
#------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)

#******************************************************************************
# CLEANFILES
#------------------------------------------------------------------------------

CLEANFILES = *.gc* *.moc.cpp
MOSTLYCLEANFILES = *~

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------

EXTRA_DIST = *.h *.hpp

#******************************************************************************
# Items from configure.ac
#-------------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@
GIT_VERSION = @GIT_VERSION@

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------

prefix = @prefix@
includedir = @sequencer64includedir@
libdir = @sequencer64libdir@
datadir = @datadir@
datarootdir = @datarootdir@
sequencer64includedir = @sequencer64includedir@
sequencer64libdir = @sequencer64libdir@

#******************************************************************************
# Source files
#----------------------------------------------------------------------------

pkginclude_HEADERS = \
 Globals.hpp \
 gui_assistant_qt5.hpp \
 gui_palette_qt5.hpp \
 keys_perform_qt5.hpp \
 qclocklayout.hpp \
 qinputcheckbox.hpp \
 qperfbase.hpp \
 qperfeditframe64.hpp \
 qperfeditframe.hpp \
 qperfnames.hpp \
 qperfroll.hpp \
 qperftime.hpp \
 qsabout.hpp \
 qsbuildinfo.hpp \
 qscrollmaster.h \
 qseditoptions.hpp \
 qseqbase.hpp \
 qseqdata.hpp \
 qseqeditex.hpp \
 qseqeditframe64.hpp \
 qseqeditframe.hpp \
 qseqkeys.hpp \
 qseqroll.hpp \
 qseqstyle.hpp \
 qseqtime.hpp \
 qskeymaps.hpp \
 qsliveframe.hpp \
 qsmacros.hpp \
 qsmaintime.hpp \
 qsmainwnd.hpp \
 qstriggereditor.hpp \
 qt5_helpers.hpp

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(pkgincludedir) manually"

#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
