#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_qt5 library
# \author     	Chris Ahlstrom
# \date       	2017-09-06
# \update      2017-09-11
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq_qt5 C/C++
# 		library.
#
#------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2 # subdir-objects
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)

#******************************************************************************
# CLEANFILES
#------------------------------------------------------------------------------
#
#  Do not forget to clean the moc/uic generated files.
#
#------------------------------------------------------------------------------

CLEANFILES = *.gc* *.moc.cpp ../forms/*.ui.h
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

CXXFLAGS = @QT_CXXFLAGS@
QT_MOC = @QT_MOC@
QT_UIC = @QT_UIC@

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
# Qt 5 results directories (avoid polluting the src directory)
#-----------------------------------------------------------------------------
#
#  The UI forms are stored in the "forms" directory, while the generated files
#  are stored in the "qt" directory, for both moc'ing and uic'ing.
#
#-----------------------------------------------------------------------------

formsdir = $(top_srcdir)/seq_qt5/forms
hppdir = ../include

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
git_branch = $(shell git branch | grep -e ^*)
git_info = "$(git_version) $(git_branch)"

#******************************************************************************
# Compiler and linker flags (later, PortMidi)
#
#	Unfortunately, we need to add the platform-specific include directories
#	because we include the perform module in some modules, and it includes
#	the platform-specific stuff.
#
# Notes:
#
#     -  None of ALSA, JACK, or LASH CFLAGS seem to be defined.
#     -  We include "-I.." and require that the Qt5-generated modules be
#        included as:   "#include "qt/generated_module.h".
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I.. \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/libseq64/include \
 -I$(top_srcdir)/seq_alsamidi/include \
 -I$(top_srcdir)/seq_portmidi/include \
 -I$(top_srcdir)/seq_rtmidi/include \
 -I$(top_srcdir)/resources \
 $(ALSA_CFLAGS) \
 $(QT_CXXFLAGS) \
 $(JACK_CFLAGS) \
 $(LASH_CFLAGS) \
 -DSEQ64_GIT_VERSION=\"$(git_info)\"

#******************************************************************************
# Qt 5 rules
#------------------------------------------------------------------------------

SUFFIXES = .hpp .cpp .moc.cpp .ui .ui.h

# The moc.cpp files are normally deposited in the include directory!

%.moc.cpp: %.hpp
	$(QT_MOC) $< -o ../src/$(notdir $@)

# The ui.h files are deposited in the forms directory.

%.ui.h: %.ui
	$(QT_UIC) $< -o $@

#******************************************************************************
# The library to build, a libtool-based library
#------------------------------------------------------------------------------

lib_LTLIBRARIES = libseq_qt5.la

#******************************************************************************
# Source files
#----------------------------------------------------------------------------
#
#  We don't want to put Qt5-generated files into the "include" or "src"
#  directory; we want them in the "qt5" directory.
#
# moc_sources = $(moc_qt_headers:.hpp=.moc.cpp)
#
#----------------------------------------------------------------------------

moc_qt_headers = \
 $(hppdir)/qseditoptions.hpp \
 $(hppdir)/qperfeditframe.hpp \
 $(hppdir)/qsabout.hpp \
 $(hppdir)/qseqeditframe.hpp \
 $(hppdir)/qsliveframe.hpp \
 $(hppdir)/qsmainwnd.hpp

# $(hppdir)/qseqmainwnd.hpp
# $(mocdir)/qseqmaindwnd.moc.cpp
# $(formsdir)/qseqmainwnd.ui
#	qseqmainwnd.cpp

moc_sources = $(moc_qt_headers:.hpp=.moc.cpp)

uic_qt_forms = \
 $(formsdir)/qseditoptions.ui \
 $(formsdir)/qperfeditframe.ui \
 $(formsdir)/qsabout.ui \
 $(formsdir)/qseqeditframe.ui \
 $(formsdir)/qsliveframe.ui \
 $(formsdir)/qsmainwnd.ui

uic_sources = $(uic_qt_forms:.ui=.ui.h)

BUILT_SOURCES = $(uic_sources) $(moc_sources)

libseq_qt5_la_SOURCES = \
 qperfeditframe.cpp \
 qperfnames.cpp \
 qperfroll.cpp \
 qperftime.cpp \
 qsabout.cpp \
 qseditoptions.cpp \
 qseqdata.cpp \
 qseqeditframe.cpp \
 qseqkeys.cpp \
 qseqroll.cpp \
 qseqstyle.cpp \
 qseqtime.cpp \
 qsliveframe.cpp \
 qsmaintime.cpp \
 qsmainwnd.cpp \
 qstriggereditor.cpp \
 $(moc_sources)

libseq_qt5_la_LDFLAGS = -version-info $(version)
libseq_qt5_la_LIBADD = $(QT_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(LASH_LIBS)

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------
#
#     We'd like to remove /usr/local/include/libseq_qt5-1.0 if it is
#     empty.  However, we don't have a good way to do it yet.
#
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(libdir) manually"

#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
