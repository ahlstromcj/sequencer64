#******************************************************************************
# Seq64qt5.pro (Seq64qt5)
#------------------------------------------------------------------------------
##
# \file       	Seq64qt5.pro
# \library    	seq64qt5 application
# \author     	Chris Ahlstrom
# \date       	2018-04-08
# \update      2018-04-24
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
TARGET = qpseq64
TEMPLATE += app
CONFIG += static qtc_runnable

# These are needed to set up platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

SOURCES += seq64qt5.cpp

INCLUDEPATH = \
 ../include/qt/portmidi \
 ../libseq64/include \
 ../seq_portmidi/include \
 ../seq_qt5/include

# Sometimes some midifile and rect member functions cannot be found at link
# time, and this is worse with static linkage of our internal libraries.
# So we add the linker options --start-group and --end-group, as discussed
# in this interesting article.
#
# https://eli.thegreenplace.net/2013/07/09/library-order-in-static-linking
#

win32:CONFIG(release, debug|release) {
 LIBS += \
  -Wl,--start-group \
  -L$$OUT_PWD/../libseq64/release -lseq64 \
  -L$$OUT_PWD/../seq_portmidi/release -lseq_portmidi \
  -L$$OUT_PWD/../seq_qt5/release -lseq_qt5 \
  -Wl,--end-group
}
else:win32:CONFIG(debug, debug|release) {
 LIBS += \
  -Wl,--start-group \
  -L$$OUT_PWD/../libseq64/debug -lseq64 \
  -L$$OUT_PWD/../seq_portmidi/debug -lseq_portmidi \
  -L$$OUT_PWD/../seq_qt5/debug -lseq_qt5 \
  -Wl,--end-group
}
else:unix {
LIBS += \
 -Wl,--start-group \
 -L$$OUT_PWD/../libseq64 -lseq64 \
 -L$$OUT_PWD/../seq_portmidi -lseq_portmidi \
 -L$$OUT_PWD/../seq_qt5 -lseq_qt5 \
 -Wl,--end-group
}

DEPENDPATH += \
 $$PWD/../libseq64 \
 $$PWD/../seq_portmidi \
 $$PWD/../seq_qt5

# Works in Linux with "CONFIG += debug".

unix {
 PRE_TARGETDEPS += \
  $$OUT_PWD/../libseq64/libseq64.a \ 
  $$OUT_PWD/../seq_portmidi/libseq_portmidi.a \ 
  $$OUT_PWD/../seq_qt5/libseq_qt5.a
}

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
