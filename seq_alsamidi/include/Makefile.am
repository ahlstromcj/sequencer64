#******************************************************************************
# Makefile.am (libseq_alsamidi)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_alsamidi library
# \author     	Chris Ahlstrom
# \date       	2016-11-20
# \update      2016-11-28
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq_alsamidi C/C++
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

CLEANFILES = *.gc*
MOSTLYCLEANFILES = *~

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------

EXTRA_DIST = *.h

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
	mastermidibus_rm.hpp \
	midibus_rm.hpp

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(pkgincludedir) manually"

#******************************************************************************
# Makefile.am (libseq_alsamidi)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
