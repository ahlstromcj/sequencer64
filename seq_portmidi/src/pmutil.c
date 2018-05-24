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
 * \file        pmutil.c
 *
 *      Some helpful utilities for building MIDI applications that use
 *      PortMidi.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-05-24
 * \license     GNU GPLv2 or above
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>                     /* bzero(3) in Linux            */

#include "portmidi.h"
#include "pmutil.h"
#include "pminternal.h"

#ifdef PLATFORM_WINDOWS
#define bzero(addr, siz)    memset(addr, 0, siz)
#endif

/**
 *  PortMidi queue element.
 */

typedef struct
{
    long head;
    long tail;
    long len;
    long overflow;
    int32_t msg_size; /* number of int32_t in a message including extra word */
    int32_t peek_overflow;
    int32_t * buffer;
    int32_t * peek;
    int32_t peek_flag;

} PmQueueRep;

/**
 *
 */

PMEXPORT PmQueue *
Pm_QueueCreate (long num_msgs, int32_t bytes_per_msg)
{
    const size_t sint32 = sizeof(int32_t);
    int32_t int32s_per_msg = (int32_t)
    (
        ( (bytes_per_msg + sint32 - 1) & ~(sint32 - 1) ) / sint32
    );
    PmQueueRep *queue = (PmQueueRep *) pm_alloc(sizeof(PmQueueRep));
    if (! queue)                            /* memory allocation failed */
        return NULL;

    /*
     * need extra word per message for non-zero encoding
     */

    queue->len = num_msgs * (int32s_per_msg + 1);
    queue->buffer = (int32_t *) pm_alloc(queue->len * sint32);
    bzero(queue->buffer, queue->len * sint32);  // memset() ???
    if (! queue->buffer)
    {
        pm_free(queue);
        return NULL;
    }
    else
    {
        /* allocate the "peek" buffer */

        queue->peek = (int32_t *) pm_alloc(int32s_per_msg * sint32);
        if (! queue->peek)
        {
            /* free everything allocated so far and return */

            pm_free(queue->buffer);
            pm_free(queue);
            return NULL;
        }
    }
    bzero(queue->buffer, queue->len * sint32);
    queue->head = 0;
    queue->tail = 0;

    /* msg_size is in words */

    queue->msg_size = int32s_per_msg + 1; /* note extra word is counted */
    queue->overflow = FALSE;
    queue->peek_overflow = FALSE;
    queue->peek_flag = FALSE;
    return queue;
}

/**
 *
 */

PMEXPORT PmError
Pm_QueueDestroy (PmQueue * q)
{
    PmQueueRep * queue = (PmQueueRep *) q;
    if (!queue || ! queue->buffer || ! queue->peek) /* arg checking */
        return pmBadPtr;

    pm_free(queue->peek);
    pm_free(queue->buffer);
    pm_free(queue);
    return pmNoError;
}

/**
 *
 */

PMEXPORT PmError
Pm_Dequeue(PmQueue * q, void * msg)
{
    static const size_t sint32 = sizeof(int32_t);
    long head;
    PmQueueRep * queue = (PmQueueRep *) q;
    int i;
    int32_t * msg_as_int32 = (int32_t *) msg;

    if (! queue)                                    /* arg checking */
        return pmBadPtr;

    /*
     * A previous peek operation encountered an overflow, but the overflow has
     * not yet been reported to client, so do it now. No message is returned,
     * but on the next call, we will return the peek buffer.
     */

    if (queue->peek_overflow)
    {
        queue->peek_overflow = FALSE;
        return pmBufferOverflow;
    }
    if (queue->peek_flag)
    {
        memcpy(msg, queue->peek, (queue->msg_size - 1) * sint32);
        queue->peek_flag = FALSE;
        return pmGotData;
    }

    head = queue->head;

    /*
     * If writer overflows, it writes queue->overflow = tail+1 so that when the
     * reader gets to that position in the buffer, it can return the overflow
     * condition to the reader. The problem is that at overflow, things have
     * wrapped around, so tail == head, and the reader will detect overflow
     * immediately instead of waiting until it reads everything in the buffer,
     * wrapping around again to the point where tail == head. So the condition
     * also checks that queue->buffer[head] is zero -- if so, then the buffer
     * is now empty, and we're at the point in the msg stream where overflow
     * occurred. It's time to signal overflow to the reader. If
     * queue->buffer[head] is non-zero, there's a message there and we should
     * read all the way around the buffer before signalling overflow.  There is
     * a write-order dependency here, but to fail, the overflow field would
     * have to be written while an entire buffer full of writes are still
     * pending. I'm assuming out-of-order writes are possible, but not that
     * many.
     */

    if (queue->overflow == head + 1 && ! queue->buffer[head])
    {
        queue->overflow = 0;                    /* non-overflow condition */
        return pmBufferOverflow;
    }

    /*
     * Test to see if there is data in the queue -- test from back to front so
     * if writer is simultaneously writing, we don't waste time discovering the
     * write is not finished.
     */

    for (i = queue->msg_size - 1; i >= 0; --i)
    {
        if (! queue->buffer[head + i])
            return pmNoData;
    }
    memcpy(msg, (char *) &queue->buffer[head+1], sint32 * (queue->msg_size-1));
    i = queue->buffer[head];                                /* fix up zeros */
    while (i < queue->msg_size)
    {
        int32_t j;
        --i;                    /* msg does not have extra word; shift down */
        j = msg_as_int32[i];
        msg_as_int32[i] = 0;
        i = j;
    }

    /* signal that data has been removed by zeroing: */

    bzero((char *) &queue->buffer[head], sint32 * queue->msg_size); // memset()
    head += queue->msg_size;                            /* update head */
    if (head == queue->len)
        head = 0;

    queue->head = head;
    return pmGotData;                                   /* success */
}

/**
 *
 */

PMEXPORT PmError
Pm_SetOverflow (PmQueue * q)
{
    PmQueueRep * queue = (PmQueueRep *) q;
    long tail;
    if (! queue)                                        /* arg checking */
        return pmBadPtr;

    /* no more enqueue until receiver acknowledges overflow */

    if (queue->overflow)
        return pmBufferOverflow;

    tail = queue->tail;
    queue->overflow = tail + 1;
    return pmBufferOverflow;
}

/**
 *
 */

PMEXPORT PmError
Pm_Enqueue (PmQueue * q, void * msg)
{
    PmQueueRep * queue = (PmQueueRep *) q;
    long tail;
    int i;
    int32_t * src = (int32_t *) msg;
    int32_t * ptr;
    int32_t * dest;
    int rslt;
    if (! queue)
        return pmBadPtr;

    /* no more enqueue until receiver acknowledges overflow */

    if (queue->overflow)
        return pmBufferOverflow;

    rslt = Pm_QueueFull(q);

    /* already checked above: if (rslt == pmBadPtr) return rslt; */

    tail = queue->tail;
    if (rslt)
    {
        queue->overflow = tail + 1;
        return pmBufferOverflow;
    }

    /* queue is has room for message, and overflow flag is cleared */

    ptr = &queue->buffer[tail];
    dest = ptr + 1;
    for (i = 1; i < queue->msg_size; i++)
    {
        int32_t j = src[i - 1];
        if (! j)
        {
            *ptr = i;
            ptr = dest;
        }
        else
        {
            *dest = j;
        }
        dest++;
    }
    *ptr = i;
    tail += queue->msg_size;
    if (tail == queue->len)
        tail = 0;

    queue->tail = tail;
    return pmNoError;
}

/**
 *  null pointer -> return "empty"
 */

PMEXPORT int
Pm_QueueEmpty (PmQueue * q)
{
    PmQueueRep * queue = (PmQueueRep *) q;
    return (! queue) || (queue->buffer[queue->head] == 0 && ! queue->peek_flag);
}

/**
 *
 */

PMEXPORT int
Pm_QueueFull (PmQueue * q)
{
    long tail;
    int i;
    PmQueueRep * queue = (PmQueueRep *) q;
    if (! queue)                                        /* arg checking */
        return pmBadPtr;

    /* test to see if there is space in the queue */

    tail = queue->tail;
    for (i = 0; i < queue->msg_size; ++i)
    {
        if (queue->buffer[tail + i])
            return TRUE;
    }
    return FALSE;
}

/**
 *
 */

PMEXPORT void *
Pm_QueuePeek (PmQueue * q)
{
    PmError rslt;
    int32_t temp;
    PmQueueRep * queue = (PmQueueRep *) q;
    if (! queue)                                        /* arg checking */
        return NULL;

    if (queue->peek_flag)
        return queue->peek;

    /*
     * This is ugly: if peek_overflow is set, then Pm_Dequeue() returns
     * immediately with pmBufferOverflow, but here, we want Pm_Dequeue() to
     * really check for data. If data is there, we can return it.
     */

    temp = queue->peek_overflow;
    queue->peek_overflow = FALSE;
    rslt = Pm_Dequeue(q, queue->peek);
    queue->peek_overflow = temp;
    if (rslt == 1)
    {
        queue->peek_flag = TRUE;
        return queue->peek;
    }
    else if (rslt == pmBufferOverflow)
    {
        /*
         * When overflow is indicated, the queue is empty and the first message
         * that was dropped by Enqueue (signalling pmBufferOverflow to its
         * caller) would have been the next message in the queue. Pm_QueuePeek
         * will return NULL, but remember that an overflow occurred. (see
         * Pm_Dequeue)
         */

        queue->peek_overflow = TRUE;
    }
    return NULL;
}

/*
 * pmutil.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

