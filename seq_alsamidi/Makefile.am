#*****************************************************************************
# Makefile.am (libseq_alsamidi)
#-----------------------------------------------------------------------------
##
# \file          Makefile.am
# \library       libseq_alsamidi
# \author        Chris Ahlstrom
# \date          2016-11-20
# \updates       2016-11-20
# \version       $Revision$
# \license       $MIDICVT_SUITE_GPL_LICENSE$
#
#  	This file is a makefile for the seq_alsamidi library project.  This
#  	makefile provides the skeleton needed to build the libseq64 project
#  	directory using GNU autotools.
#
#  	This library is simply the MIDI bus modules of libseq64 moved out
#  	so that we can build a version of Sequencer64 without that ALSA support.
#
#-----------------------------------------------------------------------------

#*****************************************************************************
# Packing targets.
#-----------------------------------------------------------------------------
#
#		Always use Automake in foreign mode (adding foreign to
#		AUTOMAKE_OPTIONS in Makefile.am). Otherwise, it requires too many
#		boilerplate files from the GNU coding standards that aren't useful to
#		us. 
#
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)

#*****************************************************************************
# EXTRA_DIST
#-----------------------------------------------------------------------------

EXTRA_DIST =

#*****************************************************************************
# SUBDIRS
#-----------------------------------------------------------------------------

SUBDIRS = include src

#*****************************************************************************
# DIST_SUBDIRS
#-----------------------------------------------------------------------------
#
#      DIST_SUBDIRS is used by targets that need to recurse into /all/
#      directories, even those which have been conditionally left out of the
#      build.
#
#      Precisely, DIST_SUBDIRS is used by:
#
#         -   make dist
#         -   make distclean
#         -   make maintainer-clean.
#
#      All other recursive targets use SUBDIRS.
#
#-----------------------------------------------------------------------------

DIST_SUBDIRS = $(SUBDIRS)

#*****************************************************************************
# all-local
#-----------------------------------------------------------------------------

all-local:
	@echo "Top source-directory 'top_srcdir' is $(top_srcdir)"
	@echo "* * * * * All libseq_gktmm2 build items completed * * * * *"

#******************************************************************************
# Directories
#------------------------------------------------------------------------------

show:
	@echo "Install directories:"
	@echo
	@echo "prefix        = $(prefix)"
	@echo "datadir       = $(datadir)"
	@echo "datarootdir   = $(datarootdir)"
	@echo "libdir        = $(libdir)"
	@echo "localedir     = $(localedir)"
	@echo
	@echo "Local directories:"
	@echo
	@echo "top_srcdir    = $(top_srcdir) [project root directory]"
	@echo "srcdir    		= $(srcdir)"
	@echo "top_builddir  = $(top_builddir)"
	@echo "builddir      = $(builddir)"

#*****************************************************************************
# Makefile.am (libseq_alsamidi)
#-----------------------------------------------------------------------------
# vim: ts=3 sw=3 noet ft=automake
#-----------------------------------------------------------------------------
