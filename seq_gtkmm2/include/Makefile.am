#******************************************************************************
# Makefile.am (libseq_gtkmm2)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_gtkmm2 library
# \author     	Chris Ahlstrom
# \date       	2015-09-11
# \update      2015-09-22
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

EXTRA_DIST =

#******************************************************************************
# Items from configure.ac
#-------------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------

prefix = @prefix@
sequencer64includedir = @sequencer64includedir@
sequencer64libdir = @sequencer64libdir@

#******************************************************************************
# Source files
#----------------------------------------------------------------------------
#
#  Requires Gtk support via the perform module:
#
#     lash.hpp
#     optionsfile.hpp
#
#  Moved to libseq64:
#
#     midifile.hpp
#     perform.hpp
#     gdk_basic_keys.h
#
#----------------------------------------------------------------------------

pkginclude_HEADERS = \
	lash.hpp \
	optionsfile.hpp \
	font.hpp \
   fruityseq.hpp \
   fruityseqroll.hpp \
   gtk_helpers.h \
   gui_assistant_gtk2.hpp \
   gui_drawingarea_gtk2.hpp \
   gui_palette_gtk2.hpp \
   gui_window_gtk2.hpp \
	keybindentry.hpp \
	maintime.hpp \
	mainwid.hpp \
	mainwnd.hpp \
	options.hpp \
	perfedit.hpp \
	perfnames.hpp \
	perfroll.hpp \
	perfroll_input.hpp \
	perftime.hpp \
	seq24seq.hpp \
	seq24seqroll.hpp \
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
