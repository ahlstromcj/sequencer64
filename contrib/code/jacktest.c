#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>

jack_port_t * m_input_port;
jack_port_t * m_output_port;
jack_client_t * m_jack_client;
int m_loopsize = 25000;
int m_xrun_occurred = 0;
int m_consecutive_xruns = 0;
float m_first_xrun = 0.0f;
float m_last_load = 0.0f;
int m_at_loop = 0;
int m_at_loop_size;
char * m_the_chunk;
int m_chunk_size = 1024 * 1024 * 10;

/**
 *    Declares a volatile integer that is unused, and sets n into a random
 *    index of m_the_chunk[].
 *
 * \param n
 *    The JACK frame number to set.
 */

void
fooey (jack_nframes_t n)
{
	volatile int x;
	m_the_chunk[random() % m_chunk_size] = n;
}

/**
 *
 * \param nf
 *    Then number of frames.
 *
 * \param arg
 *    The JACK ???
 *
 * \return
 *    Returns...
 */

int
process (jack_nframes_t nf, void * arg)
{
	jack_default_audio_sample_t * in = jack_port_get_buffer(m_input_port, nf);
	jack_default_audio_sample_t * out = jack_port_get_buffer(m_output_port, nf);
	int i;
	memcpy(out, in, sizeof(jack_default_audio_sample_t) * nf);
	for (i = 0; i < m_loopsize; ++i)
   {
		fooey (nf);
	}

	m_last_load = jack_cpu_load(m_jack_client);
	if ((m_at_loop += nf) >= m_at_loop_size)
   {
		if (m_last_load < 25.0f)
      {
			m_loopsize *= 2;
		}
      else if (m_last_load < 50.0f)
      {
			m_loopsize = (int) (1.5 * m_loopsize);
		}
      else if (m_last_load < 90.0f)
      {
			m_loopsize += (int) (0.10 * m_loopsize);
		}
      else if (m_last_load < 95.0f)
      {
			m_loopsize += (int) (0.05 * m_loopsize);
		}
      else
      {
			m_loopsize += (int) (0.001 * m_loopsize);
		}
		m_at_loop = 0;
		printf ("loopsize = %d\n", m_loopsize);
	}

	if (m_xrun_occurred > 0)
   {
		if (m_consecutive_xruns == 0)
      {
			m_first_xrun = m_last_load;
		}
		++m_consecutive_xruns;
	}
	m_xrun_occurred = 0;

	if (m_consecutive_xruns >= 10)
   {
		fprintf
      (
         stderr, "Stopping with load = %f (first xrun at %f)\n",
         m_last_load, m_first_xrun
      );
		exit(0);
	}
	return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 *
 * \param arg
 *    The JACK client pointer, but unused.
 */

void
jack_shutdown (void * arg)
{
	fprintf (stderr, "shutdown with load = %f\n", m_last_load);
	exit(1);
}

int
jack_xrun (void * arg)
{
	fprintf (stderr, "xrun occured with loop size = %d\n", m_loopsize);

	/*
    * m_xrun_occurred = 1;
    */

	++m_xrun_occurred;
	return 0;
}

int
main (int argc, char * argv[])
{
	const char ** ports;
	const char * server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	const char * client_name = "jacktester";
	if (argc > 1)
   {
		m_chunk_size = atoi(argv[1]);
		printf ("using chunksize of %d\n", m_chunk_size);
	}

	/* open a client connection to the JACK server */

	m_jack_client = jack_client_open(client_name, options, &status, server_name);
	if (m_jack_client == NULL)
   {
		fprintf (stderr, "jack_client_open() failed, status = 0x%x\n", status);
		if (status & JackServerFailed)
      {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit(1);
	}
	if (status & JackServerStarted)
   {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique)
   {
		client_name = jack_get_client_name(m_jack_client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	/*
    * Tell the JACK server to call `process()' whenever there is work to be
    * done.
    */

	jack_set_process_callback(m_jack_client, process, 0);
	jack_set_xrun_callback(m_jack_client, jack_xrun, 0);
	jack_on_shutdown(m_jack_client, jack_shutdown, 0);

	/*
    * Create two ports.
    */

	m_input_port = jack_port_register
   (
      m_jack_client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0
   );
	m_output_port = jack_port_register
   (
      m_jack_client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0
   );

	if ((m_input_port == NULL) || (m_output_port == NULL))
   {
		fprintf(stderr, "no more JACK ports available\n");
		exit(1);
	}

	m_at_loop_size = jack_get_sample_rate(m_jack_client) * 2;
	if ((m_the_chunk = (char *) malloc(m_chunk_size)) == NULL)
   {
		fprintf(stderr, "cannot allocate chunk\n");
		exit(1);
	}

	/*
    * Tell the JACK server that we are ready to roll.  Our process() callback
    * will start running now.
    */

	if (jack_activate(m_jack_client))
   {
		fprintf(stderr, "cannot activate client");
		exit(1);
	}

	/*
    * Connect the ports. Note: you can't do this before the client is
    * activated, because we can't allow connections to be made to clients that
    * aren't running.
	 */

	ports = jack_get_ports
   (
      m_jack_client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput
   );
	if (ports == NULL)
   {
		fprintf(stderr, "no physical capture ports\n");
		exit(1);
	}
	if (jack_connect(m_jack_client, ports[0], jack_port_name(m_input_port)))
   {
		fprintf(stderr, "cannot connect input ports\n");
	}
	free(ports);
	ports = jack_get_ports
   (
      m_jack_client, NULL, NULL, JackPortIsPhysical | JackPortIsInput
   );
	if (ports == NULL)
   {
		fprintf(stderr, "no physical playback ports\n");
		exit(1);
	}
	if (jack_connect(m_jack_client, jack_port_name (m_output_port), ports[0]))
   {
		fprintf(stderr, "cannot connect output ports\n");
	}
	free (ports);
	for (;;)
   {
		sleep(1);
	}
	jack_client_close(m_jack_client);
	exit(0);
}
