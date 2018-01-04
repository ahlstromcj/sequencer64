#******************************************************************************
# Makefile.am (sequencer64/data)
#------------------------------------------------------------------------------
##
# \file     	Makefile.am
# \library     sequencer64
# \author      Chris Ahlstrom
# \date        2018-01-02
# \updates     2018-01-03
# \version     $Revision$
# \license     $XPC_SUITE_GPL_LICENSE$
#
#   	This module provides an Automake makefile for the project's
#     sequencer64/data directory.  These are files we want to copy to
#     /usr/share/sequencer64 upon installation.
#
#------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)

#******************************************************************************
# EXTRA_DIST
#------------------------------------------------------------------------------
#
# 		Note that we have to add the appropriate wildcards to ensure these files
# 		get distributed.
#
#------------------------------------------------------------------------------

EXTRA_DIST = README *.rc *.usr *.midi

#*****************************************************************************
#	Macros
#-----------------------------------------------------------------------------
#
#	This section is recommended by:
#
#		http://olympus.het.brown.edu/cgi-bin/info2www?(gettext)Makefile
#
#-----------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------
#
# Common prefix for installation directories.  This directory must exist when
# you start the install.
#
# prefix = /usr/local
# datarootdir = $(prefix)/share
# datadir = $(datarootdir)
#
# Where to put the Info files.
#
# infodir = $(datarootdir)/info
#
#------------------------------------------------------------------------------

prefix = @prefix@
datadir = @datadir@
datarootdir = @datarootdir@
pkgdatadir = @pkgdatadir@
sequencer64datadir = @sequencer64datadir@

# datarootdir = $(prefix)/share

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

#------------------------------------------------------------------------------
# install-data files
#------------------------------------------------------------------------------
#
# sequencer64dir = $(datarootdir)/sequencer64
# sequencer64_DATA = GM_PSS-790_Multi.ini
#
#		Instead we use variables defined in configure.ac for now.
#
#------------------------------------------------------------------------------

#******************************************************************************
# Installing documentation and other "data" files.
#------------------------------------------------------------------------------
#
#     We need to add an install-data-hook to copy the generated
#     documentation directories to the destination directory.  The normal
#     method doesn't work because /usr/bin/install will only install files,
#     and automake doesn't give it the switch needed to install directories.
#
#------------------------------------------------------------------------------

install-data-local:
	mkdir -p $(DESTDIR)$(sequencer64datadir)
	cp -r -p $(top_builddir)/data/README $(DESTDIR)$(sequencer64datadir)
	cp -r -p $(top_builddir)/data/*.rc $(DESTDIR)$(sequencer64datadir)
	cp -r -p $(top_builddir)/data/*.usr $(DESTDIR)$(sequencer64datadir)
	cp -r -p $(top_builddir)/data/*.midi $(DESTDIR)$(sequencer64datadir)

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	rm -rf $(DESTDIR)$(sequencer64datadir)

#******************************************************************************
# Makefile.am (sequencer64/include)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 noet ft=automake
#------------------------------------------------------------------------------
