#******************************************************************************
# Makefile.am (libseq_gtkmm2)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_gtkmm2 library
# \author     	Chris Ahlstrom
# \date       	2015-09-11
# \update      2020-04-18
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
GIT_VERSION = @GIT_VERSION@

SEQ64_API_MAJOR = @SEQ64_API_MAJOR@
SEQ64_API_MINOR = @SEQ64_API_MINOR@
SEQ64_API_PATCH = @SEQ64_API_PATCH@
SEQ64_API_VERSION = @SEQ64_API_VERSION@

SEQ64_LT_CURRENT = @SEQ64_LT_CURRENT@
SEQ64_LT_REVISION = @SEQ64_LT_REVISION@
SEQ64_LT_AGE = @SEQ64_LT_AGE@

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

version = $(SEQ64_API_MAJOR):$(SEQ64_API_MINOR):$(SEQ64_API_PATCH)

#*****************************************************************************
# git_version
#-----------------------------------------------------------------------------
# git_version = $(shell git describe --abbrev=7 --always --tags)
#-----------------------------------------------------------------------------

git_version = $(shell git describe --tags --long)
git_branch =$(shell git branch | grep -e ^*)
git_info = "$(git_version) $(git_branch)"

#******************************************************************************
# Compiler and linker flags (later, PortMidi)
#
#	Unfortunately, we need to add the platform-specific include directories
#	because we include the perform module in some modules, and it includes
#	the platform-specific stuff.
#
#	Also, we're getting warning from libsigc++ now, so we're adding this flag for
#	now: -Wno-cast-function-type
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/libseq64/include \
 -I$(top_srcdir)/seq_alsamidi/include \
 -I$(top_srcdir)/seq_portmidi/include \
 -I$(top_srcdir)/seq_rtmidi/include \
 -I$(top_srcdir)/resources \
 $(ALSA_CFLAGS) \
 $(GTKMM_CFLAGS) \
 $(JACK_CFLAGS) \
 $(LASH_CFLAGS) \
 -Wno-cast-function-type \
 -DSEQ64_GIT_VERSION=\"$(git_info)\"

#******************************************************************************
# The library to build, a libtool-based library
#------------------------------------------------------------------------------

lib_LTLIBRARIES = libseq_gtkmm2.la

#******************************************************************************
# Source files
#----------------------------------------------------------------------------
#
# seq24seqroll.cpp now obsolete, removed from the build.
# seq24seq.cpp, ditto
#
#----------------------------------------------------------------------------

libseq_gtkmm2_la_SOURCES = \
   eventedit.cpp \
   eventslots.cpp \
	font.cpp \
	fruityperfroll_input.cpp \
   fruityseq.cpp \
   fruityseqroll.cpp \
   gui_assistant_gtk2.cpp \
   gui_drawingarea_gtk2.cpp \
   gui_key_tests.cpp \
   gui_palette_gtk2.cpp \
   gui_window_gtk2.cpp \
	keybindentry.cpp \
   keys_perform_gtk2.cpp \
   lfownd.cpp \
	maintime.cpp \
	mainwid.cpp \
	mainwnd.cpp \
	options.cpp \
	perfedit.cpp \
	perfnames.cpp \
	perfroll.cpp \
	perfroll_input.cpp \
	perftime.cpp \
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
