/*
 *  This file preserves commented code from seq32 that was present in
 *  jack_assistant::position()
 */

void
jack_assistant::position (bool songmode, midipulse tick)
{

#ifdef SEQ64_JACK_SUPPORT

    long current_tick = 0;
    if (songmode)                               /* master in song mode */
    {
        if (! is_null_midipulse(tick))
            current_tick = tick * 10;
    }

    int ticks_per_beat = m_ppqn * 10;
    int beats_per_minute = parent().get_beats_per_minute();
    uint64_t tick_rate = (uint64_t(m_jack_frame_rate) * current_tick * 60.0);
    long tpb_bpm = ticks_per_beat * beats_per_minute * 4.0 / m_beat_width;
    uint64_t jack_frame = tick_rate / tpb_bpm;
    jack_transport_locate(m_jack_client, jack_frame);

#ifdef SEQ64_STAZED_JACK_SUPPORT

#if 0

    /*
     * The call to jack_BBT_position() is not necessary to change JACK
     * position!  Must set these here since they are set in timebase.
     */

    jack_position_t pos;
    double jack_tick = current_tick * m_bw / 4.0;
    pos.ticks_per_beat = m_ppqn * 10;
    pos.beats_per_minute = m_master_bus.get_bpm();
    jack_BBT_position(pos, jack_tick);

    /*
     * Calculate JACK frame to put into pos.frame; it is what matters for
     * position change.  Very similar to the uncommented code above.
     */

    uint64_t tick_rate = ((uint64_t)pos.frame_rate * current_tick * 60.0);
    long tpb_bpm = pos.ticks_per_beat * pos.beats_per_minute *
        4.0 / pos.beat_type;

    pos.frame = tick_rate / tpb_bpm;

    /*
     * ticks * 10 = jack ticks;
     * jack ticks / ticks per beat = num beats;
     * num beats / beats per minute = num minutes
     * num minutes * 60 = num seconds
     * num secords * frame_rate  = frame
     */

    jack_transport_reposition(m_jack_client, &pos);

#endif  // 0

    if (parent().is_running())
        parent().set_reposition(false);

#endif  // SEQ64_STAZED_JACK_SUPPORT

Tutorial code from jack_assistant::position():

#ifdef SAMPLE_AUDIO_CODE    // disabled, shown only for reference & learning
        jack_transport_state_t ts = jack_transport_query(jack->client(), NULL);
        if (ts == JackTransportRolling)
        {
            jack_default_audio_sample_t * in;
            jack_default_audio_sample_t * out;
            if (client_state == Init)
                client_state = Run;

            in = jack_port_get_buffer(input_port, nframes);
            out = jack_port_get_buffer(output_port, nframes);
            memcpy(out, in, sizeof (jack_default_audio_sample_t) * nframes);
        }
        else if (ts == JackTransportStopped)
        {
            if (client_state == Run)
                client_state = Exit;
        }
#endif

}
