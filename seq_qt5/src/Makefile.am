#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_qt5 library
# \author     	Chris Ahlstrom
# \date       	2018-01-01
# \update      2018-05-30
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

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2 subdir-objects
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)

#******************************************************************************
# CLEANFILES
#------------------------------------------------------------------------------
#
#  Do not forget to clean the moc/uic/rcc generated files.
#
#------------------------------------------------------------------------------

CLEANFILES = *.gc* *.moc.cpp ../forms/*.ui.h qseq64.qrc.cpp
MOSTLYCLEANFILES = *~

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------

EXTRA_DIST = qseq64.qrc

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

QT_CXXFLAGS = @QT_CXXFLAGS@

QT_MOC = @QT_MOC@
QT_RCC = @QT_RCC@
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
# hppdir = ../include
#
#-----------------------------------------------------------------------------

thisdir = $(top_srcdir)/seq_qt5
formsdir = $(top_srcdir)/seq_qt5/forms
hppdir = $(top_srcdir)/seq_qt5/include
pixdir = $(top_srcdir)/resources/pixmaps

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
#------------------------------------------------------------------------------
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
# -I..
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I$(hppdir) \
 -I$(thisdir) \
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
# Moc flags
#------------------------------------------------------------------------------

MOC_CPPFLAGS = -I$(top_srcdir)/include

if BUILD_QTMIDI
   MOC_CPPFLAGS += -DQ_OS_MAC
endif

if BUILD_WINDOWS
   MOC_CPPFLAGS += -D_WIN32
endif

#******************************************************************************
# Qt 5 rules
#------------------------------------------------------------------------------
#
#     $@ is the name of the file being generated;
#     $< is the first prerequisite (the source file)
#
#------------------------------------------------------------------------------

SUFFIXES = .h .hpp .cpp .moc.cpp .qrc .qrc.cpp .ui .ui.h

# The moc.cpp files are normally deposited in the include directory!
#
#     $(QT_MOC) $(MOC_CPPFLAGS) -o ../include/$(notdir $@) $<

%.moc.cpp: %.hpp
	$(QT_MOC) $(MOC_CPPFLAGS) -o $@ $<

# The ui.h files are deposited in the forms directory.

%.ui.h: %.ui
	$(QT_UIC) -o $@ $<

# The qrc files hold resources needed that aren't code.

%.qrc.cpp: %.qrc
	$(QT_RCC) -o $@ $<

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
#  However....
#
#  Added qperfnames, qperfroll, qperftime, qseqdata, qseqkeys, qsmaintime,
#  qseqroll, qseqtime, qstriggereditor, due to vtable error for their
#  constructors or destructors.
#
#  Somehow, using the hppdir macro creates a '$(hppdir)' directory.... :-(
#
#----------------------------------------------------------------------------

moc_qt_headers = \
 ../include/qclocklayout.hpp \
 ../include/qinputcheckbox.hpp \
 ../include/qperfeditframe.hpp \
 ../include/qperfnames.hpp \
 ../include/qperfroll.hpp \
 ../include/qperftime.hpp \
 ../include/qsabout.hpp \
 ../include/qsbuildinfo.hpp \
 ../include/qseditoptions.hpp \
 ../include/qseqdata.hpp \
 ../include/qseqeditframe.hpp \
 ../include/qseqkeys.hpp \
 ../include/qseqroll.hpp \
 ../include/qseqtime.hpp \
 ../include/qsliveframe.hpp \
 ../include/qsmaintime.hpp \
 ../include/qsmainwnd.hpp \
 ../include/qstriggereditor.hpp

moc_sources = $(moc_qt_headers:.hpp=.moc.cpp)

uic_qt_forms = \
 $(formsdir)/qseditoptions.ui \
 $(formsdir)/qperfeditframe.ui \
 $(formsdir)/qsabout.ui \
 $(formsdir)/qsbuildinfo.ui \
 $(formsdir)/qseqeditframe.ui \
 $(formsdir)/qsliveframe.ui \
 $(formsdir)/qsmainwnd.ui

uic_sources = $(uic_qt_forms:.ui=.ui.h)

#****************************************************************************
# QRC file
#----------------------------------------------------------------------------
#
#  In Kepler34, this file held all of the XPM resources.  We have moved them
#  into the relevant source files, in the same style as the Gtkmm 2.4 version
#  of the application.  We remove qseq64.qrc.cpp as a dependency, though
#  we have left the code itself intact.
#
#     -- Justin Case
#
#----------------------------------------------------------------------------

qseq64.qrc.cpp: \
 qseq64.qrc \
 $(pixdir)/song-snap.xpm \
 $(pixdir)/panic.xpm \
 $(pixdir)/logo.xpm \
 $(pixdir)/drum.xpm \
 $(pixdir)/note_length.xpm \
 $(pixdir)/quantize.xpm \
 $(pixdir)/snap.xpm \
 $(pixdir)/play2.xpm \
 $(pixdir)/collapse.xpm \
 $(pixdir)/midi.xpm \
 $(pixdir)/length.xpm \
 $(pixdir)/loop.xpm \
 $(pixdir)/zoom.xpm \
 $(pixdir)/editor-rec.xpm \
 $(pixdir)/redo.xpm \
 $(pixdir)/drum.xpm \
 $(pixdir)/scale.xpm \
 $(pixdir)/bus.xpm \
 $(pixdir)/editor-q-rec.xpm \
 $(pixdir)/stop.xpm \
 $(pixdir)/rec.xpm \
 $(pixdir)/editor-play.xpm \
 $(pixdir)/expand.xpm \
 $(pixdir)/undo.xpm \
 $(pixdir)/sequences.xpm \
 $(pixdir)/play.xpm \
 $(pixdir)/key.xpm \
 $(pixdir)/editor-thru.xpm \
 $(pixdir)/copy.xpm \
 $(pixdir)/tools.xpm

BUILT_SOURCES = $(uic_sources) $(moc_sources)

#******************************************************************************
# No longer part of BUILT_SOURCES or libseq_qt5_la_SOURCES:
#
#     qseq64.qrc.cpp
#------------------------------------------------------------------------------

libseq_qt5_la_SOURCES = \
 gui_assistant_qt5.cpp \
 gui_palette_qt5.cpp \
 keys_perform_qt5.cpp \
 qclocklayout.cpp \
 qperfeditframe.cpp \
 qperfnames.cpp \
 qperfroll.cpp \
 qperftime.cpp \
 qsabout.cpp \
 qsbuildinfo.cpp \
 qseditoptions.cpp \
 qseqdata.cpp \
 qseqeditframe.cpp \
 qseqkeys.cpp \
 qseqroll.cpp \
 qseqstyle.cpp \
 qseqtime.cpp \
 qskeymaps.cpp \
 qsliveframe.cpp \
 qsmaintime.cpp \
 qsmainwnd.cpp \
 qstriggereditor.cpp \
 qt5_helpers.cpp \
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
