
int
jack_process_rtmidi_io (jack_nframes_t nframes, void * arg)
{
    static bool s_null_detected = false;
    if (nframes > 0)                        // IS THIS A USEFUL I/O CHECK????
    {
        midi_jack_data * jackdata = reinterpret_cast<midi_jack_data *>(arg);
        if (is_nullptr(jackdata->m_jack_port))          /* is port created?     */
        {
            if (! s_null_detected)
            {
                s_null_detected = true;
                apiprint("jack_process_rtmidi_io", "null jack port");
            }
            return 0;
        }
    }



    return 0;
}

/*
 * vim: ts=4 sw=4 et
 */

