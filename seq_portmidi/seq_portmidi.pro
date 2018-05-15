#******************************************************************************
# seq_portmidi.pro (qplseq64)
#------------------------------------------------------------------------------
##
# \file       	seq_portmidi.pro
# \library    	qplseq64 application
# \author     	Chris Ahlstrom
# \date       	2018-04-08
# \update      2018-05-15
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# Created by and for Qt Creator. This file was created for editing the project
# sources only.  You may attempt to use it for building too, by modifying this
# file here.  [Which we have done.  -- Chris]
#
# Important:
#
#  This project file is designed only for Qt 5 (and above?).
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

TEMPLATE = lib
CONFIG += staticlib config_prl qtc_runnable

# These are needed to set up platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

TARGET = seq_portmidi

# Common:

HEADERS += \
 include/mastermidibus_pm.hpp \
 include/midibus_pm.hpp \
 include/pminternal.h \
 include/pmutil.h \
 include/portmidi.h \
 include/porttime.h

# Linux:

unix:!macx {
   HEADERS += include/pmlinux.h include/pmlinuxalsa.h 
}

# Mac OSX
#
#  We provide the readbinaryplist.h file in case someone needs it, but
#  currently do not include that module in the seq_portmidi library.

macx: {
   HEADERS += include/pmmac.h include/pmmacosxcm.h 
}

# Windows:

windows {
 HEADERS += include/pmerrmm.h include/pmwinmm.h
 DEFINES -= UNICODE
 DEFINES -= _UNICODE
 QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter
}

# Common:

SOURCES += \
 src/finddefault.c \
 src/mastermidibus.cpp \
 src/midibus.cpp \
 src/pmutil.c \
 src/portmidi.c \
 src/porttime.c

# Linux:

unix:!macx {
   SOURCES += src/pmlinux.c src/pmlinuxalsa.c src/ptlinux.c
}

# Mac OSX
#
#  We provide the readbinaryplist.c file in case someone needs it, but
#  currently do not include that module in the seq_portmidi library.
#  The ptmacosx_cf.c module is also provide, but it is not used, as far
#  as we can tell.

macx: {
   SOURCES += src/pmmac.c pmmacosxcm.c src/ptmacosx_mach.c
}

# Windows:

windows: SOURCES += src/pmwin.c \
 src/pmerrmm.c \
 src/pmwinmm.c \
  src/ptwinmm.c

INCLUDEPATH = ../include/qt/portmidi include ../libseq64/include

#******************************************************************************
# seq_portmidi.pro (qplseq64)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
