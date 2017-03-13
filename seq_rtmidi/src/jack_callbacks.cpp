
int
jack_process_rtmidi_input (jack_nframes_t nframes, void * arg)
{
#if 0
    static bool s_null_detected = false;
    midi_jack_data * jackdata = reinterpret_cast<midi_jack_data *>(arg);
    rtmidi_in_data * rtindata = jackdata->m_jack_rtmidiin;
    if (is_nullptr(jackdata->m_jack_port))     /* is port created?        */
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_input", "null jack port");
        }
        return 0;
    }
    if (is_nullptr(rtindata))
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_input", "null rtmidi_in_data");
        }
        return 0;
    }
    s_null_detected = false;

    /*
     * Since this is an input port, buff is the area that contains data from
     * the "remote" (i.e. outside our application) port.
     */
#endif  // 0

    void * buff = jack_port_get_buffer(jackdata->m_jack_port, nframes);
    if (not_nullptr(buff))
    {
        jack_midi_event_t jmevent;
        jack_time_t jtime;
        int evcount = jack_midi_get_event_count(buff);
        for (int j = 0; j < evcount; ++j)
        {
            midi_message message;
            int rc = jack_midi_event_get(&jmevent, buff, j);
            if (rc == 0)
            {
                int eventsize = int(jmevent.size);
                for (int i = 0; i < eventsize; ++i)
                    message.push(jmevent.buffer[i]);

                jtime = jack_get_time();            /* compute delta time   */
                if (rtindata->first_message())
                {
                    rtindata->first_message(false);
                }
                else
                {
                    message.timestamp
                    (
                        (jtime - jackdata->m_jack_lasttime) * 0.000001
                    );
                }
                jackdata->m_jack_lasttime = jtime;
                if (! rtindata->continue_sysex())
                {
                    if (rtindata->using_callback())
                    {
                        rtmidi_callback_t callback = rtindata->user_callback();
                        callback(message, rtindata->user_data());
                    }
                    else
                    {
                        (void) rtindata->queue().add(message);
                    }
                }
            }
            else
            {
                if (rc == ENODATA)
                {
                    errprintf("jack_process_rtmidi_input() ENODATA = %x", rc);
                }
                else
                {
                    errprintf("jack_process_rtmidi_input() ERROR = %x", rc);
                }
            }
        }
    }
    return 0;
}

int
jack_process_rtmidi_io (jack_nframes_t nframes, void * arg)
{
    static bool s_null_detected = false;
    midi_jack_data * jackdata = reinterpret_cast<midi_jack_data *>(arg);

    /*
     * Useful for both input and output.
     */

    if (is_nullptr(jackdata->m_jack_port))          /* is port created?     */
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_output", "null jack port");
        }
        return 0;
    }

    /*
     * Useful for output.
     */

    if (is_nullptr(jackdata->m_jack_buffsize))      /* port set up?        */
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_output", "null jack buffer");
        }
        return 0;
    }

    /*
     * Useful for input.
     */

    rtmidi_in_data * rtindata = jackdata->m_jack_rtmidiin;
    if (is_nullptr(rtindata))
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_input", "null rtmidi_in_data");
        }
        return 0;
    }
    s_null_detected = false;

}

int
jack_process_rtmidi_output (jack_nframes_t nframes, void * arg)
{
#if 0
    static bool s_null_detected = false;
    midi_jack_data * jackdata = reinterpret_cast<midi_jack_data *>(arg);
    if (is_nullptr(jackdata->m_jack_port))          /* is port created?     */
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_output", "null jack port");
        }
        return 0;
    }
    if (is_nullptr(jackdata->m_jack_buffsize))      /* port set up?        */
    {
        if (! s_null_detected)
        {
            s_null_detected = true;
            apiprint("jack_process_rtmidi_output", "null jack buffer");
        }
        return 0;
    }
#endif  // 0

    static size_t soffset = 0;
    void * buf = jack_port_get_buffer(jackdata->m_jack_port, nframes);

    jack_midi_clear_buffer(buf);

    /*
     * A for-loop over the number of nframes?  See discussion above.
     */

    while (jack_ringbuffer_read_space(jackdata->m_jack_buffsize) > 0)
    {
        int space;
        (void) jack_ringbuffer_read
        (
            jackdata->m_jack_buffsize, (char *) &space, sizeof space
        );
        jack_midi_data_t * md = jack_midi_event_reserve(buf, soffset, space);
        if (not_nullptr(md))
        {
            char * mididata = reinterpret_cast<char *>(md);
            (void) jack_ringbuffer_read         /* copy into mididata */
            (
                jackdata->m_jack_buffmessage, mididata, size_t(space)
            );

#ifdef SEQ64_SHOW_API_CALLS_TMI
            printf("%d bytes read: ", space);
            for (int i = 0; i < space; ++i)
                printf("%x ", (unsigned char)(mididata[i]));

            printf("\n");
#endif
        }
        else
        {
            errprint("jack_midi_event_reserve() returned a null pointer");
        }
    }
    return 0;
}
