#******************************************************************************
# Makefile.am (libseq64)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq64 library
# \author     	Chris Ahlstrom
# \date       	2015-09-11
# \update      2018-04-12
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
CLEANFILES = *.gc*
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
# Compiler and linker flags
#------------------------------------------------------------------------------
#
#  $(GTKMM_CFLAGS)
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/seq_gtkmm2/include \
 $(JACK_CFLAGS) \
 $(LASH_CFLAGS) \
 -DSEQ64_GIT_VERSION=\"$(git_info)\" \
 -Wall $(MM_WFLAGS)

#******************************************************************************
# Source files
#
#  These files have been moved to the new seq_alsamidi library:
#
#     mastermidibus.hpp
#     midibus_common.hpp
#     midibus.hpp
#
#----------------------------------------------------------------------------

pkginclude_HEADERS = \
	app_limits.h \
   businfo.hpp \
	calculations.hpp \
	click.hpp \
	cmdlineopts.hpp \
	configfile.hpp \
	controllers.hpp \
   daemonize.hpp \
	easy_macros.h \
	easy_macros.hpp \
	editable_event.hpp \
	editable_events.hpp \
	event.hpp \
	event_list.hpp \
	file_functions.hpp \
   gdk_basic_keys.h \
	globals.h \
   gui_assistant.hpp \
   jack_assistant.hpp \
   keys_perform.hpp \
	keystroke.hpp \
	lash.hpp \
   mastermidibase.hpp \
   midibase.hpp \
	midibus_common.hpp \
   mastermidibus.hpp \
   midibus.hpp \
	midibyte.hpp \
	midifile.hpp \
   midi_container.hpp \
   midi_control.hpp \
   midi_list.hpp \
   midi_splitter.hpp \
   midi_vector.hpp \
	mutex.hpp \
	optionsfile.hpp \
   palette.hpp \
	perform.hpp \
	platform_macros.h \
	rc_settings.hpp \
   recent.hpp \
   rect.hpp \
   scales.h \
   seq64_features.h \
	sequence.hpp \
	settings.hpp \
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
