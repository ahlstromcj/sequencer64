# seq_qt5.pro

message($$_PRO_FILE_PWD_)

QT += core gui widgets
TEMPLATE = lib
CONFIG += staticlib config_prl

# Abortive attempts:
#
# TEMPLATE = lib subdirs
# SUBDIRS = forms
#
# Target file directory:
# DESTDIR = bin
#
# Intermediate object files directory:
# OBJECTS_DIR = generated_files
#
# Intermediate moc files directory:
# MOC_DIR = moc

# Not UIC_DIR :-D

UI_DIR = forms

FORMS += \
 forms/qperfeditframe.ui  \
 forms/qsabout.ui  \
 forms/qseditoptions.ui  \
 forms/qseqeditframe.ui  \
 forms/qsliveframe.ui  \
 forms/qsmainwnd.ui

RESOURCES += src/qseq64.qrc

HEADERS += \
 include/Globals.hpp \
 include/gui_assistant_qt5.hpp \
 include/gui_palette_qt5.hpp \
 include/keys_perform_qt5.hpp \
 include/qperfeditframe.hpp \
 include/qperfnames.hpp \
 include/qperfroll.hpp \
 include/qperftime.hpp \
 include/qsabout.hpp \
 include/qseditoptions.hpp \
 include/qseqdata.hpp \
 include/qseqeditframe.hpp \
 include/qseqkeys.hpp \
 include/qseqroll.hpp \
 include/qseqstyle.hpp \
 include/qseqtime.hpp \
 include/qskeymaps.hpp \
 include/qsliveframe.hpp \
 include/qsmacros.hpp \
 include/qsmaintime.hpp \
 include/qsmainwnd.hpp \
 include/qstriggereditor.hpp \
 include/qt5_helpers.hpp

SOURCES += \
 src/gui_assistant_qt5.cpp \
 src/gui_palette_qt5.cpp \
 src/keys_perform_qt5.cpp \
 src/qperfeditframe.cpp \
 src/qperfnames.cpp \
 src/qperfroll.cpp \
 src/qperftime.cpp \
 src/qsabout.cpp \
 src/qseditoptions.cpp \
 src/qseqdata.cpp \
 src/qseqeditframe.cpp \
 src/qseqkeys.cpp \
 src/qseqroll.cpp \
 src/qseqstyle.cpp \
 src/qseqtime.cpp \
 src/qskeymaps.cpp \
 src/qsliveframe.cpp \
 src/qsmaintime.cpp \
 src/qsmainwnd.cpp \
 src/qstriggereditor.cpp \
 src/qt5_helpers.cpp

# The output of the uic command goes to the seq_qt5/forms directory in
# the shadow directory, and cannot be located unless the OUT_PWD macro
# is used to find that directory.

INCLUDEPATH = \
 ../include/qt/portmidi \
 include \
 ../libseq64/include \
 ../seq_portmidi/include \
 ../resources \
 $$OUT_PWD

