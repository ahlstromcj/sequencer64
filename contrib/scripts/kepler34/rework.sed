#!/usr/bin/sed -i -f
#
#  Execution:
#
#     sed -i -f rework.sed *.cpp *.c *.h *.hpp
#
# override macro:  undefined if C++11 or higher.
#
# Order is important, the longer matches should be hit first.

# Changes Kepler34 naming to Sequencer64 naming
#

s/xxxxxxxx/xxxxxxxxxxxx/g

# Data types
#
# Some of the types are bit-holders

s/unsigned char/midibyte/g
s/unsigned int/unsigned/g

# Class names

s/ConfigFile/configfile/g
s/Globals\.hpp/globals.h/g
s/MasterMidiBus/mastermidibus/g
s/MidiBus/midibus/g
s/MidiControl/control/g
s/MidiEvent/event/g
s/MidiFile/midifile/g
s/MidiPerformance/perform/g
s/MidiSequence/sequence/g
s/MidiTrigger/trigger/g
s/Mutex/mutex/g
s/PreferencesFile/optionsfile/g
s/UserFile/userfile/g

# Variable names

s/cSeqsInBank/c_seqs_in_set/g
s/quanize/quantize/g

# Macros

# Function names

s/getBeatWidth/get_beat_width/g
s/getBeatsPerMeasure/get_beats_per_measure/g
s/setBeatLength/set_beat_width/g
s/setBeatsPerMeasure/set_beats_per_measure/g
s/updateSizes/update_sizes/g

# Follow on

s/BeatsPerMeasure/beats_per_measure/g
s/beatsPerMeasure/m_beats_per_measure/g
s/BeatWidth/beat_width/g
s/beatWidth/m_beat_width/g
s/setSnap/set_snap/g
s/setGuides/set_guides/g
s/setNote_length/set_note_length/g
s/zoomIn/zoom_in/g
s/zoomOut/zoom_out/g


# Keep this one last!

s/RtMidi/rtmidi/g
s/RtMIDI/rtmidi/g

# End of...

s/ \* End of /\r * /

# Remove extra carriage returns and trailing spaces and tabs.

s///

# Remove trailing spaces

s/  *$//g

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
