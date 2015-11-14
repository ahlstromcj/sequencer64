#******************************************************************************
# Makefile.am (libseq64)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq64 library
# \author     	Chris Ahlstrom
# \date       	2015-09-11
# \update      2015-11-14
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the sequencer64 C/C++
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

pkginclude_HEADERS = \
	app_limits.h \
	calculations.hpp \
	configfile.hpp \
	click.hpp \
	controllers.hpp \
	easy_macros.h \
	event.hpp \
	event_list.hpp \
	globals.h \
   gui_assistant.hpp \
   gui_play_base.hpp \
   jack_assistant.hpp \
   keys_perform.hpp \
	keystroke.hpp \
	lash.hpp \
	mastermidibus.hpp \
	midibus_common.hpp \
	midibus.hpp \
	midifile.hpp \
   midi_container.hpp \
   midi_list.hpp \
   midi_vector.hpp \
	mutex.hpp \
	optionsfile.hpp \
	perform.hpp \
	platform_macros.h \
	rc_settings.hpp \
   scales.h \
	sequence.hpp \
   triggers.hpp \
	userfile.hpp \
   user_instrument.hpp \
   user_midi_bus.hpp \
   user_settings.hpp 

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
