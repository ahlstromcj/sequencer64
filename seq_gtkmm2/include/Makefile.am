#******************************************************************************
# Makefile.am (libseq_gtkmm2)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_gtkmm2 library
# \author     	Chris Ahlstrom
# \date       	2015-09-11
# \update      2017-09-17
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq_gtkmm2 C++
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

EXTRA_DIST = *.h *.hpp

#******************************************************************************
# Items from configure.ac
#-------------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@
GIT_VERSION = @GIT_VERSION@

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

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
# Source files
#----------------------------------------------------------------------------
#
# seq24seqroll.h now obsolete, removed from the build.
#	seq24seq.hpp, ditto.
#
#----------------------------------------------------------------------------

pkginclude_HEADERS = \
   eventedit.hpp \
   eventslots.hpp \
	font.hpp \
	fruityperfroll_input.hpp \
   fruityseq.hpp \
   fruityseqroll.hpp \
   gtk_helpers.h \
   gui_assistant_gtk2.hpp \
   gui_drawingarea_gtk2.hpp \
   gui_key_tests.hpp \
   gui_palette_gtk2.hpp \
   gui_window_gtk2.hpp \
	keybindentry.hpp \
   keys_perform_gtk2.hpp \
   lfownd.hpp \
	maintime.hpp \
	mainwid.hpp \
	mainwnd.hpp \
	options.hpp \
	perfedit.hpp \
	perfnames.hpp \
	perfroll.hpp \
	perfroll_input.hpp \
	perftime.hpp \
	seqdata.hpp \
	seqedit.hpp \
	seqevent.hpp \
	seqkeys.hpp \
	seqmenu.hpp \
	seqroll.hpp \
	seqtime.hpp

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(pkgincludedir) manually"

#******************************************************************************
# Makefile.am (libseq_gtkmm2)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
