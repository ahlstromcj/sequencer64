#*******************************************************************************
# Makefile.am
#-------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	generic
# \author     	Chris Ahlstrom
# \date       	2015-09-10
# \update     	2016-05-28
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		Makefile for the doc/dox subdirectory of this package.  We had to do a
# 		little two-stepping to make sure that the documentation is built
# 		everytime we touch one of the doxygen files, and NOT when we touch one of
# 		the source-code files.  Tired of checking in the PDF file all the time!
#
#-------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2
MAINTAINERCLEANFILES = Makefile.in dox-warnings.log
CLEANFILES = dox-stamp

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------
#
# We need to move some of this to a Makefile.am in the directory aboe.
#
#------------------------------------------------------------------------------

EXTRA_DIST = make_dox doxy*.cfg *.dox ../*.pdf

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
srcdir = @srcdir@
sequencer64docdir = @sequencer64docdir@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------

prefix = @prefix@
datadir = @datadir@
docdir = @docdir@
datarootdir = @datarootdir@
sequencer64docdir = @sequencer64docdir@

#------------------------------------------------------------------------------
# The $(srcdir) component points us to the correct place whether the build is
# in-source or out-of-source.  Can't use pushd/popd because they are bash
# builtins and are not available, except inside a script.
#
# 	$(srcdir)/make_dox $(srcdir) reference
# 	echo "Generating dox-stamp file..."
# 	date > dox-stamp
#------------------------------------------------------------------------------

dox-stamp: $(EXTRA_DIST)
	-echo "We no longer rebuild the developer documentation for Sequencer64."
	-echo "To rebuild it, change to the doc/dox directory and run the command"
	-echo "'./make_dox reference'.  Installation is still supported, however."
	-date > dox-stamp
 
all-local: dox-stamp

clean-local:
	-rm -f *.db
	-rm -rf html
	-rm -rf latex
	-rm -f *.log

#******************************************************************************
# Installing documentation
#------------------------------------------------------------------------------
#
# 	   We need to add an install-data-hook to copy the generated
# 	   documentation directories to the destination directory.  The normal
# 	   method doesn't work because /usr/bin/install will only install files,
# 	   and automake doesn't give it the switch needed to install directories.
#
# Don't install the html files:
#
# 		cp -r -p $(top_builddir)/doc/dox/html $(xpcdoxygendir)
#
#------------------------------------------------------------------------------

install-data-local:
	mkdir -p $(DESTDIR)$(sequencer64docdir)
	cp -r -p $(top_builddir)/doc/*.pdf $(DESTDIR)$(sequencer64docdir)

#*******************************************************************************
# uninstall-hook
#-------------------------------------------------------------------------------
#
#		Quick and dirty method used.
#
#-------------------------------------------------------------------------------

uninstall-hook:
	rm -rf $(DESTDIR)$(sequencer64docdir)

#******************************************************************************
# Makefile.am (dox)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 noet ft=automake
#------------------------------------------------------------------------------
