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
 * \file        pmlinuxalsa.c
 *
 *      System-specific definitions for Linux's ALSA MIDI subsystem.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-05-24
 * \license     GNU GPLv2 or above
 *
 * Written by:
 *
 *  -   Roger Dannenberg (port to Alsa 0.9.x)
 *  -   Clemens Ladisch (provided code examples and invaluable consulting)
 *  -   Jason Cohen, Rico Colon, Matt Filippone (Alsa 0.5.x implementation)
 */

#include <string.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#include "platform_macros.h"            /* UNUSED() parameter macro         */
#include "easy_macros.h"                /* not_nullptr() macro, etc.        */
#include "portmidi.h"
#include "pmutil.h"
#include "pmlinuxalsa.h"
#include "porttime.h"
#include "pmlinux.h"

/*
 *  I used many print statements to debug this code. I left them in the
 *  source, and you can turn them on by changing false to true below:
 */

#define VERBOSE_ON          0                   // TEMPORARY

/*
 *  Check for ALSA version.
 */

#if SND_LIB_MAJOR == 0 && SND_LIB_MINOR < 9
#error needs ALSA 0.9.0 or later
#endif

/**
 *  These defines are added to help store client/port information in the
 *  device descriptor.  One issue with the existing PortMidi library is that
 *  we're casting from a pointer to an integer of different size.  So we add
 *  an intprt_t cast into the mix.
 *
 *  Weird:  In C macros, the arguments in parentheses must have no space
 *  beefore the opening parenthesis.
 */

#define MAKE_DESCRIPTOR(client, port) \
    ( (void *) (intptr_t) ( ((client) << 8) | (port)) )

#define MASK_DESCRIPTOR_CLIENT(x)       ( (((int) (intptr_t) (x)) >> 8) & 0xff )

#define MASK_DESCRIPTOR_PORT(x)         ( ((int) (intptr_t) (x)) & 0xff )

extern pm_fns_node pm_linuxalsa_in_dictionary;
extern pm_fns_node pm_linuxalsa_out_dictionary;

/**
 * All input comes here, output queue allocated on seq.
 */

static snd_seq_t * s_seq = nullptr;

/**
 *  Provides an item to hold the ALSA queue.
 */

static int s_queue;                 /* one for all ports, reference counted */

/**
 *  A boolean that prevents the ALSA queue from getting reset if it has
 *  already been set.
 */

static int s_queue_used;            /* one for all ports, reference counted */

/**
 *  Holds information about an ALSA port.
 */

typedef struct alsa_descriptor_struct
{
    int client;
    int port;
    int this_port;
    int in_sysex;
    snd_midi_event_t * parser;
    int error;                      /* host error code */

} alsa_descriptor_node, * alsa_descriptor_type;

/**
 *  Copies error text to a potentially short string.
 */

static void
get_alsa_error_text (char * msg, int len, int err)
{
    int errlen = strlen(snd_strerror(err));
    if (errlen < len)
    {
        strcpy(msg, snd_strerror(err));
    }
    else if (len > 20)
    {
        sprintf(msg, "ALSA error %d", err);
    }
    else if (len > 4)
    {
        strcpy(msg, "ALSA");
    }
    else
    {
        msg[0] = 0;
    }
}

/**
 *  s_queue is shared by both input and output, reference counted
 *
 * \todo
 *      Make the tempo and PPQN settings variable.
 */

static PmError
alsa_use_queue (void)
{
    if (s_queue_used == 0 && not_nullptr(s_seq))
    {
        snd_seq_queue_tempo_t * tempo;
        s_queue = snd_seq_alloc_queue(s_seq);
        if (s_queue < 0)
        {
            pm_hosterror = s_queue;
            return pmHostError;
        }
        snd_seq_queue_tempo_alloca(&tempo);
        snd_seq_queue_tempo_set_tempo(tempo, Pt_Get_Tempo_Microseconds());
        snd_seq_queue_tempo_set_ppq(tempo, Pt_Get_Ppqn());
        pm_hosterror = snd_seq_set_queue_tempo(s_seq, s_queue, tempo);
        if (pm_hosterror < 0)
            return pmHostError;

        snd_seq_start_queue(s_seq, s_queue, NULL);
        snd_seq_drain_output(s_seq);
    }
    ++s_queue_used;
    return pmNoError;
}

#if 0
/*
 * EXPERIMENTAL.
 *
 *  The s_queue must already exist.
 */

static void
alsa_set_beats_per_minute (int tempo_us)
{
    if (s_queue >= 0 && not_nullptr(s_seq))
    {
        snd_seq_queue_tempo_t * tempo;
        snd_seq_queue_tempo_alloca(&tempo);
        snd_seq_queue_tempo_set_tempo(tempo, tempo_us);
        (void) snd_seq_set_queue_tempo(s_seq, s_queue, tempo);

        /*
         * Do we have a snd_seq_restart_queue() to call?
         */
    }
}

static void
alsa_set_ppqn (int ppqn)
{
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);
    snd_seq_queue_tempo_set_ppq(tempo, ppqn);

    /*
     * Do we have a snd_seq_restart_queue() to call?
     */
}

#endif

/**
 *
 */

static void
alsa_unuse_queue (void)
{
    if (--s_queue_used == 0)
    {
        snd_seq_stop_queue(s_seq, s_queue, NULL);
        snd_seq_drain_output(s_seq);
        snd_seq_free_queue(s_seq, s_queue);
        if (VERBOSE_ON)
            printf("s_queue freed\n");
    }
}

/**
 *  midi_message_length() -- how many bytes in a message?
 */

static int
midi_message_length (PmMessage message)
{
    message &= 0xff;
    if (message < 0x80)
    {
        return 0;
    }
    else if (message < 0xf0)
    {
        static const int length[] = {3, 3, 3, 3, 2, 2, 3};
        return length[(message - 0x80) >> 4];
    }
    else
    {
        static const int length[] =
        {
            -1, 2, 3, 2, 0, 0, 1, -1, 1, 0, 1, 1, 1, 0, 1, 1
        };
        return length[message - 0xf0];
    }
}

/**
 *
 */

static PmError
alsa_out_open (PmInternal * midi, void * UNUSED(driverinfo))
{
    void * client_port = pm_descriptors[midi->device_id].descriptor;
    alsa_descriptor_type desc = (alsa_descriptor_type)
        pm_alloc(sizeof(alsa_descriptor_node));

    snd_seq_port_info_t * info;
    int err;
    if (! desc)
        return pmInsufficientMemory;

    snd_seq_port_info_alloca(&info);
    snd_seq_port_info_set_port(info, midi->device_id);
    snd_seq_port_info_set_capability(info, SND_SEQ_PORT_CAP_WRITE |
        SND_SEQ_PORT_CAP_READ);

    snd_seq_port_info_set_type(info, SND_SEQ_PORT_TYPE_MIDI_GENERIC |
        SND_SEQ_PORT_TYPE_APPLICATION);

    snd_seq_port_info_set_port_specified(info, 1);
    err = snd_seq_create_port(s_seq, info);
    if (err < 0)
        goto free_desc;

    /* fill in fields of desc, which is passed to pm_write routines */

    midi->descriptor = desc;

//                 ((((int) (intptr_t) (x)) >> 8) & 0xff)
//  desc->client = ((((int) (intptr_t) (client_port)) >> 8) & 0xff);

    desc->client = MASK_DESCRIPTOR_CLIENT(client_port);
    desc->port = MASK_DESCRIPTOR_PORT(client_port);
    desc->this_port = midi->device_id;
    desc->in_sysex = 0;
    desc->error = 0;
    err = snd_midi_event_new(PM_DEFAULT_SYSEX_BUFFER_SIZE, &desc->parser);
    if (err < 0)
        goto free_this_port;

    if (midi->latency > 0)          /* must delay output using a queue */
    {
        err = alsa_use_queue();
        if (err < 0)
            goto free_parser;

        err = snd_seq_connect_to
        (
            s_seq, desc->this_port, desc->client, desc->port
        );
        if (err < 0)
            goto unuse_queue;           /* clean up and return on error */
    }
    else
    {
        err = snd_seq_connect_to
        (
            s_seq, desc->this_port, desc->client, desc->port
        );
        if (err < 0)
            goto free_parser;           /* clean up and return on error */
    }
    return pmNoError;

 unuse_queue:

    alsa_unuse_queue();

 free_parser:

    snd_midi_event_free(desc->parser);

 free_this_port:

    snd_seq_delete_port(s_seq, desc->this_port);

 free_desc:

    pm_free(desc);
    pm_hosterror = err;
    if (err < 0)
    {
        get_alsa_error_text(pm_hosterror_text, PM_HOST_ERROR_MSG_LEN, err);
    }
    return pmHostError;
}

/**
 * \note
 *      For cases where the user does not supply a time function, we could
 *      optimize the code by not starting Pt_Time and using the alsa tick time
 *      instead. I didn't do this because it would entail changing the queue
 *      management to start the queue tick count when PortMidi is initialized
 *      and keep it running until PortMidi is terminated. (This should be
 *      simple, but it's not how the code works now.) -- RBD
 */

static PmError
alsa_write_byte
(
    PmInternal * midi, midibyte_t byte, PmTimestamp timestamp
)
{
    alsa_descriptor_type desc = (alsa_descriptor_type) midi->descriptor;
    snd_seq_event_t ev;
    int err;
    snd_seq_ev_clear(&ev);
    if (snd_midi_event_encode_byte(desc->parser, byte, &ev) == 1)
    {
        snd_seq_ev_set_dest(&ev, desc->client, desc->port);
        snd_seq_ev_set_source(&ev, desc->this_port);
        if (midi->latency > 0)
        {
            /* compute relative time of event = timestamp - now + latency */

            PmTimestamp now = midi->time_proc ?
                midi->time_proc(midi->time_info) : Pt_Time(NULL) ;

            /*
             * if timestamp is zero, send immediately.
             * Otherwise compute time delay and use delay if positive.
             */

            int when = timestamp;
            if (when == 0)
                when = now;

            when = (when - now) + midi->latency;
            if (when < 0)
                when = 0;

            if (VERBOSE_ON)
                printf
                (
                    "Timestamp %d now %d latency %d, ",
                    (int) timestamp, (int) now, midi->latency
                );

            if (VERBOSE_ON)
                printf("Scheduling event after %d\n", when);

            /*
             * Message is sent in relative ticks, where 1 tick = 1 ms. See the
             * function banner note.
             */

            snd_seq_ev_schedule_tick(&ev, s_queue, 1, when);
        }
        else
        {
            /*
             * ev.queue = SND_SEQ_QUEUE_DIRECT;
             * ev.dest.client = SND_SEQ_ADDRESS_SUBSCRIBERS;
             */

            snd_seq_ev_set_direct(&ev);     /* send event out without queueing */
        }
        if (VERBOSE_ON)
            printf("Sending event\n");

        err = snd_seq_event_output(s_seq, &ev);
        if (err < 0)
        {
            desc->error = err;
            return pmHostError;
        }
    }
    return pmNoError;
}

/**
 *
 */

static PmError
alsa_out_close (PmInternal * midi)
{
    alsa_descriptor_type desc = (alsa_descriptor_type) midi->descriptor;
    if (! desc)
        return pmBadPtr;

    pm_hosterror =
        snd_seq_disconnect_to(s_seq, desc->this_port, desc->client, desc->port);

    if (pm_hosterror)
    {
        /*
         * If there's an error, try to delete the port anyway, but don't
         * change the pm_hosterror value so we retain the first error
         */

        snd_seq_delete_port(s_seq, desc->this_port);
    }
    else
    {
        /*
         * If there's no error, delete the port and retain any error
         */

        pm_hosterror = snd_seq_delete_port(s_seq, desc->this_port);
    }
    if (midi->latency > 0)
        alsa_unuse_queue();

    snd_midi_event_free(desc->parser);
    midi->descriptor = nullptr;    /* destroy pointer to signify "closed" */
    pm_free(desc);
    if (pm_hosterror)
    {
        get_alsa_error_text
        (
            pm_hosterror_text, PM_HOST_ERROR_MSG_LEN, pm_hosterror
        );
        return pmHostError;
    }
    return pmNoError;
}

/**
 *
 */

static PmError
alsa_in_open (PmInternal * midi, void * UNUSED(driverinfo))
{
    void * client_port = pm_descriptors[midi->device_id].descriptor;
    alsa_descriptor_type desc = (alsa_descriptor_type)
        pm_alloc(sizeof(alsa_descriptor_node));

    snd_seq_port_info_t * info;
    snd_seq_port_subscribe_t * sub;
    snd_seq_addr_t addr;
    int err;
    if (! desc)
        return pmInsufficientMemory;

    err = alsa_use_queue();
    if (err < 0)
        goto free_desc;

    snd_seq_port_info_alloca(&info);
    snd_seq_port_info_set_port(info, midi->device_id);
    snd_seq_port_info_set_capability(info, SND_SEQ_PORT_CAP_WRITE |
        SND_SEQ_PORT_CAP_READ);

    snd_seq_port_info_set_type(info, SND_SEQ_PORT_TYPE_MIDI_GENERIC |
        SND_SEQ_PORT_TYPE_APPLICATION);

    snd_seq_port_info_set_port_specified(info, 1);
    err = snd_seq_create_port(s_seq, info);
    if (err < 0)
        goto free_queue;

    /*
     * Fill in the fields of desc, which is passed to pm_write routines.
     */

    midi->descriptor = desc;
    desc->client = MASK_DESCRIPTOR_CLIENT(client_port);
    desc->port = MASK_DESCRIPTOR_PORT(client_port);
    desc->this_port = midi->device_id;
    desc->in_sysex = 0;
    desc->error = 0;

    if (VERBOSE_ON)
        printf
        (
            "snd_seq_connect_from: %d %d %d\n",
            desc->this_port, desc->client, desc->port
        );

    snd_seq_port_subscribe_alloca(&sub);
    addr.client = snd_seq_client_id(s_seq);
    addr.port = desc->this_port;
    snd_seq_port_subscribe_set_dest(sub, &addr);
    addr.client = desc->client;
    addr.port = desc->port;
    snd_seq_port_subscribe_set_sender(sub, &addr);
    snd_seq_port_subscribe_set_time_update(sub, 1);

    /*
     * This doesn't seem to work: messages come in with real timestamps.
     */

    snd_seq_port_subscribe_set_time_real(sub, 0);
    err = snd_seq_subscribe_port(s_seq, sub);

    /*
     * err = snd_seq_connect_from(s_seq, desc->this_port, desc->client,
     *      desc->port);
     */

    if (err < 0)
        goto free_this_port;            /* clean up and return on error */

    return pmNoError;

 free_this_port:

    snd_seq_delete_port(s_seq, desc->this_port);

 free_queue:

    alsa_unuse_queue();

 free_desc:

    pm_free(desc);
    pm_hosterror = err;
    if (err < 0)
    {
        get_alsa_error_text(pm_hosterror_text, PM_HOST_ERROR_MSG_LEN, err);
    }
    return pmHostError;
}

/**
 *
 */

static PmError
alsa_in_close (PmInternal * midi)
{
    alsa_descriptor_type desc = (alsa_descriptor_type) midi->descriptor;
    if (! desc)
        return pmBadPtr;

    pm_hosterror = snd_seq_disconnect_from
    (
        s_seq, desc->this_port, desc->client, desc->port
    );
    if (pm_hosterror)
    {
        snd_seq_delete_port(s_seq, desc->this_port);    /* try to close port */
    }
    else
    {
        pm_hosterror = snd_seq_delete_port(s_seq, desc->this_port);
    }
    alsa_unuse_queue();
    pm_free(desc);
    if (pm_hosterror)
    {
        get_alsa_error_text(pm_hosterror_text, PM_HOST_ERROR_MSG_LEN,
            pm_hosterror);

        return pmHostError;
    }
    return pmNoError;
}

/**
 * \note
 *      ALSA documentation is vague. This is supposed to remove any pending
 *      output messages. If you can test and confirm this code is correct,
 *      please update this comment.  Unfortunately, I can't even compile it --
 *      my ALSA version does not implement snd_seq_remove_events_t, so this
 *      does not compile. I'll try again, but it looks like I'll need to
 *      upgrade my entire Linux OS. -- RBD
 */

static PmError
alsa_abort (PmInternal * UNUSED(midi))
{
    /*
    alsa_descriptor_type desc = (alsa_descriptor_type) midi->descriptor;
    snd_seq_remove_events_t info;
    snd_seq_addr_t addr;
    addr.client = desc->client;
    addr.port = desc->port;
    snd_seq_remove_events_set_dest(&info, &addr);
    snd_seq_remove_events_set_condition(&info, SND_SEQ_REMOVE_DEST);
    pm_hosterror = snd_seq_remove_events(s_seq, &info);
    if (pm_hosterror)
    {
        get_alsa_error_text(pm_hosterror_text, PM_HOST_ERROR_MSG_LEN, pm_hosterror);
        return pmHostError;
    }
    */

    printf("WARNING: alsa_abort() not implemented.\n");
    return pmNoError;
}

/*
 * Removed the #ifdef GARBAGE code here.
 */

/**
 *
 */

static PmError
alsa_write_flush (PmInternal * midi, PmTimestamp UNUSED(timestamp))
{
    alsa_descriptor_type desc = (alsa_descriptor_type) midi->descriptor;

#if 0
    if (VERBOSE_ON)
        printf("snd_seq_drain_output: 0x%x\n", (unsigned) s_seq);
#endif

    desc->error = snd_seq_drain_output(s_seq);
    if (desc->error < 0)
        return pmHostError;

    desc->error = pmNoError;
    return pmNoError;
}

/**
 *
 */

static PmError
alsa_write_short (PmInternal * midi, PmEvent * event)
{
    int bytes = midi_message_length(event->message);
    PmMessage msg = event->message;
    int i;
    alsa_descriptor_type desc = (alsa_descriptor_type) midi->descriptor;
    for (i = 0; i < bytes; ++i)
    {
        midibyte_t byte = msg;
        if (VERBOSE_ON)
            printf("Sending 0x%x\n", byte);

        alsa_write_byte(midi, byte, event->timestamp);
        if (desc->error < 0)
            break;

        msg >>= 8;                  /* shift next byte into position */
    }
    if (desc->error < 0)
        return pmHostError;

    desc->error = pmNoError;
    return pmNoError;
}

/**
 * alsa_sysex -- implements begin_sysex and end_sysex
 */

PmError
alsa_sysex (PmInternal * UNUSED(midi), PmTimestamp UNUSED(timestamp))
{
    return pmNoError;
}

/**
 *  Linux implementation does not use this synchronize function.  Apparently,
 *  ALSA data is relative to the time you send it, and there is no reference.
 *  If this is true, this is a serious shortcoming of ALSA. If not true, then
 *  PortMidi has a serious shortcoming -- it should be scheduling relative to
 *  ALSA's time reference.
 */

static PmTimestamp
alsa_synchronize (PmInternal * UNUSED(midi))
{
    return 0;
}

/**
 *
 */

static void
handle_event (snd_seq_event_t * ev)
{
    int device_id = ev->dest.port;
    PmInternal * midi = pm_descriptors[device_id].internalDescriptor;
    PmTimeProcPtr time_proc = midi->time_proc;
    PmEvent pm_ev;
    PmTimestamp timestamp;

    /*
     * Time stamp should be in ticks, using our queue, where 1 tick = 1 ms.
     */

    assert((ev->flags & SND_SEQ_TIME_STAMP_MASK) == SND_SEQ_TIME_STAMP_TICK);
    if (time_proc == NULL)
    {
        timestamp = ev->time.tick;  /* no time_proc, return native ticks, ms */
    }
    else                            /* translate time to time_proc basis */
    {

        snd_seq_queue_status_t *queue_status;
        snd_seq_queue_status_alloca(&queue_status);
        snd_seq_get_queue_status(s_seq, s_queue, queue_status);

        /* return (now - alsa_now) + alsa_timestamp */

        timestamp = (*time_proc)(midi->time_info) + ev->time.tick -
            snd_seq_queue_status_get_tick_time(queue_status);
    }
    pm_ev.timestamp = timestamp;
    switch (ev->type)
    {
    case SND_SEQ_EVENT_NOTEON:
        pm_ev.message = Pm_Message(0x90 | ev->data.note.channel,
            ev->data.note.note & 0x7f, ev->data.note.velocity & 0x7f);

        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_NOTEOFF:
        pm_ev.message = Pm_Message(0x80 | ev->data.note.channel,
            ev->data.note.note & 0x7f, ev->data.note.velocity & 0x7f);

        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_KEYPRESS:
        pm_ev.message = Pm_Message(0xa0 | ev->data.note.channel,
            ev->data.note.note & 0x7f, ev->data.note.velocity & 0x7f);

        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_CONTROLLER:
        pm_ev.message = Pm_Message(0xb0 | ev->data.note.channel,
            ev->data.control.param & 0x7f, ev->data.control.value & 0x7f);

        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_PGMCHANGE:
        pm_ev.message = Pm_Message(0xc0 | ev->data.note.channel,
            ev->data.control.value & 0x7f, 0);

        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_CHANPRESS:
        pm_ev.message = Pm_Message(0xd0 | ev->data.note.channel,
            ev->data.control.value & 0x7f, 0);

        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_PITCHBEND:
        pm_ev.message = Pm_Message(0xe0 | ev->data.note.channel,
            (ev->data.control.value + 0x2000) & 0x7f,
            ((ev->data.control.value + 0x2000) >> 7) & 0x7f);

        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_CONTROL14:
        if (ev->data.control.param < 0x20)
        {
            pm_ev.message = Pm_Message(0xb0 | ev->data.note.channel,
                ev->data.control.param, (ev->data.control.value >> 7) & 0x7f);

            pm_read_short(midi, &pm_ev);
            pm_ev.message = Pm_Message(0xb0 | ev->data.note.channel,
                ev->data.control.param + 0x20, ev->data.control.value & 0x7f);

            pm_read_short(midi, &pm_ev);
        }
        else
        {
            pm_ev.message = Pm_Message(0xb0 | ev->data.note.channel,
                ev->data.control.param & 0x7f, ev->data.control.value & 0x7f);

            pm_read_short(midi, &pm_ev);
        }
        break;

    case SND_SEQ_EVENT_SONGPOS:
        pm_ev.message = Pm_Message(0xf2,
            ev->data.control.value & 0x7f, (ev->data.control.value >> 7) & 0x7f);

        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_SONGSEL:
        pm_ev.message = Pm_Message(0xf3, ev->data.control.value & 0x7f, 0);
        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_QFRAME:
        pm_ev.message = Pm_Message(0xf1, ev->data.control.value & 0x7f, 0);
        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_START:
        pm_ev.message = Pm_Message(0xfa, 0, 0);
        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_CONTINUE:
        pm_ev.message = Pm_Message(0xfb, 0, 0);
        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_STOP:
        pm_ev.message = Pm_Message(0xfc, 0, 0);
        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_CLOCK:
        pm_ev.message = Pm_Message(0xf8, 0, 0);
        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_TUNE_REQUEST:
        pm_ev.message = Pm_Message(0xf6, 0, 0);
        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_RESET:
        pm_ev.message = Pm_Message(0xff, 0, 0);
        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_SENSING:
        pm_ev.message = Pm_Message(0xfe, 0, 0);
        pm_read_short(midi, &pm_ev);
        break;

    case SND_SEQ_EVENT_SYSEX:
        {
            /*
             * Assume there is one SysEx byte to process.
             */

            const midibyte_t * ptr = (const midibyte_t *) ev->data.ext.ptr;
            pm_read_bytes(midi, ptr, ev->data.ext.len, timestamp);
            break;
        }
    }
}

#ifdef USE_PORTMIDI_SIMPLE_ALSA_POLL

/**
 *  This version just check for data.  DOES NOT WORK.
 *
 * \return
 *      Returns pmGotData if any data is pending.  Otherwise, pmNoData is
 *      returned.
 */

static PmError
alsa_poll (PmInternal * UNUSED(midi))
{
    int bytes = snd_seq_event_input_pending(s_seq, FALSE);
#if defined PLATFORM_DEBUG_TMI
    if (bytes > 0)
    {
        infoprint("alsa_poll(): incoming MIDI events detected");
    }
#endif
    return bytes > 0 ?  pmGotData : pmNoData ;
}

#else   // USE_PORTMIDI_SIMPLE_ALSA_POLL

/**
 *  Poll!  Checks for and ignore errors, e.g. input overflow.
 *
 *  There is an expensive check (while loop) for input data, and it gets data
 *  from device.
 *
 *  snd_seq_event_input_pending(s_seq, TRUE) checks the presence of events on
 *  the sequencer FIFO; these events are transferred to the input buffer, and
 *  the number of received events is returned.
 *
 *  snd_seq_event_input_pending(s_seq, FALSE) returns 0 if no events remain
 *  in the input buffer.  We think this is all we need for the poll function!
 *
 * \note
 *      If there's overflow, this should be reported all the way through to
 *      client. Since input from all devices is merged, we need to find all
 *      input devices and set all to the overflow state.  NOTE: this assumes
 *      every input is ALSA based.
 */

static PmError
alsa_poll (PmInternal * UNUSED(midi))
{
    snd_seq_event_t * ev;
    while (snd_seq_event_input_pending(s_seq, TRUE) > 0)    /* expensive!   */
    {
        /*
         * Cheap check on local input buffer.
         */

        while (snd_seq_event_input_pending(s_seq, FALSE) > 0)
        {
#if defined PLATFORM_DEBUG_TMI
            infoprint("alsa_poll(): incoming MIDI events detected");
#endif
            int rslt = snd_seq_event_input(s_seq, &ev);
            if (rslt >= 0)
            {
                handle_event(ev);                           /* much work!   */
            }
            else if (rslt == -ENOSPC)
            {
                int i;
                for (i = 0; i < pm_descriptor_index; i++)
                {
                    if (pm_descriptors[i].pub.input)
                    {
                        PmInternal * midi = (PmInternal *)
                                pm_descriptors[i].internalDescriptor;

                        /* careful, device may not be open! */

                        if (not_nullptr(midi))
                            Pm_SetOverflow(midi->queue);
                    }
                }
            }
        }
    }
    return pmNoError;
}

#endif   // USE_PORTMIDI_SIMPLE_ALSA_POLL

/**
 *
 */

static unsigned
alsa_has_host_error (PmInternal * midi)
{
    alsa_descriptor_type desc = (alsa_descriptor_type) midi->descriptor;
    return desc->error;
}

/**
 *
 *  alsa_get_host_error (PmInternal * midi, char * msg, unsigned len)
 */

static void
alsa_get_host_error (struct pm_internal_struct * midi, char * msg, unsigned len)
{
    alsa_descriptor_type desc = (alsa_descriptor_type) midi->descriptor;
    int err = (pm_hosterror || desc->error);
    get_alsa_error_text(msg, len, err);
}

/**
 *
 */

pm_fns_node pm_linuxalsa_in_dictionary =
{
    none_write_short,
    none_sysex,
    none_sysex,
    none_write_byte,
    none_write_short,
    none_write_flush,
    alsa_synchronize,
    alsa_in_open,
    alsa_abort,
    alsa_in_close,
    alsa_poll,
    alsa_has_host_error,
    alsa_get_host_error
};

/**
 *
 */

pm_fns_node pm_linuxalsa_out_dictionary =
{
    alsa_write_short,
    alsa_sysex,
    alsa_sysex,
    alsa_write_byte,
    alsa_write_short, /* short realtime message */
    alsa_write_flush,
    alsa_synchronize,
    alsa_out_open,
    alsa_abort,
    alsa_out_close,
    none_poll,
    alsa_has_host_error,
    alsa_get_host_error
};

/**
 *  Copies a string to the heap. Use this function, rather than strdup(), so
 *  that we call pm_alloc(), not malloc(). This allows PortMidi to avoid
 *  malloc() which might cause priority inversion. Probably ALSA is going to
 *  call malloc()l anyway, so this extra work here may be pointless.
 */

char *
pm_strdup (const char * s)
{
    int len = strlen(s);
    char * dup = (char *) pm_alloc(len + 1);
    strcpy(dup, s);
    return dup;
}

/**
 *  Previously, the last parameter to snd_seq_opten() was SND_SEQ_NONBLOCK,
 *  but this would cause messages to be dropped if the ALSA buffer fills up.
 *  The correct behavior is for writes to block until there is room to send
 *  all the data. The client should normally allocate a large enough buffer to
 *  avoid blocking on output.  Now that blocking is enabled, the
 *  seq_event_input() will block if there is no input data. This is not what
 *  we want, so must call seq_event_input_pending() to avoid blocking.
 */

PmError
pm_linuxalsa_init (void)
{
    snd_seq_client_info_t * cinfo;
    snd_seq_port_info_t * pinfo;
    unsigned caps;
    int err = snd_seq_open(&s_seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if (err < 0)
        return err;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(s_seq, cinfo) == 0)
    {
        int client = snd_seq_client_info_get_client(cinfo);
        int port = -1;
        const char * portname = "unknown";
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, port);
        while (snd_seq_query_next_port(s_seq, pinfo) == 0)
        {
            client = snd_seq_client_info_get_client(cinfo);
            if (client == SND_SEQ_CLIENT_SYSTEM)
                continue; /* ignore Timer and Announce ports on client 0 */

            caps = snd_seq_port_info_get_capability(pinfo);
            if
            (
                ! (
                    caps &
                    (SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_SUBS_WRITE)
                )
            )
            {
                continue; /* ignore if you cannot read or write port */
            }

            if (caps & SND_SEQ_PORT_CAP_SUBS_WRITE)
            {
                if (pm_default_output_device_id == -1)
                    pm_default_output_device_id = pm_descriptor_index;

                portname = snd_seq_port_info_get_name(pinfo);
                client = snd_seq_port_info_get_client(pinfo);
                port = snd_seq_port_info_get_port(pinfo);
                pm_add_device
                (
                    "ALSA", pm_strdup(portname), FALSE,
                    MAKE_DESCRIPTOR(client, port),
                    &pm_linuxalsa_out_dictionary,
                    client, port                    /* new parameters   */
                );
            }
            if (caps & SND_SEQ_PORT_CAP_SUBS_READ)
            {
                if (pm_default_input_device_id == -1)
                    pm_default_input_device_id = pm_descriptor_index;

                pm_add_device
                (
                    "ALSA", pm_strdup(portname), TRUE,
                    MAKE_DESCRIPTOR(client, port),
                    &pm_linuxalsa_in_dictionary,
                    client, port                    /* new parameters   */
                );
            }
        }
    }
    return pmNoError;
}

/**
 *
 */

void
pm_linuxalsa_term (void)
{
    if (s_seq)
    {
        snd_seq_close(s_seq);
        pm_free(pm_descriptors);
        pm_descriptors = nullptr;
        pm_descriptor_index = 0;
        pm_descriptor_max = 0;
    }
}

/*
 * pmlinuxalsa.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

