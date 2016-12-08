/**
 * \file          midi_alsa_info.cpp
 *
 *    A class for obrtaining ALSA information
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-12-06
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

/*
 * Initialization of static members.
 */

static unsigned midi_alsa_info::sm_input_caps =
    SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;

static unsigned midi_alsa_info::sm_output_caps =
    SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;

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

unsigned
midi_alsa_info::get_all_port_info ()
{
    unsigned count = 0;
    snd_seq_t * seq = &m_alsa_seq;                      /* point to member  */
    snd_seq_port_info_t * pinfo = &m_alsa_port_info;    /* point to member  */
    snd_seq_client_info_t * cinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    input_ports().clear();
    output_ports().clear();
    while (snd_seq_query_next_client(seq, cinfo) >= 0)
    {
        int client = snd_seq_client_info_get_client(cinfo);
        if (client == 0)
            continue;

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
            std::string clientname = snd_seq_client_info_get_name(cinfo);
            int portnumber = snd_seq_port_info_get_port(pinfo);
            char temp[80];
            snprintf
            (
                temp, sizeof temp, "%s %d:%d",
                clientname.c_str(), client, portnumber
            );
            if ((caps & sm_input_caps) == sm_input_caps)
            {
                input_ports().add(clientnumber, portnumber, clientname);
                ++count;
            }
            else if ((caps & sm_output_caps) == sm_output_caps)
            {
                output_ports().add(clientnumber, portnumber, clientname);
                ++count;
            }
            else
            {
                infoprintf("Non-I/O port '%s'", clientname.c_str());
            }
        }
    }
    return count;
}

}           // namespace seq64

/*
 * midi_alsa_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

