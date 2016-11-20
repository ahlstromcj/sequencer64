#ifndef SEQ64_RTERROR_HPP
#define SEQ64_RTERROR_HPP

/**
 * \file          rterror.hpp
 *
 *  An abstract base class for MIDI error handling.
 *
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2016-11-19
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *    In this refactoring...
 */

#include <exception>
#include <string>

#include "easy_macros.h"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  Exception handling class for rtexmidi.
 *
 *  The rterror class is quite simple but it does allow errors to be
 *  "caught" by rterror::Type. See the rtexmidi documentation to know
 *  which methods can throw an rterror.
 *
 *  Please note that, in this refactoring of rtmidi, we've done away with all
 *  the exception specifications, on the advice of Herb Sutter.  They may be
 *  more relevent to C++11 and beyond, but this library is too small to worry
 *  about them, for now.
 */

class rterror : public std::exception
{

public:

    enum Type
    {
        WARNING,           /**< A non-critical error.                       */
        DEBUG_WARNING,     /**< Non-critical error useful for debugging.    */
        UNSPECIFIED,       /**< The default, unspecified error type.        */
        NO_DEVICES_FOUND,  /**< No devices found on system.                 */
        INVALID_DEVICE,    /**< An invalid device ID was specified.         */
        MEMORY_ERROR,      /**< An error occured during memory allocation.  */
        INVALID_PARAMETER, /**< Invalid parameter specified to a function.  */
        INVALID_USE,       /**< The function was called incorrectly.        */
        DRIVER_ERROR,      /**< A system driver error occured.              */
        SYSTEM_ERROR,      /**< A system error occured.                     */
        THREAD_ERROR       /**< A thread error occured.                     */
    };

private:

    /**
     *  Holds the latest message information for the exception.
     */

    std::string m_message;

    /**
     *  Holds the type or severity of the exception.
     */

    Type m_type;

public:

    rterror
    (
        const std::string & message,
        Type type = rterror::UNSPECIFIED
    ) :
        m_message(message),
        m_type(type)
    {
        // no code
    }

    virtual ~rterror ()
    {
        // no code
    }

    /**
     *  Prints thrown error message to stderr.
     */

    virtual void printMessage () const
    {
       infoprint(m_message.c_str());
    }

    /**
     *  Returns the thrown error message type.
     */

    virtual const Type & getType () const
    {
        return m_type;
    }

    /**
     *  Returns the thrown error message string.
     */

    virtual const std::string & get_message () const
    {
        return m_message;
    }

    /**
     *  Returns the thrown error message as a c-style string.
     */

    virtual const char * what () const noexcept
    {
        return m_message.c_str();
    }

};

/**
 *  rtmidi error callback function prototype.
 *
 *  Note that class behaviour is undefined after a critical error (not
 *  a warning) is reported.
 *
 * \param type
 *      Type of error.
 *
 * \param errorText
 *      Error description.
 */

typedef void (* rterror_callback)
(
    rterror::Type type,
    const std::string & errormsg,
    void * userdata
);

}           // namespace seq64

#endif      // SEQ64_RTEXERROR_HPP

/*
 * rterror.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

