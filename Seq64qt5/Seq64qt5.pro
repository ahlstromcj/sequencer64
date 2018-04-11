#******************************************************************************
# Seq64qt5.pro (Seq64qt5)
#------------------------------------------------------------------------------
##
# \file       	Seq64qt5.pro
# \library    	seq64qt5 application
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
# Important: This project file is designed only for Qt 5 (and above?).
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

QT += core gui widgets
TARGET = qplseq64
TEMPLATE += app
CONFIG += static

# These are needed to set up platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

SOURCES += seq64qt5.cpp

# Tries to link in non-existentic static libraries for Qt, ALSA (libasound),
# etc.
#
# QMAKE_LFLAGS = -Xlinker -Bstatic $$QMAKE_LFLAGS

INCLUDEPATH = \
 ../include/qt/portmidi \
 ../libseq64/include \
 ../seq_portmidi/include \
 ../seq_qt5/include

DEPENDPATH += \
 $$PWD/../libseq64 \
 $$PWD/../seq_portmidi \
 $$PWD/../seq_qt5

## win32:CONFIG(release, debug|release): PRE_TARGETDEPS += 
##  $$OUT_PWD/../../../projects/mylib/release/mylib.lib
## else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += 
##   $$OUT_PWD/../../../projects/mylib/debug/mylib.lib
## else:unix: PRE_TARGETDEPS += 
##    $$OUT_PWD/../../../projects/mylib/libmylib.a

CONFIG(release, debug|release) {
   LIBOUTDIR = release
} else:CONFIG(debug, debug|release) {
   LIBOUTDIR = debug
} else {
   LIBOUTDIR = .
}

## unix: PRE_TARGETDEPS +=
## PRE_TARGETDEPS +=

PRE_TARGETDEPS += \
 $$OUT_PWD/../libseq64/$$LIBOUTDIR/libseq64.a \ 
 $$OUT_PWD/../seq_portmidi/$$LIBOUTDIR/libseq_portmidi.a \ 
 $$OUT_PWD/../seq_qt5/$$LIBOUTDIR/libseq_qt5.a

## win32:CONFIG(release, debug|release): LIBS += 
##  -L$$OUT_PWD/../../../projects/mylib/release/ -lmylib
## else:win32:CONFIG(debug, debug|release): LIBS += 
##  -L$$OUT_PWD/../../../projects/mylib/debug/ -lmylib
## else:unix: LIBS += 
##  -L$$OUT_PWD/../../../projects/mylib/ -lmylib

# Sometimes some midifile and rect member functions cannot be found at link
# time, and this is worse with static linkage of our internal libraries.
# So we add the linker options --start-group and --end-group, as discussed
# in this interesting article.
#
# https://eli.thegreenplace.net/2013/07/09/library-order-in-static-linking
#
## unix: LIBS +=

LIBS += \
 -Wl,--start-group \
 -L$$OUT_PWD/../libseq64/$$LIBOUTDIR -lseq64 \
 -L$$OUT_PWD/../seq_portmidi/$$LIBOUTDIR -lseq_portmidi \
 -L$$OUT_PWD/../seq_qt5/$$LIBOUTDIR -lseq_qt5 \
 -Wl,--end-group

# -L$$OUT_PWD/../libseq64 -lseq64

# May consider adding:  /usr/include/lash-1.0 and -llash

unix:!macx: LIBS += \
 -lasound \
 -ljack \
 -lrt

windows: LIBS += -lwinmm

#******************************************************************************
# Seq64qt5.pro (Seq64qt5)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
