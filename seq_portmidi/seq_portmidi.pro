#******************************************************************************
# seq_portmidi.pro (qplseq64)
#------------------------------------------------------------------------------
##
# \file       	seq_portmidi.pro
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

TEMPLATE = lib
TARGET = seq_portmidi
CONFIG += staticlib config_prl

# These are needed to set up platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

# Windows:
#   include/pmwinmm.h
#
# Linux:
#   include/pmlinux.h
#   include/pmlinuxalsa.h

HEADERS += \
 include/mastermidibus_pm.hpp \
 include/midibus_pm.hpp \
 include/pminternal.h \
 include/pmlinux.h \
 include/pmlinuxalsa.h \
 include/pmutil.h \
 include/portmidi.h \
 include/porttime.h

# Windows:
#   src/pmwin.c
#   src/pmwinmm.c
#   src/ptwinmm.c
#
# Linux:
#   src/pmlinux.c \
#   src/pmlinuxalsa.c \
#   src/ptlinux.c

SOURCES += \
 src/finddefault.c \
 src/mastermidibus.cpp \
 src/midibus.cpp \
 src/pmlinux.c \
 src/pmlinuxalsa.c \
 src/pmutil.c \
 src/portmidi.c \
 src/porttime.c \
 src/ptlinux.c

INCLUDEPATH = \
 ../include/qt/portmidi \
 include \
 ../libseq64/include

#******************************************************************************
# seq_portmidi.pro (qplseq64)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
