/*
 *  This file is part of sequencer64, adapted from the PortMIDI project.
 *
 *  sequencer64 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  sequencer64 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file        portmidi.c
 *
 *      Provides the basic PortMIDI API.
 *
 * \library     sequencer64 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-05-23
 * \license     GNU GPLv2 or above
 *
 * Notes on host error reporting:
 *
 *      PortMidi errors (of type PmError) are generic, system-independent
 *      errors.  When an error does not map to one of the more specific
 *      PmErrors, the catch-all code pmHostError is returned. This means that
 *      PortMidi has retained a more specific system-dependent error code. The
 *      caller can get more information by calling Pm_HasHostError() to test if
 *      there is a pending host error, and Pm_GetHostErrorText() to get a text
 *      string describing the error. Host errors are reported on a per-device
 *      basis because only after you open a device does PortMidi have a place
 *      to record the host error code. I.e. only those routines that receive a
 *      (PortMidiStream *) argument check and report errors. One exception to
 *      this is that Pm_OpenInput() and Pm_OpenOutput() can report errors even
 *      though when an error occurs, there is no PortMidiStream* to hold the
 *      error. Fortunately, both of these functions return any error
 *      immediately, so we do not really need per-device error memory. Instead,
 *      any host error code is stored in a global, pmHostError is returned, and
 *      the user can call Pm_GetHostErrorText() to get the error message (and
 *      the invalid stream parameter will be ignored.) The functions pm_init
 *      and pm_term do not fail or raise errors. The job of pm_init is to
 *      locate all available devices so that the caller can get information via
 *      PmDeviceInfo(). If an error occurs, the device is simply not listed as
 *      available.
 *
 *      Host errors come in two flavors:
 *
 *          -    host error
 *          -    host error during callback
 *
 *    These can occur w/midi input or output devices. (b) can only happen
 *    asynchronously (during callback routines), whereas (a) only occurs while
 *    synchronously running PortMidi and any resulting system dependent calls.
 *    Both (a) and (b) are reported by the next read or write call. You can
 *    also query for asynchronous errors (b) at any time by calling
 *    Pm_HasHostError().
 *
 * Notes on compile-time switches:
 *
 *    DEBUG assumes stdio and a console. Use this if you want automatic,
 *    simple error reporting, e.g. for prototyping. If you are using MFC or
 *    some other graphical interface with no console, DEBUG probably should be
 *    undefined.  Actually, for Sequencer64, the output can be re-routed to
 *    a log-file for trouble-shooting.
 *
 *    PM_CHECK_ERRORS more-or-less takes over error checking for return values,
 *    stopping your program and printing error messages when an error occurs.
 *    This also uses stdio for console text I/O.  For Sequencer64, we want to
 *    see these errors, all the time, and they can be redirected to a log file
 *    via the "-o log=filename.log" or "--option log=filename.log"
 *    command-line options.
 */

#include <string.h>                     /* C::strcasestr() GNU function.    */

#ifdef _MSC_VER
#pragma warning(disable: 4244) // stop warnings about downsize typecasts
#pragma warning(disable: 4018) // stop warnings about signed/unsigned

/*
 *  We will need an implementation of strcasestr() in terms of a Microsoft
 *  non-case-sensitive string comparison, for the freakin' Microsoft compiler.
 */

#endif

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#include "easy_macros.h"
#include "portmidi.h"
#include "porttime.h"
#include "pmutil.h"
// #include "pminternal.h"  // already included

/*
 * Permanently activated!
 */

#define PM_CHECK_ERRORS

#ifdef PM_CHECK_ERRORS
#include <stdio.h>
#endif

/**
 *  MIDI status (event) values.
 */

#define MIDI_STATUS_MASK    0x80
#define MIDI_NOTE_ON        0x90
#define MIDI_NOTE_OFF       0x80
#define MIDI_CHANNEL_AT     0xD0
#define MIDI_POLY_AT        0xA0
#define MIDI_PROGRAM        0xC0
#define MIDI_CONTROL        0xB0
#define MIDI_PITCHBEND      0xE0
#define MIDI_MTC            0xF1
#define MIDI_SONGPOS        0xF2
#define MIDI_SONGSEL        0xF3
#define MIDI_TUNE           0xF6
#define MIDI_CLOCK          0xF8
#define MIDI_ACTIVE         0xFE
#define MIDI_SYSEX          0xF0
#define MIDI_EOX            0xF7
#define MIDI_START          0xFA
#define MIDI_STOP           0xFC
#define MIDI_CONTINUE       0xFB
#define MIDI_F9             0xF9
#define MIDI_FD             0xFD
#define MIDI_RESET          0xFF

/**
 *  Tests for an empty queue.
 */

#define is_empty(midi)                  ((midi)->tail == (midi)->head)

/**
 *  Indicates an errant or unassigned device ID.
 */

#define PORTMIDI_BAD_DEVICE_ID          (-1)

/*
 *  This value is not static so that pm_init can set it directly if needed.
 *  (see pmmac.c:pm_init())
 */

int pm_initialized = FALSE;

/**
 *
 */

int pm_hosterror;

/**
 *
 */

char pm_hosterror_text[PM_HOST_ERROR_MSG_LEN];

/**
 *
 *  System implementation of portmidi interface.  We should wrap this in a
 *  function as well.
 */

int pm_descriptor_max = 0;
int pm_descriptor_index = 0;
descriptor_type pm_descriptors = nullptr;

/*
 * Improved access to error messages and debugging features.  These will be
 * accessible only via external functions defined after this section of static
 * declarations.
 */

/**
 *  A boolean to indicate to show debug messages.  Will start at false by
 *  default for now.
 */

static int pm_show_debug = 1;

/**
 *  A boolean to indicate to exit upon error.  Will start at true by default
 *  for now.
 */

static int pm_exit_on_error = 1;

/**
 *  A boolean to indicate the presence of a PortMidi error.
 */

static int pm_error_present = 0;

/**
 *  Like pm_host_error_text[], but will be exposed to the outside world and
 *  might contain additional information.
 */

static char pm_hosterror_message [PM_HOST_ERROR_MSG_LEN];

/**
 *  Provides a case-insensitive implementation of strstr() that doesn't need a
 *  C extension function.  Not necessarily efficient, and handles only string
 *  that are relatively small.
 *
 * \param src
 *      Provides the string that is suspected of containing the target.
 *
 * \param target
 *      Provides the string to be searched for.
 *
 * \return
 *      If all the parameters are suitable (not null, non-zero length, not too
 *      long [256 bytes]), returns true if the target was found.  Otherwise,
 *      returns false.
 */

static int
strstrcase (const char * src, const char * target)
{
    int result = false;
    int lensrc = 0;
    int lentarget = 0;
    int ok = not_nullptr(src);
    if (ok)
    {
        lensrc = strlen(src);
        ok = lensrc > 0;
    }
    if (ok)
    {
        ok = not_nullptr(target);
        if (ok)
        {
            lentarget = strlen(target);
            ok = lentarget > 0;
        }
    }
    if (ok)
    {
        char tempsrc[PM_STRING_MAX];
        char temptarget[PM_STRING_MAX];
        int isrc;
        int itarget;
        (void) snprintf(tempsrc, sizeof(tempsrc), src);
        (void) snprintf(temptarget, sizeof(temptarget), target);
        for (isrc = 0; isrc < lensrc; ++isrc)
            tempsrc[isrc] = (char) tolower((unsigned char) src[isrc]);

        for (itarget = 0; itarget < lentarget; ++itarget)
            temptarget[itarget] = (char) tolower((unsigned char) target[itarget]);

        result = not_nullptr(strstr(tempsrc, temptarget));
    }
    return result;
}

/**
 * \setter pm_show_debug
 */

void
Pm_set_show_debug (int flag)
{
    pm_show_debug = flag;
}

/**
 * \getter pm_show_debug
 */

int
Pm_show_debug (void)
{
    return pm_show_debug;
}

/**
 * \setter pm_exit_on_error
 */

void
Pm_set_exit_on_error (int flag)
{
    pm_exit_on_error = flag;
}

/**
 * \getter pm_exit_on_error
 */

int
Pm_exit_on_error (void)
{
    return pm_exit_on_error;
}

/**
 * \setter pm_error_present
 */

void
Pm_set_error_present (int flag)
{
    pm_error_present = flag;
}

/**
 * \getter pm_error_present
 */

int
Pm_error_present (void)
{
    return pm_error_present;
}

/**
 * \setter pm_host_error_message
 *      Also sets pm_error_present; sets it to true if the message has a
 *      non-zero length, and false if 0 length or the pointer is null.
 *      Thus, "Pm_set_hosterror_message(nullptr)" is a good canonical way to
 *      clear the error message.
 */

void
Pm_set_hosterror_message (const char * msg)
{
    if (not_nullptr(msg))
    {
        if (strlen(msg) == 0)
        {
            pm_hosterror_message[0] = 0;
            pm_error_present = FALSE;
        }
        else
        {
            snprintf(pm_hosterror_message, sizeof pm_hosterror_message, msg);
            pm_error_present = TRUE;
        }
    }
    else
        pm_error_present = FALSE;
}

/**
 * \getter pm_host_error_message
 */

const char *
Pm_hosterror_message (void)
{
    return &pm_hosterror_message[0];
}

#ifdef PM_CHECK_ERRORS

/**
 *
 */

static void
prompt_and_exit (void)
{

#if defined PLATFORM_DEBUG_XXX
    char line[PM_STRING_MAX];
    printf("Type Enter...");
    fgets(line, PM_STRING_MAX, stdin);
#endif

    if (pm_exit_on_error)
        exit(-1);                       /* this will clean up open ports */
}

#endif      // PM_CHECK_ERRORS

/**
 *  It seems pointless to allocate memory and copy the string, so do the work
 *  of Pm_GetHostErrorText() directly.
 */

static PmError
pm_errmsg (PmError err, int deviceid)
{
    if (err == pmHostError)
    {
        char temp[PM_HOST_ERROR_MSG_LEN];
        (void) snprintf
        (
            temp, sizeof temp, "PortMidi host error: [%d] '%s'\n",
            deviceid, pm_hosterror_text
        );
        Pm_set_hosterror_message(temp);
        pm_hosterror = FALSE;                       /* Why????              */
        pm_hosterror_text[0] = 0;                   /* clear the message    */
#ifdef PM_CHECK_ERRORS
        printf("%s", temp);
        prompt_and_exit();
#endif
    }
    else if (err < 0)
    {
        char temp[PM_HOST_ERROR_MSG_LEN];
        const char * errmsg = Pm_GetErrorText(err);
        (void) snprintf
        (
            temp, sizeof temp, "PortMidi call failed: [%d] '%s'\n",
            deviceid, errmsg
        );
        Pm_set_hosterror_message(temp);
#ifdef PM_CHECK_ERRORS
        printf("%s", temp);
        prompt_and_exit();
#endif
    }
    return err;
}

/**
 *  Describe interface/device pair to library.  This is called at
 *  intialization time, once for each interface (e.g. DirectSound) and device
 *  (e.g. SoundBlaster 1). The strings are retained but NOT COPIED, so do not
 *  destroy them!
 *
 * \param interf
 *      The name of the interface.  It is "MMSystem" for Windows.
 *
 * \param name
 *      The name of the device.  This is a pointer to an external entity, such
 *      as midi_in_mapper_caps.szPname.
 *
 * \param input
 *      A boolean that is set to 1 (true) if the device is an input device,
 *      and 0 (false) if it is an output device.
 *
 * \param descriptor
 *      Provides additional information for a given API, we think.
 *
 * \param dictionary
 *      Indicates the function signature of MIDI handler functions, we think.
 *
 * \param client
 *      Provides a client number for the device.  This value is an ALSA
 *      concept.  For other APIs, it is simply the ordinal of the device as it
 *      was looked up.
 *
 * \param port
 *      Provides a port number for the device.  This value is an ALSA
 *      concept.  For other APIs, it is simply the ordinal of the port as it
 *      was looked up, or simply 0.
 *
 * \return
 *      pmInvalidDeviceId if device memory is exceeded otherwise returns
 *      pmNoError.
 */

PmError
pm_add_device
(
    char * interf, char * name, int input,
    void * descriptor, pm_fns_type dictionary,
    int client, int port
)
{
    int ismapper = not_nullptr(strstrcase(name, "mapper"));
    if (pm_descriptor_index >= pm_descriptor_max)
    {
        const size_t sdesc = sizeof(descriptor_node); // expand descriptors
        descriptor_type new_descriptors =
            (descriptor_type) pm_alloc(sdesc * (pm_descriptor_max + 32));

        if (is_nullptr(new_descriptors))
            return pmInsufficientMemory;

        if (not_nullptr(pm_descriptors))
        {
            memcpy(new_descriptors, pm_descriptors, sdesc * pm_descriptor_max);
            free(pm_descriptors);
        }
        pm_descriptor_max += 32;
        pm_descriptors = new_descriptors;
    }
    pm_descriptors[pm_descriptor_index].pub.structVersion = PM_STRUCTURE_VERSION;
    pm_descriptors[pm_descriptor_index].pub.interf = interf;
    pm_descriptors[pm_descriptor_index].pub.name = name;
    pm_descriptors[pm_descriptor_index].pub.input = input;
    pm_descriptors[pm_descriptor_index].pub.output = ! input;
    pm_descriptors[pm_descriptor_index].pub.mapper = ismapper;
    pm_descriptors[pm_descriptor_index].pub.client = client;
    pm_descriptors[pm_descriptor_index].pub.port = port;

    /*
     * Default state: nothing to close (for automatic device closing).
     */

    pm_descriptors[pm_descriptor_index].pub.opened = FALSE;

    /*
     * ID number passed to Win32 multimedia API open function.
     */

    pm_descriptors[pm_descriptor_index].descriptor = descriptor;

    /*
     * Points to PmInternal, allows automatic device closing.
     */

    pm_descriptors[pm_descriptor_index].internalDescriptor = nullptr;
    pm_descriptors[pm_descriptor_index].dictionary = dictionary;
    ++pm_descriptor_index;

#if defined PLATFORM_DEBUG_TMI
    printf("Device '%s' added to PortMidi\n", name);
#endif

    return pmNoError;
}

/**
 *  Utility to look up device, given a pattern. Note: the pattern is modified.
 */

int
pm_find_default_device (char * pattern, int is_input)
{
    int id = pmNoDevice;
    int i;

    /* first parse pattern into name, interf parts */

    char * interf_pref = "";            /* initially assume it's not there  */
    char * name_pref = strstr(pattern, ", ");
    if (name_pref)                      /* found separator, adjust pointer  */
    {
        interf_pref = pattern;
        name_pref[0] = 0;
        name_pref += 2;
    }
    else
        name_pref = pattern;            /* whole string is the name pattern */

    for (i = 0; i < pm_descriptor_index; ++i)
    {
        const PmDeviceInfo * info = Pm_GetDeviceInfo(i);
        if
        (
            info->input == is_input &&
            strstr(info->name, name_pref) && strstr(info->interf, interf_pref)
        )
        {
            id = i;
            break;
        }
    }
    return id;
}

/**
 *  Get devices count; IDs range from 0 to Pm_CountDevices()-1.
 */

PMEXPORT int
Pm_CountDevices (void)
{
    Pm_Initialize();                /* no error checking; does not fail */
    return pm_descriptor_index;
}

/**
 *  Pm_GetDeviceInfo() returns a pointer to a PmDeviceInfo structure referring
 *  to the device specified by id.  If ID is out of range the function returns
 *  NULL.
 *
 *  The returned structure is owned by the PortMidi implementation and must not
 *  be manipulated or freed. The pointer is guaranteed to be valid between
 *  calls to Pm_Initialize() and Pm_Terminate().
 */

PMEXPORT const PmDeviceInfo *
Pm_GetDeviceInfo (PmDeviceID id)
{
    Pm_Initialize();                            /* no error check needed */
    if (id >= 0 && id < pm_descriptor_index)
        return &pm_descriptors[id].pub;

    return nullptr;
}

/**
 *  Provides a "no-op" function pointer.
 */

PmError
pm_success_fn (PmInternal * UNUSED(midi))
{
    return pmNoError;
}

/**
 *  Returns an error if called.
 */

PmError
none_write_short (PmInternal * UNUSED(midi), PmEvent * UNUSED(buffer))
{
    return pmBadPtr;
}

/**
 *  Provides a placeholder for begin_sysex() and flush().
 */

PmError
pm_fail_timestamp_fn (PmInternal * UNUSED(midi), PmTimestamp UNUSED(timestamp))
{
    return pmBadPtr;
}

/**
 *
 */

PmError
none_write_byte
(
    PmInternal * UNUSED(midi), midibyte_t UNUSED(byte),
    PmTimestamp UNUSED(timestamp)
)
{
    return pmBadPtr;
}

/**
 *  A generic function, returns error if called.
 */

PmError
pm_fail_fn (PmInternal * UNUSED(midi))
{
    return pmBadPtr;
}

/**
 *
 */

static PmError
none_open (PmInternal * UNUSED(midi), void * UNUSED(driverinfo))
{
    return pmBadPtr;
}

/**
 *
 */

static void
none_get_host_error (PmInternal * UNUSED(midi), char * msg, unsigned UNUSED(len))
{
    *msg = 0;       // empty string
}

/**
 *
 */

static unsigned
none_has_host_error (PmInternal * UNUSED(midi))
{
    return FALSE;
}

/**
 *
 */

PmTimestamp
none_synchronize (PmInternal * UNUSED(midi))
{
    return 0;
}

/**
 *  Defines for the abort and close functions.
 */

#define none_abort pm_fail_fn
#define none_close pm_fail_fn

/**
 * Lookup?
 */

pm_fns_node pm_none_dictionary =
{
    none_write_short,
    none_sysex,
    none_sysex,
    none_write_byte,
    none_write_short,
    none_write_flush,
    none_synchronize,
    none_open,
    none_abort,
    none_close,
    none_poll,
    none_has_host_error,
    none_get_host_error         /* incompatible function signature! */
};

/**
 *  Translate portmidi error number into human readable message.  These strings
 *  are constants (set at compile time) so client has no need to allocate
 *  storage
 */

PMEXPORT const char *
Pm_GetErrorText (PmError errnum)
{
    const char * msg;
    switch(errnum)
    {
    case pmNoError:
        msg = "";
        break;

    case pmHostError:
        msg = "Host error";
        break;

    case pmInvalidDeviceId:
        msg = "Invalid device ID";
        break;

    case pmInsufficientMemory:
        msg = "Insufficient memory";
        break;

    case pmBufferTooSmall:
        msg = "Buffer too small";
        break;

    case pmBadPtr:
        msg = "Bad pointer";
        break;

    case pmInternalError:
        msg = "Internal PortMidi Error";
        break;

    case pmBufferOverflow:
        msg = "Buffer overflow";
        break;

    case pmBadData:
        msg = "Invalid MIDI message Data";
        break;

    case pmBufferMaxSize:
        msg = "Buffer cannot be made larger";
        break;

    case pmDeviceClosed:
        msg = "Device is closed";
        break;

    case pmDeviceOpen:
        msg = "Device already open";
        break;

    case pmWriteToInput:
        msg = "Writing to input device";
        break;

    case pmReadFromOutput:
        msg = "Reading from output device";
        break;

    case pmErrOther:
        msg = "Unspecified error";
        break;

    default:
        msg = "Illegal error number";
        break;
    }
    return msg;
}

/**
 *  Translate portmidi host error into human readable message.  These strings
 *  are computed at run time, so client has to allocate storage.  After this
 *  routine executes, the host error is cleared.  This can be called whenever
 *  you get a pmHostError return value.  The error will always be in the
 *  global pm_hosterror_text.
 */

PMEXPORT void
Pm_GetHostErrorText (char * msg, unsigned len)
{
    assert(msg);
    assert(len > 0);
    if (pm_hosterror)
    {
        strncpy(msg, (char *) pm_hosterror_text, len);
        pm_hosterror = FALSE;

        /*
         * Clear the message; not necessary, but it might help with debugging
         */

        pm_hosterror_text[0] = 0;
        msg[len - 1] = 0;               /* make sure string is terminated */
    }
    else
        msg[0] = 0;                     /* no string to return */
}

/**
 *  Test whether stream has a pending host error. Normally, the client finds
 *  out about errors through returned error codes, but some errors can occur
 *  asynchronously where the client does not explicitly call a function, and
 *  therefore cannot receive an error code.  The client can test for a pending
 *  error using Pm_HasHostError(). If true, the error can be accessed and
 *  cleared by calling Pm_GetErrorText().  Errors are also cleared by calling
 *  other functions that can return errors, e.g. Pm_OpenInput(),
 *  Pm_OpenOutput(), Pm_Read(), Pm_Write(). The client does not need to call
 *  Pm_HasHostError(). Any pending error will be reported the next time the
 *  client performs an explicit function call on the stream, e.g. an input or
 *  output operation. Until the error is cleared, no new error codes will be
 *  obtained, even for a different stream.
 */

PMEXPORT int
Pm_HasHostError (PortMidiStream * stream)
{
    if (pm_hosterror)
        return TRUE;

    if (stream)
    {
        PmInternal * midi = (PmInternal *) stream;
        pm_hosterror = (*midi->dictionary->has_host_error)(midi);
        if (pm_hosterror)
        {
            midi->dictionary->host_error
            (
                midi, pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
            );
            return TRUE;                /* now error message is global */
        }
    }
    return FALSE;
}

/**
 *  Pm_Initialize() is the library initialisation function - call this before
 *  using the library.
 */

PMEXPORT PmError
Pm_Initialize (void)
{
    if (! pm_initialized)
    {
        pm_hosterror = FALSE;
        pm_hosterror_text[0] = 0;       /* the null string */
        pm_init();
        pm_initialized = TRUE;
    }
    return pmNoError;
}

/**
 *  Pm_Terminate() is the library termination function - call this after
 *  using the library.
 */

PMEXPORT PmError
Pm_Terminate (void)
{
    if (pm_initialized)
    {
        pm_term();

        /*
         * If there are no devices, descriptors might still be null. The Linux
         * code has already done this, but the Windows version does not
         * AFAICT.
         */

        if (not_nullptr(pm_descriptors))
        {
            free(pm_descriptors);
            pm_descriptors = nullptr;
        }
        pm_descriptor_index = 0;
        pm_descriptor_max = 0;
        pm_initialized = FALSE;
    }
    return pmNoError;
}

/**
 *  Read up to length messages from source into buffer.  Pm_Read() retrieves
 *  midi data into a buffer, and returns the number of events read. Result is
 *  a non-negative number unless an error occurs, in which case a PmError
 *  value will be returned.
 *
 * Buffer Overflow:
 *
 *      The problem: if an input overflow occurs, data will be lost,
 *      ultimately because there is no flow control all the way back to the
 *      data source.  When data is lost, the receiver should be notified and
 *      some sort of graceful recovery should take place, e.g. you shouldn't
 *      resume receiving in the middle of a long sysex message.
 *
 *  With a lock-free fifo, which is pretty much what we're stuck with to
 *  enable portability to the Mac, it's tricky for the producer and consumer
 *  to synchronously reset the buffer and resume normal operation.
 *
 *  Solution: the buffer managed by PortMidi will be flushed when an overflow
 *  occurs. The consumer (Pm_Read()) gets an error message (pmBufferOverflow)
 *  and ordinary processing resumes as soon as a new message arrives. The
 *  remainder of a partial sysex message is not considered to be a "new
 *  message" and will be flushed as well.
 *
 *  This function polls for MIDI data in the buffer.  It either simply checks
 *  for data, or attempts first to fill the buffer with data from the MIDI
 *  hardware; this depends on the implementation.  We could call Pm_Poll()
 *  here, but that would redo a lot of redundant parameter checking, so I
 *  copied some code from Pm_Poll to here.
 *
 *  For Linux, the poll function is alsa_poll() in the pmlinuxalsa module.
 *  That function does a lot of work, so a simpler version that just check for
 *  data is called, to support Sequencer64's midibus object.
 *
 * \return
 *      Returns the number of messages actually read, or an error code.
 */

PMEXPORT int
Pm_Read (PortMidiStream * stream, PmEvent * buffer, int32_t length)
{
    /*
     * From here to the END marker, basically identical to Pm_Poll().
     */

    PmInternal * midi = (PmInternal *) stream;
    int deviceid = PORTMIDI_BAD_DEVICE_ID;
    PmError err = pmNoError;
    int n = 0;
    pm_hosterror = FALSE;
    if (is_nullptr(midi))
        err = pmBadPtr;
    else
    {
        deviceid = midi->device_id;
        if (! pm_descriptors[deviceid].pub.opened)
            err = pmDeviceClosed;
        else if (! pm_descriptors[deviceid].pub.input)
            err = pmReadFromOutput;
        else
            err = (*(midi->dictionary->poll))(midi);    /* see the banner   */
    }
    if (err != pmNoError)
    {
        if (err == pmHostError)
        {
            midi->dictionary->host_error
            (
                midi, pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
            );
            pm_hosterror = TRUE;
        }
        return pm_errmsg(err, deviceid);
    }

    /*
     * END: return ! Pm_QueueEmpty(midi->queue);
     */

    while (n < length)
    {
        PmError err = Pm_Dequeue(midi->queue, buffer++);
        if (err == pmBufferOverflow)        /* ignore data retrieved so far */
            return pm_errmsg(pmBufferOverflow, deviceid);
        else if (err == 0)                  /* empty queue                  */
            break;

        ++n;
    }
    return n;
}

/**
 *  Pm_Poll() tests whether input is available, returning TRUE (PmGotData = 1),
 *  FALSE (PmNoData = 0), or an error value.  This is a pretty weird way of
 *  dealing with polling status.
 */

PMEXPORT PmError
Pm_Poll (PortMidiStream * stream)
{
    PmInternal * midi = (PmInternal *) stream;
    int deviceid = PORTMIDI_BAD_DEVICE_ID;
    PmError result = pmNoError;
    pm_hosterror = FALSE;
    if (is_nullptr(midi))
        result = pmBadPtr;
    else
    {
        deviceid = midi->device_id;
        if (! pm_descriptors[deviceid].pub.opened)
            result = pmDeviceClosed;
        else if (! pm_descriptors[deviceid].pub.input)
            result = pmReadFromOutput;
        else
            result = (*(midi->dictionary->poll))(midi);
    }
    if (result != pmNoError)
    {
        if (result == pmHostError)
        {
            midi->dictionary->host_error
            (
                midi, pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
            );
           pm_hosterror = TRUE;
        }
        return pm_errmsg(result, deviceid);
    }
    return ! Pm_QueueEmpty(midi->queue);
}

/**
 *  This is called from Pm_Write and Pm_WriteSysEx to issue a call to the
 *  system-dependent end_sysex function and handle the error return.
 */

static PmError
pm_end_sysex (PmInternal * midi)
{
    PmError err = (*midi->dictionary->end_sysex)(midi, 0);
    midi->sysex_in_progress = FALSE;
    if (err == pmHostError)
    {
        midi->dictionary->host_error
        (
            midi, pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
        pm_hosterror = TRUE;
    }
    return err;
}

/**
 *  To facilitate correct error-handling, Pm_Write(), Pm_WriteShort(), and
 *  Pm_WriteSysEx() all operate a state machine that "outputs" calls to
 *  write_short(), begin_sysex(), write_byte(), end_sysex(), and
 *  write_realtime().
 *
 *  Pm_Write() writes midi data from a buffer. This may contain:
 *
 *      - short messages
 *      - sysex messages that are converted into a sequence of PmEvent
 *        structures, e.g. sending data from a file or forwarding them
 *        from midi input.
 *
 *  Use Pm_WriteSysEx() to write a SysEx message stored as a contiguous
 *  array of bytes.  SysEX data may contain embedded real-time messages.
 */

PMEXPORT PmError
Pm_Write (PortMidiStream * stream, PmEvent * buffer, int32_t length)
{
    PmInternal * midi = (PmInternal *) stream;
    int deviceid = PORTMIDI_BAD_DEVICE_ID;
    PmError err = pmNoError;
    pm_hosterror = FALSE;
    int i;
    int bits;
    if (is_nullptr(midi))
        err = pmBadPtr;
    else
    {
        deviceid = midi->device_id;
        if (! pm_descriptors[deviceid].pub.opened)
            err = pmDeviceClosed;
        else if (! pm_descriptors[deviceid].pub.output)
            err = pmWriteToInput;
        else
            err = pmNoError;
    }

    if (err != pmNoError)
        goto pm_write_error;

    if (midi->latency == 0)
    {
        midi->now = 0;
    }
    else
    {
        midi->now = (*(midi->time_proc))(midi->time_info);
        if (midi->first_message || midi->sync_time + 100 /*ms*/ < midi->now)
        {
            midi->now = (*midi->dictionary->synchronize)(midi); /* resync */
            midi->first_message = FALSE;
        }
    }

    /*  Error recovery:
     *
     *      When a SysEx is detected, we call dictionary->begin_sysex()
     *      followed by calls to dictionary->write_byte() and
     *      dictionary->write_realtime() until an end-of-SysEx is detected,
     *      when we call dictionary->end_sysex(). After an error occurs,
     *      Pm_Write() continues to call functions. For example, it will
     *      continue to call write_byte() even after an error sending a SysEx
     *      message, and end_sysex() will be called when an EOX or
     *      non-real-time status is found.  When errors are detected,
     *      Pm_Write() returns immediately, so it is possible that this will
     *      drop data and leave SysEx messages in a partially transmitted
     *      state.
     */

    for (i = 0; i < length; ++i)
    {
        uint32_t msg = buffer[i].message;
        bits = 0;

        /* is this a SysEx message? */

        if (Pm_MessageStatus(msg) == MIDI_SYSEX)
        {
            if (midi->sysex_in_progress)
            {
                /* error: previous SysEx was not terminated by EOX */

                midi->sysex_in_progress = FALSE;
                err = pmBadData;
                goto pm_write_error;
            }
            midi->sysex_in_progress = TRUE;
            if
            (
                (
                    err = (*midi->dictionary->begin_sysex)
                    (
                        midi, buffer[i].timestamp
                    )
                ) != pmNoError
            )
            {
                goto pm_write_error;
            }
            if
            (
                (
                    err = (*midi->dictionary->write_byte)
                    (
                        midi, MIDI_SYSEX, buffer[i].timestamp
                    )
                ) != pmNoError)
            {
                goto pm_write_error;
            }
            bits = 8;

            /* fall through to continue SysEx processing */

        }
        else if ((msg & MIDI_STATUS_MASK) && (Pm_MessageStatus(msg) != MIDI_EOX))
        {
            if (midi->sysex_in_progress)        /* a non-SysEx message?     */
            {
                if (is_real_time(msg))      /* should be a realtime message */
                {
                    err = (*midi->dictionary->write_realtime)(midi, &(buffer[i]));
                    if (err!= pmNoError)
                        goto pm_write_error;
                }
                else
                {
                    midi->sysex_in_progress = FALSE;
                    err = pmBadData;

                    /*
                     * Ignore any error from this, because we already have
                     * one.  Pass 0 as timestamp -- it's ignored.
                     */

                    (*midi->dictionary->end_sysex)(midi, 0);
                    goto pm_write_error;
                }
            }
            else
            {
                /*
                 * Regular short MIDI message
                 */

                err = (*midi->dictionary->write_short)(midi, &(buffer[i]));
                if (err != pmNoError)
                    goto pm_write_error;

                continue;
            }
        }
        if (midi->sysex_in_progress)
        {
            /*
             * Send SysEx bytes until EOX.  See if we can accelerate data
             * transfer.
             */

            if
            (
                bits == 0 && midi->fill_base &&     /* 4 bytes to copy */
                (*midi->fill_offset_ptr) + 4 <= midi->fill_length &&
                (msg & 0x80808080) == 0
            )
            {
                /*
                 * All data.  Copy 4 bytes from msg to fill_base + fill_offset
                 */

                midibyte_t * ptr = midi->fill_base + *(midi->fill_offset_ptr);
                ptr[0] = msg; ptr[1] = msg >> 8;
                ptr[2] = msg >> 16; ptr[3] = msg >> 24;
                (*midi->fill_offset_ptr) += 4;
                 continue;
            }
            while (bits < 32)       /* no acceleration, copy byte-by-byte   */
            {
                midibyte_t midi_byte = (midibyte_t) (msg >> bits);
                err = (*midi->dictionary->write_byte)
                (
                    midi, midi_byte, buffer[i].timestamp
                );
                if (err != pmNoError)
                    goto pm_write_error;

                if (midi_byte == MIDI_EOX)
                {
                    err = pm_end_sysex(midi);
                    if (err != pmNoError) goto error_exit;
                    break;
                }
                bits += 8;
            }
        }
        else
        {
            /* not in SysEx mode, but message did not start with status */

            err = pmBadData;
            goto pm_write_error;
        }
    }

    /* after all messages are processed, send the data */

    if (! midi->sysex_in_progress)
        err = (*midi->dictionary->write_flush)(midi, 0);

pm_write_error:

    if (err == pmHostError)
    {
        midi->dictionary->host_error
        (
            midi, pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
        pm_hosterror = TRUE;
    }

error_exit:

    return pm_errmsg(err, deviceid);
}

/**
 *  Pm_WriteShort() writes a timestamped non-system-exclusive midi message.
 *  Messages are delivered in order as received, and timestamps must be
 *  non-decreasing. (But timestamps are ignored if the stream was opened
 *  with latency = 0.)
 */

PMEXPORT PmError
Pm_WriteShort (PortMidiStream * stream, PmTimestamp when, PmMessage msg)
{
    PmEvent event;
    event.timestamp = when;
    event.message = msg;
    return Pm_Write(stream, &event, 1);
}

/**
 *
 */

#define BUFLEN ((int) (PM_DEFAULT_SYSEX_BUFFER_SIZE / sizeof(PmMessage)))

/**
 *  Pm_WriteSysEx() writes a timestamped system-exclusive midi message.
 *  Allocate buffer space for PM_DEFAULT_SYSEX_BUFFER_SIZE bytes.  Each
 *  PmEvent holds sizeof(PmMessage) bytes of SysEx data.
 */

PMEXPORT PmError
Pm_WriteSysEx (PortMidiStream * stream, PmTimestamp when, midibyte_t * msg)
{
    PmInternal * midi = (PmInternal *) stream;
    PmEvent buffer[BUFLEN];
    int buffer_size = 1;    /* first time, send 1. After that, it's BUFLEN */

    /*
     * The next byte in the buffer is represented by an index, bufx, and
     * a shift in bits.
     */

    int shift = 0;
    int bufx = 0;
    buffer[0].message = 0;
    buffer[0].timestamp = when;
    for (;;)
    {
        buffer[bufx].message |= ((*msg) << shift); /* put next byte in buffer */
        shift += 8;
        if (*msg++ == MIDI_EOX)
            break;

        if (shift == 32)
        {
            shift = 0;
            if (++bufx == buffer_size)                          // ++bufx;
            {
                PmError err = Pm_Write(stream, buffer, buffer_size);
                if (err)
                    return err;         /* Pm_Write() called errmsg()       */

                bufx = 0;               /* prepare to fill another buffer   */
                buffer_size = BUFLEN;
                if (midi->fill_base)    /* optimization? just copy bytes?   */
                {
                    while (*(midi->fill_offset_ptr) < midi->fill_length)
                    {
                        midi->fill_base[(*midi->fill_offset_ptr)++] = *msg;
                        if (*msg++ == MIDI_EOX)
                        {
                            err = pm_end_sysex(midi);
                            if (err != pmNoError)
                                return pm_errmsg(err, midi->device_id);

                            goto end_of_sysex;
                        }
                    }

                    /*
                     *  I thought that I could do a pm_Write here and change
                     *  this if to a loop, avoiding calls in Pm_Write() to the
                     *  slower write_byte, but since sysex_in_progress is
                     *  true, this will not flush the buffer, and we'll
                     *  infinite loop:
                     *
                     * err = Pm_Write(stream, buffer, 0);
                     * if (err)
                     *      return err;
                     *
                     *  Instead, the way this works is that Pm_Write() calls
                     *  write_byte on 4 bytes. The first, since the buffer is
                     *  full, will flush the buffer and allocate a new one.
                     *  This primes the buffer so that we can return to the
                     *  loop above and fill it efficiently without a lot of
                     *  function calls.
                     */

                    buffer_size = 1;        /* get another message started */
                }
            }
            buffer[bufx].message = 0;
            buffer[bufx].timestamp = when;
        }
    }                   /* keep inserting bytes until you find MIDI_EOX     */

end_of_sysex:

    /*
     * Finished sending full buffers, but there may be a partial one left.
     */

    if (shift != 0)
        ++bufx;                 /* add partial message to buffer length     */

    if (bufx > 0)               /* number of PmEvents to send from buffer   */
    {
        PmError err = Pm_Write(stream, buffer, bufx);
        if (err)
            return err;
    }
    return pmNoError;
}

/**
 *  Pm_OpenInput() and Pm_OpenOutput() open devices.
 *
 *  stream is the address of a PortMidiStream pointer which will receive a
 *  pointer to the newly opened stream.
 *
 *  inputDevice is the id of the device used for input (see PmDeviceID above).
 *
 *  inputDriverInfo is a pointer to an optional driver specific data structure
 *  containing additional information for device setup or handle processing.
 *  inputDriverInfo is never required for correct operation. If not used
 *  inputDriverInfo should be NULL.
 *
 *  outputDevice is the id of the device used for output (see PmDeviceID
 *  above.)
 *
 *  outputDriverInfo is a pointer to an optional driver specific data structure
 *  containing additional information for device setup or handle processing.
 *  outputDriverInfo is never required for correct operation. If not used
 *  outputDriverInfo should be NULL.
 *
 *  For input, the buffersize specifies the number of input events to be
 *  buffered waiting to be read using Pm_Read(). For output, buffersize
 *  specifies the number of output events to be buffered waiting for output.
 *  (In some cases -- see below -- PortMidi does not buffer output at all and
 *  merely passes data to a lower-level API, in which case buffersize is
 *  ignored.)
 *
 *  latency is the delay in milliseconds applied to timestamps to determine
 *  when the output should actually occur. (If latency is < 0, 0 is assumed.)
 *  If latency is zero, timestamps are ignored and all output is delivered
 *  immediately. If latency is greater than zero, output is delayed until the
 *  message timestamp plus the latency. (NOTE: the time is measured relative to
 *  the time source indicated by time_proc. Timestamps are absolute, not
 *  relative delays or offsets.) In some cases, PortMidi can obtain better
 *  timing than your application by passing timestamps along to the device
 *  driver or hardware. Latency may also help you to synchronize midi data to
 *  audio data by matching midi latency to the audio buffer latency.
 *
 *  time_proc is a pointer to a procedure that returns time in milliseconds. It
 *  may be NULL, in which case a default millisecond timebase (PortTime) is
 *  used. If the application wants to use PortTime, it should start the timer
 *  (call Pt_Start) before calling Pm_OpenInput or Pm_OpenOutput. If the
 *  application tries to start the timer *after* Pm_OpenInput or Pm_OpenOutput,
 *  it may get a ptAlreadyStarted error from Pt_Start, and the application's
 *  preferred time resolution and callback function will be ignored.  time_proc
 *  result values are appended to incoming MIDI data, and time_proc times are
 *  used to schedule outgoing MIDI data (when latency is non-zero).
 *
 *  time_info is a pointer passed to time_proc.
 *
 *  Example: If I provide a timestamp of 5000, latency is 1, and time_proc
 *  returns 4990, then the desired output time will be when time_proc returns
 *  timestamp+latency = 5001. This will be 5001-4990 = 11ms from now.
 *
 * \return
 *      Upon success Pm_Open() returns PmNoError and places a pointer to a
 *      valid PortMidiStream in the stream argument.  If a call to Pm_Open()
 *      fails a nonzero error code is returned (see PMError above) and the
 *      value of port is invalid.  Any stream that is successfully opened
 *      should eventually be closed by calling Pm_Close().
 */

PMEXPORT PmError
Pm_OpenInput
(
    PortMidiStream ** stream,
    PmDeviceID inputDevice,
    void * inputDriverInfo,
    int32_t bufferSize,
    PmTimeProcPtr time_proc,
    void * time_info
)
{
    PmInternal * midi;
    PmError err = pmNoError;
    pm_hosterror = FALSE;
    if (not_nullptr(stream))
        *stream = nullptr;
    else
        err = pmBadPtr;

    if (inputDevice < 0 || inputDevice >= pm_descriptor_index)
        err = pmInvalidDeviceId;
    else if (! pm_descriptors[inputDevice].pub.input)
        err = pmInvalidDeviceId;
    else if (pm_descriptors[inputDevice].pub.opened)
        err = pmDeviceOpen;

    if (err != pmNoError)
        goto error_return;

    /* create portMidi internal data */

    midi = (PmInternal *) pm_alloc(sizeof(PmInternal));
    *stream = midi;
    if (is_nullptr(midi))
    {
        err = pmInsufficientMemory;
        goto error_return;
    }
    midi->device_id = inputDevice;
    midi->write_flag = FALSE;
    midi->time_proc = time_proc;
    midi->time_info = time_info;

    /*
     * Windows adds timestamps in the driver, and these are more accurate than
     * using a time_proc, so do not automatically provide a time proc. Non-win
     * implementations may want to provide a default time_proc in their
     * system-specific midi_out_open() method.
     */

    if (bufferSize <= 0)
        bufferSize = 256;                   /* default buffer size */

    midi->queue = Pm_QueueCreate(bufferSize, (int32_t) sizeof(PmEvent));
    if (! midi->queue)
    {
        /* free portMidi data */

        *stream = nullptr;
        pm_free(midi);
        err = pmInsufficientMemory;
        goto error_return;
    }
    midi->buffer_len = bufferSize;          /* portMidi input storage */
    midi->latency = 0;                      /* not used */
    midi->sysex_in_progress = FALSE;
    midi->sysex_message = 0;
    midi->sysex_message_count = 0;
    midi->filters = PM_FILT_ACTIVE;
    midi->channel_mask = 0xFFFF;
    midi->sync_time = 0;
    midi->first_message = TRUE;
    midi->dictionary = pm_descriptors[inputDevice].dictionary;
    midi->fill_base = nullptr;
    midi->fill_offset_ptr = nullptr;
    midi->fill_length = 0;
    pm_descriptors[inputDevice].internalDescriptor = midi;

    /* open system dependent input device */

    err = (*midi->dictionary->open)(midi, inputDriverInfo);
    if (err)
    {
        *stream = nullptr;
        pm_descriptors[inputDevice].internalDescriptor = nullptr;

        /* free portMidi data */

        Pm_QueueDestroy(midi->queue);
        pm_free(midi);
    }
    else
    {
        /* portMidi input open successful */

        pm_descriptors[inputDevice].pub.opened = TRUE;
    }

error_return:

    /*
     * Note: If there is a pmHostError, it is the responsibility of the
     * system-dependent code (*midi->dictionary->open)() to set pm_hosterror
     * and pm_hosterror_text.
     */

    return pm_errmsg(err, inputDevice);
}

/**
 *
 */

PMEXPORT PmError
Pm_OpenOutput
(
    PortMidiStream ** stream,
    PmDeviceID outputDevice,
    void * outputDriverInfo,
    int32_t bufferSize,
    PmTimeProcPtr time_proc,
    void * time_info,
    int32_t latency
)
{
    PmInternal * midi;
    PmError err = pmNoError;
    pm_hosterror = FALSE;

    if (not_nullptr(stream))
        *stream = nullptr;
    else
        err = pmBadPtr;

    if (outputDevice < 0 || outputDevice >= pm_descriptor_index)
        err = pmInvalidDeviceId;
    else if (! pm_descriptors[outputDevice].pub.output)
        err = pmInvalidDeviceId;
    else if (pm_descriptors[outputDevice].pub.opened)
        err = pmDeviceOpen;
    if (err != pmNoError)
        goto error_return;

    /* create portMidi internal data */

    midi = (PmInternal *) pm_alloc(sizeof(PmInternal));
    *stream = midi;
    if (! midi)
    {
        err = pmInsufficientMemory;
        goto error_return;
    }
    midi->device_id = outputDevice;
    midi->write_flag = TRUE;
    midi->time_proc = time_proc;

    /*
     * If latency > 0, we need a time reference. If none is provided,
     * use PortTime library.
     */

    if (is_nullptr(time_proc) && latency != 0)
    {
        if (! Pt_Started())
            Pt_Start(1, 0, 0);

        /* time_get does not take a parameter, so coerce */

        midi->time_proc = (PmTimeProcPtr) Pt_Time;
    }
    midi->time_info = time_info;
    midi->buffer_len = bufferSize;
    midi->queue = nullptr;                          /* unused by output     */

    /*
     * If latency zero, output immediate (timestamps ignored).  if latency <
     * 0, use 0, but don't return an error.
     */

    if (latency < 0)
        latency = 0;

    midi->latency = latency;
    midi->sysex_in_progress = FALSE;
    midi->sysex_message = 0;                        /* unused by output     */
    midi->sysex_message_count = 0;                  /* unused by output     */
    midi->filters = 0;                              /* not used for output  */
    midi->channel_mask = 0xFFFF;
    midi->sync_time = 0;
    midi->first_message = TRUE;
    midi->dictionary = pm_descriptors[outputDevice].dictionary;
    midi->fill_base = nullptr;
    midi->fill_offset_ptr = nullptr;
    midi->fill_length = 0;
    pm_descriptors[outputDevice].internalDescriptor = midi;

    /* open system dependent output device */

    err = (*midi->dictionary->open)(midi, outputDriverInfo);
    if (err)
    {
        *stream = nullptr;
        pm_descriptors[outputDevice].internalDescriptor = nullptr;
        pm_free(midi);                          /* free portMidi data */
    }
    else
        pm_descriptors[outputDevice].pub.opened = TRUE; /* input-open success */

error_return:

    /*
     * Note: system-dependent code must set pm_hosterror and pm_hosterror_text
     * if a pmHostError occurs.
     */

    return pm_errmsg(err, outputDevice);
}

/**
 *  Pm_SetChannelMask() filters incoming messages based on channel.  The mask
 *  is a 16-bit bitfield corresponding to appropriate channels.  The Pm_Channel
 *  macro can assist in calling this function.  i.e. to set receive only input
 *  on channel 1, call with Pm_SetChannelMask(Pm_Channel(1)); Multiple channels
 *  should be OR'd together, like Pm_SetChannelMask(Pm_Channel(10) |
 *  Pm_Channel(11))
 *
 *  Note that channels are numbered 0 to 15 (not 1 to 16). Most synthesizer and
 *  interfaces number channels starting at 1, but PortMidi numbers channels
 *  starting at 0.
 *
 *  All channels are allowed by default
 */

PMEXPORT PmError
Pm_SetChannelMask (PortMidiStream * stream, int mask)
{
    PmInternal * midi = (PmInternal *) stream;
    PmError err = pmNoError;
    if (is_nullptr(midi))
        err = pmBadPtr;
    else
        midi->channel_mask = mask;

    return pm_errmsg(err, PORTMIDI_BAD_DEVICE_ID);
}

/**
 *  Pm_SetFilter() sets filters on an open input stream to drop selected input
 *  types. By default, only active sensing messages are filtered.  To prohibit,
 *  say, active sensing and sysex messages, call Pm_SetFilter(stream,
 *  PM_FILT_ACTIVE | PM_FILT_SYSEX);
 *
 *  Filtering is useful when midi routing or midi thru functionality is being
 *  provided by the user application.  For example, you may want to exclude
 *  timing messages (clock, MTC, start/stop/continue), while allowing
 *  note-related messages to pass.  Or you may be using a sequencer or
 *  drum-machine for MIDI clock information but want to exclude any notes it
 *  may play.
 */

PMEXPORT PmError
Pm_SetFilter (PortMidiStream * stream, int32_t filters)
{
    int deviceid = PORTMIDI_BAD_DEVICE_ID;
    PmInternal * midi = (PmInternal *) stream;
    PmError err = pmNoError;
    if (is_nullptr(midi))
        err = pmBadPtr;
    else
    {
        deviceid = midi->device_id;
        if (! pm_descriptors[deviceid].pub.opened)
            err = pmDeviceClosed;
        else
            midi->filters = filters;
    }
    return pm_errmsg(err, deviceid);
}

/**
 *  Pm_Close() closes a midi stream, flushing any pending buffers.  (PortMidi
 *  attempts to close open streams when the application exits -- this is
 *  particularly difficult under Windows.) If it is an open device, the
 *  device_id will be valid and the device should be in the opened state.
 *
 *  For Sequencer64, we have added a check for pm_descriptors being null.
 *  This happens because the ~mastermidbus() function calls Pm_Terminate(),
 *  which frees that array.  Then ~midibus() calls Pm_Close().
 *
 *  I think we ought to straighten this out at some point.
 */

PMEXPORT PmError
Pm_Close (PortMidiStream * stream)
{
    int deviceid = PORTMIDI_BAD_DEVICE_ID;
    PmInternal * midi = (PmInternal *) stream;
    PmError err = pmNoError;
    pm_hosterror = FALSE;
    if (not_nullptr(pm_descriptors))
    {
        if (is_nullptr(midi))
            err = pmBadPtr;
        else
        {
            deviceid = midi->device_id;
            if (deviceid < 0 || deviceid >= pm_descriptor_index)
                err = pmInvalidDeviceId;
            else if (! pm_descriptors[deviceid].pub.opened)
                err = pmDeviceClosed;
        }

        if (err == pmNoError)
        {
            err = (*midi->dictionary->close)(midi);     /* close the device */

            /* even if an error occurred, continue with cleanup */

            pm_descriptors[deviceid].internalDescriptor = nullptr;
            pm_descriptors[deviceid].pub.opened = FALSE;
            if (midi->queue)
                Pm_QueueDestroy(midi->queue);

            pm_free(midi);
        }
    }

    /*
     * System-dependent code must set pm_hosterror and pm_hosterror_text if a
     * pmHostError occurs.
     */

    return pm_errmsg(err, deviceid);
}

/**
 *  Pm_Synchronize() instructs PortMidi to (re)synchronize to the time_proc
 *  passed when the stream was opened. Typically, this is used when the stream
 *  must be opened before the time_proc reference is actually advancing. In
 *  this case, message timing may be erratic, but since timestamps of zero mean
 *  "send immediately," initialization messages with zero timestamps can be
 *  written without a functioning time reference and without problems. Before
 *  the first MIDI message with a non-zero timestamp is written to the stream,
 *  the time reference must begin to advance (for example, if the time_proc
 *  computes time based on audio samples, time might begin to advance when an
 *  audio stream becomes active). After time_proc return values become valid,
 *  and BEFORE writing the first non-zero timestamped MIDI message, call
 *  Pm_Synchronize() so that PortMidi can observe the difference between the
 *  current time_proc value and its MIDI stream time.
 *
 *  In the more normal case where time_proc values advance continuously, there
 *  is no need to call Pm_Synchronize. PortMidi will always synchronize at the
 *  first output message and periodically thereafter.
 */

PmError
Pm_Synchronize (PortMidiStream * stream )
{
    PmInternal * midi = (PmInternal *) stream;
    int deviceid = PORTMIDI_BAD_DEVICE_ID;
    PmError err = pmNoError;
    if (is_nullptr(midi))
        err = pmBadPtr;
    else
    {
        deviceid = midi->device_id;
        if (! pm_descriptors[deviceid].pub.output)
            err = pmErrOther;
        else if (! pm_descriptors[deviceid].pub.opened)
            err = pmDeviceClosed;
        else
            midi->first_message = TRUE;
    }

    return err;
}

/**
 *  Pm_Abort() terminates outgoing messages immediately The caller should
 *  immediately close the output port; this call may result in transmission of
 *  a partial midi message.  There is no abort for Midi input because the user
 *  can simply ignore messages in the buffer and close an input device at any
 *  time.
 */

PMEXPORT PmError
Pm_Abort (PortMidiStream * stream)
{
    PmInternal * midi = (PmInternal *) stream;
    int deviceid = PORTMIDI_BAD_DEVICE_ID;
    PmError err = pmNoError;
    if (is_nullptr(midi))
        err = pmBadPtr;
    else
    {
        deviceid = midi->device_id;
        if (! pm_descriptors[deviceid].pub.output)
            err = pmErrOther;
        else if (! pm_descriptors[deviceid].pub.opened)
            err = pmDeviceClosed;
        else
            err = (*midi->dictionary->abort)(midi);
    }

    if (err == pmHostError)
    {
        midi->dictionary->host_error
        (
            midi, pm_hosterror_text, PM_HOST_ERROR_MSG_LEN
        );
        pm_hosterror = TRUE;
    }
    return pm_errmsg(err, deviceid);
}

/**
 *  pm_channel_filtered returns non-zero if the channel mask is blocking the
 *  current channel.
 */

#define pm_channel_filtered(status, mask) \
    ((((status) & 0xF0) != 0xF0) && (!(Pm_Channel((status) & 0x0F) & (mask))))

/*
 * The following two functions will checks to see if a MIDI message matches
 * the filtering criteria.  Since the SysEx routines only want to filter
 * realtime messages, we need to have separate routines.
 */

/**
 *  pm_realtime_filtered returns non-zero if the filter will kill the current
 *  message.  Note that only realtime messages are checked here.
 */

#define pm_realtime_filtered(status, filters) \
    ((((status) & 0xF0) == 0xF0) && ((1 << ((status) & 0xF)) & (filters)))

/*
 *  return ((status == MIDI_ACTIVE) && (filters & PM_FILT_ACTIVE))
 *     ||  ((status == MIDI_CLOCK) && (filters & PM_FILT_CLOCK))
 *     ||  ((status == MIDI_START) && (filters & PM_FILT_PLAY))
 *     ||  ((status == MIDI_STOP) && (filters & PM_FILT_PLAY))
 *     ||  ((status == MIDI_CONTINUE) && (filters & PM_FILT_PLAY))
 *     ||  ((status == MIDI_F9) && (filters & PM_FILT_F9))
 *     ||  ((status == MIDI_FD) && (filters & PM_FILT_FD))
 *     ||  ((status == MIDI_RESET) && (filters & PM_FILT_RESET))
 *     ||  ((status == MIDI_MTC) && (filters & PM_FILT_MTC))
 *     ||  ((status == MIDI_SONGPOS) && (filters & PM_FILT_SONG_POSITION))
 *     ||  ((status == MIDI_SONGSEL) && (filters & PM_FILT_SONG_SELECT))
 *     ||  ((status == MIDI_TUNE) && (filters & PM_FILT_TUNE));
 */

/*
 * pm_status_filtered returns non-zero if a filter will kill the current
 * message, based on status.  Note that SysEx and real time are not checked.
 * It is up to the subsystem (winmm, core midi, alsa) to filter SysEx, as it
 * is handled more easily and efficiently at that level.  Realtime message are
 * filtered in pm_realtime_filtered.
 */

#define pm_status_filtered(status, filters) \
    ((1 << (16 + ((status) >> 4))) & (filters))

/*
 *  return  ((status == MIDI_NOTE_ON) && (filters & PM_FILT_NOTE))
 *     ||  ((status == MIDI_NOTE_OFF) && (filters & PM_FILT_NOTE))
 *     ||  ((status == MIDI_CHANNEL_AT) && (filters & PM_FILT_CHANNEL_AFTERTOUCH))
 *     ||  ((status == MIDI_POLY_AT) && (filters & PM_FILT_POLY_AFTERTOUCH))
 *     ||  ((status == MIDI_PROGRAM) && (filters & PM_FILT_PROGRAM))
 *     ||  ((status == MIDI_CONTROL) && (filters & PM_FILT_CONTROL))
 *     ||  ((status == MIDI_PITCHBEND) && (filters & PM_FILT_PITCHBEND));
 */

/**
 *
 */

static void
pm_flush_sysex (PmInternal * midi, PmTimestamp timestamp)
{
    PmEvent event;

    /* there may be nothing in the buffer */

    if (midi->sysex_message_count == 0)
        return;                                 /* nothing to flush */

    event.message = midi->sysex_message;
    event.timestamp = timestamp;

    /* copied from pm_read_short, avoids filtering */

    if (Pm_Enqueue(midi->queue, &event) == pmBufferOverflow)
    {
        midi->sysex_in_progress = FALSE;
    }
    midi->sysex_message_count = 0;
    midi->sysex_message = 0;
}


/**
 *  pm_read_short() and pm_read_bytes() are the interface between
 *  system-dependent MIDI input handlers and the system-independent PortMIDI
 *  code.  The input handler MUST obey these rules:
 *
 *  -# All short input messages must be sent to pm_read_short(), which
 *     enqueues them to a FIFO for the application.
 *  -# Each buffer of sysex bytes should be reported by calling
 *     pm_read_bytes() (which sets midi->sysex_in_progress). After the EOX
 *     byte, pm_read_bytes() will clear sysex_in_progress
 */

/**
 *  pm_read_short() is the place where all input messages arrive from
 *  system-dependent code such as pmwinmm.c. Here, the messages are entered
 *  into the PortMidi input buffer.
 */

void
pm_read_short (PmInternal * midi, PmEvent * event)
{
    int status;
    assert(midi != NULL);

    /* MIDI filtering is applied here */

    status = Pm_MessageStatus(event->message);
    if
    (
        ! pm_status_filtered(status, midi->filters) &&
        (!is_real_time(status) || ! pm_realtime_filtered(status, midi->filters))
        && !pm_channel_filtered(status, midi->channel_mask)
    )
    {
        /*
         * if SysEx is in progress and we get a status byte, it had better be
         * a realtime message or the starting SYSEX byte; otherwise, we exit
         * the sysex_in_progress state
         */

        if (midi->sysex_in_progress && (status & MIDI_STATUS_MASK))
        {
            /*
             * Two choices: real-time or not. If it's real-time, then this
             * should be delivered as a SysEx byte because it is embedded in a
             * SysEx message.
             */

            if (is_real_time(status))
            {
                midi->sysex_message |=
                        (status << (8 * midi->sysex_message_count++));

                if (midi->sysex_message_count == 4)
                    pm_flush_sysex(midi, event->timestamp);
            }
            else
            {
                /*
                 * Otherwise, it's not real-time. This interrupts a SysEx
                 * message in progress
                 */

                midi->sysex_in_progress = FALSE;
            }
        }
        else if (Pm_Enqueue(midi->queue, event) == pmBufferOverflow)
            midi->sysex_in_progress = FALSE;
    }
}

/**
 *  Reads one (partial) SysEx msg from MIDI data.
 *
 * \return
 *      Returns how many bytes were processed.
 */

unsigned
pm_read_bytes
(
    PmInternal * midi, const midibyte_t * data,
    int len, PmTimestamp timestamp
)
{
    int i = 0;              /* index into data, must not be unsigned (!) */
    PmEvent event;
    event.timestamp = timestamp;
    assert(midi);

    /*
     * Note that since buffers may not have multiples of 4 bytes,
     * pm_read_bytes may be called in the middle of an outgoing 4-byte
     * PortMidi message. sysex_in_progress indicates that a SysEx has been
     * sent but no eox.
     */

    if (len == 0)
        return 0;                                   /* sanity check         */
    if (! midi->sysex_in_progress)
    {
        while (i < len)                             /* process all data     */
        {

            midibyte_t byte = data[i++];
            if (byte == MIDI_SYSEX && !pm_realtime_filtered(byte, midi->filters))
            {
                midi->sysex_in_progress = TRUE;
                i--;            /* back up so code below gets SYSEX byte    */
                break;          /* continue looping below to process msg    */
            }
            else if (byte == MIDI_EOX)
            {
                midi->sysex_in_progress = FALSE;
                return i;       /* done with one message                    */
            }
            else if (byte & MIDI_STATUS_MASK)
            {
                /*
                 * We're getting MIDI but no sysex in progress.  Either the
                 * SYSEX status byte was dropped or the message was filtered.
                 * Drop the data, but send any embedded realtime bytes.
                 * Assume that this is a real-time message: it is an error to
                 * pass non-real-time messages to pm_read_bytes
                 */

                event.message = byte;
                pm_read_short(midi, &event);
            }
        }                   /* all bytes in the buffer are processed */
    }

    /*
     * Now, i<len implies sysex_in_progress. If sysex_in_progress
     * becomes false in the loop, there must have been an overflow
     * and we can just drop all remaining bytes
     */

    while (i < len && midi->sysex_in_progress)
    {
        if
        (
            midi->sysex_message_count == 0 && i <= len - 4 &&
            (
                (event.message = (((PmMessage) data[i]) |
                             (((PmMessage) data[i+1]) << 8) |
                             (((PmMessage) data[i+2]) << 16) |
                             (((PmMessage) data[i+3]) << 24))) &
             0x80808080) == 0
        )
        {                               /* all data, no status */
            if (Pm_Enqueue(midi->queue, &event) == pmBufferOverflow)
                midi->sysex_in_progress = FALSE;

            i += 4;
        }
        else
        {
            while (i < len)
            {
                midibyte_t byte = data[i++];    /* send one byte at a time */
                if
                (
                    is_real_time(byte) &&
                    pm_realtime_filtered(byte, midi->filters)
                )
                {
                    continue;           /* real-time data is filtered; omit */
                }
                midi->sysex_message |=
                    (byte << (8 * midi->sysex_message_count++));

                if (byte == MIDI_EOX)
                {
                    midi->sysex_in_progress = FALSE;
                    pm_flush_sysex(midi, event.timestamp);
                    return i;
                }
                else if (midi->sysex_message_count == 4)
                {
                    pm_flush_sysex(midi, event.timestamp);

                    /*
                     * After handling at least one non-data byte and reaching
                     * a 4-byte message boundary, resume trying to send 4 at a
                     * time in the outer loop.
                     */

                    break;
                }
            }
        }
    }
    return i;
}

/*
 * Ad hoc safe C accessors.
 */

/**
 *  Returns true if the given device number is valid and the device is opened.
 *
 * \param deviceid
 *      The device to be tested, ranging from 0 to less than
 *      pm_descriptor_index.
 *
 * \return
 *      Returns the opened status of a valid device.
 */

int
Pm_device_opened (int deviceid)
{
    int result = not_nullptr(pm_descriptors);
    if (result)
    {
        result = pm_descriptor_index >= 0 && deviceid < pm_descriptor_index;
        if (result)
            result = pm_descriptors[deviceid].pub.opened ? TRUE : FALSE ;
    }
    return result;
}

/**
 *  The official accessor of pm_descriptor_index.  Unlike Pm_CountDevices(),
 *  this function does not call Pm_Initialize() first.
 */

int
Pm_device_count (void)
{
    return pm_descriptor_index;
}

/**
 *
 */

void
Pm_print_devices (void)
{
    int dev;
    for (dev = 0; dev < pm_descriptor_index; ++dev)
    {
        int status = Pm_device_opened(dev);
        const PmDeviceInfo * dev_info = Pm_GetDeviceInfo(dev);
        const char * io = dev_info->output == 1 ? "output" : "unknown" ;
        const char * opstat = status == 1 ? "opened" : "closed" ;
        if (dev_info->input == 1)
            io = "input";

        printf
        (
            "PortMidi %s %d: %s %s %s\n",
            dev_info->interf, dev, dev_info->name, io, opstat
        );
    }
}

/*
 * portmidi.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */

