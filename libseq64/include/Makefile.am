#******************************************************************************
# Makefile.am (libseq64)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq64 library
# \author     	Chris Ahlstrom
# \date       	2015-09-11
# \update      2015-09-20
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the sequencer26 C/C++
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

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------

prefix = @prefix@
sequencer64includedir = @sequencer64includedir@
sequencer64libdir = @sequencer64libdir@

#******************************************************************************
# Source files
#----------------------------------------------------------------------------
#
#  Requires Gtk support via the perform module:
#
#     lash.hpp
#     optionsfile.hpp
#
#----------------------------------------------------------------------------

pkginclude_HEADERS = \
	configfile.hpp \
	controllers.hpp \
	easy_macros.h \
	event.hpp \
	event_list.hpp \
	globals.h \
   gui_assistant.hpp \
   jack_assistant.hpp \
   keys_perform.hpp \
	mastermidibus.hpp \
	midibus_common.hpp \
	midibus.hpp \
	midifile.hpp \
	mutex.hpp \
	perform.hpp \
	platform_macros.h \
	sequence.hpp \
	userfile.hpp

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(pkgincludedir) manually"

#******************************************************************************
# Makefile.am (libseq64)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
