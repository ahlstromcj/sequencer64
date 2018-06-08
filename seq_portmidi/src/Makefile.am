#******************************************************************************
# Makefile.am (libseq_portmidi)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_portmidi library
# \author     	Chris Ahlstrom
# \date       	2015-09-11
# \update      2018-05-24
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq_portmidi
# 		C/C++ library.
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

sequencer64docdir = @sequencer64docdir@
sequencer64includedir = @sequencer64includedir@
sequencer64libdir = @sequencer64libdir@

prefix = @prefix@
libdir = @sequencer64libdir@
datadir = @datadir@

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

#******************************************************************************
# aclocal support
#------------------------------------------------------------------------------

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

#*****************************************************************************
# libtool
#-----------------------------------------------------------------------------

version = $(SEQ64_API_MAJOR):$(SEQ64_API_MINOR):$(SEQ64_API_PATCH)

#******************************************************************************
# Compiler and linker flags (later, PortMidi)
#------------------------------------------------------------------------------

AM_CFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/libseq64/include

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/libseq64/include

#******************************************************************************
# The library to build, a libtool-based library
#------------------------------------------------------------------------------

lib_LTLIBRARIES = libseq_portmidi.la

#******************************************************************************
# Source files
#----------------------------------------------------------------------------
#
#	We don't use the finddefault*.c modules because Sequencer64 already has a
#	more tractable way to store configuration than in Java "prefs".  We've added
#	some functions to porttime.c, so that is now included.
#
#	   finddefault.c
#
#----------------------------------------------------------------------------

if BUILD_WINDOWS

libseq_portmidi_la_SOURCES = \
	mastermidibus.cpp \
	midibus.cpp \
	pmerrmm.c \
	pmwin.c \
	pmwinmm.c \
	pmutil.c \
	portmidi.c \
	porttime.c \
   ptwinmm.c

libseq_portmidi_la_LDFLAGS = -version-info $(version)
libseq_portmidi_la_LIBADD =

else

libseq_portmidi_la_SOURCES = \
	mastermidibus.cpp \
	midibus.cpp \
	pmlinuxalsa.c \
	pmlinux.c \
	pmutil.c \
	portmidi.c \
	porttime.c \
	ptlinux.c

# libseq_portmidi_la_LDFLAGS = -version-info $(version)

libseq_portmidi_la_LDFLAGS = -no-undefined -version-info $(version)
libseq_portmidi_la_LIBADD = $(ALSA_LIBS) $(JACK_LIBS) $(LASH_LIBS)

endif

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
# Makefile.am (libseq_portmidi)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
