#******************************************************************************
# Makefile.am (seq64rtmidi)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	seq64rtmidi application
# \author     	Chris Ahlstrom
# \date       	2016-12-03
# \update      2020-04-18
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the seq64rtmidi C/C++
# 		application.
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

#******************************************************************************
# Items from configure.ac
#-------------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------
#
# 	Not needed, yet, since we won't be installing the app for awhile.
#
#------------------------------------------------------------------------------

prefix = @prefix@
libdir = @xpclibdir@
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

libseq64dir = $(builddir)/libseq64/src/.libs
libseq_rtmididir = $(builddir)/seq_rtmidi/src/.libs
libseq_gtkmm2dir = $(builddir)/seq_gtkmm2/src/.libs

#******************************************************************************
# AM_CPPFLAGS [formerly "INCLUDES"]
#------------------------------------------------------------------------------
#
# 	'AM_CPPFLAGS' is the set of directories needed to access all of the
# 	library header files used in this project.
#
#	Also, we're getting warning from libsigc++ now, so we're adding this flag for
#	now: -Wno-cast-function-type
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I$(top_srcdir)/libseq64/include \
 -I$(top_srcdir)/seq_rtmidi/include \
 -I$(top_srcdir)/seq_gtkmm2/include \
 $(GTKMM_CFLAGS) \
 $(JACK_CFLAGS) \
 $(LASH_CFLAGS) \
 -Wno-cast-function-type

#******************************************************************************
# libmath
#------------------------------------------------------------------------------
#
# 		One day, we got errors about sqrt() undefined, which we fixed by
# 		adding -lm.  Then one day we got errors about various items in
# 		sys/stat.h being multiply-defined, and it turned out to be the -lm.
#
# 		We make it (an empty) define for how to handle it more easily.
#
#------------------------------------------------------------------------------

libmath = -lm

#****************************************************************************
# Project-specific library files
#----------------------------------------------------------------------------
#
#	These files are the ones built in the source tree, not the installed ones.
#	Sometimes one has to change the order of the libraries in this list.
#
#----------------------------------------------------------------------------

libraries = \
 -L$(libseq_gtkmm2dir) -lseq_gtkmm2 \
 -L$(libseq64dir) -lseq64 \
 -L$(libseq_rtmididir) -lseq_rtmidi

#****************************************************************************
# Project-specific dependency files
#----------------------------------------------------------------------------
#
#  Provdies the specific list of dependencies, to assure that the make
#  detects all changes, if they are available.
#
#----------------------------------------------------------------------------

dependencies = \
 $(libseq_rtmididir)/libseq_rtmidi.la \
 $(libseq_gtkmm2dir)/libseq_gtkmm2.la \
 $(libseq64dir)/libseq64.la

#******************************************************************************
# The programs to build
#------------------------------------------------------------------------------

bin_PROGRAMS = seq64

#******************************************************************************
# seq64
#----------------------------------------------------------------------------

seq64_SOURCES = seq64rtmidi.cpp
seq64_DEPENDENCIES = $(dependencies)
seq64_LDADD = $(libraries) $(GTKMM_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(LASH_LIBS) $(AM_LDFLAGS)

#******************************************************************************
# Testing
#------------------------------------------------------------------------------
#
# 	   http://www.gnu.org/software/hello/manual/automake/Simple-Tests.html
#
#     Not supported for this application.  We supply a bogus test script.
#
#------------------------------------------------------------------------------

.PHONY: test_script

TESTS = test_script
 
test: check

#******************************************************************************
#  distclean
#------------------------------------------------------------------------------

distclean-local:
	-rm -rf $(testsubdir)

#******************************************************************************
# Makefile.am (seq64rtmidi)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
