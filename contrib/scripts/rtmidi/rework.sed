#!/usr/bin/sed -i -f
#
#  Execution:
#
#     sed -i -f rework.sed *.cpp *.c *.h
#
# override macro:  undefined if C++11 or higher.
#
# Order is important, the longer matches should be hit first.

# Original RtMidi naming conventions
#

s/xxxxxxxx/xxxxxxxxxxxx/g

# Data types
#
# Some of the types are bit-holders

s/unsigned char/midibyte/g
s/unsigned int/unsigned/g

# Macros

# s/UNSPECIFIED/SEQ64_API_UNSPECIFIED/g
# s/MACOSX_CORE/SEQ64_API_MACOSX_CORE/g
# s/LINUX_ALSA/SEQ64_API_LINUX_ALSA/g
# s/UNIX_JACK/SEQ64_API_UNIX_JACK/g
# s/WINDOWS_MM/SEQ64_API_WINDOWS_MM/g
# s/RTMIDI_DUMMY/SEQ64_API_RTMIDI_DUMMY/g

s/__LINUX_ALSA__/SEQ64_BUILD_LINUX_ALSA/g
s/__UNIX_JACK__/SEQ64_BUILD_UNIX_JACK/g
s/__MACOSX_CORE__/SEQ64_BUILD_MACOSX_CORE/g
s/__WINDOWS_MM__/SEQ64_BUILD_WINDOWS_MM/g
s/__RTMIDI_DUMMY__/SEQ64_BUILD_RTMIDI_DUMMY/g

# Member naming conventions

s/apiData_/m_api_data/g
s/connected_/m_connected/g
s/errorCallbackUserData_/m_error_callback_user_data/g
s/errorCallback_/m_error_callback/g
s/errorString_/m_error_string/g
s/firstErrorOccurred_/first_error_occurred/g
s/inputData_/m_input_data/g
s/rtapi_/m_rtapi/g

# Argument naming conventions

s/clientName/clientname/g
s/midiSense/midisense/g
s/midiSysex/midisysex/g
s/midiTime/miditime/g
s/portName/portname/g
s/portNumber/portnumber/g
s/queueSizeLimit/queuesizelimit/g
s/userData/userdata/g

# Class and function naming conventions

s/cancelCallback/cancel_callback/g
s/closePort/close_port/g
s/errorCallback/errorcallback/g
s/ErrorCallback/errorcallback/g
s/errorString/errorstring/g
s/getCompiledApi/get_compiled_api/g
s/getCurrentApi/get_current_api/g
s/getMessage/get_message/g
s/getPortCount/get_port_count/g
s/getPortName/get_port_name/g
s/getVersion/get_version/g
s/ignoreTypes/ignore_types/g
s/isPortOpen/is_port_open/g
s/MidiApi/midi_api/g
s/MidiInAlsa/midi_in_alsa/g
s/MidiInApi/midi_in_api/g
s/MidiInCore/midi_in_core/g
s/MidiInDummy/midi_in_dummy/g
s/MidiInJack/midi_in_jack/g
s/MidiInWinMM/midi_in_winmm/g
s/MidiMessage/midi_message/g
s/MidiOutAlsa/midi_out_alsa/g
s/MidiOutApi/midi_out_api/g
s/MidiOutCore/midi_out_core/g
s/MidiOutDummy/midi_out_dummy/g
s/MidiOutJack/midi_out_jack/g
s/MidiOutWinMM/midi_out_winmm/g
s/MidiQueue/midi_queue/g
s/openPort/open_port/g
s/openVirtualPort/open_virtual_port/g
s/rtmidiCallback/rtmidi_callback_t/g
s/RtMidiCallback/rtmidi_callback_t/g
s/rtmidiErrorCallback/rtmidi_error_callback_t/g
s/RtMidiErrorCallback/rtmidi_error_callback_t/g
s/rtmidiError/rterror/g
s/RtMidiError/rterror/g
s/rtmidiInData/rtmidi_in_data/g
s/RtMidiInData/rtmidi_in_data/g
s/rtmidiIn/rtmidi_in/g
s/RtMidiIn/rtmidi_in/g
s/rtmidiout/rtmidi_out/g
s/rtmidiOut/rtmidi_out/g
s/RtMidiOut/rtmidi_out/g
s/sendMessage/send_message/g
s/setCallback/set_callback/g
s/setErrorCallback/set_error_callback/g

# Keep this one last!

s/RtMidi/rtmidi/g
s/RtMIDI/rtmidi/g

# End of...

s/ \* End of /\r * /

# Remove extra carriage returns and trailing spaces and tabs.

s///

# Remove trailing spaces

s/  *$//g

# Delete the first long line of asterisks, and next line
 
/^\/\*\*\*\*\*\*.*\*$/{N;d;}

# Replace a long line of dashes of format " *-----------*//**"
# with just "/**"

s/^ \*-----------.*\/\/\*\*/\/**/

# Replace a long line of dashes of format " *//*-----------*/"
# with just " */"

s/^ \*\/\/\*------------.*\*\// *\//

# Replace a long line of asterisks of format "/**********" with
# just "/*".  But we like just deleting those lines and the next better.
#
# s/\/\*\*\*\*\*\*\*.*\*$/\/*/

# s/ \* End of / * /

/ Local variables:/d

/ End:$/d

s/ \*----------------------.*---------------------$/ */

s/ \*----------------------.*---------------------\*\/$/ *\//

#-------------------------------------------------------------------------------
# Convert "// " at line's start to " * "
#-------------------------------------------------------------------------------

s/^\/\/ / * /

#-------------------------------------------------------------------------------
# Convert "//" at line's start to " *"
#-------------------------------------------------------------------------------

s/^\/\// */

#-------------------------------------------------------------------------------
# Shorten long lines having nothing but spaces, that end in *
#-------------------------------------------------------------------------------

s/      *\*$/ */

#-------------------------------------------------------------------------------
# End of simplify.sed
#-------------------------------------------------------------------------------
# vim: ts=3 sw=3 et ft=sed
#-------------------------------------------------------------------------------
