#******************************************************************************
# seq_portmidi.pro (qplseq64)
#------------------------------------------------------------------------------
##
# \file       	seq_portmidi.pro
# \library    	qplseq64 application
# \author     	Chris Ahlstrom
# \date       	2018-04-08
# \update      2018-04-10
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
CONFIG += staticlib config_prl

# These are needed to set up platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

CONFIG(release, debug|release) {
   LIBOUTDIR = release
} else:CONFIG(debug, debug|release) {
   LIBOUTDIR = debug
} else {
   LIBOUTDIR = .
}

TARGET = $$LIBOUTDIR/seq_portmidi

# Common:

HEADERS += \
 include/mastermidibus_pm.hpp \
 include/midibus_pm.hpp \
 include/pminternal.h \
 include/pmutil.h \
 include/portmidi.h \
 include/porttime.h

# Linux:

unix: HEADERS += \
 include/pmlinux.h \
 include/pmlinuxalsa.h \

# Windows:

windows: HEADERS += \
 include/pmwinmm.h

# Common:

SOURCES += \
 src/finddefault.c \
 src/mastermidibus.cpp \
 src/midibus.cpp \
 src/pmutil.c \
 src/portmidi.c \
 src/porttime.c

# Linux:

unix: SOURCES += \
 src/pmlinux.c \
 src/pmlinuxalsa.c \
 src/ptlinux.c

# Windows:

windows: SOURCES += \
 src/pmwin.c \
 src/pmwinmm.c \
 src/ptwinmm.c

INCLUDEPATH = \
 ../include/qt/portmidi \
 include \
 ../libseq64/include

#******************************************************************************
# seq_portmidi.pro (qplseq64)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
