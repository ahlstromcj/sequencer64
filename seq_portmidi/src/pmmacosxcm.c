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
 * \file        pmmacosxcm.c
 *
 *      System specific definitions for the Mac OSX CoreMIDI API.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2018-05-13
 * \updates     2018-05-20
 * \license     GNU GPLv2 or above
 *
 *  A platform interface to the MacOS X CoreMIDI framework.
 *
 * Jon Parise <jparise at cmu.edu>
 * and subsequent work by Andrew Zeldis and Zico Kolter,
 * and Roger B. Dannenberg.
 *
 * $Id: pmmacosx.c,v 1.17 2002/01/27 02:40:40 jon Exp $
 *
 * \note
 *      Since the input and output streams are represented by MIDIEndpointRef
 *      values and almost no other state, we store the MIDIEndpointRef on
 *      descriptors[midi->device_id].descriptor. The only other state we need is
 *      for errors: we need to know if there is an error and if so, what is the
 *      error text. We use a structure with two kinds of host error: "error" and
 *      "callback_error". That way, asynchronous callbacks do not interfere with
 *      other error information.
 *
 *      OS X does not seem to have an error-code-to-text function, so we will just
 *      use text messages instead of error codes.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <CoreServices/CoreServices.h>
#include <CoreMIDI/MIDIServices.h>
#include <CoreAudio/HostTime.h>

#include "portmidi.h"
#include "pmutil.h"
#include "pminternal.h"
#include "porttime.h"
#include "pmmac.h"
#include "pmmacosxcm.h"

/**
 *
 */

#define PACKET_BUFFER_SIZE      1024

/**
 *  Maximum overall data rate (the OS X limit is 15000 bytes/second).
 */

#define MAX_BYTES_PER_S         14000

/**
 *  Apple reports that packets are dropped when the MIDI bytes/sec exceeds 15000.
 *  This is computed by "tracking the number of MIDI bytes scheduled into 1-second
 *  buckets over the last six seconds and averaging these counts."
 *
 *  This is apparently based on timestamps, not on real time, so we have to avoid
 *  constructing packets that schedule high speed output even if the actual writes
 *  are delayed (which was my first solution).
 *
 *  The LIMIT_RATE symbol, if defined, enables code to modify timestamps as
 *  follows:
 *
 *  After each packet is formed, the next allowable timestamp is computed as
 *  this_packet_time + this_packet_len * delay_per_byte This is the minimum
 *  timestamp allowed in the next packet.  Note that this distorts accurate
 *  timestamps somewhat.
 */

#define LIMIT_RATE              1

/**
 *
 */

#define SYSEX_BUFFER_SIZE       128

/**
 *
 */

#define MIDI_SYSEX              0xf0
#define MIDI_EOX                0xf7
#define MIDI_STATUS_MASK        0x80

/**
 *  "Ref"s are pointers on 32-bit machines and ints on 64 bit machines.
 *  NULL_REF is our representation of either 0 or NULL.  We'll start using the
 *  nullptr macro at some point.
 */

#ifdef __LP64__
#define NULL_REF                0
#else
#define NULL_REF                NULL
#endif

/*
 *  Client handle to the MIDI server.
 */

static MIDIClientRef client = NULL_REF;

/*
 *  Input port handle.
 */

static MIDIPortRef portIn = NULL_REF;

/*
 * Output port handle.
 */

static MIDIPortRef portOut = NULL_REF;

extern pm_fns_node pm_macosx_in_dictionary;
extern pm_fns_node pm_macosx_out_dictionary;

/**
 *  Oy, another big-ass state structure!
 */

typedef struct midi_macosxcm_struct
{
    PmTimestamp sync_time;      /**< When did we last determine delta?          */
    UInt64 delta;               /**< Delta between stream-time & real-time, ns. */
    UInt64 last_time;           /**< Last output time in host units.            */
    int first_message;          /**< Tells midi_write() to synch timestamps.    */
    int sysex_mode;             /**< In middle of sending SysEx.                */
    uint32_t sysex_word;        /**< Accumulate data when receiving SysEx.      */
    uint32_t sysex_byte_count;  /**< Count how many received. */
    char error[PM_HOST_ERROR_MSG_LEN];          /**< todo. */
    char callback_error[PM_HOST_ERROR_MSG_LEN]; /**< todo. */
    Byte packetBuffer[PACKET_BUFFER_SIZE];      /**< todo. */
    MIDIPacketList * packetList;                /**< Pointer to packetBuffer.   */
    MIDIPacket * packet;                        /**< Points to MIDI packet.     */
    Byte sysex_buffer[SYSEX_BUFFER_SIZE];   /**< temp storage for sysex data */
    MIDITimeStamp sysex_timestamp; /**< timestamp to use with sysex data */

    /**<
     *  Allow for running status (is running status possible here? -rbd): -cpr
     */

    unsigned char last_command;
    int32_t last_msg_length;                    /**< todo. */

    /*
     * Limit MIDI data rate (a CoreMidi requirement).  When can the next send take
     * place?
     */

    UInt64 min_next_time;
    int byte_count;             /**< How many bytes in the next packet list?    */
    Float64 us_per_host_tick;   /**< Host clock freq, units of min_next_time.   */
    UInt64 host_ticks_per_byte; /**< Host clock units/byte at maximum rate.     */

} midi_macosxcm_node, * midi_macosxcm_type;

/*
 *  private function declarations.
 */

MIDITimeStamp timestamp_pm_to_cm (PmTimestamp timestamp);
PmTimestamp timestamp_cm_to_pm (MIDITimeStamp timestamp);
char * cm_get_full_endpoint_name (MIDIEndpointRef endpoint);

/**
 *
 */

static int
midi_length (int32_t msg)
{
    int status, high, low;
    static int high_lengths[] =
    {
        1, 1, 1, 1, 1, 1, 1, 1,         /* 0x00 through 0x70 */
        3, 3, 3, 3, 2, 2, 3, 1          /* 0x80 through 0xf0 */
    };
    static int low_lengths[] =
    {
        1, 2, 3, 2, 1, 1, 1, 1,         /* 0xf0 through 0xf8 */
        1, 1, 1, 1, 1, 1, 1, 1          /* 0xf9 through 0xff */
    };

    status = msg & 0xFF;
    high = status >> 4;
    low = status & 15;
    return high != 0xF ? high_lengths[high] : low_lengths[low];
}

/**
 *
 */

static PmTimestamp
midi_synchronize (PmInternal * midi)
{
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    UInt64 pm_stream_time_2 = AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());
    PmTimestamp real_time;
    UInt64 pm_stream_time;

    /*
     * If latency is zero and this is an output, there is no time reference and
     * midi_synchronize should never be called.
     */

    assert(midi->time_proc);
    assert(!(midi->write_flag && midi->latency == 0));
    do
    {
        /*
         * read real_time between two reads of stream time.
         */

        pm_stream_time = pm_stream_time_2;
        real_time = (*midi->time_proc)(midi->time_info);
        pm_stream_time_2 = AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());

        /*
         * repeat if more than 0.5 ms has elapsed.
         */
    }
    while (pm_stream_time_2 > pm_stream_time + 500000)
        ;
    m->delta = pm_stream_time - ((UInt64) real_time * (UInt64) 1000000);
    m->sync_time = real_time;
    return real_time;
}

/**
 *  Handles a packet of MIDI messages from CoreMIDI.  There may be multiple short
 *  messages in one packet (!)
 */

static void
process_packet
(
    MIDIPacket * packet, PmEvent * event, PmInternal * midi, midi_macosxcm_type m
)
{
    unsigned remaining_length = packet->length;
    unsigned char * cur_packet_data = packet->data;
    while (remaining_length > 0)
    {
        /*
         * Are we in the middle of a SysEx message?
         */

        if
        (
            cur_packet_data[0] == MIDI_SYSEX ||
            (m->last_command == 0 && ! (cur_packet_data[0] & MIDI_STATUS_MASK))
        )
        {
            m->last_command = 0;                    /* no running status */
            unsigned amt = pm_read_bytes
            (
                midi, cur_packet_data, remaining_length, event->timestamp
            );
            remaining_length -= amt;
            cur_packet_data += amt;
        }
        else if (cur_packet_data[0] == MIDI_EOX)
        {
            /*
             * This should never happen, because pm_read_bytes() should
             * get and read all EOX bytes.
             */

            midi->sysex_in_progress = FALSE;
            m->last_command = 0;
        }
        else if (cur_packet_data[0] & MIDI_STATUS_MASK)
        {
            /*
             * Compute the length of the next (short) message in the packet.
             */

            unsigned cur_message_length = midi_length(cur_packet_data[0]);
            if (cur_message_length > remaining_length)
            {
#if defined PLATFORM_DEBUG
                printf("PortMidi debug msg: not enough data");
#endif
                return;             /* since there's no more data, we're done */
            }
            m->last_msg_length = cur_message_length;
            m->last_command = cur_packet_data[0];
            switch (cur_message_length)
            {
            case 1:
                event->message = Pm_Message(cur_packet_data[0], 0, 0);
                break;

            case 2:
                event->message = Pm_Message
                (
                    cur_packet_data[0], cur_packet_data[1], 0
                );
                break;

            case 3:
                event->message = Pm_Message
                (
                    cur_packet_data[0], cur_packet_data[1], cur_packet_data[2]
                );
                break;

            default:
                /*
                 * A PortMIDI internal error; should never happen.  Give up on the
                 * packet if continued after asserttion.
                 */

                assert(cur_message_length == 1);
                return;
            }
            pm_read_short(midi, event);
            remaining_length -= m->last_msg_length;
            cur_packet_data += m->last_msg_length;
        }
        else if (m->last_msg_length > remaining_length + 1)
        {
            /*
             * We have running status, but not enough data.
             */

#if defined PLATFORM_DEBUG
            printf("PortMidi debug msg: not enough data in CoreMIDI packet");
#endif
            return;                 /* since there's no more data, we're done */
        }
        else                        /* output message using running status */
        {
            switch (m->last_msg_length)
            {
            case 1:
                event->message = Pm_Message(m->last_command, 0, 0);
                break;

            case 2:
                event->message = Pm_Message(m->last_command, cur_packet_data[0], 0);
                break;

            case 3:
                event->message = Pm_Message
                (
                    m->last_command, cur_packet_data[0], cur_packet_data[1]
                );
                break;

            default:
                /*
                 * The last_msg_length is invalid -- internal PortMIDI error.
                 */

                assert(m->last_msg_length == 1);
            }
            pm_read_short(midi, event);
            remaining_length -= (m->last_msg_length - 1);
            cur_packet_data += (m->last_msg_length - 1);
        }
    }
}

/**
 *  This function is called when MIDI packets are received.
 */

static void
readProc (const MIDIPacketList * newPackets, void * refCon, void * connRefCon)
{
    PmInternal * midi;
    midi_macosxcm_type m;
    PmEvent event;
    MIDIPacket * packet;
    unsigned packetIndex;
    uint32_t now;
    unsigned status;

#if defined PLATFORM_DEBUG
    printf("readProc: numPackets %d: ", newPackets->numPackets);
#endif

    midi = (PmInternal *) connRefCon;   /* retrieve context for this connection */
    m = (midi_macosxcm_type) midi->descriptor;
    assert(m);
    now = (*midi->time_proc)(midi->time_info);      /* synch time every 100 ms  */
    if (m->first_message || m->sync_time + 100 < now)    /* milliseconds        */
    {
        now = midi_synchronize(midi);               /* time to resync           */
        m->first_message = FALSE;
    }

    packet = (MIDIPacket *) &newPackets->packet[0];
    for (packetIndex = 0; packetIndex < newPackets->numPackets; packetIndex++)
    {
        /*
         * Set the timestamp and dispatch this message.  Do an explicit conversion.
         */

        event.timestamp = (PmTimestamp)
        (
            (AudioConvertHostTimeToNanos(packet->timeStamp) - m->delta) /
            (UInt64) 1000000
        );
        status = packet->data[0];

        /*
         * Process packet as SysEx data if it begins with MIDI_SYSEX, or
         * MIDI_EOX or non-status byte with no running status.
         */

        if
        (
            status == MIDI_SYSEX || status == MIDI_EOX ||
            ((! (status & MIDI_STATUS_MASK)) && !m->last_command)
        )
        {
            /*
             * Previously was: !(status & MIDI_STATUS_MASK)) {, but this could
             * mistake running status for SysEx data.  So reset running status data
             * -- cpr
             */

            m->last_command = 0;
            m->last_msg_length = 0;
            pm_read_bytes(midi, packet->data, packet->length, event.timestamp);
        }
        else
            process_packet(packet, &event, midi, m);

        packet = MIDIPacketNext(packet);
    }
}

/**
 *
 */

static PmError
midi_in_open (PmInternal * midi, void * driverInfo)
{
    MIDIEndpointRef endpoint;
    midi_macosxcm_type m;
    OSStatus macHostError;

    if (midi->time_proc == NULL)i   /* insure we have a time_proc for timing */
    {
        if (! Pt_Started())
            Pt_Start(1, 0, 0);

        /*
         * time_get() does not take a parameter, so coerce it.
         */

        midi->time_proc = (PmTimeProcPtr) Pt_Time;
    }
    endpoint = (MIDIEndpointRef)(long) descriptors[midi->device_id].descriptor;
    if (endpoint == NULL_REF)
    {
        return pmInvalidDeviceId;
    }

    m = (midi_macosxcm_type) pm_alloc(sizeof(midi_macosxcm_node)); /* create */
    midi->descriptor = m;
    if (is nullptr(m))
        return pmInsufficientMemory;

    m->error[0] = 0;
    m->callback_error[0] = 0;
    m->sync_time = 0;
    m->delta = 0;
    m->last_time = 0;
    m->first_message = TRUE;
    m->sysex_mode = FALSE;
    m->sysex_word = 0;
    m->sysex_byte_count = 0;
    m->packetList = NULL;
    m->packet = NULL;
    m->last_command = 0;
    m->last_msg_length = 0;
    macHostError = MIDIPortConnectSource(portIn, endpoint, midi);
    if (macHostError != noErr)
    {
        pm_hosterror = macHostError;
        sprintf
        (
            pm_hosterror_text,
            "Host error %ld: MIDIPortConnectSource() in midi_in_open()",
            (long) macHostError
        );
        midi->descriptor = NULL;
        pm_free(m);
        return pmHostError;
    }
    return pmNoError;
}

/**
 *
 */

static PmError
midi_in_close (PmInternal * midi)
{
    MIDIEndpointRef endpoint;
    OSStatus macHostError;
    PmError err = pmNoError;
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    if (is_nullptr(m))
        return pmBadPtr;

    endpoint = (MIDIEndpointRef)(long) descriptors[midi->device_id].descriptor;
    if (endpoint == NULL_REF)
        pm_hosterror = pmBadPtr;

    /*
     * Shut off the incoming messages before freeing data structures.
     */

    macHostError = MIDIPortDisconnectSource(portIn, endpoint);
    if (macHostError != noErr)
    {
        pm_hosterror = macHostError;
        sprintf
        (
            pm_hosterror_text,
            "Host error %ld: MIDIPortDisconnectSource() in midi_in_close()",
            (long) macHostError
        );
        err = pmHostError;
    }
    midi->descriptor = NULL;
    pm_free(midi->descriptor);
    return err;
}

/**
 *
 */

static PmError
midi_out_open (PmInternal * midi, void * driverInfo)
{
    midi_macosxcm_type m =
        (midi_macosxcm_type) pm_alloc(sizeof(midi_macosxcm_node)); /* create */

    midi->descriptor = m;
    if (is_nullptr(m))
    {
        return pmInsufficientMemory;
    }
    m->error[0] = 0;
    m->callback_error[0] = 0;
    m->sync_time = 0;
    m->delta = 0;
    m->last_time = 0;
    m->first_message = TRUE;
    m->sysex_mode = FALSE;
    m->sysex_word = 0;
    m->sysex_byte_count = 0;
    m->packetList = (MIDIPacketList *) m->packetBuffer;
    m->packet = NULL;
    m->last_command = 0;
    m->last_msg_length = 0;
    m->min_next_time = 0;
    m->byte_count = 0;
    m->us_per_host_tick = 1000000.0 / AudioGetHostClockFrequency();
    m->host_ticks_per_byte = (UInt64)
    (
        1000000.0 / (m->us_per_host_tick * MAX_BYTES_PER_S)
    );
    return pmNoError;
}

/**
 *
 */

static PmError
midi_out_close (PmInternal * midi)
{
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    if (is_nullptr(m))
        return pmBadPtr;

    midi->descriptor = NULL;
    pm_free(midi->descriptor);
    return pmNoError;
}

/**
 *
 */

static PmError
midi_abort (PmInternal * midi)
{
    PmError err = pmNoError;
    OSStatus macHostError;
    MIDIEndpointRef endpoint = (MIDIEndpointRef) (long)
        descriptors[midi->device_id].descriptor;

    macHostError = MIDIFlushOutput(endpoint);
    if (macHostError != noErr)
    {
        pm_hosterror = macHostError;
        sprintf
        (
            pm_hosterror_text, "Host error %ld: MIDIFlushOutput()",
            (long) macHostError
        );
        err = pmHostError;
    }
    return err;
}

/**
 *
 */

static PmError
midi_write_flush(PmInternal * midi, PmTimestamp timestamp)
{
    OSStatus macHostError;
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    MIDIEndpointRef endpoint = (MIDIEndpointRef)(long)
        descriptors[midi->device_id].descriptor;

    assert(m);
    assert(endpoint);
    if (m->packet != NULL)
    {
        /*
         * We are out of space; send the buffer and start refilling it.  Before we
         * can send, maybe delay to limit the data rate. OS X allows (only) 15KB/s.
         */

        UInt64 now = AudioGetCurrentHostTime();
        if (now < m->min_next_time)
        {
            usleep
            (
                (useconds_t) ((m->min_next_time - now) * m->us_per_host_tick)
            );
        }
        macHostError = MIDISend(portOut, endpoint, m->packetList);
        m->packet = NULL;               /* indicate no data in packetList now */
        m->min_next_time = now + m->byte_count * m->host_ticks_per_byte;
        m->byte_count = 0;
        if (macHostError != noErr) goto send_packet_error;
    }
    return pmNoError;

send_packet_error:

    pm_hosterror = macHostError;
    sprintf                             /* PLEASE make these a function.    */
    (
        pm_hosterror_text, "Host error %ld: MIDISend() in midi_write()",
        (long) macHostError
    );
    return pmHostError;
}

/**
 *
 */

static PmError
send_packet
(
    PmInternal * midi,
    Byte * message, unsigned messageLength,
    MIDITimeStamp timestamp
)
{
    PmError err;
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    assert(m);
    m->packet = MIDIPacketListAdd
    (
        m->packetList, sizeof(m->packetBuffer),
        m->packet, timestamp, messageLength, message
    );
    m->byte_count += messageLength;
    if (m->packet == NULL)
    {
        /*
         * We are out of space; send the buffer and start refilling it.  Make
         * midi->packet non-null to fool midi_write_flush into sending.  The
         * timestamp is 0 because midi_write_flush() ignores the timestamp since
         * timestamps are already in packets. The timestamp parameter is here
         * because other API's need it.  midi_write_flush() can be called from
         * system-independent code that must be cross-API.
         */

        m->packet = (MIDIPacket *) 4;
        if ((err = midi_write_flush(midi, 0)) != pmNoError)
            return err;

        m->packet = MIDIPacketListInit(m->packetList);
        assert(m->packet);          /* if this fails, it's a programming error */
        m->packet = MIDIPacketListAdd
        (
            m->packetList, sizeof(m->packetBuffer), m->packet, timestamp,
            messageLength, message
        );
        assert(m->packet);          /* can't run out of space on first message */
    }
    return pmNoError;
}

/**
 *
 */

static PmError
midi_write_short (PmInternal * midi, PmEvent * event)
{
    PmTimestamp when = event->timestamp;
    PmMessage what = event->message;
    MIDITimeStamp timestamp;
    UInt64 when_ns;
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    Byte message[4];
    unsigned messageLength;
    if (m->packet == NULL)
    {
        m->packet = MIDIPacketListInit(m->packetList);

        /*
         * This can never fail, right? Failure would indicate something
         * unrecoverable.
         */

        assert(m->packet);
    }

    if (when == 0)
        when = midi->now;                       /* compute timestamp */

    /*
     * If latency == 0, midi->now is not valid. We will just set it to zero.
     */

    if (midi->latency == 0)
        when = 0;

    when_ns = ((UInt64)(when + midi->latency) * (UInt64) 1000000) + m->delta;
    timestamp = (MIDITimeStamp) AudioConvertNanosToHostTime(when_ns);
    message[0] = Pm_MessageStatus(what);
    message[1] = Pm_MessageData1(what);
    message[2] = Pm_MessageData2(what);
    messageLength = midi_length(what);

    /*
     * Make sure we go foreward in time.
     */

    if (timestamp < m->min_next_time)
        timestamp = m->min_next_time;

#ifdef LIMIT_RATE
    if (timestamp < m->last_time)
        timestamp = m->last_time;

    m->last_time = timestamp + messageLength * m->host_ticks_per_byte;
#endif

    /*
     * Add this message to the packet list.
     */

    return send_packet(midi, message, messageLength, timestamp);
}

/**
 *
 */

static PmError
midi_begin_sysex (PmInternal * midi, PmTimestamp when)
{
    UInt64 when_ns;
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    assert(m);
    m->sysex_byte_count = 0;
    if (when == 0)
        when = midi->now;                       /* compute timestamp */

    /*
     * If latency == 0, midi->now is not valid. We will just set it to zero.
     */

    if (midi->latency == 0)
        when = 0;

    when_ns = ((UInt64)(when + midi->latency) * (UInt64) 1000000) + m->delta;
    m->sysex_timestamp = (MIDITimeStamp) AudioConvertNanosToHostTime(when_ns);
    if (m->packet == NULL)
    {
        m->packet = MIDIPacketListInit(m->packetList);

        /*
         * This can never fail, right? failure would indicate something
         * unrecoverable.
         */

        assert(m->packet);
    }
    return pmNoError;
}

/**
 *
 */

static PmError
midi_end_sysex (PmInternal * midi, PmTimestamp when)
{
    PmError err;
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    assert(m);

    if (m->sysex_timestamp < m->min_next_time) /* ensure we go foreward in time */
        m->sysex_timestamp = m->min_next_time;

#ifdef LIMIT_RATE
    if (m->sysex_timestamp < m->last_time)
        m->sysex_timestamp = m->last_time;

    m->last_time = m->sysex_timestamp + m->sysex_byte_count * m->host_ticks_per_byte;
#endif

    /*
     * Now send what's in the buffer.
     */

    err = send_packet(midi, m->sysex_buffer, m->sysex_byte_count, m->sysex_timestamp);
    m->sysex_byte_count = 0;
    if (err != pmNoError)
    {
        m->packet = NULL;           /* flush everything in the packet list */
        return err;
    }
    return pmNoError;
}

/**
 *
 */

static PmError
midi_write_byte (PmInternal * midi, unsigned char byte, PmTimestamp timestamp)
{
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    assert(m);
    if (m->sysex_byte_count >= SYSEX_BUFFER_SIZE)
    {
        PmError err = midi_end_sysex(midi, timestamp);
        if (err != pmNoError)
            return err;
    }
    m->sysex_buffer[m->sysex_byte_count++] = byte;
    return pmNoError;
}

/**
 *  To send a realtime message during a SysEx message, first flush all pending
 *  SysEx bytes into the packet list.  then we can just do a normal
 *  midi_write_short().
 *
 */

static PmError
midi_write_realtime (PmInternal * midi, PmEvent * event)
{
    PmError err = midi_end_sysex(midi, 0);
    if (err != pmNoError)
        return err;

    return midi_write_short(midi, event);
}

/**
 *
 */

static unsigned
midi_has_host_error (PmInternal * midi)
{
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    return (m->callback_error[0] != 0) || (m->error[0] != 0);
}

/**
 *
 */

static void
midi_get_host_error(PmInternal *midi, char *msg, unsigned len)
{
    midi_macosxcm_type m = (midi_macosxcm_type) midi->descriptor;
    msg[0] = 0;                                 /* initialize to empty string */
    if (m)                          /* ensure there's an open device to examine */
    {
        if (m->error[0])
        {
            strncpy(msg, m->error, len);
            m->error[0] = 0;                    /* clear the error      */
        }
        else if (m->callback_error[0])
        {
            strncpy(msg, m->callback_error, len);
            m->callback_error[0] = 0;           /* clear the error      */
        }
        msg[len - 1] = 0;                       /* terminate string     */
    }
}

/**
 *
 */

MIDITimeStamp
timestamp_pm_to_cm (PmTimestamp timestamp)
{
    UInt64 nanos;
    if (timestamp <= 0)
    {
        return (MIDITimeStamp) 0;
    }
    else
    {
        nanos = (UInt64)timestamp * (UInt64) 1000000;
        return (MIDITimeStamp)AudioConvertNanosToHostTime(nanos);
    }
}

/**
 *
 */

PmTimestamp timestamp_cm_to_pm(MIDITimeStamp timestamp)
{
    UInt64 nanos;
    nanos = AudioConvertHostTimeToNanos(timestamp);
    return (PmTimestamp)(nanos / (UInt64)1000000);
}

/**
 *  Code taken from http://developer.apple.com/qa/qa2004/qa1374.html.
 *
 *  Obtain the name of an endpoint without regard for whether it has connections.
 *  The result should be released by the caller.
 */

CFStringRef
EndpointName (MIDIEndpointRef endpoint, bool isExternal)
{
    CFMutableStringRef result = CFStringCreateMutable(NULL, 0);
    CFStringRef str;

    // begin with the endpoint's name

    str = NULL;
    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &str);
    if (str != NULL)
    {
        CFStringAppend(result, str);
        CFRelease(str);
    }

    MIDIEntityRef entity = NULL_REF;
    MIDIEndpointGetEntity(endpoint, &entity);
    if (entity == NULL_REF)                     // probably virtual
        return result;

    if (CFStringGetLength(result) == 0)
    {
        // endpoint name has zero length -- try the entity

        str = NULL;
        MIDIObjectGetStringProperty(entity, kMIDIPropertyName, &str);
        if (str != NULL)
        {
            CFStringAppend(result, str);
            CFRelease(str);
        }
    }

    // now consider the device's name

    MIDIDeviceRef device = NULL_REF;
    MIDIEntityGetDevice(entity, &device);
    if (device == NULL_REF)
        return result;

    str = NULL;
    MIDIObjectGetStringProperty(device, kMIDIPropertyName, &str);
    if (CFStringGetLength(result) == 0)
    {
        CFRelease(result);
        return str;
    }
    if (str != NULL)
    {
        /*
         * If an external device has only one entity, throw away the endpoint name
         * and just use the device name.
         */

        if (isExternal && MIDIDeviceGetNumberOfEntities(device) < 2)
        {
            CFRelease(result);
            return str;
        }
        else
        {
            if (CFStringGetLength(str) == 0)
            {
                CFRelease(str);
                return result;
            }

            /*
             * Does the entity name already start with the device name?  (Some
             * drivers do this, though they shouldn't).  If so, do not prepend.
             */

            if
            (
                CFStringCompareWithOptions
                (
                    result,                             /* endpoint name    */
                    str,                                /* device name      */
                    CFRangeMake(0, CFStringGetLength(str)), 0
                ) != kCFCompareEqualTo)
            {
                /*
                 * Prepend the device name to the entity name.
                 */

                if (CFStringGetLength(result) > 0)
                    CFStringInsert(result, 0, CFSTR(" "));

                CFStringInsert(result, 0, str);
            }
            CFRelease(str);
        }
    }
    return result;
}

/**
 *  Obtains the name of an endpoint, following connections.  The result should be
 *  released by the caller.
 */

static CFStringRef
ConnectedEndpointName (MIDIEndpointRef endpoint)
{
    CFMutableStringRef result = CFStringCreateMutable(NULL, 0);
    CFStringRef str;
    OSStatus err;
    long i;

    CFDataRef connections = NULL;       // Does the endpoint have connections?
    long nConnected = 0;
    bool anyStrings = false;
    err = MIDIObjectGetDataProperty
    (
        endpoint, kMIDIPropertyConnectionUniqueID, &connections
    );
    if (connections != NULL)
    {
        /*
         * It has connections, follow them.  Concatenate the names of all connected
         * devices.
         */

        nConnected = CFDataGetLength(connections) / (int32_t) sizeof(MIDIUniqueID);
        if (nConnected)
        {
            const SInt32 * pid = (const SInt32 *) CFDataGetBytePtr(connections);
            for (i = 0; i < nConnected; ++i, ++pid)
            {
                MIDIUniqueID id = EndianS32_BtoN(*pid);
                MIDIObjectRef connObject;
                MIDIObjectType connObjectType;
                err = MIDIObjectFindByUniqueID(id, &connObject, &connObjectType);
                if (err == noErr)
                {
                    if
                    (
                        connObjectType == kMIDIObjectType_ExternalSource  ||
                        connObjectType == kMIDIObjectType_ExternalDestination
                    )
                    {
                        /*
                         * Connected to an external device's endpoint (10.3 and
                         * later).
                         */

                        str = EndpointName((MIDIEndpointRef)(connObject), true);
                    }
                    else
                    {
                        /*
                         * Connected to an external device (10.2) (or something
                         * else, catch-all).
                         */

                        str = NULL;
                        MIDIObjectGetStringProperty
                        (
                            connObject, kMIDIPropertyName, &str
                        );
                    }
                    if (str != NULL)
                    {
                        if (anyStrings)
                            CFStringAppend(result, CFSTR(", "));
                        else
                            anyStrings = true;

                        CFStringAppend(result, str);
                        CFRelease(str);
                    }
                }
            }
        }
        CFRelease(connections);
    }
    if (anyStrings)
        return result;

    /*
     * Here, either the endpoint had no connections, or we failed to obtain names
     * for any of them.
     */

    return EndpointName(endpoint, false);
}

/**
 *
 */

char * cm_get_full_endpoint_name (MIDIEndpointRef endpoint)
{
#ifdef OLDCODE
    MIDIEntityRef entity;
    MIDIDeviceRef device;
    CFStringRef endpointName = NULL;
    CFStringRef deviceName = NULL;
#endif

    CFStringRef fullName = NULL;
    CFStringEncoding defaultEncoding;
    char * newName;

    defaultEncoding = CFStringGetSystemEncoding(); /* get default string encoding */
    fullName = ConnectedEndpointName(endpoint);

#ifdef OLDCODE
    MIDIEndpointGetEntity(endpoint, &entity); /* get the entity and device info */
    MIDIEntityGetDevice(entity, &device);

    /* create the nicely formated name */

    MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &endpointName);
    MIDIObjectGetStringProperty(device, kMIDIPropertyName, &deviceName);
    if (deviceName != NULL)
    {
        fullName = CFStringCreateWithFormat
        (
            NULL, NULL, CFSTR("%@: %@"), deviceName, endpointName
        );
    }
    else
    {
        fullName = endpointName;
    }
#endif

    /* copy the string into our buffer */

    newName = (char *) malloc(CFStringGetLength(fullName) + 1);
    CFStringGetCString
    (
        fullName, newName, CFStringGetLength(fullName) + 1, defaultEncoding
    );

    /* clean up */

#ifdef OLDCODE
    if (endpointName)
        CFRelease(endpointName);

    if (deviceName)
        CFRelease(deviceName);
#endif
    if (fullName)
        CFRelease(fullName);

    return newName;
}

/**
 *
 */

pm_fns_node pm_macosx_in_dictionary =
{
    none_write_short,
    none_sysex,
    none_sysex,
    none_write_byte,
    none_write_short,
    none_write_flush,
    none_synchronize,
    midi_in_open,
    midi_abort,
    midi_in_close,
    success_poll,
    midi_has_host_error,
    midi_get_host_error,
};

/**
 *
 */

pm_fns_node pm_macosx_out_dictionary =
{
    midi_write_short,
    midi_begin_sysex,
    midi_end_sysex,
    midi_write_byte,
    midi_write_realtime,
    midi_write_flush,
    midi_synchronize,
    midi_out_open,
    midi_abort,
    midi_out_close,
    success_poll,
    midi_has_host_error,
    midi_get_host_error,
};

/**
 *
 */

PmError
pm_macosxcm_init (void)
{
    ItemCount numInputs, numOutputs, numDevices;
    MIDIEndpointRef endpoint;
    int i;
    OSStatus macHostError;
    char *error_text;

    /*
     * Determine the number of MIDI devices on the system.
     */

    numDevices = MIDIGetNumberOfDevices();
    numInputs = MIDIGetNumberOfSources();
    numOutputs = MIDIGetNumberOfDestinations();

    /*
     * Return prematurely if no devices exist on the system Note that this is not
     * an error. There may be no devices.  Pm_CountDevices() will return zero,
     * which is correct and useful information
     */

    if (numDevices <= 0)
        return pmNoError;

    /*
     * Initialize the client handle. Then create the input port.  Then create the
     * output port.
     */

    macHostError = MIDIClientCreate(CFSTR("PortMidi"), NULL, NULL, &client);
    if (macHostError != noErr)
    {
        error_text = "MIDIClientCreate() in pm_macosxcm_init()";
        goto error_return;
    }
    macHostError = MIDIInputPortCreate
    (
        client, CFSTR("Input port"), readProc, NULL, &portIn
    );
    if (macHostError != noErr)
    {
        error_text = "MIDIInputPortCreate() in pm_macosxcm_init()";
        goto error_return;
    }
    macHostError = MIDIOutputPortCreate(client, CFSTR("Output port"), &portOut);
    if (macHostError != noErr)
    {
        error_text = "MIDIOutputPortCreate() in pm_macosxcm_init()";
        goto error_return;
    }

    for (i = 0; i < numInputs; ++i)         /* iterate over MIDI in devices */
    {
        endpoint = MIDIGetSource(i);
        if (endpoint == NULL_REF)
            continue;

        /*
         * Set the first input we see to the default.  Then register this device
         * with PortMidi.
         */

        if (pm_default_input_device_id == -1)
            pm_default_input_device_id = pm_descriptor_index;

        pm_add_device
        (
            "CoreMIDI", cm_get_full_endpoint_name(endpoint),
            TRUE, (void *) (long) endpoint, &pm_macosx_in_dictionary,
            i, 0                            /* client and port, TODO        */
        );
    }

    for (i = 0; i < numOutputs; ++i)        /* iterate over MIDI out devs   */
    {
        endpoint = MIDIGetDestination(i);
        if (endpoint == NULL_REF)
            continue;

        /*
         * Set the first output we see to the default.  Then register this device
         * with PortMidi.
         */

        if (pm_default_output_device_id == -1)
            pm_default_output_device_id = pm_descriptor_index;

        pm_add_device
        (
            "CoreMIDI", cm_get_full_endpoint_name(endpoint),
            FALSE, (void *)(long) endpoint, &pm_macosx_out_dictionary,
            i, 0                            /* client and port, TODO        */
        );
    }
    return pmNoError;

error_return:

    pm_hosterror = macHostError;
    sprintf
    (
        pm_hosterror_text, "Host error %ld: %s\n",
        (long) macHostError, error_text
    );
    pm_macosxcm_term();                     /* clear out any opened ports */
    return pmHostError;
}

/**
 *
 */

void
pm_macosxcm_term (void)
{
    if (client != NULL_REF)
        MIDIClientDispose(client);

    if (portIn != NULL_REF)
        MIDIPortDispose(portIn);

    if (portOut != NULL_REF)
        MIDIPortDispose(portOut);
}

/*
 * pmmacosxcm.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


