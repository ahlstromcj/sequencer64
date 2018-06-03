#ifndef SEQ64_PMINTERNAL_H
#define SEQ64_PMINTERNAL_H

/*
 *  This file is part of sequencer64, adapted from the PortMIDI project.
 *
 *  sequencer64 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  sequencer64 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file        pminternal.h
 *
 *  This file is included by files that implement library internals.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-05-25
 * \license     GNU GPLv2 or above
 *
 * Here is a guide to implementers:
 *
 *  -   Provide an initialization function similar to pm_winmm_init().
 *  -   Add your initialization function to pm_init().  Note that your init
 *      function should never require non-standard libraries or fail in any
 *      way. If the interface is not available, simply do not call
 *      pm_add_device(). This means that non-standard libraries should try to do
 *      dynamic linking at runtime using a DLL and return without error if the
 *      DLL cannot be found or if there is any other failure.
 *  -   Implement functions as indicated in pm_fns_type to open, read, write,
 *      close, etc.
 *  -   Call pm_add_device() for each input and output device, passing it a
 *      pm_fns_type structure.
 *
 *  Assumptions about pm_fns_type functions are given below.
 */

#include <stdlib.h>

#include "platform_macros.h"        // PLATFORM_WINDOWS, etc.

/**
 *  Rather than having users install a special .h file for Windows, just put
 *  the required definitions inline here. The porttime.h header file uses
 *  these too, so the definitions are (unfortunately) duplicated there.
 */

#if defined PLATFORM_WINDOWS        //  WIN32
#ifndef INT32_DEFINED
#define INT32_DEFINED
typedef int int32_t;
typedef unsigned int uint32_t;
#endif
#else
#include <stdint.h>                 // Linux and OS X have stdint.h
#endif                              // PLATFORM_WINDOWS

/**
 *  Default size of buffers for SysEx transmission.
 */

#define PM_DEFAULT_SYSEX_BUFFER_SIZE    1024

/**
 *  Length of a message header?
 */

#define HDRLENGTH                       50

/**
 *  Any host error message will occupy less than this number of characters.
 */

#define PM_HOST_ERROR_MSG_LEN           256
#define PM_STRING_MAX                   256

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

/**
 *  TRUE if t1 before t2.
 */

#define PmBefore(t1, t2)            ((t1-t2) < 0)
#define none_write_flush            pm_fail_timestamp_fn
#define none_sysex                  pm_fail_timestamp_fn
#define none_poll                   pm_fail_fn
#define success_poll                pm_success_fn
#define MIDI_REALTIME_MASK          0xf8
#define is_real_time(msg) \
    ((Pm_MessageStatus(msg) & MIDI_REALTIME_MASK) == MIDI_REALTIME_MASK)

#ifdef __cplusplus
extern "C"
{
#endif

/**
 *  This typedef is the same as seq64::midibyte, but is for use by the C
 *  modules of PortMIDI.
 */

typedef unsigned char midibyte_t;

/**
 *  Pm_Message() encodes a short MIDI message into a 32-bit word. If data1
 *  and/or data2 are not present, use zero.
 *
 *  Pm_MessageStatus(), Pm_MessageData1(), and
 *  Pm_MessageData2() extract fields from a 32-bit midi message.
 */

#define Pm_Message(status, data1, data2) ((((data2) << 16) & 0xFF0000) | \
    (((data1) << 8) & 0xFF00) | ((status) & 0xFF))

#define Pm_MessageStatus(msg) ((msg) & 0xFF)
#define Pm_MessageData1(msg) (((msg) >> 8) & 0xFF)
#define Pm_MessageData2(msg) (((msg) >> 16) & 0xFF)

typedef int32_t PmMessage;          /**< see PmEvent */

/**
 *  PmTimestamp is used to represent a millisecond clock with arbitrary
 *  start time. The type is used for all MIDI timestampes and clocks.
 */

typedef int32_t PmTimestamp;

/**
 *  Indicates the lack of a device.  :-)
 */

#define pmNoDevice -1

/**
 *  Indicates the structure version of PmDeviceInfo.
 */

#define PM_STRUCTURE_VERSION  950           /* 0.95.0 */

/**
 *  Holds information about the device and its platform.  We are going to
 *  extend this structure by adding the client and port numbers.  These
 *  will be the ALSA client and port numbers under Linux, and just the ordinal
 *  numbers under Windows.  We will also update structVersion.  This value was
 *  never assigned, and was just a random value.  We will start using it with
 *  a value of 950 (for 0.95.0 in Sequencer64).
 */

typedef struct
{
    int structVersion;      /**< This internal structure version.           */
    const char * interf;    /**< Underlying MIDI API, MMSystem, DirectX.i   */
    const char * name;      /**< Device name, e.g. USB MidiSport 1x1.       */
    int input;              /**< True iff input is available.               */
    int output;             /**< True iff output is available.              */
    int opened;             /**< Generic PortMidi code, argument-checking.  */
    int mapper;             /**< True iff this device is a MIDI Mapper.     */
    int client;             /**< Provides the (ALSA) client number.         */
    int port;               /**< Provides the (ALSA) port number.           */

} PmDeviceInfo;

/**
 *  A type definition for a timer callback.
 */

typedef PmTimestamp (* PmTimeProcPtr) (void * time_info);

/**
 *  Provides an obvious declaration for PortMIDI queues.
 */

typedef void PmQueue;

/**
 *  All midi data comes in the form of PmEvent structures. A SysEx message is
 *  encoded as a sequence of PmEvent structures, with each structure carrying
 *  4 bytes of the message, i.e. only the first PmEvent carries the status
 *  byte.
 *
 *  Note that MIDI allows nested messages: the so-called "real-time" MIDI
 *  messages can be inserted into the MIDI byte stream at any location,
 *  including within a SysEx message. MIDI real-time messages are one-byte
 *  messages used mainly for timing (see the MIDI spec). PortMidi retains the
 *  order of non-real-time MIDI messages on both input and output, but it does
 *  not specify exactly how real-time messages are processed. This is
 *  particulary problematic for MIDI input, because the input parser must
 *  either prepare to buffer an unlimited number of SysEx message bytes or to
 *  buffer an unlimited number of real-time messages that arrive embedded in a
 *  long SysEx message. To simplify things, the input parser is allowed to
 *  pass real-time MIDI messages embedded within a SysEx message, and it is up
 *  to the client to detect, process, and remove these messages as they
 *  arrive.
 *
 *  When receiving SysEx messages, the SysEx message is terminated by either
 *  an EOX status byte (anywhere in the 4 byte messages) or by a non-real-time
 *  status byte in the low order byte of the message.  If you get a
 *  non-real-time status byte but there was no EOX byte, it means the SysEx
 *  message was somehow truncated. This is not considered an error; e.g., a
 *  missing EOX can result from the user disconnecting a MIDI cable during
 *  SysEx transmission.
 *
 *  A real-time message can occur within a SysEx message. A real-time message
 *  will always occupy a full PmEvent with the status byte in the low-order
 *  byte of the PmEvent message field. (This implies that the byte-order of
 *  SysEx bytes and real-time message bytes may not be preserved -- for
 *  example, if a real-time message arrives after 3 bytes of a SysEx message,
 *  the real-time message will be delivered first. The first word of the SysEx
 *  message will be delivered only after the 4th byte arrives, filling the
 *  4-byte PmEvent message field.
 *
 *  The timestamp field is observed when the output port is opened with a
 *  non-zero latency. A timestamp of zero means "use the current time", which
 *  in turn means to deliver the message with a delay of latency (the latency
 *  parameter used when opening the output port.) Do not expect PortMidi to
 *  sort data according to timestamps -- messages should be sent in the
 *  correct order, and timestamps MUST be non-decreasing. See also "Example"
 *  for Pm_OpenOutput() above.
 *
 *  A SysEx message will generally fill many PmEvent structures. On output to
 *  a PortMidiStream with non-zero latency, the first timestamp on SysEx
 *  message data will determine the time to begin sending the message.
 *  PortMidi implementations may ignore timestamps for the remainder of the
 *  SysEx message.
 *
 *  On input, the timestamp ideally denotes the arrival time of the status
 *  byte of the message. The first timestamp on SysEx message data will be
 *  valid. Subsequent timestamps may denote when message bytes were actually
 *  received, or they may be simply copies of the first timestamp.
 *
 *  Timestamps for nested messages: If a real-time message arrives in the
 *  middle of some other message, it is enqueued immediately with the
 *  timestamp corresponding to its arrival time. The interrupted non-real-time
 *  message or 4-byte packet of SysEx data will be enqueued later. The
 *  timestamp of interrupted data will be equal to that of the interrupting
 *  real-time message to insure that timestamps are non-decreasing.
 */

typedef struct
{
    PmMessage message;
    PmTimestamp timestamp;

} PmEvent;

/**
 *  Device enumeration mechanism.
 *  Device ids range from 0 to Pm_CountDevices()-1.
 */

typedef int PmDeviceID;

/**
 *  List of PortMIDI errors.
 *
 *  -   pmNoData is a "No error" return, also indicates no data available.
 *  -   pmGotData is a "No error" return, also indicates data available.
 *  -   pmInvalidDeviceId is an out of range or output device when input is
 *      requested or input device when output is requested or device is
 *      already opened.
 *  -   pmBadPtr means the PortMidiStream parameter is NULL, or stream is not
 *      opened, or stream is output when input is required, or stream is input
 *      when output is required.
 *  -   pmBadData means illegal MIDI data, e.g. missing EOX.
 *  -   pmBufferMaxSize means the buffer is already as large as it can be.
 *
 * Note:
 *
 *      If you add a new error type, be sure to update Pm_GetErrorText().
 */

typedef enum
{
    pmNoError = 0,
    pmNoData = 0,
    pmGotData = 1,
    pmHostError = -10000,
    pmInvalidDeviceId,
    pmInsufficientMemory,
    pmBufferTooSmall,
    pmBufferOverflow,
    pmBadPtr,
    pmBadData,
    pmInternalError,
    pmBufferMaxSize,
    pmDeviceClosed,
    pmDeviceOpen,
    pmWriteToInput,
    pmReadFromOutput,
    pmErrOther,

    /*
     * \note
     *      If you add a new error type here, be sure to update
     *      Pm_GetErrorText()!!
     */

    pmErrMax

} PmError;

extern int pm_initialized;              /* see note in portmidi.c           */

/*
 *  These are defined in system-specific file
 */

extern void * pm_alloc (size_t s);
extern void pm_free (void * ptr);

/*
 *  If an error occurs while opening or closing a midi stream, set these.
 */

extern int pm_hosterror;
extern char pm_hosterror_text [PM_HOST_ERROR_MSG_LEN];

struct pm_internal_struct;                  /* forward declaration  */

/*
 *  These do not use PmInternal because it is not defined yet....
 */

/**
 *
 */

typedef PmError (* pm_write_short_fn)
(
    struct pm_internal_struct * midi,
    PmEvent * buffer
);

/**
 *
 */

typedef PmError (* pm_begin_sysex_fn)
(
    struct pm_internal_struct * midi,
    PmTimestamp timestamp
);

/**
 *
 */

typedef PmError (* pm_end_sysex_fn)
(
    struct pm_internal_struct * midi,
    PmTimestamp timestamp
);

/**
 *
 */

typedef PmError (* pm_write_byte_fn)
(
    struct pm_internal_struct * midi,
    midibyte_t byte,
    PmTimestamp timestamp
);

/**
 *
 */

typedef PmError (* pm_write_realtime_fn)
(
    struct pm_internal_struct * midi,
    PmEvent * buffer
);

/**
 *
 */

typedef PmError (* pm_write_flush_fn)
(
    struct pm_internal_struct * midi,
     PmTimestamp timestamp
);

/**
 *
 */

typedef PmTimestamp (* pm_synchronize_fn) (struct pm_internal_struct * midi);

/**
 *  pm_open_fn() should clean up all memory and close the device if any part
 *  of the open fails.
 */

typedef PmError (* pm_open_fn)
(
    struct pm_internal_struct * midi,
    void * driverInfo
);

typedef PmError (* pm_abort_fn) (struct pm_internal_struct * midi);

/**
 *  pm_close_fn() should clean up all memory and close the device if any part
 *  of the close fails.
 */

typedef PmError (* pm_close_fn) (struct pm_internal_struct * midi);

/**
 *
 */

typedef PmError (* pm_poll_fn) (struct pm_internal_struct * midi);

/**
 *
 */

typedef void (* pm_host_error_fn)
(
    struct pm_internal_struct * midi,   // PmInternal * not defined until below
    char * msg,
    unsigned
);

/**
 *
 */

typedef unsigned (* pm_has_host_error_fn)
(
    struct pm_internal_struct * midi
);

/**
 *
 */

typedef struct
{
    pm_write_short_fn write_short;      /**< Output short MIDI msg.         */
    pm_begin_sysex_fn begin_sysex;      /**< Prepare to send SysEx message. */
    pm_end_sysex_fn end_sysex;          /**< Marks end of SysEx message.    */
    pm_write_byte_fn write_byte;        /**< Accumulate 1 more SysEx byte.  */
    pm_write_realtime_fn write_realtime; /**< Send real-time message in SysEx. */
    pm_write_flush_fn write_flush;      /**< Send accumulated unsent data.  */
    pm_synchronize_fn synchronize;      /**< Synch PM time to stream time.  */
    pm_open_fn open;                    /**< Open MIDI device.              */
    pm_abort_fn abort;                  /**< Abort.                         */
    pm_close_fn close;                  /**< Close the device.              */
    pm_poll_fn poll;                    /**< Read events into PM buffer.    */
    pm_has_host_error_fn has_host_error; /**< Device has host error message */
    pm_host_error_fn host_error; /**< Readable device error, clears/resets. */

} pm_fns_node, * pm_fns_type;

typedef struct
{
    /**
     *  Some portmidi state also saved in here (for autmatic
     *  device closing (see PmDeviceInfo struct).
     */

    PmDeviceInfo pub;

    /**
     *  ID number passed to win32 multimedia API open.
     */

    void * descriptor;

    /**
     *  Points to PmInternal device, allows automatic device closing.
     */

    void * internalDescriptor;

    pm_fns_type dictionary;

} descriptor_node, * descriptor_type;

/**
 *  When open fails, the dictionary gets this set of functions.
 */

extern pm_fns_node pm_none_dictionary;
extern int pm_descriptor_max;
extern descriptor_type pm_descriptors;
extern int pm_descriptor_index;

/**
 *
 */

typedef uint32_t (* time_get_proc_type) (void * time_info);

/**
 *
 */

typedef struct pm_internal_struct
{
    /**
     *  which device is open (index to descriptors).
     */

    int device_id;

    /**
     *  MIDI_IN, or MIDI_OUT.
     */

    short write_flag;

    /**
     *  where to get the time.
     */

    PmTimeProcPtr time_proc;

    /**
     *  pass this to get_time().
     */

    void * time_info;

    /**
     *  how big is the buffer or queue?.
     */

    int32_t buffer_len;

    /**
     *
     */

    PmQueue * queue;

    /**
     *  Time delay in ms between timestamps and actual output set to zero to
     *  get immediate, simple blocking output if latency is zero, timestamps
     *  will be ignored; if midi input device, this field ignored.
     */

    int32_t latency;

    /**
     *  When SysEx status is seen, this flag becomes true until EOX is seen.
     *  When true, new data is appended to the stream of outgoing bytes. When
     *  overflow occurs, SysEx data is dropped (until an EOX or non-real-timei
     *  status byte is seen) so that, if the overflow condition is cleared, we
     *  don't start sending data from the middle of a SysEx message. If a
     *  SysEx message is filtered, sysex_in_progress is false, causing the
     *  message to be dropped.
     */

    int sysex_in_progress;

    /**
     *  buffer for 4 bytes of SysEx data.
     */

    PmMessage sysex_message;

    /**
     *  how many bytes in sysex_message so far.
     */

    int sysex_message_count;

    /**
     *  flags that filter incoming message classes.
     */

    int32_t filters;

    /**
     *  filter incoming messages based on channel.
     */

    int32_t channel_mask;

    /**
     *  timestamp of last message.
     */

    PmTimestamp last_msg_time;

    /**
     *  time of last synchronization.
     */

    PmTimestamp sync_time;

    /**
     *  set by PmWrite to current time.
     */

    PmTimestamp now;

    /**
     *  initially true, used to run first synchronization.
     */

    int first_message;

    /**
     *  implementation functions.
     */

    pm_fns_type dictionary;

    /**
     *  system-dependent state.
     */

    void * descriptor;

    /*
     *  The following are used to expedite SysEx data on Windows, in debug
     *  mode. Based on some profiling, these optimizations cut the time to
     *  process SysEx bytes from about 7.5 to 0.26 usec/byte, but this does
     *  not count time in the driver, so I don't know if it is important.
     */

    /**
     *  addr of ptr to SysEx data.
     */

    midibyte_t * fill_base;

    /**
     *  offset of next SysEx byte.
     */

    uint32_t * fill_offset_ptr;

    /**
     *  how many SysEx bytes to write.
     */

    uint32_t fill_length;                   /* changed from int32_t */

} PmInternal;

/* defined by system specific implementation, e.g. pmwinmm, used by PortMidi */

extern void pm_init (void);
extern void pm_term (void);

/* defined by portMidi, used by pmwinmm */

extern PmError none_write_short (PmInternal * midi, PmEvent * buffer);
extern PmError none_write_byte
(
    PmInternal * midi,
    midibyte_t byte,
    PmTimestamp timestamp
);
extern PmTimestamp none_synchronize (PmInternal * midi);
extern PmError pm_fail_fn (PmInternal * midi);
extern PmError pm_fail_timestamp_fn (PmInternal * midi, PmTimestamp timestamp);
extern PmError pm_success_fn (PmInternal * midi);
extern PmError pm_add_device
(
    char * interf, char * name, int input,
    void * descriptor, pm_fns_type dictionary,
    int client, int port                        /* new values, TODO     */
);
extern uint32_t pm_read_bytes
(
    PmInternal * midi, const midibyte_t * data,
    int len, PmTimestamp timestamp
);
extern void pm_read_short (PmInternal * midi, PmEvent * event);

extern int pm_find_default_device (char * pattern, int is_input);

#ifdef __cplusplus
}           // extern "C"
#endif

#endif      // SEQ64_PMINTERNAL_H

/*
 * pminternal.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

