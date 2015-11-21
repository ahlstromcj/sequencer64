#******************************************************************************
# Makefile.am (libseq64)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq64 library
# \author     	Chris Ahlstrom
# \date       	2015-09-11
# \update      2015-11-20
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq64 C/C++
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

EXTRA_DIST =

#******************************************************************************
# Items from configure.ac
#-------------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@

SEQUENCER64_API_MAJOR = @SEQUENCER64_API_MAJOR@
SEQUENCER64_API_MINOR = @SEQUENCER64_API_MINOR@
SEQUENCER64_API_PATCH = @SEQUENCER64_API_PATCH@
SEQUENCER64_API_VERSION = @SEQUENCER64_API_VERSION@

SEQUENCER64_LT_CURRENT = @SEQUENCER64_LT_CURRENT@
SEQUENCER64_LT_REVISION = @SEQUENCER64_LT_REVISION@
SEQUENCER64_LT_AGE = @SEQUENCER64_LT_AGE@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------

prefix = @prefix@
sequencer64includedir = @sequencer64includedir@
sequencer64libdir = @sequencer64libdir@

#******************************************************************************
# localedir
#------------------------------------------------------------------------------
#
# 	'localedir' is the normal system directory for installed localization
#  files.
#
#------------------------------------------------------------------------------

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

#*****************************************************************************
# libtool
#-----------------------------------------------------------------------------

version = $(SEQUENCER64_API_MAJOR):$(SEQUENCER64_API_MINOR):$(SEQUENCER64_API_PATCH)

#******************************************************************************
# Compiler and linker flags
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/seq_gtkmm2/include \
 $(GTKMM_CFLAGS) \
 $(JACK_CFLAGS) \
 $(LASH_CFLAGS) \
 -Wall $(MM_WFLAGS)

#******************************************************************************
# The library to build, a libtool-based library
#------------------------------------------------------------------------------

lib_LTLIBRARIES = libseq64.la

#******************************************************************************
# Source files
#----------------------------------------------------------------------------

libseq64_la_SOURCES = \
	calculations.cpp \
	cmdlineopts.cpp \
	configfile.cpp \
	click.cpp \
	easy_macros.cpp \
	event.cpp \
	event_list.cpp \
	file_functions.cpp \
	globals.cpp \
   gui_assistant.cpp \
   gui_play_base.cpp \
   jack_assistant.cpp \
   keys_perform.cpp \
	keystroke.cpp \
	lash.cpp \
	mastermidibus.cpp \
   midibus.cpp \
   midifile.cpp \
   midi_container.cpp \
   midi_list.cpp \
   midi_vector.cpp \
	mutex.cpp \
	optionsfile.cpp \
   perform.cpp \
	rc_settings.cpp \
	sequence.cpp \
	triggers.cpp \
	user_instrument.cpp \
	user_midi_bus.cpp \
	user_settings.cpp \
	userfile.cpp

libseq64_la_LDFLAGS = -version-info $(version)
libseq64_la_LIBADD = $(GTKMM_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(LASH_LIBS)

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------
#
#     We'd like to remove /usr/local/include/libseq64-1.0 if it is
#     empty.  However, we don't have a good way to do it yet.
#
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(libdir) manually"

#******************************************************************************
# Makefile.am (libseq64)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
