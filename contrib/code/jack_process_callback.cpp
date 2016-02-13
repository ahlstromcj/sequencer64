
/**
 * copy relevant BBT info from jack_position_t
 */

static void
remember_pos (struct bbtpos * xp0, jack_position_t * xp1)
{
   if (! (xp1->valid & JackPositionBBT))
      return;

   xp0->valid = xp1->valid;
   xp0->bar   = xp1->bar;
   xp0->beat  = xp1->beat;
   xp0->tick  = xp1->tick;
   xp0->bar_start_tick = xp1->bar_start_tick;
}

/**
 * compare two BBT positions
 */

static int
pos_changed
(
   struct bbtpos *xp0, jack_position_t *xp1
)
{
   if (! (xp0->valid & JackPositionBBT))
      return -1;

   if (! (xp1->valid & JackPositionBBT))
      return -2;

   if
   (
      xp0->bar == xp1->bar
      && xp0->beat == xp1->beat
      && xp0->tick == xp1->tick
   )
      return 0;

   return 1;
}

static const int64_t
send_pos_message (void * port_buf, jack_position_t * xpos, int off)
{
   if (msg_filter & MSG_NO_POSITION)
      return -1;

   uint8_t * buffer;
   const int64_t bcnt = calc_song_pos(xpos, off);

   /* send '0xf2' Song Position Pointer.
    * This is an internal 14 bit register that holds the number of
    * MIDI beats (1 beat = six MIDI clocks) since the start of the song.
    */

   if (bcnt < 0 || bcnt >= 16384)
      return -1;

   buffer = jack_midi_event_reserve(port_buf, 0, 3);
   if (!buffer)
      return -1;

   buffer[0] = 0xf2;
   buffer[1] = (bcnt)      & 0x7f; // LSB
   buffer[2] = (bcnt >> 7) & 0x7f; // MSB
   return bcnt;
}

/**
 * send 1 byte MIDI Message
 * @param port_buf buffer to write event to
 * @param time sample offset of event
 * @param rt_msg message byte
 */

static void
send_rt_message
(
   void * port_buf, jack_nframes_t time, uint8_t rt_msg
) {
   uint8_t * buffer = jack_midi_event_reserve(port_buf, time, 1);
   if (buffer)
      buffer[0] = rt_msg;
}

/**
 * calculate song position (14 bit integer)
 * from current jack BBT info.
 *
 * see "Song Position Pointer" at
 * http://www.midi.org/techspecs/midimessages.php
 *
 * Because this value is also used internally to sync/send
 * start/continue realtime messages, a 64 bit integer
 * is used to cover the full range of jack transport.
 */

static const int64_t
calc_song_pos (jack_position_t * xpos, int off)
{
  if (! (xpos->valid & JackPositionBBT))
     return -1;

  if (off < 0)
  {
    /* auto offset */
    if (xpos->bar == 1 && xpos->beat == 1 && xpos->tick == 0)
       off = 0;
    else
       off = rintf(xpos->beats_per_minute * 4.0 * resync_delay / 60.0);
  }

  /*
   * MIDI Beat Clock: 24 ticks per quarter note one MIDI-beat = six MIDI clocks
   * -> 4 MIDI-beats per quarter note (JACK beat) Note: JACK counts bars and
   * beats starting at 1
   */

  int64_t pos = off
    + 4 * ((xpos->bar - 1) * xpos->beats_per_bar + (xpos->beat - 1))
    + floor(4.0 * xpos->tick / xpos->ticks_per_beat)
    ;

  return pos;
}

/**
 *    The JACK process callback from the jack_midi_clock project.
 *    Do the work: query jack-transport, send MIDI messages.
 */

int
jack_process_callback (jack_nframes_t nframes, void * arg)
{
   jack_position_t xpos;
   double samples_per_beat;
   jack_nframes_t bbt_offset = 0;
   int ticks_sent_this_cycle = 0;
   jack_transport_state_t xstate = jack_transport_query(j_client, &xpos);

   /* send position updates if stopped and located */

   if (xstate == JackTransportStopped && xstate == m_xstate)
   {
      if (pos_changed(&last_xpos, &xpos) > 0)
      {
         song_position_sync = send_pos_message(port_buf, &xpos, -1);
      }
   }
   remember_pos(&last_xpos, &xpos);

   /* send RT messages start/stop/continue if transport state changed */

   if (xstate != m_xstate)
   {
      switch (xstate)
      {
      case JackTransportStopped:

         if (! (msg_filter & MSG_NO_TRANSPORT))
            send_rt_message(port_buf, 0, MIDI_RT_STOP);

         song_position_sync = send_pos_message(port_buf, &xpos, -1);
         break;

      case JackTransportRolling:

      /* handle transport locate while rolling.
       * jack transport state changes  Rolling -> Starting -> Rolling
       */

      if (m_xstate == JackTransportStarting && !(msg_filter & MSG_NO_POSITION))
      {
         if (song_position_sync < 0)
         {
            /* send stop IFF not stopped, yet */
            send_rt_message(port_buf, 0, MIDI_RT_STOP);
         }
         if (song_position_sync != 0)
         {
            /* re-set 'continue' message sync point */
            if ((song_position_sync = send_pos_message(port_buf, &xpos, -1)) < 0)
            {
               if (!(msg_filter & MSG_NO_TRANSPORT))
                  send_rt_message(port_buf, 0, MIDI_RT_CONTINUE);
            }
         }
         else
         {
            /* 'start' at 0, don't queue 'continue' message */
            song_position_sync = -1;
         }
         break;
      }

      case JackTransportStarting:

         if (m_xstate == JackTransportStarting)
         {
            break;
         }
         if (xpos.frame == 0)
         {
            if (! (msg_filter & MSG_NO_TRANSPORT))
            {
               send_rt_message(port_buf, 0, MIDI_RT_START);
               song_position_sync = 0;
            }
         }
         else
         {
            /* only send continue message here if song-position
             * is not used .
             * w/song-pos it queued just-in-time
             */

            if
            (
               !(msg_filter & MSG_NO_TRANSPORT) &&
               (msg_filter & MSG_NO_POSITION)
            )
            {
               send_rt_message(port_buf, 0, MIDI_RT_CONTINUE);
            }
         }
         break;

      default:
         break;
      }

      /* initial beat tick */

      if
      (
         xstate == JackTransportRolling &&
         ((xpos.frame == 0) || (msg_filter & MSG_NO_POSITION))
      )
      {
         send_rt_message(port_buf, 0, MIDI_RT_CLOCK);
      }
      mclk_last_tick = xpos.frame;
      m_xstate = xstate;
   }

   if ((xstate != JackTransportRolling))
   {
      return 0;
   }

   /* calculate clock tick interval */

   if (force_bpm && user_bpm > 0)
   {
      samples_per_beat = (double) xpos.frame_rate * 60.0 / user_bpm;
   }
   else if (xpos.valid & JackPositionBBT)
   {
      samples_per_beat = (double) xpos.frame_rate * 60.0 / xpos.beats_per_minute;
      if (xpos.valid & JackBBTFrameOffset)
      {
         bbt_offset = xpos.bbt_offset;
      }
   }
   else if (user_bpm > 0)
   {
      samples_per_beat = (double) xpos.frame_rate * 60.0 / user_bpm;
   }
   else
   {
      return 0; /* no tempo known */
   }

   /* quarter-notes per beat is [usually] independent of the meter:
    *
    * certainly for 2/4, 3/4, 4/4 etc.
    * should be true as well for 6/8, 2/2
    * TODO x-check w/jack-timecode-master implementations
    *
    * quarter_notes_per_beat = xpos.beat_type / 4.0; // ?!
    */

   const double quarter_notes_per_beat = 1.0;

   /* MIDI Beat Clock: Send 24 ticks per quarter note  */

   const double samples_per_quarter_note =
      samples_per_beat / quarter_notes_per_beat;

   const double clock_tick_interval = samples_per_quarter_note / 24.0;

   /* send clock ticks for this cycle */

   while (1)
   {
      const double next_tick = mclk_last_tick + clock_tick_interval;
      const int64_t next_tick_offset =
         llrint(next_tick) - xpos.frame - bbt_offset;

      if (next_tick_offset >= nframes)
         break;

      if (next_tick_offset >= 0)
      {
         if (song_position_sync > 0 && !(msg_filter & MSG_NO_POSITION))
         {
            /* send 'continue' realtime message on time */

            const int64_t sync = calc_song_pos(&xpos, 0);

            /* 4 MIDI-beats per quarter note (jack beat) */

            if (sync + ticks_sent_this_cycle / 4 >= song_position_sync)
            {
               if (!(msg_filter & MSG_NO_TRANSPORT))
               {
                  send_rt_message(port_buf, next_tick_offset, MIDI_RT_CONTINUE);
               }
               song_position_sync = -1;
            }
         }

         /* enqueue clock tick */

         send_rt_message(port_buf, next_tick_offset, MIDI_RT_CLOCK);
      }
      mclk_last_tick = next_tick;
      ticks_sent_this_cycle++;
   }
   return 0;
}

