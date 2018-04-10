#******************************************************************************
# qplseq64.pro (qplseq64)
#------------------------------------------------------------------------------
##
# \file       	qplseq64.pro
# \library    	qplseq64 application
# \author     	Chris Ahlstrom
# \date       	2018-04-08
# \update      2018-04-08
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# Created by and for Qt Creator This file was created for editing the project
# sources only.  You may attempt to use it for building too, by modifying this
# file here.
#
# Important:
#
#  This project file is designed only for Qt 5 (and above?).
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

CONFIG += static link_prl
TEMPLATE = subdirs
SUBDIRS =  libseq64 seq_portmidi seq_qt5 Seq64qt5

#******************************************************************************
# qplseq64.pro (qplseq64)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
