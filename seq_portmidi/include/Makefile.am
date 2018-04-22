#******************************************************************************
# Makefile.am (libseq_portmidi)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_portmidi library
# \author     	Chris Ahlstrom
# \date       	2015-09-11
# \update      2018-04-21
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq_portmidi C/C++
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
#
# pmwinmm.h
#
#----------------------------------------------------------------------------

if BUILD_WINDOWS
pkginclude_HEADERS = \
	mastermidibus_pm.hpp \
	midibus_pm.hpp \
	pminternal.h \
	pmutil.h \
	pmerrmm.h \
	pmwinmm.h \
	portmidi.h \
	porttime.h
else
pkginclude_HEADERS = \
	mastermidibus_pm.hpp \
	midibus_pm.hpp \
	pminternal.h \
	pmlinuxalsa.h \
	pmlinux.h \
	pmutil.h \
	portmidi.h \
	porttime.h
endif

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(pkgincludedir) manually"

#******************************************************************************
# Makefile.am (libseq_portmidi)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
