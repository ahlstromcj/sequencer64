# seq_portmidi.pro

message($$_PRO_FILE_PWD_)

TEMPLATE = lib
TARGET = seq_portmidi
CONFIG += staticlib config_prl

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

