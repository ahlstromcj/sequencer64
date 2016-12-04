/**
 * \file          rtmidi_types.cpp
 *
 *    Classes that use to be structs.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-01
 * \updates       2016-12-01
 * \license       See the r3exmidi.lic file.  Too big for a header file.
 *
 *
 */

#include "easy_macros.h"                /* errprintfunc() macro, etc.   */
#include "rtmidi_types.hpp"             /* seq64::rtmidi, etc.          */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

// #if DEFINE_MESSAGE_CONCATENATE_HERE

/**
 *  This function concatenates two C string pointers and returns them as
 *  a string message.  Note that we don't bother with error-checking the
 *  pointers.  You're on your own, Hoss.
 *
 * \param m1
 *      The first message, often a __func__ macro.
 *
 * \param m2
 *      The second message.
 *
 * \return
 *      Returns "m1: m2" as a standard C++ string.
 */

std::string
message_concatenate (const char * m1, const char * m2)
{
    std::string result(m1);
    result += ": ";
    result += m2;
    return result;
}

/**
 *  Common-code for console messages.  Adds markers and a newline.
 *
 * \param msg
 *      The message to print, sans the newline.
 *
 * \return
 *      Returns true.
 */

bool
info_message (const std::string & msg)
{
    std::string temp = "[";
    temp += msg;
    temp += "]\n";
    printf(temp.c_str());
    return true;
}

/**
 *  Common-code for error messages.  Adds markers, and sets m_jack_running to
 *  false.
 *
 * \param msg
 *      The message to print, sans the newline.
 *
 * \return
 *      Returns false for convenience/brevity in setting function return
 *      values.
 */

bool
error_message (const std::string & msg)
{
    (void) info_message(msg);
    return false;
}

// #endif

/*
 * class midi_queue
 */

/**
 *  Default constructor.
 */

midi_queue::midi_queue ()
 :
    m_front     (0),
    m_back      (0),
    m_size      (0),
    m_ring_size (0),
    m_ring      (nullptr)
{
    // Empty body
}

/**
 *
 *  This would be better off as a constructor operation.  But one step at a
 *  time.
 */

void
midi_queue::allocate (unsigned queuesize)
{
    if (queuesize > 0 && is_nullptr(m_ring))
    {
        m_ring_size = queuesize;
        m_ring = new (std::nothrow) midi_message[queuesize];
    }
}

/**
 *
 *  This would be better off as a destructor operation.  But one step at a
 *  time.
 */

void
midi_queue::deallocate ()
{
    if (not_nullptr(m_ring))
        delete [] m_ring;
}

/**
 *
 *  As long as we haven't reached our queue size limit, push the message.
 */

bool
midi_queue::add (const midi_message & mmsg)
{
    bool result = ! full();
    if (result)
    {
        m_ring[m_back++] = mmsg;
        if (m_back == m_ring_size)
            m_back = 0;

        ++m_size;
    }
    else
        errprintfunc("message queue limit reached");

    return result;
}

/**
 *
 */

void
midi_queue::pop ()
{
    --m_size;
    ++m_front;
    if (m_front == m_ring_size)
        m_front = 0;
}

/*
 * class rtmidi_in_data
 */

rtmidi_in_data::rtmidi_in_data ()
 :
    m_queue             (),
    m_message           (),
    m_ignore_flags      (7),
    m_do_input          (false),
    m_first_message     (true),
    m_api_data          (nullptr),
    m_using_callback    (false),
    m_user_callback     (nullptr),
    m_user_data         (nullptr),
    m_continue_sysex    (false)
{
    // no body
}

}           // namespace seq64

/*
 * rtmidi_types.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

