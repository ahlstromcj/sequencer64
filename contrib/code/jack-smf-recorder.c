/*-
 * Copyright (c) 2007, 2008 Edward Tomasz Napierała <trasz@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * ALTHOUGH THIS SOFTWARE IS MADE OF SCIENCE AND WIN, IT IS PROVIDED BY THE
 * AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * This is jack-smf-recorder, Standard MIDI File recorder for JACK MIDI.
 *
 * For questions and comments, contact Edward Tomasz Napierala
 * <trasz@FreeBSD.org>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sysexits.h>
#include <errno.h>
#include <signal.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <glib.h>

#include "config.h"
#include "smf.h"

#ifdef WITH_LASH
#include <lash/lash.h>
#endif

/*
 * Same as other module until here.
 */

#define INPUT_PORT_NAME       "midi_in"
#define PROGRAM_NAME          "jack-smf-recorder"

jack_port_t * input_port;
volatile int ctrl_c_pressed = 0;
smf_t * smf = NULL;
smf_track_t * tracks[16];     /* We allocate one track per MIDI channel. */

/*
 * START OF COMMON CODE.
 */

#define PROGRAM_VERSION       PACKAGE_VERSION

jack_client_t * jack_client = NULL;

#ifdef WITH_LASH
lash_client_t * lash_client;
#endif

/*
 * Will emit a warning if time between jack callbacks is longer than this.
 */

#define MAX_TIME_BETWEEN_CALLBACKS   0.1

/*
 * Will emit a warning if execution of jack callback takes longer than this.
 */

#define MAX_PROCESSING_TIME   0.01

double
get_time(void)
{
   double seconds;
   struct timeval tv;
   int ret = gettimeofday(&tv, NULL);
   if (ret)
   {
      perror("gettimeofday");
      exit(EX_OSERR);
   }
   seconds = tv.tv_sec + tv.tv_usec / 1000000.0;
   return seconds;
}

double
get_delta_time(void)
{
   static double previously = -1.0;
   double delta;
   double now = get_time();
   if (previously == -1.0)
   {
      previously = now;
      return 0;
   }
   delta = now - previously;
   previously = now;
   // assert(delta >= 0.0);
   return delta;
}

static gboolean
warning_async (gpointer s)
{
   const char * str = (const char *) s;
   g_warning(str);
   return FALSE;
}

static void
warn_from_jack_thread_context (const char * str)
{
   g_idle_add(warning_async, (gpointer)str);
}

static double
nframes_to_ms (jack_nframes_t nframes)
{
   jack_nframes_t sr = jack_get_sample_rate(jack_client);
   // assert(sr > 0);
   return (nframes * 1000.0) / (double)sr;
}

static double
nframes_to_seconds (jack_nframes_t nframes)
{
   return nframes_to_ms(nframes) / 1000.0;
}

/*
 * END OF COMMON CODE.
 */

void
process_midi_input (jack_nframes_t nframes)
{
   static int time_of_first_event = -1;
   int /*read,*/ events, i, channel;
   jack_midi_event_t event;
   int last_frame_time = jack_last_frame_time(jack_client);
   void * port_buffer = jack_port_get_buffer(input_port, nframes);
   if (port_buffer == NULL)
   {
      warn_from_jack_thread_context
      (
         "jack_port_get_buffer failed, cannot receive anything."
      );
      return;
   }

#ifdef JACK_MIDI_NEEDS_NFRAMES
   events = jack_midi_get_event_count(port_buffer, nframes);
#else
   events = jack_midi_get_event_count(port_buffer);
#endif

   for (i = 0; i < events; ++i)
   {
      smf_event_t * smf_event;

#ifdef JACK_MIDI_NEEDS_NFRAMES
      int read = jack_midi_event_get(&event, port_buffer, i, nframes);
#else
      int read = jack_midi_event_get(&event, port_buffer, i);
#endif
      if (read) {
         warn_from_jack_thread_context("jack_midi_event_get failed, RECEIVED NOTE LOST.");
         continue;
      }
      if (event.buffer[0] >= 0xF8)              /* Ignore realtime messages. */
         continue;

      if (time_of_first_event == -1)            /* First event received? */
         time_of_first_event = last_frame_time + event.time;

      smf_event = smf_event_new_from_pointer(event.buffer, event.size);
      if (smf_event == NULL)
      {
         warn_from_jack_thread_context
         (
            "smf_event_from_pointer failed, RECEIVED NOTE LOST."
         );
         continue;
      }
   //    assert(smf_event->midi_buffer_length >= 1);
      channel = smf_event->midi_buffer[0] & 0x0F;
      smf_track_add_event_seconds
      (
         tracks[channel], smf_event,
         nframes_to_seconds
         (
            jack_last_frame_time(jack_client) + event.time - time_of_first_event
         )
      );
   }
}

/*
 * Except for process_midi_input() this is COMMON CODE.
 */

static int
process_callback (jack_nframes_t nframes, void * notused)
{
#ifdef MEASURE_TIME
   if (get_delta_time() > MAX_TIME_BETWEEN_CALLBACKS)
   {
      warn_from_jack_thread_context
      (
         "Had to wait too long for JACK callback; scheduling problem?"
      );
   }
#endif

   /*
    * Check for impossible condition that actually happened to me, caused by
    * some problem between jackd and OSS4.
    */

   if (nframes <= 0)
   {
      warn_from_jack_thread_context
      (
         "Process callback called with nframes = 0; bug in JACK?"
      );
      return 0;
   }

   process_midi_input(nframes);

#ifdef MEASURE_TIME
   if (get_delta_time() > MAX_PROCESSING_TIME)
   {
      warn_from_jack_thread_context
      (
         "Processing took too long; scheduling problem?"
      );
   }
#endif

   return 0;
}

/*
 * Connects to the specified input port, disconnecting already connected ports.
 */

int
connect_to_output_port (const char * port)
{
   int ret = jack_port_disconnect(jack_client, input_port);
   if (ret)
   {
      g_warning("Cannot disconnect MIDI port.");
      return -3;
   }
   ret = jack_connect(jack_client, port, jack_port_name(input_port));
   if (ret)
   {
      g_warning("Cannot connect to %s.", port);
      return -4;
   }
   g_warning("Connected to %s.", port);
   return 0;
}

void
init_jack (void)
{
   int err;

#ifdef WITH_LASH
   lash_event_t *event;
#endif

   jack_client = jack_client_open(PROGRAM_NAME, JackNullOption, NULL);

   if (jack_client == NULL)
   {
      g_critical("Could not connect to the JACK server; run jackd first?");
      exit(EX_UNAVAILABLE);
   }

#ifdef WITH_LASH
   event = lash_event_new_with_type(LASH_Client_Name);
   // assert(event); /* Documentation does not say anything about return value. */
   lash_event_set_string(event, jack_get_client_name(jack_client));
   lash_send_event(lash_client, event);
   lash_jack_client_name(lash_client, jack_get_client_name(jack_client));
#endif

   err = jack_set_process_callback(jack_client, process_callback, 0);
   if (err)
   {
      g_critical("Could not register JACK process callback.");
      exit(EX_UNAVAILABLE);
   }

   /*
    * The above is pretty close to COMMON CODE.
    */

   input_port = jack_port_register
   (
      jack_client, INPUT_PORT_NAME, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0
   );
   if (input_port == NULL)
   {
      g_critical("Could not register JACK input port.");
      exit(EX_UNAVAILABLE);
   }

   if (jack_activate(jack_client)) {
      g_critical("Cannot activate JACK client.");
      exit(EX_UNAVAILABLE);
   }
}

#ifdef WITH_LASH

/*
 * COMMON CODE
 */

static gboolean
lash_callback(gpointer notused)
{
   lash_event_t * event;
   while ((event = lash_get_event(lash_client)))
   {
      switch (lash_event_get_type(event))
      {
         case LASH_Restore_Data_Set:
         case LASH_Save_Data_Set:
            break;

         case LASH_Quit:
            g_warning("Exiting due to LASH request.");
            ctrl_c_pressed = 1;
            break;

         default:
            g_warning
            (
               "Received unknown LASH event of type %d.",
               lash_event_get_type(event)
            );
            lash_event_destroy(event);
      }
   }
   return TRUE;
}

/*
 * COMMON CODE if program name is a parameter.
 */

static void
init_lash (lash_args_t * args)
{
   /* XXX: Am I doing the right thing wrt protocol version? */

   lash_client = lash_init
   (
      args, PROGRAM_NAME, LASH_Config_Data_Set, LASH_PROTOCOL(2, 0)
   );

   if (! lash_server_connected(lash_client))
   {
      g_critical("Cannot initialize LASH.  Continuing anyway.");
      return;           /* exit(EX_UNAVAILABLE); */
   }

   /* Schedule a function to process LASH events, ten times per second. */

   g_timeout_add(100, lash_callback, NULL);
}

#endif /* WITH_LASH */

gboolean
writer_timeout (gpointer file_name_gpointer)
{
   int i;
   char * file_name = (char *) file_name_gpointer;

   /*
    * XXX: It should be done like this:
    * http://wwwtcs.inf.tu-dresden.de/~tews/Gtk/x2992.html
    */

   if (ctrl_c_pressed == 0)
      return TRUE;

   jack_deactivate(jack_client);
   smf_rewind(smf);                 /* Get rid of empty tracks. */
   for (i = 0; i < 16; ++i)
   {
      if (tracks[i]->number_of_events == 0)
      {
         smf_remove_track(tracks[i]);
         smf_track_delete(tracks[i]);
      }
   }
   if (smf->number_of_tracks == 0)
   {
      g_message("No events recorded, not saving anything.");
      exit(0);
   }
   if (smf_save(smf, file_name))
   {
      g_critical("Could not save file '%s', sorry.", file_name);
      exit(-1);
   }
   g_message("File '%s' saved successfully.", file_name);
   exit(0);
}

/*
 * COMMON CODE
 */

void
ctrl_c_handler(int signum)
{
   ctrl_c_pressed = 1;
}

/*
 * COMMON CODE
 */

static void
log_handler
(
   const gchar * log_domain,
   GLogLevelFlags log_level,
   const gchar * message,
   gpointer notused
)
{
   fprintf(stderr, "%s: %s\n", log_domain, message);
}

/*
 * COMMON CODE, if program name is parameter.
 */

static void
show_version (void)
{
   fprintf
   (
      stdout, "%s %s, libsmf %s\n",
      PROGRAM_NAME, PROGRAM_VERSION, smf_get_version()
   );
   exit(EX_OK);
}

static void
usage(void)
{
   fprintf(stderr, "Usage: jack-smf-recorder [-V] [ -a <out port>] file_name\n");
   exit(EX_USAGE);
}

/*
 * Some COMMON CODE.
 */

int
main (int argc, char * argv [])
{
   int ch, i;
   char * file_name, * autoconnect_port_name = NULL;
   g_thread_init(NULL);

#ifdef WITH_LASH
   lash_args_t * lash_args = lash_extract_args(&argc, &argv);
#endif

   g_log_set_default_handler(log_handler, NULL);
   while ((ch = getopt(argc, argv, "a:V")) != -1)
   {
      switch (ch)
      {
         case 'a':
            autoconnect_port_name = strdup(optarg);
            break;

         case 'V':
            show_version();
            break;

         case '?':
         default:
            usage();
      }
   }
   argc -= optind;
   argv += optind;
   if (argc == 0)
      usage();

   file_name = argv[0];
   smf = smf_new();
   if (smf == NULL)
      exit(-1);

   for (i = 0; i < 16; ++i)
   {
      tracks[i] = smf_track_new();
      if (tracks[i] == NULL)
         exit(-1);

      smf_add_track(smf, tracks[i]);
   }

#ifdef WITH_LASH
   init_lash(lash_args);
#endif

   init_jack();
   if (autoconnect_port_name)
   {
      if (connect_to_output_port(autoconnect_port_name))
      {
         g_critical("Couldn't connect to '%s', exiting.", autoconnect_port_name);
         exit(EX_UNAVAILABLE);
      }
   }
   g_timeout_add(100, writer_timeout, (gpointer)argv[0]);
   signal(SIGINT, ctrl_c_handler);
   g_message
   (
      "Recording will start at the first received note; "
      "press ^C to write the file and exit."
   );
   g_main_loop_run(g_main_loop_new(NULL, TRUE));

   /* Not reached. */

   return 0;
}

