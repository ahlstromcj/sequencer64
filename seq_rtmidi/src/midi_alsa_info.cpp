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
 *
 *  This class is meant to collect a whole bunch of ALSA information
 *  about client number, port numbers, and port names, and hold them
 *  for usage when creating ALSA midibus objects and midi_alsa API objects.
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

midi_alsa_info::midi_alsa_info
(
) :
    midi_info           (),
    m_alsa_seq          (),             // snd_seq_t pointer or reference
    m_alsa_port_info    ()              // snd_seq_port_info_t * pinfo,
{
    //
}

/**
 *  Destructor.  Closes a connection if it exists, shuts down the input
 *  thread, and then cleans up any API resources in use.
 */

midi_alsa_info::~midi_alsa_info ()
{
    //
}

#define SEQ64_PORT_CLIENT      0xFF000000
#define SEQ64_PORT_COUNT       (-1)

/**
 *  Gets information on ALL ports, putting input data into one midi_info
 *  container, and putting output data into another container.
 *
 * \return
 *      Returns the total number of ports found.
 */

int
midi_alsa_info::get_all_port_info ()
{
    int count = 0;
    snd_seq_t * seq = &m_alsa_seq;
    snd_seq_port_info_t * pinfo = &m_alsa_port_info;
    snd_seq_client_info_t * cinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    unsigned input_caps =
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;

    unsigned output_caps =
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;

    input_ports().clear();
    output_ports().clear();
    while (snd_seq_query_next_client(seq, cinfo) >= 0)
    {
        int client = snd_seq_client_info_get_client(cinfo);
        if (client == 0)
        {
            continue;
        }
        else if (type == SEQ64_ALSA_PORT_CLIENT)
        {
            infoprint("Returning client number");
            return client;
        }

        snd_seq_port_info_set_client(pinfo, client);    /* reset query info */
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seq, pinfo) >= 0)
        {
            unsigned alsatype = snd_seq_port_info_get_type(pinfo);
            if
            (
                ((alsatype & SND_SEQ_PORT_TYPE_MIDI_GENERIC) == 0) &&
                ((alsatype & SND_SEQ_PORT_TYPE_SYNTH) == 0)
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
 *      SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE.  We add a new
 *      value, SEQ64_ALSA_PORT_CLIENT = 0xFF000000, to indicate we only want the
 *      client value.
 *
 * \param portnumber
 *      The port number to look up. If this value is -1, then the ports are
 *      counted.
 *
 * \return
 *      Returns the port count if portnumber is -1, 1 if the port number
 *      queried is portnumber, or 0 if there is no match to portnumber.  If
 *      type is the new value, 0xFF000000, then the client value is returned
 *      without further querying of the ALSA subsystem.
 */

unsigned
midi_alsa_info::alsa_port_info
(
    unsigned type,
    int portnumber
)
{
    int client;
    int count = 0;
    snd_seq_t * seq = &m_alsa_seq;
    snd_seq_port_info_t * pinfo = &m_alsa_port_info;
    snd_seq_client_info_t * cinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(m_alsa_seq, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        if (client == 0)
        {
            continue;
        }
        else if (type == SEQ64_ALSA_PORT_CLIENT)
        {
            infoprint("Returning client number");
            return client;
        }

        snd_seq_port_info_set_client(pinfo, client);    /* reset query info */
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(m_alsa_seq, pinfo) >= 0)
        {
            unsigned alsatype = snd_seq_port_info_get_type(pinfo);
            if
            (
                ((alsatype & SND_SEQ_PORT_TYPE_MIDI_GENERIC) == 0) &&
                ((alsatype & SND_SEQ_PORT_TYPE_SYNTH) == 0)
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

