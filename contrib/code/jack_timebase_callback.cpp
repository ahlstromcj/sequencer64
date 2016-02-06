/**
 * \file          jack_timebase_callback.cpp
 *
 *  This module simply preserve the old Seq24 JACK timebase code, just in
 *  case.
 *
 * \updates       2016-02-05
 */

/**
 * Define this macro to go back to the original version of the
 * jack_timebase_callback() function, which handles the JACK Master mode of
 * sequencer64.
 */

#undef  USE_ORIGINAL_TIMEBASE_CALLBACK

/**
 *  The JACK timebase function defined here sets the JACK position structure.
 *  The original version of the function, enabled by defining
 *  USE_ORIGINAL_TIMEBASE_CALLBACK, worked properly with Hydrogen, but not
 *  with Klick.
 *
 * \param state
 *      Indicates the current state of JACK transport.
 *
 * \param nframes
 *      The number of JACK frames in the current time period.
 *
 * \param pos
 *      Provides the position structure to be filled in, the
 *      address of the position structure for the next cycle; pos->frame will
 *      be its frame number. If new_pos is FALSE, this structure contains
 *      extended position information from the current cycle. If TRUE, it
 *      contains whatever was set by the requester. The timebase_callback's
 *      task is to update the extended information here.
 *
 * \param new_pos
 *      TRUE (non-zero) for a newly requested pos, or for the first cycle
 *      after the timebase_callback is defined.  This is usually 0 in
 *      Sequencer64 at present, and 1 if one, say, presses "rewind" in
 *      qjackctl.
 *
 * \param arg
 *      Provides the jack_assistant pointer, currently unchecked for nullity.
 */

#ifdef USE_ORIGINAL_TIMEBASE_CALLBACK

void
jack_timebase_callback
(
    jack_transport_state_t state,
    jack_nframes_t nframes,
    jack_position_t * pos,
    int new_pos,
    void * arg
)
{
    static jack_nframes_t s_current_frame;
    static jack_transport_state_t s_state_last;
    static jack_transport_state_t s_state_current;
    jack_assistant * jack = (jack_assistant *)(arg);
    s_state_current = state;
    s_current_frame = jack_get_current_transport_frame(jack->m_jack_client);
    if (is_nullptr(pos))
    {
        errprint("jack_timebase_callback(): null position pointer");
        return;
    }
    pos->valid = JackPositionBBT;
    pos->beats_per_bar = jack->m_beats_per_measure;
    pos->beat_type = jack->m_beat_width;
    pos->ticks_per_beat = jack->m_ppqn * 10;    // why 10?
    pos->beats_per_minute = jack->parent().get_beats_per_minute();

    /*
     * If we are in a new position, then compute BBT (Bar:Beats.ticks).
     * (I wonder why the new_pos variable was never used here?)
     */

    if
    (
        s_state_last == JackTransportStarting &&
        s_state_current == JackTransportRolling
    )
    {
        double d_jack_tick;                     /* was static           */
        double jack_delta_tick =
            s_current_frame * pos->ticks_per_beat *
            pos->beats_per_minute / (pos->frame_rate * 60.0);

        d_jack_tick = (jack_delta_tick < 0) ? -jack_delta_tick : jack_delta_tick ;

        long ptick = 0, pbeat = 0, pbar = 0;
        long ticks_per_bar = long(pos->ticks_per_beat * pos->beats_per_bar);
        pbar = long(long(d_jack_tick) / ticks_per_bar);
        pbeat = long(long(d_jack_tick) % ticks_per_bar);
        pbeat /= long(pos->ticks_per_beat);
        ptick = long(d_jack_tick) % long(pos->ticks_per_beat);
        pos->bar = pbar + 1;
        pos->beat = pbeat + 1;
        pos->tick = ptick;
        pos->bar_start_tick = pos->bar * ticks_per_bar;
    }
    s_state_last = s_state_current;
}

/*
 * jack_timebase_callback.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

