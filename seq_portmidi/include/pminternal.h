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
 * \library       sequencer64 application
 * \author        PortMIDI team; modifications by Chris Ahlstrom
 * \date          2017-08-21
 * \updates       2017-08-29
 * \license       GNU GPLv2 or above
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

#ifdef __cplusplus
extern "C"
{
#endif

/**
 *  Provides an obvious declaration for PortMIDI queues.
 */

typedef void PmQueue;

extern int pm_initialized;                  /* see note in portmidi.c */

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

typedef PmError (* pm_write_short_fn)
(
    struct pm_internal_struct * midi,
    PmEvent * buffer
);

typedef PmError (* pm_begin_sysex_fn)
(
    struct pm_internal_struct * midi,
    PmTimestamp timestamp
);

typedef PmError (* pm_end_sysex_fn)
(
    struct pm_internal_struct * midi,
    PmTimestamp timestamp
);

typedef PmError (* pm_write_byte_fn)
(
    struct pm_internal_struct * midi,
    unsigned char byte,
    PmTimestamp timestamp
);

typedef PmError (* pm_write_realtime_fn)
(
    struct pm_internal_struct * midi,
    PmEvent * buffer
);

typedef PmError (* pm_write_flush_fn)
(
    struct pm_internal_struct * midi,
     PmTimestamp timestamp
);

typedef PmTimestamp (* pm_synchronize_fn) (struct pm_internal_struct * midi);

/*
 * pm_open_fn() should clean up all memory and close the device if any part
 * of the open fails.
 */

typedef PmError (* pm_open_fn)
(
    struct pm_internal_struct * midi,
    void * driverInfo
);

typedef PmError (* pm_abort_fn) (struct pm_internal_struct * midi);

/*
 * pm_close_fn() should clean up all memory and close the device if any
 * part of the close fails.
 */

typedef PmError (* pm_close_fn) (struct pm_internal_struct * midi);

typedef PmError (* pm_poll_fn) (struct pm_internal_struct * midi);

typedef void (* pm_host_error_fn)
(
    struct pm_internal_struct * midi,
    char * msg,
    int len
);

typedef unsigned int (* pm_has_host_error_fn)
(
    struct pm_internal_struct * midi
);

typedef struct
{
    pm_write_short_fn write_short;          /* output short MIDI msg */
    pm_begin_sysex_fn begin_sysex;          /* prepare to send sysex message */
    pm_end_sysex_fn end_sysex;              /* marks end of sysex message */
    pm_write_byte_fn write_byte;            /* accumulate one more sysex byte */
    pm_write_realtime_fn write_realtime;    /* send real-time message in sysex */
    pm_write_flush_fn write_flush;          /* send accumulated unsent data */
    pm_synchronize_fn synchronize;          /* synch PM time to stream time */
    pm_open_fn open;                        /* open MIDI device */
    pm_abort_fn abort;                      /* abort */
    pm_close_fn close;                      /* close device */
    pm_poll_fn poll;                        /* read events into PM buffer */
    pm_has_host_error_fn has_host_error;    /* device has host error message */
    pm_host_error_fn host_error;            /* readable error message for device (clears and resets) */
} pm_fns_node, * pm_fns_type;

/*
 *  When open fails, the dictionary gets this set of functions.
 */

extern pm_fns_node pm_none_dictionary;

typedef struct
{
    /*
     *  Some portmidi state also saved in here (for autmatic
     *  device closing (see PmDeviceInfo struct).
     */

    PmDeviceInfo pub;

    /*
     *  ID number passed to win32 multimedia API open.
     */

    void * descriptor;

    /*
     *  Points to PmInternal device, allows automatic device closing.
     */

    void * internalDescriptor;

    pm_fns_type dictionary;

} descriptor_node, * descriptor_type;

extern int pm_descriptor_max;
extern descriptor_type descriptors;
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
    /* which device is open (index to descriptors) */

    int device_id;

    /* MIDI_IN, or MIDI_OUT */

    short write_flag;

    /* where to get the time */

    PmTimeProcPtr time_proc;

    /* pass this to get_time() */

    void * time_info;

    /* how big is the buffer or queue? */

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
     *  When sysex status is seen, this flag becomes true until EOX is seen.
     *  When true, new data is appended to the stream of outgoing bytes. When
     *  overflow occurs, sysex data is dropped (until an EOX or non-real-timei
     *  status byte is seen) so that, if the overflow condition is cleared, we
     *  don't start sending data from the middle of a sysex message. If a sysex
     *  message is filtered, sysex_in_progress is false, causing the message to
     *  be dropped.
     */

    int sysex_in_progress;

    /* buffer for 4 bytes of sysex data */

    PmMessage sysex_message;

    /* how many bytes in sysex_message so far */

    int sysex_message_count;

    /* flags that filter incoming message classes */

    int32_t filters;

    /* filter incoming messages based on channel */

    int32_t channel_mask;

    /* timestamp of last message */

    PmTimestamp last_msg_time;

    /* time of last synchronization */

    PmTimestamp sync_time;

    /* set by PmWrite to current time */

    PmTimestamp now;

    /* initially true, used to run first synchronization */

    int first_message;

    /* implementation functions */

    pm_fns_type dictionary;

    /* system-dependent state */

    void * descriptor;

    /* the following are used to expedite sysex data
     * on windows, in debug mode, based on some profiling, these optimizations
     * cut the time to process sysex bytes from about 7.5 to 0.26 usec/byte,
     * but this does not count time in the driver, so I don't know if it is
     * important
     */

    /* addr of ptr to sysex data */

    unsigned char * fill_base;

    /* offset of next sysex byte */

    uint32_t * fill_offset_ptr;

    /* how many sysex bytes to write */

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
    unsigned char byte,
    PmTimestamp timestamp
);
extern PmTimestamp none_synchronize (PmInternal * midi);
extern PmError pm_fail_fn (PmInternal * midi);
extern PmError pm_fail_timestamp_fn (PmInternal * midi, PmTimestamp timestamp);
extern PmError pm_success_fn (PmInternal * midi);
extern PmError pm_add_device
(
    char * interf, char * name, int input,
    void * descriptor, pm_fns_type dictionary
);
extern uint32_t pm_read_bytes
(
    PmInternal * midi, const unsigned char * data,
    int len, PmTimestamp timestamp
);
extern void pm_read_short (PmInternal * midi, PmEvent * event);

#define none_write_flush    pm_fail_timestamp_fn
#define none_sysex          pm_fail_timestamp_fn
#define none_poll           pm_fail_fn
#define success_poll        pm_success_fn

#define MIDI_REALTIME_MASK  0xf8

#define is_real_time(msg) \
    ((Pm_MessageStatus(msg) & MIDI_REALTIME_MASK) == MIDI_REALTIME_MASK)

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

