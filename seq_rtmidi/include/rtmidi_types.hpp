#ifndef SEQ64_RTMIDI_TYPES_HPP
#define SEQ64_RTMIDI_TYPES_HPP

/**
 * \file          rtmidi_types.hpp
 *
 *  Type definitions pulled out for the needs of the refactoring.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2016-12-18
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  The lack of hiding of these types within a class is a little to be
 *  regretted.  On the other hand, it does make the code much easier to
 *  refactor and partition, and slightly easier to read.
 */

#include <string>                           /* std::string                  */
#include <vector>                           /* std::vector container        */

#include "midibyte.hpp"                     /* seq64::midibyte typedef      */

/**
 * This was the version of the RtMidi library from which this reimplementation
 * was forked.
 */

#define SEQ64_RTMIDI_VERSION "2.1.1"        /* revision at fork time        */

/**
 *  Macros for selecting input versus output ports in a more obvious way.
 *  These items are needed for the midi_mode() setter function.  Note that
 *  midi_mode() has no functionality in the midi_api base class, which has a
 *  number of such stub functions so that we can use the midi_info and midi_api
 *  derived classes.
 */

#define SEQ64_MIDI_OUTPUT       false       /* the MIDI mode is not input   */
#define SEQ64_MIDI_INPUT        true        /* the MIDI mode is input       */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  A macro to prepend a fully qualified function name to a string.
 *  Cannot get circular reference to message_concatenate() resolved!
 *  In fact any true functions added to easy_macros are unresolved.
 *  WTF!?
 *
 * #define func_message(x)         seq64::message_concatenate(__func__, x)
 *
 * #ifdef DEFINE_MESSAGE_CONCATENATE_HERE
 */

extern std::string message_concatenate (const char * m1, const char * m2);
extern bool info_message (const std::string & msg);
extern bool error_message (const std::string & msg);

/**
 *    MIDI API specifier arguments.  These items used to be nested in
 *    the rtmidi class, but that only worked when RtMidi.cpp/h were
 *    large monolithic modules.
 */

enum rtmidi_api
{
    RTMIDI_API_UNSPECIFIED,     /**< Search for a working compiled API.     */
    RTMIDI_API_LINUX_ALSA,      /**< Advanced Linux Sound Architecture API. */
    RTMIDI_API_UNIX_JACK,       /**< JACK Low-Latency MIDI Server API.      */

#ifdef USE_RTMIDI_API_ALL

    /*
     * We're not supporting these until we get a simplified
     * sequencer64-friendly API worked out.
     */

    RTMIDI_API_MACOSX_CORE,     /**< Macintosh OS-X Core Midi API.          */
    RTMIDI_API_WINDOWS_MM,      /**< Microsoft Multimedia MIDI API.         */
    RTMIDI_API_DUMMY,           /**< A compilable but non-functional API.   */

#endif

    RTMIDI_API_MAXIMUM          /**< A count of APIs; an erroneous value.   */

};

}           // namespace seq64

#endif      // SEQ64_RTMIDI_TYPES_HPP

/*
 * rtmidi_types.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

