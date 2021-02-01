#******************************************************************************
# Makefile.am (seq64qt5)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	seq64qt5 application
# \author     	Chris Ahlstrom
# \date       	2017-09-05
# \update      2021-01-29
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the seq64qt5 C/C++
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
#  EXTRA_DIST
#------------------------------------------------------------------------------
#
#  getopt_test.c is not ready and is not included at this time.
#	$(TESTS) is not included because it is derived files.
#
#------------------------------------------------------------------------------

# EXTRA_DIST = dl_leaks.supp make-tests README

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
libseq_qt5dir = $(builddir)/seq_qt5/src/.libs

#******************************************************************************
# AM_CPPFLAGS [formerly "INCLUDES"]
#------------------------------------------------------------------------------
#
# 	'AM_CPPFLAGS' is the set of directories needed to access all of the
# 	library header files used in this project.
#
# 	Note that we include both the portmidi and rtmidi include directories.
# 	There are no conflicts in name of header files, and we don't want to
# 	try to test SEQ64_RTMIDI_SUPPORT and SEQ64_PORTMIDI_SUPPORT until we have
# 	to.
#
#   -I$(top_srcdir)/seq_portmidi/include
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I$(top_srcdir)/libseq64/include \
 -I$(top_srcdir)/seq_rtmidi/include \
 -I$(top_srcdir)/seq_qt5/include \
 $(QT_CXXFLAGS) \
 $(JACK_CFLAGS) \
 $(LASH_CFLAGS)

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
#	These files are the ones built in the source tree, not the installed
#	ones.
#
#  Sometimes one has to change the order of the libraries in this list.
#
# $(libmath)
#
#----------------------------------------------------------------------------

libraries = \
 -L$(libseq_qt5dir) -lseq_qt5 \
 -L$(libseq64dir) -lseq64 \
 -L$(libseq_rtmididir) -lseq_rtmidi

#****************************************************************************
# Project-specific dependency files
#----------------------------------------------------------------------------
#
#  Provides the specific list of dependencies, to assure that the make
#  detects all changes, if they are available.
#
#----------------------------------------------------------------------------

dependencies = \
 $(libseq_rtmididir)/libseq_rtmidi.la \
 $(libseq_qt5dir)/libseq_qt5.la \
 $(libseq64dir)/libseq64.la

#******************************************************************************
# The programs to build
#------------------------------------------------------------------------------

bin_PROGRAMS = qseq64

#******************************************************************************
# seq64
#----------------------------------------------------------------------------
#
#  Added a LDFLAGS line to avoid a "DSO missing" error on some systems when
#  building in debug mode:
#
#     As a workaround it's possible to switch back to the more permissive
#     view of what symbols are available by using the option
#
#        -Wl,--copy-dt-needed-entries
#
#  See this article:
#
#  https://flameeyes.blog/2010/11/26/
#     it-s-not-all-gold-that-shines-why-underlinking-is-a-bad-thing/
#
#----------------------------------------------------------------------------

qseq64_SOURCES = seq64qt5.cpp
qseq66_LDFLAGS = -Wl,--copy-dt-needed-entries
qseq64_DEPENDENCIES = $(dependencies)
qseq64_LDADD = $(libraries) $(QT_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(LASH_LIBS) $(AM_LDFLAGS)

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
# Makefile.am (seq64qt5)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
