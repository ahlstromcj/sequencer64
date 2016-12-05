/**
 * \file          midi_alsa_info.cpp
 *
 *    A class for obrtaining ALSA information
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-03
 * \license       See the rtexmidi.lic file.  Too big.
 *
 *  API information found at:
 *
 *      - http://www.alsa-project.org/documentation.php#Library
 */

#include <sstream>

#include <pthread.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>

#include "calculations.hpp"             /* beats_per_minute_from_tempo_us() */
#include "midi_alsa_info.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Principal constructor.
 *
 * \param clientname
 *      Provides the name of the MIDI input port.
 *
 * \param queuesize
 *      Provides the upper limit of the queue size.
 */

midi_alsa_info::midi_alsa_info ()
 :
    m_alsa_input    (),
    m_alsa_output   ()
{
}

/**
 *  Destructor.  Closes a connection if it exists, shuts down the input
 *  thread, and then cleans up any API resources in use.
 */

midi_alsa_info::~midi_alsa_info ()
{
    //
}

/*
 * This function is used to count or get the pinfo structure for a given port
 * number.
 *
 * \param seq
 *      TO DO.
 *
 * \param pinfo
 *      TO DO.
 *
 * \param type
 *      The type of port to look up.  It is a one of the following masks:
 *      SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ, and
 *      SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE.
 *
 * \param portnumber
 *      The port number to look up. If this value is -1, then the ports are
 *      counted.
 *
 * \return
 *      Returns the port count, or 0.
 */

unsigned
midi_alsa_info::alsa_port_info
(
    snd_seq_t * seq, snd_seq_port_info_t * pinfo,
    unsigned type, int portnumber
)
{
    int client;
    int count = 0;
    snd_seq_client_info_t * cinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        if (client == 0)
            continue;

        // Reset query info

        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seq, pinfo) >= 0)
        {
            unsigned atyp = snd_seq_port_info_get_type(pinfo);
            if
            (
                ((atyp & SND_SEQ_PORT_TYPE_MIDI_GENERIC) == 0) &&
                ((atyp & SND_SEQ_PORT_TYPE_SYNTH) == 0)
            )
            {
                continue;
            }

            unsigned caps = snd_seq_port_info_get_capability(pinfo);
            if ((caps & type) != type)
                continue;

            if (count == portnumber)
                return 1;

            ++count;
        }
    }

    /*
     * If a negative portnumber was used, return the port count.
     */

    if (portnumber < 0)
        return count;

    return 0;
}

/**
 *  Gets the input sequencer port count from ALSA.
 *
 * \return
 *      Returns the result of a alsa_port_info() call.
 */

unsigned
midi_alsa_info::get_port_count (bool input)
{
    snd_seq_port_info_t * pinfo;
    snd_seq_port_info_alloca(&pinfo);
//? alsa_midi_data_t * alsadata = static_cast<alsa_midi_data_t *>(m_api_data);
    if (input)
    {
        return alsa_port_info
        (
            alsadata->seq /* ? */, pinfo,
            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ, -1
        );
    }
    else
    {
        return alsa_port_info
        (
            alsadata->seq, pinfo,
            SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, -1
        );
    }
}

}           // namespace seq64

/*
 * midi_alsa_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

