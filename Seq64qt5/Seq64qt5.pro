# Created by and for Qt Creator This file was created for editing the project
# sources only.  You may attempt to use it for building too, by modifying this
# file here.
#
# Important:
#
#  This project file is designed only for Qt 5 (and above?).

message($$_PRO_FILE_PWD_)

QT += core gui widgets
TARGET = qplseq64
TEMPLATE += app
CONFIG += static
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

unix: PRE_TARGETDEPS += \
 $$OUT_PWD/../libseq64/libseq64.a \ 
 $$OUT_PWD/../seq_portmidi/libseq_portmidi.a \ 
 $$OUT_PWD/../seq_qt5/libseq_qt5.a

## win32:CONFIG(release, debug|release): LIBS += 
##  -L$$OUT_PWD/../../../projects/mylib/release/ -lmylib
## else:win32:CONFIG(debug, debug|release): LIBS += 
##  -L$$OUT_PWD/../../../projects/mylib/debug/ -lmylib
## else:unix: LIBS += 
##  -L$$OUT_PWD/../../../projects/mylib/ -lmylib

# Sometimes some midifile and rect member functions cannot be found at link
# time, so we include libseq64 twice.
#
# https://eli.thegreenplace.net/2013/07/09/library-order-in-static-linking

unix: LIBS += \
 -L$$OUT_PWD/../libseq64 -lseq64 \
 -L$$OUT_PWD/../seq_portmidi -lseq_portmidi \
 -L$$OUT_PWD/../seq_qt5 -lseq_qt5 \
 -L$$OUT_PWD/../libseq64 -lseq64

# May consider adding:  /usr/include/lash-1.0 and -llash

unix:!macx: LIBS += \
 -lasound \
 -ljack \
 -lrt

