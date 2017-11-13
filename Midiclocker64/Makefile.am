#******************************************************************************
# Makefile.am (midiclocker64)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	midiclocker64 application
# \author     	Chris Ahlstrom
# \date       	2017-11-11
# \update      2017-11-11
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the midiclocker64 C/C++
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
libseq_portmididir = $(builddir)/seq_portmidi/src/.libs

#******************************************************************************
# AM_CPPFLAGS [formerly "INCLUDES"]
#------------------------------------------------------------------------------
#
# 	'AM_CPPFLAGS' is the set of directories needed to access all of the
# 	library header files used in this project.
#
#   -I$(top_srcdir)/seq_gtkmm2/include \
#
#------------------------------------------------------------------------------

if BUILD_WINDOWS
AM_CXXFLAGS = -I$(top_srcdir)/libseq64/include
else
AM_CXXFLAGS = -I$(top_srcdir)/libseq64/include $(JACK_CFLAGS)
endif

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

if BUILD_WINDOWS
libraries = -L$(libseq64dir) -lseq64 -lwinmm
else
libraries = -L$(libseq64dir) -lseq64 -L$(libseq_rtmididir) -lseq_rtmidi
endif

#****************************************************************************
# Project-specific dependency files
#----------------------------------------------------------------------------
#
#  Provdies the specific list of dependencies, to assure that the make
#  detects all changes, if they are available.
#
#  $(libseq_gtkmm2dir)/libseq_gtkmm2.la \
#----------------------------------------------------------------------------

if BUILD_WINDOWS
dependencies = $(libseq_portmididir)/libseq_portmidi.la \
 $(libseq64dir)/libseq64.la
else
dependencies = $(libseq_rtmididir)/libseq_rtmidi.la \
 $(libseq64dir)/libseq64.la
endif

#******************************************************************************
# The programs to build
#------------------------------------------------------------------------------

bin_PROGRAMS = midiclocker64

#******************************************************************************
# midiclocker64
#----------------------------------------------------------------------------

midiclocker64_SOURCES = midiclocker64.cpp midi_clocker.cpp midi_clocker.hpp
midiclocker64_DEPENDENCIES = $(dependencies)

if BUILD_WINDOWS
midiclocker64_LDADD = $(libraries) $(AM_LDFLAGS) $(PTHREAD_LIBS)
else
midiclocker64_LDADD = $(libraries) $(GTKMM_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(LASH_LIBS) $(AM_LDFLAGS)
endif

#******************************************************************************
#  distclean
#------------------------------------------------------------------------------

distclean-local:
	-rm -rf $(testsubdir)

#******************************************************************************
# Makefile.am (midiclocker64)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
