#*****************************************************************************
# Makefile.am (sequencer64)
#-----------------------------------------------------------------------------
##
# \file        Makefile.am
# \library     sequencer64
# \author      Chris Ahlstrom
# \date        2015-09-11
# \updates     2018-09-26
# \version     $Revision$
# \license     $XPC_SUITE_GPL_LICENSE$
#
#     This is the top-level project makefile for the sequencer64 project.
#
#     This makefile provides the skeleton needed to build the library
#     and application directories using GNU autotools.  It supports the
#     following sub-projects, depending on configuration options:
#
#        resources/pixmaps
#        libseq64
#        seq_alsamidi
#        seq_gtkmm2
#        Sequencer64
#        Seq64portmidi
#        Seq64rtmidi
#        Seq64cli
#        Midiclocker64
#        man
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Packing targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2 subdir-objects
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)
MOSTLYCLEANFILES = *~

#*****************************************************************************
# Extra files in the top-level directory
#-----------------------------------------------------------------------------

EXTRA_DIST = bootstrap pack README VERSION COPYING AUTHORS INSTALL NEWS ChangeLog

#*****************************************************************************
# Packaging
#-----------------------------------------------------------------------------
#
#     This section is recommended for the top-level Makefile.am by the
#     reference.
#
#-----------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@

SEQ64_API_MAJOR = @SEQ64_API_MAJOR@
SEQ64_API_MINOR = @SEQ64_API_MINOR@
SEQ64_API_PATCH = @SEQ64_API_PATCH@
SEQ64_API_VERSION = @SEQ64_API_VERSION@

SEQ64_LT_CURRENT = @SEQ64_LT_CURRENT@
SEQ64_LT_REVISION = @SEQ64_LT_REVISION@
SEQ64_LT_AGE = @SEQ64_LT_AGE@

#*****************************************************************************
# Installed directories
#-----------------------------------------------------------------------------
#
# The 'libdir' define is necessary to cause the proper subdirectory to
# be made during installation.  'sequencer64libdir' is defined in the
# configure.ac script.  The 'libdir' define is needed for work with
# libtool.  Not sure about 'pkglibdr'.
#
#     pkglibdir=$(sequencer64libdir)
#
#	Directories and macros:
#
#		prefix                = /usr/local
#		libdir                = /usr/local/lib/sequencer64
#		datadir               = /usr/local/share
#		datarootdir           = /usr/local/share
#		sequencer64libdir     = /usr/local/lib/sequencer64
#		sequencer64docdir     = /usr/local/share/doc/sequencer64.1
#		sequencer64doxygendir = doc
#		sequencer64includedir = /usr/local/include/sequencer64.1
#		localedir     = /usr/local/share/locale
#
#  'localedir' is the normal system directory for installed localization
#  files.
#
#-----------------------------------------------------------------------------

prefix = @prefix@
libdir = @sequencer64libdir@
datadir = @datadir@
datarootdir = @datarootdir@
sequencer64docdir = @sequencer64docdir@
sequencer64doxygendir = @sequencer64doxygendir@
sequencer64includedir = @sequencer64includedir@
sequencer64libdir = @sequencer64libdir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------
#
#		top_srcdir    = ../../.. [XPC Basic directory]
#		builddir      = /home/ahlstrom/ca/mls/git/XPC Basic-1.1/debug
#
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
#	${SHELL} $(top_srcdir)/config.status --recheck
#-----------------------------------------------------------------------------

LIBTOOL_DEPS = @LIBTOOL_DEPS@

libtool: $(LIBTOOL_DEPS)
	$(top_srcdir)/config.status --recheck

#*****************************************************************************
# SUBDIRS
#-----------------------------------------------------------------------------
#
#  We decided to build the Doxygen documentation only manually, and have
#  commented it out.
#
#  # doc/dox
#
#  Note the order of these tests is important to handling the mixing and
#  matching of MIDI engine versus user-interface.
#
#     RTMIDI/GTKMM
#     RTMIDI/QT5
#     PORTMIDI/GTKMM
#     Legacy ALSA/GTKMM
#     RTMIDI/CLI
#     PORTMIDI/WINDOWS/CLI build: QT5 Windows build is done via qmake.
#
#  See INSTALL.
#
#-----------------------------------------------------------------------------

SUBDIRS = resources/pixmaps libseq64

if BUILD_ALSAMIDI
SUBDIRS += seq_alsamidi seq_gtkmm2 Sequencer64
endif

if BUILD_PORTMIDI
SUBDIRS += seq_portmidi seq_gtkmm2 Seq64portmidi
endif

if BUILD_QTMIDI
SUBDIRS += seq_rtmidi seq_qt5 Seq64qt5
endif

if BUILD_RTCLI
SUBDIRS += seq_rtmidi Seq64cli
endif

if BUILD_RTMIDI
SUBDIRS += seq_rtmidi seq_gtkmm2 Seq64rtmidi Midiclocker64
endif

# Not supported.  Use the Qt project file and qmake+mingw to build the
# Windows version.  See the INSTALL file, section "Qmake-based Install".
#
# if BUILD_WINDOWS
# SUBDIRS += Seq64cli
# endif

SUBDIRS += man data

#*****************************************************************************
# DIST_SUBDIRS
#-----------------------------------------------------------------------------

DIST_SUBDIRS = $(SUBDIRS)

#*****************************************************************************
# all-local
#-----------------------------------------------------------------------------

all-local:
	@echo "Top source-directory 'top_srcdir' is $(top_srcdir)"
	@echo "* * * * * All build items completed * * * * *"

#******************************************************************************
# Debugging targets
#------------------------------------------------------------------------------

show:
	@echo "Install directories:"
	@echo
	@echo "prefix                    = $(prefix)"
	@echo "datadir                   = $(datadir)"
	@echo "datarootdir               = $(datarootdir)"
	@echo "libdir                    = $(libdir)"
	@echo "sequencer64libdir         = $(sequencer64libdir)"
	@echo "sequencer64datadir        = $(sequencer64datadir)"
	@echo "sequencer64docdir         = $(sequencer64docdir)"
	@echo "sequencer64doxygendir     = $(sequencer64doxygendir)"
	@echo "sequencer64includedir     = $(sequencer64includedir)"
	@echo "localedir                 = $(localedir)"
	@echo
	@echo "Local directories:"
	@echo
	@echo "top_srcdir                = $(top_srcdir) [project root directory]"
	@echo "srcdir                    = $(srcdir)"
	@echo "top_builddir              = $(top_builddir)"
	@echo "builddir                  = $(builddir)"

showver:
	@echo "PACKAGE                   = $(PACKAGE)"
	@echo "VERSION                   = $(VERSION)"
	@echo "SEQ64_API_MAJOR           = $(SEQ64_API_MAJOR)"
	@echo "SEQ64_API_MINOR           = $(SEQ64_API_MINOR)"
	@echo "SEQ64_API_PATCH           = $(SEQ64_API_PATCH)"
	@echo "SEQ64_API_VERSION         = $(SEQ64_API_VERSION)"
	@echo "SEQ64_LT_CURRENT          = $(SEQ64_LT_CURRENT)"
	@echo "SEQ64_LT_REVISION         = $(SEQ64_LT_REVISION)"
	@echo "SEQ64_LT_AGE              = $(SEQ64_LT_AGE)"

#****************************************************************************
# Makefile.am (sequencer64 top-level)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=automake
#----------------------------------------------------------------------------
