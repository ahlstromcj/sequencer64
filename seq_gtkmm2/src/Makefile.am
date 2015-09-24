#******************************************************************************
# Makefile.am (libseq_gtkmm2)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_gtkmm2 library
# \author     	Chris Ahlstrom
# \date       	2015-09-11
# \update      2015-09-24
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq_gtkmm2 C/C++
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
# Compiler and linker flags (later, PortMidi)
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/libseq64/include \
 -I$(top_srcdir)/resources \
 $(GTKMM_CFLAGS) \
 $(JACK_CFLAGS) \
 $(LASH_CFLAGS) \
 -Wall $(MM_WFLAGS)

#******************************************************************************
# The library to build, a libtool-based library
#------------------------------------------------------------------------------

lib_LTLIBRARIES = libseq_gtkmm2.la

#******************************************************************************
# Source files
#----------------------------------------------------------------------------
#
#  Requires Gtk support via the perform module:
#
#     lash.cpp
#
#  Moved to libseq64:
#
#     midifile.cpp
#     optionsfile.cpp
#     perform.cpp
#
#----------------------------------------------------------------------------

libseq_gtkmm2_la_SOURCES = \
	lash.cpp \
	font.cpp \
   fruityseq.cpp \
   fruityseqroll.cpp \
   gui_assistant_gtk2.cpp \
   gui_drawingarea_gtk2.cpp \
   gui_palette_gtk2.cpp \
   gui_window_gtk2.cpp \
	keybindentry.cpp \
   keys_perform_gtk2.cpp \
	maintime.cpp \
	mainwid.cpp \
	mainwnd.cpp \
	options.cpp \
	perfedit.cpp \
	perfnames.cpp \
	perfroll.cpp \
	perfroll_input.cpp \
	perftime.cpp \
	seq24seq.cpp \
	seq24seqroll.cpp \
	seqdata.cpp \
	seqedit.cpp \
	seqevent.cpp \
	seqkeys.cpp \
	seqmenu.cpp \
	seqroll.cpp \
	seqtime.cpp

libseq_gtkmm2_la_LDFLAGS = -version-info $(version)
libseq_gtkmm2_la_LIBADD = $(GTKMM_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(LASH_LIBS)

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------
#
#     We'd like to remove /usr/local/include/libseq_gtkmm2-1.0 if it is
#     empty.  However, we don't have a good way to do it yet.
#
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(libdir) manually"

#******************************************************************************
# Makefile.am (libseq_gtkmm2)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
