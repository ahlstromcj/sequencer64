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
 * \file        pmwinmm.c
 *
 *      System specific definitions for the Windows MM API.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-05-27
 * \license     GNU GPLv2 or above
 *
 *  Check out this site:
 *
 *      http://donyaquick.com/midi-on-windows/
 *      "Working with MIDI on Windows (Outside of a DAW)"
 */

#ifdef _MSC_VER
#pragma warning(disable: 4133)      // stop warnings about implicit typecasts
#endif

/*
 *  Without this #define, InitializeCriticalSectionAndSpinCount is undefined.
 *  This version level means "Windows 2000 and higher".
 */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <mmsystem.h>
#include <string.h>

#include "easy_macros.h"
#include "portmidi.h"                   /* UNUSED() macro, and PortMidi API */
#include "pmerrmm.h"                    /* error-message support, debugging */
#include "pmutil.h"
#include "pmwinmm.h"
#include "porttime.h"

/*
 *  Asserts are used to verify PortMidi code logic is sound; later may want
 *  something more graceful.
 */

#include <assert.h>

/*
 *  WinMM API callback routines.
 */

static void CALLBACK winmm_in_callback
(
    HMIDIIN hMidiIn,
    WORD wMsg, DWORD_PTR dwInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2
);

static void CALLBACK winmm_streamout_callback
(
    HMIDIOUT hmo, UINT wMsg,
    DWORD_PTR dwInstance, DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

#ifdef USE_SYSEX_BUFFERS
static void CALLBACK winmm_out_callback
(
    HMIDIOUT hmo, UINT wMsg,
    DWORD_PTR dwInstance, DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);
#endif

static void winmm_out_delete (PmInternal * midi);

extern pm_fns_node pm_winmm_in_dictionary;
extern pm_fns_node pm_winmm_out_dictionary;

/**
 * \note
 *      WinMM seems to hold onto buffers longer than one would expect, e.g.
 *      when I tried using 2 small buffers to send long SysEx messages, at
 *      some point WinMM held both buffers. This problem was fixed by making
 *      buffers bigger. Therefore, it seems that there should be enough buffer
 *      space to hold a whole SysEx message.
 *
 *  The bufferSize passed into Pm_OpenInput (passed into here as buffer_len)
 *  will be used to estimate the largest SysEx message (= buffer_len * 4
 *  bytes).  Call that the max_sysex_len = buffer_len * 4.
 *
 *  For simple midi output (latency == 0), allocate 3 buffers, each with half
 *  the size of max_sysex_len, but each at least 256 bytes.
 *
 *  For stream output, there will already be enough space in very short
 *  buffers, so use them, but make sure there are at least 16.
 *
 *  For input, use many small buffers rather than 2 large ones so that when
 *  there are short SysEx messages arriving frequently (as in control surfaces)
 *  there will be more free buffers to fill. Use max_sysex_len / 64 buffers,
 *  but at least 16, of size 64 bytes each.
 *
 *  The following constants help to represent these design parameters.
 */

#define NUM_SIMPLE_SYSEX_BUFFERS      3
#define MIN_SIMPLE_SYSEX_LEN        256
#define MIN_STREAM_BUFFERS           16
#define STREAM_BUFFER_LEN            24
#define INPUT_SYSEX_LEN              64
#define MIN_INPUT_BUFFERS            16

/**
 *  If we run out of space for output (assuming this is due to a SysEx
 *  message), then expand the output buffer by up to NUM_EXPANSION_BUFFERS, in
 *  increments of EXPANSION_BUFFER_LEN.
 */

#define NUM_EXPANSION_BUFFERS       128
#define EXPANSION_BUFFER_LEN       1024

/**
 *  A SysEx buffer has 3 DWORDS as a header plus the actual message size.
 *
 * \warning
 *      The size was assumed to be long, we changed it to DWORD.
 */

#define MIDIHDR_SYSEX_BUFFER_LENGTH(x) ((x) + sizeof(DWORD) * 3)

/**
 *  A MIDIHDR with a SysEx message is the buffer length plus the header size.
 */

#define MIDIHDR_SYSEX_SIZE(x) (MIDIHDR_SYSEX_BUFFER_LENGTH(x) + sizeof(MIDIHDR))

/**
 *  Size of a MIDIHDR with a buffer contaning multiple MIDIEVENT structures.
 */

#ifdef USE_SYSEX_BUFFERS
#define MIDIHDR_SIZE(x) ((x) + sizeof(MIDIHDR))
#endif

/**
 *  win32 mmedia system specific structure passed to midi callbacks;
 *  global winmm device info
 */

static MIDIINCAPS * midi_in_caps = nullptr;
static MIDIINCAPS midi_in_mapper_caps;
static UINT midi_num_inputs = 0;

static MIDIOUTCAPS * midi_out_caps = nullptr;
static MIDIOUTCAPS midi_out_mapper_caps;
static UINT midi_num_outputs = 0;

/**
 *  This structure provides per-device information.
 */

typedef struct midiwinmm_struct
{
    union
    {
        HMIDISTRM stream;   /**< Windows handle for stream.                 */
        HMIDIOUT out;       /**< Windows handle for out calls.              */
        HMIDIIN in;         /**< Windows handle for in calls.               */

    } handle;

    /**
     * MIDI output messages are sent in these buffers, which are allocated
     * in a round-robin fashion, using next_buffer as an index.
     */

    LPMIDIHDR * buffers;    /**< Pool of buffers for midi in or out data.   */
    int max_buffers;        /**< Length of buffers array.                   */
    int buffers_expanded;   /**< Buffers array expanded for extra messages? */
    int num_buffers;        /**< How many buffers allocated in the array.   */
    int next_buffer;        /**< Index of next buffer to send.              */
    HANDLE buffer_signal;   /**< Used to wait for buffer to become free.    */

    /*
     * SysEx buffers will be allocated only when
     * a SysEx message is sent. The size of the buffer is fixed.
     */

#ifdef USE_SYSEX_BUFFERS
    LPMIDIHDR sysex_buffers[NUM_SYSEX_BUFFERS]; /**< Pool for SysEx data.   */
    int next_sysex_buffer;      /**< Index of next SysEx buffer to send.    */
#endif

    unsigned long last_time;    /**< Last output time.                      */
    int first_message;          /**< Flag: treat first message differently. */
    int sysex_mode;             /**< Middle of sending SysEx.               */
    unsigned long sysex_word;   /**< Accumulate data when receiving SysEx.  */
    unsigned sysex_byte_count;  /**< Count how many SysEx bytes received.   */
    LPMIDIHDR hdr;              /**< Message accumulating SysEx to send (?) */
    unsigned long sync_time;    /**< When did we last determine delta?      */
    long delta;                 /**< Stream time minus real time.           */
    int error;                  /**< Host error from doing port MIDI call.  */
    CRITICAL_SECTION lock;      /**< Prevents reentrant callbacks (input).  */

} midiwinmm_node, * midiwinmm_type;

/**
 * general MIDI device queries
 */

static void
pm_winmm_general_inputs (void)
{
    UINT i;
    midi_num_inputs = midiInGetNumDevs();
    midi_in_caps = midi_num_inputs > 0 ?
        (MIDIINCAPS *) pm_alloc(sizeof(MIDIINCAPS) * midi_num_inputs) :
        nullptr
        ;

    if (is_nullptr(midi_in_caps))
    {
        /*
         * If you can't open a particular system-level midi interface (such as
         * winmm), we just consider that system or API to be unavailable and
         * move on without reporting an error.
         */

        printf("pm_winmm_general_inputs(): no input devices\n");
    }
    else
    {
        for (i = 0; i < midi_num_inputs; ++i)
        {
            WORD winerrcode = midiInGetDevCaps
            (
                i, (LPMIDIINCAPS) & midi_in_caps[i], sizeof(MIDIINCAPS)
            );
            if (winerrcode == MMSYSERR_NOERROR)
            {
                (void) pm_add_device
                (
                    "MMSystem", (char *) midi_in_caps[i].szPname, TRUE,
                    (void *) i, &pm_winmm_in_dictionary,
                    i, 0                            /* client/port, TODO    */
                );
            }
            else
            {
                /*
                 * Ignore errors here. If pm_descriptor_max is exceeded, some
                 * devices will not be accessible.
                 */

#if defined PLATFORM_DEBUG
                const char * errmsg = midi_io_get_dev_caps_error
                (
                    (const char *) midi_in_caps[i].szPname,
                    "general in : midiInGetDevCaps", winerrcode
                );
                printf("[%d] '%s'\n", i, errmsg);
#endif
            }
        }
    }
}

/**
 * \note
 *      If MIDIMAPPER opened as input (then documentation implies you can, but
 *      the current system fails to retrieve input mapper capabilities), then
 *      we still should retrieve some form of setup information.
 */

static void
pm_winmm_mapper_input (void)
{
    WORD winerrcode = midiInGetDevCaps
    (
        (UINT) MIDIMAPPER, (LPMIDIINCAPS) & midi_in_mapper_caps,
        sizeof(MIDIINCAPS)
    );
    if (winerrcode == MMSYSERR_NOERROR)
    {
        const char * devname = (const char *) midi_in_mapper_caps.szPname;
        pm_add_device
        (
            "MMSystem", (char *) devname, TRUE,
            (void *) MIDIMAPPER, &pm_winmm_in_dictionary,
            0, 0                                    /* client/port, TODO    */
        );
    }
    else
    {
#if defined PLATFORM_DEBUG
        const char * devname = (const char *) midi_in_mapper_caps.szPname;
        if (strlen(devname) == 0)
            devname = "MIDIMAPPER";

        const char * errmsg = midi_io_get_dev_caps_error
        (
            devname, "mapper in : midiInGetDevCaps", winerrcode
        );
        printf("[%s] '%s'\n", devname, errmsg);
#endif
    }
}

/**
 *  No error is reported, see pm_winmm_general_inputs().
 */

static void
pm_winmm_general_outputs (void)
{
    UINT i;
    midi_num_outputs = midiOutGetNumDevs();
    midi_out_caps = pm_alloc(sizeof(MIDIOUTCAPS) * midi_num_outputs);
    if (is_nullptr(midi_out_caps))
        return;

    for (i = 0; i < midi_num_outputs; ++i)
    {
        DWORD winerrcode = midiOutGetDevCaps
        (
            i, (LPMIDIOUTCAPS) & midi_out_caps[i], sizeof(MIDIOUTCAPS)
        );
        if (winerrcode == MMSYSERR_NOERROR)
        {
            pm_add_device
            (
                "MMSystem", (char *) midi_out_caps[i].szPname, FALSE,
                (void *) i, &pm_winmm_out_dictionary,
                i, 0                                /* client/port, TODO    */
            );
        }
        else
        {
#if defined PLATFORM_DEBUG
            const char * devname = (const char *) midi_out_caps[i].szPname;
            const char * errmsg = midi_io_get_dev_caps_error
            (
                devname, "general : midiOutGetDevCaps", winerrcode
            );
            printf("[%d (%s) '%s'\n", i, devname, errmsg);
#endif
        }
    }
}

/**
 * \note
 *      If MIDIMAPPER opened as output (a pseudo MIDI device maps
 *      device-independent messages into device dependant ones, via the
 *      Windows NT midimapper program), we still should get some setup
 *      information.
 */

static void
pm_winmm_mapper_output (void)
{
    WORD winerrcode = midiOutGetDevCaps
    (
        (UINT) MIDIMAPPER, (LPMIDIOUTCAPS) &midi_out_mapper_caps,
        sizeof(MIDIOUTCAPS)
    );
    if (winerrcode == MMSYSERR_NOERROR)
    {
        pm_add_device
        (
            "MMSystem", (char *) midi_out_mapper_caps.szPname, FALSE,
            (void *) MIDIMAPPER, &pm_winmm_out_dictionary,
            0, 0                                    /* client/port, TODO    */
        );
    }
    else
    {
#if defined PLATFORM_DEBUG
        const char * devname = (const char *) midi_out_mapper_caps.szPname;
        const char * errmsg = midi_io_get_dev_caps_error
        (
            devname, "mapper out : midiOutGetDevCaps", winerrcode
        );
        printf("[%s] %s\n", devname, errmsg);
#endif
    }
}

/*
 * Host error handling.
 */

/**
 *
 */

static unsigned
winmm_has_host_error (PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    return m->error;
}

/**
 *  str_copy_len() is like strcat, but it won't overrun the destination
 *  string.  Just in case the suffix is greater then len, terminate with
 *  zero.
 *
 * \return
 *      Length of resulting string.
 */

static int
str_copy_len (char * dst, char * src, int len)
{
    strncpy(dst, src, len);
    dst[len - 1] = 0;
    return strlen(dst);
}

/**
 *  Precondition: midi != null.
 */

static void
winmm_get_host_error (PmInternal * midi, char * msg, UINT len)
{
    midiwinmm_node * m = nullptr;
    char * hdr1 = "Host error: ";
    if (not_nullptr(midi))
        m = (midiwinmm_node *) midi->descriptor;

    if (not_nullptr(msg))
        msg[0] = 0;                     /* set the result string to empty   */

    if (not_nullptr_2(m, msg))
    {
        if (pm_descriptors[midi->device_id].pub.input)
        {
            /*
             * Input and output use different WinMM API calls.  Make sure
             * there is an open device (m) to examine. If there is an error,
             * then read and record the host error.  Note that the error codes
             * returned by the get-error-text functions are for that function.
             * We disable those asserts here.
             */

            if (m->error != MMSYSERR_NOERROR)
            {
                int n = str_copy_len(msg, hdr1, len);
                int err = midiInGetErrorText(m->error, msg + n, len - n);
                if (err == MMSYSERR_NOERROR)    // assert(err == MMSYSERR_NOERROR)
                    m->error = MMSYSERR_NOERROR;
            }
        }
        else                            /* output port                      */
        {
            if (m->error != MMSYSERR_NOERROR)
            {
                int n = str_copy_len(msg, hdr1, len);
                int err = midiOutGetErrorText(m->error, msg + n, len - n);
                if (err == MMSYSERR_NOERROR)    // assert(err == MMSYSERR_NOERROR)
                    m->error = MMSYSERR_NOERROR;
            }
        }
    }
}

/**
 *  Buffer handling.
 */

static MIDIHDR *
allocate_buffer (long data_size)
{
    LPMIDIHDR hdr = nullptr;
    if (data_size > 0)
    {
        hdr = (LPMIDIHDR) pm_alloc(MIDIHDR_SYSEX_SIZE(data_size));
        if (not_nullptr(hdr))
        {
            MIDIEVENT * evt = (MIDIEVENT *)(hdr + 1);   /* placed after header */
            hdr->lpData = (LPSTR) evt;
            hdr->dwBufferLength = MIDIHDR_SYSEX_BUFFER_LENGTH(data_size);
            hdr->dwBytesRecorded = 0;
            hdr->dwFlags = 0;
            hdr->dwUser = hdr->dwBufferLength;
        }
    }
    return hdr;
}

#ifdef USE_SYSEX_BUFFERS

/**
 * we're actually allocating more than data_size because the buffer
 * will include the MIDIEVENT header in addition to the data
 */

static MIDIHDR *
allocate_sysex_buffer (long data_size)
{
    LPMIDIHDR hdr = (LPMIDIHDR) pm_alloc(MIDIHDR_SYSEX_SIZE(data_size));
    MIDIEVENT * evt;
    if (not_nullptr(hdr))
    {
        MIDIEVENT * evt = (MIDIEVENT *)(hdr + 1);   /* placed after header */
        hdr->lpData = (LPSTR) evt;
        hdr->dwFlags = 0;
        hdr->dwUser = 0;
    }
    return hdr;
}

#endif  // USE_SYSEX_BUFFERS

/**
 *  Buffers is an array of 'count' pointers to an IDIHDR/MIDIEVENT struct.
 */

static PmError
allocate_buffers (midiwinmm_type m, long data_size, long count)
{
    int i;
    m->num_buffers = 0;             /* in case no memory can be allocated   */
    m->buffers = (LPMIDIHDR *) pm_alloc(sizeof(LPMIDIHDR) * count);
    if (is_nullptr(m->buffers))
        return pmInsufficientMemory;

    m->max_buffers = count;
    for (i = 0; i < count; ++i)
    {
        LPMIDIHDR hdr = allocate_buffer(data_size);
        if (is_nullptr(hdr))        /* free all allocations and return      */
        {
            for (i = i - 1; i >= 0; --i)
                pm_free(m->buffers[i]);

            pm_free(m->buffers);    /* TODO: zero out the pointers          */
            m->max_buffers = 0;
            return pmInsufficientMemory;
        }
        m->buffers[i] = hdr;        /* this may be null if allocation fails */
    }
    m->num_buffers = count;
    return pmNoError;
}

#ifdef USE_SYSEX_BUFFERS

/**
 *
 * sysex_buffers is an array of count pointers to MIDIHDR/MIDIEVENT struct
 */

static PmError
allocate_sysex_buffers (midiwinmm_type m, long data_size)
{
    PmError rslt = pmNoError;
    int i;
    for (i = 0; i < NUM_SYSEX_BUFFERS; ++i)
    {
        LPMIDIHDR hdr = allocate_sysex_buffer(data_size);
        if (is_nullptr(hdr))
            rslt = pmInsufficientMemory;

        m->sysex_buffers[i] = hdr;  /* may be null if allocation fails */
        if (not_nullptr(hdr))
            hdr->dwFlags = 0;           /* mark as free */
    }
    return rslt;
}

/**
 *
 */

static LPMIDIHDR
get_free_sysex_buffer (PmInternal * midi)
{
    LPMIDIHDR r = nullptr;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    if (! m->sysex_buffers[0])
    {
        if (allocate_sysex_buffers(m, SYSEX_BYTES_PER_BUFFER))
            return nullptr;
    }
    for (;;)                            /* busy wait to find a free buffer  */
    {
        int i;
        for (i = 0; i < NUM_SYSEX_BUFFERS; ++i)
        {
            /* cycle through buffers, modulo NUM_SYSEX_BUFFERS */

            m->next_sysex_buffer++;
            if (m->next_sysex_buffer >= NUM_SYSEX_BUFFERS)
                m->next_sysex_buffer = 0;

            r = m->sysex_buffers[m->next_sysex_buffer];
            if ((r->dwFlags & MHDR_PREPARED) == 0)
                goto found_sysex_buffer;
        }

        /* after scanning every buffer and not finding anything, block */

        if (WaitForSingleObject(m->buffer_signal, 1000) == WAIT_TIMEOUT)
        {
#if defined PLATFORM_DEBUG
            printf
            (
                "PortMidi warning: get_free_sysex_buffer() wait timed "
                "out after 1000ms\n"
            );
#endif
        }
    }

found_sysex_buffer:

    r->dwBytesRecorded = 0;
    r->dwBufferLength = 0; /* changed to correct value later */
    return r;
}

#endif      // USE_SYSEX_BUFFERS

/**
 *
 */

static LPMIDIHDR
get_free_output_buffer (PmInternal * midi)
{
    LPMIDIHDR r = nullptr;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    for (;;)
    {
        int i;
        for (i = 0; i < m->num_buffers; ++i)
        {
            /* cycle through buffers, modulo m->num_buffers */

            m->next_buffer++;
            if (m->next_buffer >= m->num_buffers)
                m->next_buffer = 0;

            r = m->buffers[m->next_buffer];
            if ((r->dwFlags & MHDR_PREPARED) == 0)
                goto found_buffer;
        }

        /* after scanning every buffer and not finding anything, block */

        if (WaitForSingleObject(m->buffer_signal, 1000) == WAIT_TIMEOUT)
        {
#if defined PLATFORM_DEBUG
            printf
            (
                "PortMidi warning: get_free_output_buffer() "
                " wait timed out after 1000ms\n"
            );
#endif
            /*
             * If we're trying to send a sysex message, maybe the message is
             * too big and we need more message buffers.  Expand the buffer
             * pool by 128KB using 1024-byte buffers.  First, expand the
             * buffers array if necessary.
             */

            if (! m->buffers_expanded)
            {
                LPMIDIHDR * new_buffers = (LPMIDIHDR *) pm_alloc
                (
                    (m->num_buffers + NUM_EXPANSION_BUFFERS) * sizeof(LPMIDIHDR)
                );

                /*
                 * If no memory, we could return a no-memory error, but user
                 * probably will be unprepared to deal with it. Maybe the MIDI
                 * driver is temporarily hung so we should just wait.  I don't
                 * know the right answer, but waiting is easier.
                 */

                if (is_nullptr(new_buffers))
                    continue;

                /*
                 * Copy buffers to new_buffers and replace buffers.
                 */

                memcpy
                (
                    new_buffers, m->buffers, m->num_buffers * sizeof(LPMIDIHDR)
                );
                pm_free(m->buffers);
                m->buffers = new_buffers;
                m->max_buffers = m->num_buffers + NUM_EXPANSION_BUFFERS;
                m->buffers_expanded = TRUE;
            }

            /*
             * Next, add one buffer and return it.
             */

            if (m->num_buffers < m->max_buffers)
            {
                r = allocate_buffer(EXPANSION_BUFFER_LEN);

                /*
                 * If no memory, we might not be dead; maybe the system is
                 * hung and we can wait longer for a message buffer.
                 */

                if (is_nullptr(r))
                    continue;

                m->buffers[m->num_buffers++] = r;
                goto found_buffer;                  /* break out of 2 loops */
            }

            /*
             * Otherwise, we've allocated all NUM_EXPANSION_BUFFERS buffers,
             * and we have no free buffers to send. We'll just keep polling
             * to see if any buffers show up.
             */
        }
    }

found_buffer:

    /*
     * Actual buffer length is saved in dwUser field.
     */

    r->dwBytesRecorded = 0;
    r->dwBufferLength = (DWORD) r->dwUser;
    return r;
}

#ifdef EXPANDING_SYSEX_BUFFERS

/*
 * Note:
 *
 *      This is not working code, but might be useful if you want to grow
 *      sysex buffers.
 */

static PmError
resize_sysex_buffer (PmInternal * midi, long old_size, long new_size)
{
    LPMIDIHDR big;
    int i;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;

    /* buffer must be smaller than 64k, but be also a multiple of 4 */

    if (new_size > 65520)
    {
        if (old_size >= 65520)
            return pmBufferMaxSize;
        else
            new_size = 65520;
    }

    /* allocate a bigger message  */

    big = allocate_sysex_buffer(new_size);
    if (is_nullptr(big))
        return pmInsufficientMemory;

    m->error = midiOutPrepareHeader(m->handle.out, big, sizeof(MIDIHDR));
    if (m->error)
    {
        pm_free(big);
        return pmHostError;
    }

    /* make sure we're not going to overwrite any memory */

    assert(old_size <= new_size);
    if (old_size <= new_size)
        memcpy(big->lpData, m->hdr->lpData, old_size);

    /* keep track of how many sysex bytes are in message so far */

    big->dwBytesRecorded = m->hdr->dwBytesRecorded;
    big->dwBufferLength = new_size;

    /* find which buffer this was, and replace it */

    for (i = 0; i < NUM_SYSEX_BUFFERS; ++i)
    {
        if (m->sysex_buffers[i] == m->hdr)
        {
            m->sysex_buffers[i] = big;
            m->sysex_buffer_size[i] = new_size;
            pm_free(m->hdr);
            m->hdr = big;
            break;
        }
    }
    assert(i != NUM_SYSEX_BUFFERS);
    return pmNoError;
}

#endif

/*
 * begin midi input implementation
 */

static PmError
allocate_input_buffer (HMIDIIN h, long buffer_len)
{
    LPMIDIHDR hdr = allocate_buffer(buffer_len);
    if (is_nullptr(hdr))
        return pmInsufficientMemory;

    pm_hosterror = midiInPrepareHeader(h, hdr, sizeof(MIDIHDR));
    if (pm_hosterror)
    {
        pm_free(hdr);
        return pm_hosterror;
    }
    pm_hosterror = midiInAddBuffer(h, hdr, sizeof(MIDIHDR));
    return pm_hosterror;
}


/**
 *
 */

static PmError
winmm_in_open (PmInternal * midi, void * driverInfo)
{
    int i = midi->device_id;
    int max_sysex_len = midi->buffer_len * 4;
    int num_input_buffers = max_sysex_len / INPUT_SYSEX_LEN;
    midiwinmm_type m;
    DWORD dwDevice = (DWORD) pm_descriptors[i].descriptor;

    /* create system dependent device data */

    m = (midiwinmm_type) pm_alloc(sizeof(midiwinmm_node));      /* create */
    midi->descriptor = m;
    if (is_nullptr(m))
        goto no_memory;

    m->handle.in = nullptr;
    m->buffers = nullptr;               /* not used for input */
    m->num_buffers = 0;                 /* not used for input */
    m->max_buffers = FALSE;             /* not used for input */
    m->buffers_expanded = 0;            /* not used for input */
    m->next_buffer = 0;                 /* not used for input */
    m->buffer_signal = 0;               /* not used for input */

#ifdef USE_SYSEX_BUFFERS
    for (i = 0; i < NUM_SYSEX_BUFFERS; ++i)
        m->sysex_buffers[i] = nullptr;  /* not used for input */

    m->next_sysex_buffer = 0;           /* not used for input */
#endif

    m->last_time = 0;
    m->first_message = TRUE;            /* not used for input */
    m->sysex_mode = FALSE;
    m->sysex_word = 0;
    m->sysex_byte_count = 0;
    m->hdr = nullptr;                   /* not used for input */
    m->sync_time = 0;
    m->delta = 0;
    m->error = MMSYSERR_NOERROR;

    /*
     * 4000 is based on Windows documentation -- that's the value used in the
     * memory manager. It's small enough that it should not hurt performance
     * even if it's not optimal.
     */

    InitializeCriticalSectionAndSpinCount(&m->lock, 4000);

    /* open device */

    pm_hosterror = midiInOpen
    (
        &(m->handle.in),                /* input device handle */
        dwDevice,                       /* device ID */
        (DWORD_PTR) winmm_in_callback,  /* callback address */
        (DWORD_PTR) midi,               /* callback instance data */
        CALLBACK_FUNCTION               /* callback is a procedure */
    );
    if (pm_hosterror)
        goto free_descriptor;

    if (num_input_buffers < MIN_INPUT_BUFFERS)
        num_input_buffers = MIN_INPUT_BUFFERS;

    for (i = 0; i < num_input_buffers; ++i)
    {
        if (allocate_input_buffer(m->handle.in, INPUT_SYSEX_LEN))
        {
            /*
             * Either pm_hosterror was set, or the proper return code is
             * pmInsufficientMemory.
             */

            goto close_device;
        }
    }
    pm_hosterror = midiInStart(m->handle.in);       /* start device */
    if (pm_hosterror)
        goto reset_device;

    return pmNoError;

    /* undo steps leading up to the detected error */

reset_device:

    /* ignore return code (we already have an error to report) */

    midiInReset(m->handle.in);

close_device:

    midiInClose(m->handle.in); /* ignore return code */

free_descriptor:

    midi->descriptor = nullptr;
    pm_free(m);

no_memory:

    if (pm_hosterror)
    {
#if defined PLATFORM_DEBUG
        int err = midiInGetErrorText
        (
            pm_hosterror, (char *) pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
        assert(err == MMSYSERR_NOERROR);
#else
        (void) midiInGetErrorText
        (
            pm_hosterror, (char *) pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
#endif
        return pmHostError;
    }

    /*
     * if ! pm_hosterror, then the error must be pmInsufficientMemory.
     *
     * Note: if we return an error code, the device will be closed and memory
     * will be freed. It's up to the caller to free the parameter midi.
     */

    return pmInsufficientMemory;
}

/**
 *
 */

static PmError
winmm_in_poll (PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    return m->error;
}

/**
 * Closes an open MIDI input device.
 * It assumes that the MIDI parameter is not null (checked by caller).
 */

static PmError
winmm_in_close(PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    if (is_nullptr(m))
        return pmBadPtr;

    /* device to close */

    if ((pm_hosterror = midiInStop(m->handle.in)) != 0)
    {
        midiInReset(m->handle.in);      /* try to reset and close port      */
        midiInClose(m->handle.in);
    }
    else if ((pm_hosterror = midiInReset(m->handle.in)) != 0)
    {
        midiInClose(m->handle.in);      /* best effort to close midi port   */
    }
    else
    {
        pm_hosterror = midiInClose(m->handle.in);
    }
    midi->descriptor = nullptr;
    DeleteCriticalSection(&m->lock);
    pm_free(m);
    if (pm_hosterror)
    {
#if defined PLATFORM_DEBUG
        int err = midiInGetErrorText
        (
            pm_hosterror, (char *) pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
        assert(err == MMSYSERR_NOERROR);
#else
        (void) midiInGetErrorText
        (
            pm_hosterror, (char *) pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
#endif
        return pmHostError;
    }
    return pmNoError;
}

/**
 *  Callback function executed via midiInput SW interrupt [via midiInOpen()].
 */

static void FAR PASCAL
winmm_in_callback
(
    HMIDIIN hMidiIn,            /* midiInput device Handle */
    WORD wMsg,                  /* MIDI msg */
    DWORD_PTR dwInstance,       /* application data */
    DWORD_PTR dwParam1,         /* MIDI data */
    DWORD_PTR dwParam2          /* device timestamp (re recent midiInStart) */
)
{
    // unused: static int entry = 0;
    PmInternal * midi = (PmInternal *) dwInstance;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;

    /*
     * NOTE: we do not just EnterCriticalSection() here because an
     * MIM_CLOSE message arrives when the port is closed, but then
     * the m->lock has been destroyed.
     */

    switch (wMsg)
    {
    case MIM_DATA:
    {
        /*
         * If this callback is reentered with data, we're in trouble.  It's
         * hard to imagine that Microsoft would allow callbacks to be
         * reentrant -- isn't the model that this is like a hardware
         * interrupt? -- but I've seen reentrant behavior using a debugger, so
         * it happens.
         */

        EnterCriticalSection(&m->lock);

        /*
         * dwParam1 is MIDI data received, packed into DWORD w/ 1st byte of
         * message LOB; dwParam2 is time message received by input device
         * driver, specified in [ms] from when midiInStart called.  each
         * message is expanded to include the status byte
         */

        if ((dwParam1 & 0x80) == 0)
        {
            /*
             * Not a status byte; ignore it. This happened running the sysex.c
             * test under Win2K with MidiMan USB 1x1 interface, but I can't
             * reproduce it. -RBD
             */
        }
        else                            /* data to process */
        {
            PmEvent event;
            if (midi->time_proc)
                dwParam2 = (*midi->time_proc)(midi->time_info);

            event.timestamp = (PmTimestamp) dwParam2;
            event.message = (PmMessage) dwParam1;
            pm_read_short(midi, &event);
        }
        LeaveCriticalSection(&m->lock);
        break;
    }
    case MIM_LONGDATA:
    {
        MIDIHDR * lpMidiHdr = (MIDIHDR *) dwParam1;
        midibyte_t * data = (midibyte_t *) lpMidiHdr->lpData;
        unsigned processed = 0;
        int remaining = lpMidiHdr->dwBytesRecorded;
        EnterCriticalSection(&m->lock);
        if (midi->time_proc)
            dwParam2 = (*midi->time_proc)(midi->time_info);

        /*
         * Can there be more than 1 message in a buffer?  Assume yes and
         * iterate through them.
         */

        while (remaining > 0)
        {
            unsigned amt = pm_read_bytes
            (
                midi, data + processed, remaining, dwParam2
            );
            remaining -= amt;
            processed += amt;
        }

        /*
         * When a device is closed, the pending MIM_LONGDATA buffers are
         * returned to this callback with dwBytesRecorded == 0. In this
         * case, we do not want to send them back to the interface (if
         * we do, the interface will not close, and Windows OS may (!!!) hang).
         * Note: no error checking.  I don't think this can fail except
         * possibly for MMSYSERR_NOMEM, but the pain of reporting this
         * unlikely but probably catastrophic error does not seem worth it.
         */

        if (lpMidiHdr->dwBytesRecorded > 0)
        {
            MMRESULT rslt;
            lpMidiHdr->dwBytesRecorded = 0;
            lpMidiHdr->dwFlags = 0;
            rslt = midiInPrepareHeader(hMidiIn, lpMidiHdr, sizeof(MIDIHDR));
            assert(rslt == MMSYSERR_NOERROR);

            /*
             */

            if (rslt == MMSYSERR_NOERROR)
                rslt = midiInAddBuffer(hMidiIn, lpMidiHdr, sizeof(MIDIHDR));

            assert(rslt == MMSYSERR_NOERROR);
            LeaveCriticalSection(&m->lock);
        }
        else
        {
            midiInUnprepareHeader(hMidiIn, lpMidiHdr, sizeof(MIDIHDR));
            LeaveCriticalSection(&m->lock);
            pm_free(lpMidiHdr);
        }
        break;
    }
    case MIM_OPEN:
        break;

    case MIM_CLOSE:
        break;

    case MIM_ERROR:
#if defined PLATFORM_DEBUG
        printf("MIM_ERROR\n");
#endif
        break;

    case MIM_LONGERROR:
#if defined PLATFORM_DEBUG
        printf("MIM_LONGERROR\n");
#endif
        break;

    default:
        break;
    }
}

/*
 * MIDI output implementation.
 *
 *  This section begins the helper routines used by midiOutStream interface.
 */

/**
 *  Adds timestamped short msg to buffer, and returns fullp.
 *
 * Huh?
 *      If the addition of three more words (a message) would extend beyond
 *      the buffer length, then return TRUE (full).
 */

static int
add_to_buffer
(
    midiwinmm_type m, LPMIDIHDR hdr, unsigned long delta, unsigned long msg
)
{
    unsigned long * ptr = (unsigned long *) (hdr->lpData + hdr->dwBytesRecorded);
    *ptr++ = delta;                                     /* dwDeltaTime  */
    *ptr++ = 0;                                         /* dwStream     */
    *ptr++ = msg;                                       /* dwEvent      */
    hdr->dwBytesRecorded += 3 * sizeof(long);
    return hdr->dwBytesRecorded + 3 * sizeof(long) > hdr->dwBufferLength;
}

/**
 *
 */

static PmTimestamp
pm_time_get (midiwinmm_type m)
{
    MMTIME mmtime;
    MMRESULT winerrcode;
    mmtime.wType = TIME_TICKS;
    mmtime.u.ticks = 0;
    winerrcode = midiStreamPosition(m->handle.stream, &mmtime, sizeof(mmtime));
    assert(winerrcode == MMSYSERR_NOERROR);
    return winerrcode == MMSYSERR_NOERROR ?  mmtime.u.ticks : 0 ;
}

/*
 * End helper routines used by midiOutStream interface
 */

/**
 *
 */

static PmError
winmm_out_open (PmInternal * midi, void * UNUSED(driverinfo))
{
    DWORD dwDevice;
    int i = midi->device_id;
    midiwinmm_type m;
    MIDIPROPTEMPO propdata;
    MIDIPROPTIMEDIV divdata;
    int max_sysex_len = midi->buffer_len * 4;
    int output_buffer_len;
    int num_buffers;
    dwDevice = (DWORD) pm_descriptors[i].descriptor;

    /* create system dependent device data */

    m = (midiwinmm_type) pm_alloc(sizeof(midiwinmm_node)); /* create */
    midi->descriptor = m;
    if (is_nullptr(m))
        goto no_memory;

    m->handle.out = nullptr;
    m->buffers = nullptr;
    m->num_buffers = 0;
    m->max_buffers = 0;
    m->buffers_expanded = FALSE;
    m->next_buffer = 0;

#ifdef USE_SYSEX_BUFFERS
    m->sysex_buffers[0] = nullptr;
    m->sysex_buffers[1] = nullptr;
    m->next_sysex_buffer = 0;
#endif

    m->last_time = 0;
    m->first_message = TRUE; /* we treat first message as special case */
    m->sysex_mode = FALSE;
    m->sysex_word = 0;
    m->sysex_byte_count = 0;
    m->hdr = nullptr;
    m->sync_time = 0;
    m->delta = 0;
    m->error = MMSYSERR_NOERROR;

    /* create a signal */

    m->buffer_signal = CreateEvent(NULL, FALSE, FALSE, NULL);

    /* this should only fail when there are very serious problems */

    assert(m->buffer_signal);

    if (midi->latency == 0)                             /* open device */
    {
        /*
         * Use simple MIDI out calls.  Note that winmm_streamout_callback() is
         *
         * Note: the same callback function as for StreamOpen().
         */

        pm_hosterror = midiOutOpen
        (
            (LPHMIDIOUT) & m->handle.out,               /* device Handle    */
            dwDevice,                                   /* device ID        */
            (DWORD_PTR) winmm_streamout_callback,       /* callback fn      */
            (DWORD_PTR) midi,                           /* cb instance data */
            CALLBACK_FUNCTION                           /* callback type    */
        );
    }
    else
    {
        /* use stream-based midi output (schedulable in future) */

        pm_hosterror = midiStreamOpen
        (
            &m->handle.stream,                          /* device Handle    */
            (LPUINT) & dwDevice,                        /* device ID ptr    */
            1,                                          /* must = 1         */
            (DWORD_PTR) winmm_streamout_callback,
            (DWORD_PTR) midi,                           /* callback data    */
            CALLBACK_FUNCTION
        );
    }
    if (pm_hosterror != MMSYSERR_NOERROR)
        goto free_descriptor;

    if (midi->latency == 0)
    {
        num_buffers = NUM_SIMPLE_SYSEX_BUFFERS;
        output_buffer_len = max_sysex_len / num_buffers;
        if (output_buffer_len < MIN_SIMPLE_SYSEX_LEN)
            output_buffer_len = MIN_SIMPLE_SYSEX_LEN;
    }
    else
    {
        num_buffers = max(midi->buffer_len, midi->latency / 2);
        if (num_buffers < MIN_STREAM_BUFFERS)
            num_buffers = MIN_STREAM_BUFFERS;

        output_buffer_len = STREAM_BUFFER_LEN;
        propdata.cbStruct = sizeof(MIDIPROPTEMPO);
        propdata.dwTempo = Pt_Get_Tempo_Microseconds(); /* us per quarter   */
        pm_hosterror = midiStreamProperty
        (
            m->handle.stream, (LPBYTE) & propdata, MIDIPROP_SET | MIDIPROP_TEMPO
        );

        if (pm_hosterror)
            goto close_device;

        divdata.cbStruct = sizeof(MIDIPROPTEMPO);
        divdata.dwTimeDiv = Pt_Get_Ppqn();              /* divs per 1/4     */
        pm_hosterror = midiStreamProperty
        (
            m->handle.stream, (LPBYTE) & divdata, MIDIPROP_SET | MIDIPROP_TIMEDIV
        );
        if (pm_hosterror)
            goto close_device;
    }

    if (allocate_buffers(m, output_buffer_len, num_buffers))
        goto free_buffers;


    if (midi->latency != 0)                     /* start device */
    {
        pm_hosterror = midiStreamRestart(m->handle.stream);
        if (pm_hosterror != MMSYSERR_NOERROR) goto free_buffers;
    }
    return pmNoError;

free_buffers:

    /* buffers are freed below by winmm_out_delete */

close_device:

    midiOutClose(m->handle.out);

free_descriptor:

    midi->descriptor = nullptr;
    winmm_out_delete(midi); /* frees buffers and m */

no_memory:

    if (pm_hosterror)
    {
#if defined PLATFORM_DEBUG
        int err = midiOutGetErrorText
        (
            pm_hosterror, (char *) pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
        assert(err == MMSYSERR_NOERROR);
#else
        (void) midiOutGetErrorText
        (
            pm_hosterror, (char *) pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
#endif
        return pmHostError;
    }
    return pmInsufficientMemory;
}

/**
 *  Carefully frees the data associated with MIDI.  It deletes
 *  system-dependent device data.  We don't report errors here, better not
 *  to stop cleanup.
 */

static void
winmm_out_delete (PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    if (not_nullptr(m))
    {
        int i;
        if (m->buffer_signal)
        {
            CloseHandle(m->buffer_signal);
        }

        /* if using stream output, free buffers */

        for (i = 0; i < m->num_buffers; ++i)
        {
            if (m->buffers[i])
                pm_free(m->buffers[i]);
        }
        m->num_buffers = 0;
        pm_free(m->buffers);
        m->max_buffers = 0;

#ifdef USE_SYSEX_BUFFERS                /* free sysex buffers */
        for (i = 0; i < NUM_SYSEX_BUFFERS; ++i)
            if (m->sysex_buffers[i]) pm_free(m->sysex_buffers[i]);
#endif

    }
    midi->descriptor = nullptr;
    pm_free(m);                         /* delete */
}

/*
 * see comments for winmm_in_close
 */

static PmError
winmm_out_close (PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    if (m->handle.out)
    {
        /* device to close */
        if (midi->latency == 0)
        {
            pm_hosterror = midiOutClose(m->handle.out);
        }
        else
        {
            pm_hosterror = midiStreamClose(m->handle.stream);
        }
        winmm_out_delete(midi);     /* regardless of outcome, free memory */
    }
    if (pm_hosterror)
    {
#if defined PLATFORM_DEBUG
        int err = midiOutGetErrorText
        (
            pm_hosterror, (char *) pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
        assert(err == MMSYSERR_NOERROR);
#else
        (void) midiOutGetErrorText
        (
            pm_hosterror, (char *) pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
#endif
        return pmHostError;
    }
    return pmNoError;
}

/**
 *
 */

static PmError
winmm_out_abort (PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    m->error = MMSYSERR_NOERROR;

    /* only stop output streams */

    if (midi->latency > 0)
    {
        m->error = midiStreamStop(m->handle.stream);
    }
    return m->error ? pmHostError : pmNoError;
}

/**
 *
 */

static PmError
winmm_write_flush (PmInternal * midi, PmTimestamp timestamp)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    assert(m);
    if (not_nullptr(m->hdr))
    {
        m->error = midiOutPrepareHeader(m->handle.out, m->hdr, sizeof(MIDIHDR));
        if (m->error)
        {
            /* do not send message */
        }
        else if (midi->latency == 0)
        {
            /*
             * As pointed out by Nigel Brown, 20Sep06, dwBytesRecorded
             * should be zero. This is set in get_free_sysex_buffer().
             * The msg length goes in dwBufferLength in spite of what
             * Microsoft documentation says (or doesn't say).
             */

            m->hdr->dwBufferLength = m->hdr->dwBytesRecorded;
            m->hdr->dwBytesRecorded = 0;
            m->error = midiOutLongMsg(m->handle.out, m->hdr, sizeof(MIDIHDR));
        }
        else
        {
            m->error = midiStreamOut(m->handle.stream, m->hdr, sizeof(MIDIHDR));
        }
        midi->fill_base = nullptr;
        m->hdr = nullptr;
        if (m->error)
        {
            m->hdr->dwFlags = 0; /* release the buffer */
            return pmHostError;
        }
    }
    return pmNoError;
}

#ifdef GARBAGE

static PmError
winmm_write_sysex_byte (PmInternal * midi, midibyte_t byte)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    midibyte_t * msg_buffer;

    /*
     * At the beginning of SysEx, m->hdr is null.  Allocate a buffer if none
     * allocated yet.
     */

    if (is_nullptr(m->hdr))
    {
        m->hdr = get_free_output_buffer(midi);
        if (is_nullptr(m->hdr))
            return pmInsufficientMemory;

        m->sysex_byte_count = 0;
    }

    msg_buffer = (midibyte_t *) (m->hdr->lpData);       /* where to write   */
    assert(m->hdr->lpData == (char *) (m->hdr + 1));
    if (m->sysex_byte_count >= m->hdr->dwBufferLength)  /* check overflow   */
    {
        /* allocate a bigger message -- double it every time */

        LPMIDIHDR big = allocate_buffer(m->sysex_byte_count * 2);
        if (is_nullptr(big))
            return pmInsufficientMemory;

        m->error = midiOutPrepareHeader(m->handle.out, big, sizeof(MIDIHDR));
        if (m->error)
        {
            m->hdr = nullptr;
            return pmHostError;
        }
        memcpy(big->lpData, msg_buffer, m->sysex_byte_count);
        msg_buffer = (midibyte_t *)(big->lpData);
        if (m->buffers[0] == m->hdr)
        {
            m->buffers[0] = big;
            pm_free(m->hdr);
        }
        else if (m->buffers[1] == m->hdr)
        {
            m->buffers[1] = big;
            pm_free(m->hdr);
        }
        m->hdr = big;
    }
    msg_buffer[m->sysex_byte_count++] = byte;   /* append byte to message   */
    if (byte == MIDI_EOX)                       /* have a complete message? */
    {
        m->hdr->dwBytesRecorded = m->sysex_byte_count;
        m->error = midiOutLongMsg(m->handle.out, m->hdr, sizeof(MIDIHDR));
        m->hdr = nullptr;                       /* stop using message buffer */
        if (m->error)
            return pmHostError;
    }
    return pmNoError;
}

#endif  // GARBAGE

/**
 *
 */

static PmError
winmm_write_short (PmInternal * midi, PmEvent * event)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    PmError rslt = pmNoError;
    assert(m);

    if (midi->latency == 0)   /* use midiOut interface, ignore timestamps */
    {
        m->error = midiOutShortMsg(m->handle.out, event->message);
        if (m->error) rslt = pmHostError;
    }
    else      /* use midiStream interface -- pass data through buffers */
    {
        unsigned long when = event->timestamp;
        unsigned long delta;
        int full;
        if (when == 0)
            when = midi->now;

        /* when is in real_time; translate to intended stream time */

        when = when + m->delta + midi->latency;

        /* make sure we don't go backward in time */

        if (when < m->last_time)
            when = m->last_time;

        delta = when - m->last_time;
        m->last_time = when;

        /* before we insert any data, we must have a buffer */

        if (is_nullptr(m->hdr))
        {
            /* stream interface: buffers allocated when stream is opened */

            m->hdr = get_free_output_buffer(midi);
        }
        full = add_to_buffer(m, m->hdr, delta, event->message);
        if (full)
            rslt = winmm_write_flush(midi, when);
    }
    return rslt;
}

#define winmm_begin_sysex winmm_write_flush

#ifndef winmm_begin_sysex

/**
 *
 */

static PmError
winmm_begin_sysex (PmInternal * midi, PmTimestamp timestamp)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    PmError rslt = pmNoError;

    if (midi->latency == 0)
    {
        /* do nothing -- it's handled in winmm_write_byte */
    }
    else
    {
        /* sysex expects an empty sysex buffer, so send whatever is here */

        rslt = winmm_write_flush(midi);
    }
    return rslt;
}

#endif

/**
 *
 */

static PmError
winmm_end_sysex (PmInternal * midi, PmTimestamp timestamp)
{
    /*
     * Could check for callback_error here, but I haven't checked what happens
     * if we exit early and don't finish the sysex msg and clean up.
     */

    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    PmError rslt = pmNoError;
    LPMIDIHDR hdr = m->hdr;
    if (! hdr)
        return rslt;

    /*
     * Something bad happened earlier, do not report an error because it would
     * have been reported (at least) once already.  a(n old) version of MIDI
     * YOKE requires a zero byte after the sysex message, but do not increment
     * dwBytesRecorded.
     */

    hdr->lpData[hdr->dwBytesRecorded] = 0;
    if (midi->latency == 0)
    {

#ifdef DEBUG_PRINT_BEFORE_SENDING_SYSEX /* DEBUG CODE: */
        {
            int i;
            int len = m->hdr->dwBufferLength;
            printf("OutLongMsg %d ", len);
            for (i = 0; i < len; ++i)
                printf("%2x ", (midibyte_t)(m->hdr->lpData[i]));

            printf("\n");
        }
#endif
    }
    else
    {
        /*
         * Using stream interface. There are accumulated bytes in m->hdr to
         * send using midiStreamOut.  add bytes recorded to MIDIEVENT length,
         * but don't count the MIDIEVENT data (3 longs)
         */

        MIDIEVENT * evt = (MIDIEVENT *) (hdr->lpData);
        evt->dwEvent += hdr->dwBytesRecorded - 3 * sizeof(long);

        /* round up BytesRecorded to multiple of 4 */

        hdr->dwBytesRecorded = (hdr->dwBytesRecorded + 3) & ~3;
    }
    rslt = winmm_write_flush(midi, timestamp);
    return rslt;
}

/**
 *
 */

static PmError
winmm_write_byte
(
    PmInternal * midi, midibyte_t byte, PmTimestamp timestamp
)
{
    /* write a sysex byte */

    PmError rslt = pmNoError;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    LPMIDIHDR hdr = m->hdr;
    midibyte_t * msg_buffer;
    assert(m);
    if (is_nullptr(hdr))
    {
        m->hdr = hdr = get_free_output_buffer(midi);
        assert(hdr);
        midi->fill_base = (midibyte_t *) m->hdr->lpData;
        midi->fill_offset_ptr = (uint32_t *)(&(hdr->dwBytesRecorded));

        /*
         * When buffer fills, Pm_WriteSysEx will revert to calling
         * pmwin_write_byte, which expect to have space, so leave one byte free
         * for pmwin_write_byte. Leave another byte of space for zero after
         * message to make early version of MIDI YOKE driver happy -- therefore
         * dwBufferLength - 2
         */

        midi->fill_length = hdr->dwBufferLength - 2;
        if (midi->latency != 0)
        {
            unsigned long when = (unsigned long) timestamp;
            unsigned long delta;
            unsigned long * ptr;
            if (when == 0)
                when = midi->now;

            /* when is in real_time; translate to intended stream time */

            when = when + m->delta + midi->latency;

            /* make sure we don't go backward in time */

            if (when < m->last_time) when = m->last_time;
            delta = when - m->last_time;
            m->last_time = when;

            ptr = (unsigned long *) hdr->lpData;
            *ptr++ = delta;
            *ptr++ = 0;
            *ptr = MEVT_F_LONG;
            hdr->dwBytesRecorded = 3 * sizeof(long);

            /* data will be added at an offset of dwBytesRecorded ... */
        }
    }

    /* add the data byte */

    msg_buffer = (midibyte_t *)(hdr->lpData);
    msg_buffer[hdr->dwBytesRecorded++] = byte;

    /* see if buffer is full, leave one byte extra for pad */

    if (hdr->dwBytesRecorded >= hdr->dwBufferLength - 1)
    {
        /* write what we've got and continue */

        rslt = winmm_end_sysex(midi, timestamp);
    }
    return rslt;
}

#ifdef EXPANDING_SYSEX_BUFFERS

/*
 * Note:
 *
 *  This code is here as an aid in case you want sysex buffers to expand to
 *  hold large messages completely. If so, you will want to change
 *  SYSEX_BYTES_PER_BUFFER above to some variable that remembers the buffer
 *  size. A good place to put this value would be in the hdr->dwUser field.
 *
 *  rslt = resize_sysex_buffer(midi, m->sysex_byte_count,
 *      m->sysex_byte_count * 2); *
 *  if (rslt == pmBufferMaxSize) // if the buffer can't be resized
 */

    /* this field gets wiped out, so we'll save it */

    int bytesRecorded = hdr->dwBytesRecorded;
    rslt = resize_sysex_buffer(midi, bytesRecorded, 2 * bytesRecorded);
    hdr->dwBytesRecorded = bytesRecorded;

    if (rslt == pmBufferMaxSize)            /* if buffer can't be resized */
        ;
#endif

/**
 *
 */

static PmTimestamp
winmm_synchronize (PmInternal * midi)
{
    midiwinmm_type m;
    unsigned long pm_stream_time_2;
    unsigned long real_time;
    unsigned long pm_stream_time;

    /* only synchronize if we are using stream interface */

    if (midi->latency == 0)
        return 0;

    /* figure out the time */

    m = (midiwinmm_type) midi->descriptor;
    pm_stream_time_2 = pm_time_get(m);
    do
    {
        /* read real_time between two reads of stream time */

        pm_stream_time = pm_stream_time_2;
        real_time = (*midi->time_proc)(midi->time_info);
        pm_stream_time_2 = pm_time_get(m);

        /* repeat if more than 1ms elapsed */

    } while (pm_stream_time_2 > pm_stream_time + 1);

    m->delta = pm_stream_time - real_time;
    m->sync_time = real_time;
    return real_time;
}

#ifdef USE_SYSEX_BUFFERS

/**
 * winmm_out_callback -- recycle sysex buffers
 */

static void CALLBACK
winmm_out_callback
(
    HMIDIOUT hmo, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2
)
{
    PmInternal * midi = (PmInternal *) dwInstance;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    LPMIDIHDR hdr = (LPMIDIHDR) dwParam1;
    int err = 0;  /* set to 0 so that no buffer match will also be an error */

    /*
     * Future optimization: eliminate UnprepareHeader calls -- they aren't
     * necessary; however, this code uses the prepared-flag to indicate which
     * buffers are free, so we need to do something to flag empty buffers if
     * we leave them prepared
     */

    if (wMsg == MOM_DONE)
    {
        MMRESULT ret = midiOutUnprepareHeader
        (
            m->handle.out, hdr, sizeof(MIDIHDR)
        );
        assert(ret == MMSYSERR_NOERROR);
    }


    err = SetEvent(m->buffer_signal);   /* notify sender, buffer available  */
    assert(err);                        /* false -> error                   */
}

#endif  // USE_SYSEX_BUFFERS

/**
 *  winmm_streamout_callback -- unprepare (free) buffer header
 */

static void CALLBACK
winmm_streamout_callback
(
    HMIDIOUT hmo, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2
)
{
    PmInternal * midi = (PmInternal *) dwInstance;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    LPMIDIHDR hdr = (LPMIDIHDR) dwParam1;
#if defined PLATFORM_DEBUG
    int err;
#endif

    /*
     * Even if an error is pending, I think we should unprepare messages and
     * signal their arrival in case the client is blocked waiting for the
     * buffer.
     */

    if (wMsg == MOM_DONE)
    {
#if defined PLATFORM_DEBUG
        MMRESULT r = midiOutUnprepareHeader(m->handle.out, hdr, sizeof(MIDIHDR));
        assert(r == MMSYSERR_NOERROR);
#else
        (void) midiOutUnprepareHeader(m->handle.out, hdr, sizeof(MIDIHDR));
#endif
    }

#if defined PLATFORM_DEBUG
    err = SetEvent(m->buffer_signal);
    assert(err);                            /* false -> error */
#else
    (void) SetEvent(m->buffer_signal);
#endif
}


/*
 * Begin exported functions
 */

#define winmm_in_abort pm_fail_fn

/**
 *
 */

pm_fns_node pm_winmm_in_dictionary =
{
    none_write_short,
    none_sysex,
    none_sysex,
    none_write_byte,
    none_write_short,
    none_write_flush,
    winmm_synchronize,
    winmm_in_open,
    winmm_in_abort,
    winmm_in_close,
    winmm_in_poll,
    winmm_has_host_error,
    winmm_get_host_error
};

/**
 *
 */

pm_fns_node pm_winmm_out_dictionary =
{
    winmm_write_short,
    winmm_begin_sysex,
    winmm_end_sysex,
    winmm_write_byte,
    winmm_write_short,  /* short realtime message */
    winmm_write_flush,
    winmm_synchronize,
    winmm_out_open,
    winmm_out_abort,
    winmm_out_close,
    none_poll,
    winmm_has_host_error,
    winmm_get_host_error
};

/**
 *  Initialize WinMM interface. Note that if there is something wrong with
 *  winmm (e.g. it is not supported or installed), it is not an error. We
 *  should simply return without having added any devices to the table.
 *  Hence, no error code is returned. Furthermore, this init code is called
 *  along with every other supported interface, so the user would have a
 *  very hard time figuring out what hardware and API generated the error.
 *  Finally, it would add complexity to pmwin.c to remember where the error
 *  code came from in order to convert to text.
 */

void
pm_winmm_init (void)
{
    // TRIAL COMMENTING OUT???
    pm_winmm_mapper_input();
    pm_winmm_mapper_output();
    pm_winmm_general_inputs();
    pm_winmm_general_outputs();
}

/**
 *  No error codes are returned, even if errors are encountered, because
 *  there is probably nothing the user could do (e.g. it would be an error
 *  to retry.
 */

void
pm_winmm_term (void)
{
#if defined PLATFORM_DEBUG
    int doneAny = 0;
#endif
    int i;
    for (i = 0; i < pm_descriptor_index; ++i)
    {
        PmInternal * midi = pm_descriptors[i].internalDescriptor;
        if (not_nullptr(midi))
        {
            midiwinmm_type m = (midiwinmm_type) midi->descriptor;
            if (m->handle.out)
            {
#if defined PLATFORM_DEBUG
                if (doneAny == 0)           /* close next open device   */
                {
                    printf("Begin closing open devices...\n");
                    doneAny = 1;
                }

                /*
                 * Report any host errors; this EXTREMELY useful when
                 * trying to debug client app.
                 */

                if (winmm_has_host_error(midi))
                {
                    char msg[PM_HOST_ERROR_MSG_LEN];
                    winmm_get_host_error(midi, msg, PM_HOST_ERROR_MSG_LEN);
                    printf("[%d] '%s'\n", i, msg);
                }
#endif
                (*midi->dictionary->close)(midi); /* close all open ports */
            }
        }
    }
    if (midi_in_caps)
    {
        pm_free(midi_in_caps);
        midi_in_caps = nullptr;
    }
    if (midi_out_caps)
    {
        pm_free(midi_out_caps);
        midi_out_caps = nullptr;
    }
#if defined PLATFORM_DEBUG
    if (doneAny)
        printf("Warning: devices were left open. They have been closed.\n");
#endif
    pm_descriptor_index = 0;
}

/*
 * pmwinmm.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

