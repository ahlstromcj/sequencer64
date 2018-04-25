#******************************************************************************
# libseq64.pro (qplseq64)
#------------------------------------------------------------------------------
##
# \file       	libseq64.pro
# \library    	qplseq64 application
# \author     	Chris Ahlstrom
# \date       	2018-04-08
# \update      2018-04-24
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# Important:
#
#  This project file is designed only for Qt 5 (and above?).
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

TEMPLATE = lib
CONFIG += staticlib config_prl qtc_runnable
TARGET = seq64

# These are needed to set up platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

HEADERS += \
 include/app_limits.h \
 include/businfo.hpp \
 include/calculations.hpp \
 include/click.hpp \
 include/cmdlineopts.hpp \
 include/configfile.hpp \
 include/controllers.hpp \
 include/daemonize.hpp \
 include/easy_macros.h \
 include/editable_event.hpp \
 include/editable_events.hpp \
 include/event.hpp \
 include/event_list.hpp \
 include/file_functions.hpp \
 include/gdk_basic_keys.h \
 include/globals.h \
 include/gui_assistant.hpp \
 include/jack_assistant.hpp \
 include/keys_perform.hpp \
 include/keystroke.hpp \
 include/lash.hpp \
 include/mastermidibase.hpp \
 include/mastermidibus.hpp \
 include/midi_container.hpp \
 include/midi_control.hpp \
 include/midi_list.hpp \
 include/midi_splitter.hpp \
 include/midi_vector.hpp \
 include/midibase.hpp \
 include/midibus.hpp \
 include/midibus_common.hpp \
 include/midibyte.hpp \
 include/midifile.hpp \
 include/mutex.hpp \
 include/optionsfile.hpp \
 include/palette.hpp \
 include/perform.hpp \
 include/platform_macros.h \
 include/rc_settings.hpp \
 include/recent.hpp \
 include/rect.hpp \
 include/scales.h \
 include/seq64_features.h \
 include/sequence.hpp \
 include/settings.hpp \
 include/triggers.hpp \
 include/user_instrument.hpp \
 include/user_midi_bus.hpp \
 include/user_settings.hpp \
 include/userfile.hpp

SOURCES += \
 src/businfo.cpp \
 src/calculations.cpp \
 src/click.cpp \
 src/cmdlineopts.cpp \
 src/configfile.cpp \
 src/controllers.cpp \
 src/daemonize.cpp \
 src/easy_macros.cpp \
 src/editable_event.cpp \
 src/editable_events.cpp \
 src/event.cpp \
 src/event_list.cpp \
 src/file_functions.cpp \
 src/gui_assistant.cpp \
 src/jack_assistant.cpp \
 src/keys_perform.cpp \
 src/keystroke.cpp \
 src/lash.cpp \
 src/mastermidibase.cpp \
 src/midi_container.cpp \
 src/midi_control.cpp \
 src/midi_list.cpp \
 src/midi_splitter.cpp \
 src/midi_vector.cpp \
 src/midibase.cpp \
 src/midibyte.cpp \
 src/midifile.cpp \
 src/mutex.cpp \
 src/optionsfile.cpp \
 src/palette.cpp \
 src/perform.cpp \
 src/rc_settings.cpp \
 src/recent.cpp \
 src/rect.cpp \
 src/seq64_features.cpp \
 src/sequence.cpp \
 src/settings.cpp \
 src/triggers.cpp \
 src/user_instrument.cpp \
 src/user_midi_bus.cpp \
 src/user_settings.cpp \
 src/userfile.cpp

INCLUDEPATH = \
 ../include/qt/portmidi \
 include \
 ../seq_portmidi/include

#******************************************************************************
# libseq64.pro (qplseq64)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
