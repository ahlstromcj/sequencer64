#ifndef SEQ64_MIDI_INFO_HPP
#define SEQ64_MIDI_INFO_HPP

/**
 * \file          midi_info.hpp
 *
 *      A class for holding the current status of the MIDI system on the host.
 *
 * \library       sequencer64 application
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-12-05
 * \updates       2017-03-21
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *      We need to have a way to get all of the API information from each
 *      framework, without supporting the full API.  The Sequencer64
 *      masteridibus and midibus classes require certain information to be
 *      known when they are created:
 *
 *      -   Port counts.  The number of input ports and output ports needs to
 *          be known so that we can iterate properly over them to create
 *          midibus objects.
 *      -   Port information.  We want to assemble port names just once, and
 *          never have to deal with it again (assuming that MIDI ports do not
 *          come and go during the execution of Sequencer64.
 *      -   Client information.  We want to assemble client names or numbers
 *          just once.
 *
 *      Note that, while the other midi_api-based classes access port via the
 *      port numbers assigned by the MIDI subsystem, midi_info-based classes
 *      use the concept of an "index", which ranges from 0 to one less than
 *      the number of input or output ports.  These values are indices into a
 *      vector of port_info_t structures, and are easily looked up when
 *      mastermidibus creates a midibus object.
 *
 *      An alternate name for this class could be "midi_master".  :-)
 */

#include "app_limits.h"                 /* SEQ64_DEFAULT_PPQN etc.  */
#include "easy_macros.h"
#include "rterror.hpp"
#include "rtmidi_types.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq64
{
    class event;
    class mastermidibus;
    class midibus;

/**
 *  A class for holding port information.
 */

class midi_port_info
{

private:

    /**
     *  Hold the information for a single port.  Except for the
     *  virtual-vs-normal status, this information is obtained by scanning
     *  the system at the startup time of the application.
     */

    typedef struct
    {
        int m_client_number;        /**< The major buss number of the port.  */
        std::string m_client_name;  /**< The system's name for the client.   */
        int m_port_number;          /**< The minor port number of the port.  */
        std::string m_port_name;    /**< The system's name for the port.     */
        int m_queue_number;         /**< A number used in some APIs.         */
        bool m_is_input;            /**< Indicates an input port.            */
        bool m_is_virtual;          /**< Indicates an manual/virtual port.   */
        bool m_is_system;           /**< Built-in port, almost always false. */

    } port_info_t;

    /**
     *  Holds the number of ports counted.
     */

    int m_port_count;

    /**
     *  Holds information on all of the ports that were "scanned".
     */

    std::vector<port_info_t> m_port_container;

public:

    midi_port_info ();

    void add
    (
        int clientnumber,                   // buss number/ID
        const std::string & clientname,     // buss name
        int portnumber,
        const std::string & portname,
        bool makevirtual,                   // SEQ64_MIDI_VIRTUAL_PORT
        bool makesystem,                    // SEQ64_MIDI_SYSTEM_PORT
        bool makeinput,                     // SEQ64_MIDI_INPUT_PORT
        int queuenumber = SEQ64_BAD_QUEUE_ID
    );
    void add (const midibus * m);

    /**
     *  This function is useful in replacing the discovered system ports with
     *  the manual/virtual ports added in "manual" mode.
     */

    void clear ()
    {
        m_port_container.clear();
        m_port_count = 0;
    }

    int get_port_count () const
    {
        return m_port_count;
    }

    int get_bus_id (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_client_number;
        else
            return SEQ64_BAD_BUS_ID;
    }

    std::string get_bus_name (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_client_name;
        else
            return std::string("");
    }

    int get_port_id (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_port_number;
        else
            return SEQ64_BAD_PORT_ID;
    }

    std::string get_port_name (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_port_name;
        else
            return std::string("");
    }

    bool get_input (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_is_input;
        else
            return SEQ64_MIDI_OUTPUT_PORT;          /* i.e. false */
    }

    bool get_virtual (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_is_virtual;
        else
            return SEQ64_MIDI_NORMAL_PORT;          /* i.e. false */
    }

    bool get_system (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_is_system;
        else
            return false;
    }

    int get_queue_number (int index) const
    {
        if (index < get_port_count())
            return m_port_container[index].m_queue_number;
        else
            return SEQ64_BAD_QUEUE_ID;
    }

    /**
     *  Provides the bus name and port name in canonical JACK format:
     *  "busname:portname".  This function is basically the same as
     *  midibase::connect_name() function.  If either the bus name or port
     *  name are empty, then an empty string is returned.
     */

    std::string connect_name (int index) const
    {
        std::string result = get_bus_name(index);
        if (! result.empty())
        {
            std::string pname = get_port_name(index);
            if (! pname.empty())
            {
                result += ":";
                result += pname;
            }
        }
        return result;
    }

};          // class midi_port_info

/**
 *  The class for holding basic information on the MIDI input and output ports
 *  currently present in the system.
 */

class midi_info
{

    friend class rtmidi_info;

private:

    /**
     *  Indicates which mode we're in, input or output.  We have to pick the
     *  mode we need to be in with the set_mode() function before we do a
     *  series of operations.  This clumsy two-step is needed in order to
     *  preserve the midi_api interface.
     */

    bool m_midi_mode_input;

    /**
     *  Holds data on the ALSA/JACK/Core/WinMM inputs.
     */

    midi_port_info m_input;

    /**
     *  Holds data on the ALSA/JACK/Core/WinMM outputs.
     */

    midi_port_info m_output;

    /**
     *  Holds pointers to the ports that were created, so that, after
     *  activation, we can call the connect_port() function on those that are
     *  not virtual.  See the add_bus() and bus_container() member functions.
     */

    std::vector<midibus *> m_bus_container;

    /**
     *  The ID of the ALSA MIDI queue.
     */

    int m_global_queue;

    /**
     *  Provides a handle to the main ALSA or JACK implementation object.
     *  Created by the class derived from midi_info.
     */

    void * m_midi_handle;

    /**
     *  Holds this value for passing along, to reduce the number of arguments
     *  needed.  This value is the main application name as determined at
     *  ./configure time.
     */

    const std::string m_app_name;

    /**
     *  Hold this value for passing along to some ports that get created.
     *  Some APIs can use this value.
     */

    int m_ppqn;

    /**
     *  Hold this value for passing along to some ports that get created.
     *  Some APIs can use this value.
     */

    midibpm m_bpm;

protected:

    /**
     *  Error string for the midi_info interface.
     */

    std::string m_error_string;

public:

    midi_info                                   /* similar to mastermidibus */
    (
        const std::string & appname,
        int ppqn    = SEQ64_DEFAULT_PPQN,       /* 192  */
        midibpm bpm = SEQ64_DEFAULT_BPM         /* 120  */
    );

    virtual ~midi_info ()
    {
        // Empty body
    }

    /**
     * \getter m_midi_mode_input
     */

    bool midi_mode () const
    {
        return m_midi_mode_input;
    }

    /**
     * \setter m_midi_mode_input
     */

    void midi_mode (bool flag)
    {
        m_midi_mode_input = flag;
    }

    /**
     * \getter m_midi_handle
     */

    void * midi_handle ()
    {
        return m_midi_handle;
    }

    /**
     * \getter m_input
     */

    midi_port_info & input_ports ()
    {
        return m_input;
    }

    /**
     * \getter m_output
     */

    midi_port_info & output_ports ()
    {
        return m_output;
    }

    /**
     * \getter
     *      Total port count.
     */

    int full_port_count () const
    {
        return m_input.get_port_count() + m_output.get_port_count();
    }

    void clear ()
    {
        m_input.clear();
        m_output.clear();
    }

    /**
     * \getter m_app_name
     */

    const std::string & app_name () const
    {
        return m_app_name;
    }

    /**
     * \getter m_ppqn, simple version, also see api_set_ppqn().
     */

    int ppqn () const
    {
        return m_ppqn;
    }

    /**
     * \getter m_bpm, simple version, also see api_set_beats_per_minute().
     */

    midibpm bpm () const
    {
        return m_bpm;
    }

    /**
     *  Special setter.
     */

    virtual void api_set_ppqn (int p)
    {
        m_ppqn = p;
    }

    /**
     *  Special setter.
     */

    virtual void api_set_beats_per_minute (midibpm b)
    {
        m_bpm = b;
    }

    /**
     *  An ALSA-specific function at the moment.
     */

    virtual void api_port_start
    (
        mastermidibus & /* masterbus */,
        int /* bus */, int /* port */
    )
    {
        // Empty body
    }

    virtual bool api_get_midi_event (event * inev) = 0;
    virtual int api_poll_for_midi () = 0;       /* disposable??? */
    virtual void api_flush () = 0;

    /**
     *  Used only in the midi_jack_info class.
     */

    virtual bool api_connect ()
    {
        return true;
    }

    /**
     *
     */

    virtual int get_port_count () const
    {
        const midi_port_info & mpi = nc_midi_port_info();
        return mpi.get_port_count();
    }

    virtual int get_bus_id (int index) const
    {
        const midi_port_info & mpi = nc_midi_port_info();
        return mpi.get_bus_id(index);
    }

    virtual std::string get_bus_name (int index) const
    {
        const midi_port_info & mpi = nc_midi_port_info();
        return mpi.get_bus_name(index);
    }

    virtual int get_port_id (int index) const
    {
        const midi_port_info & mpi = nc_midi_port_info();
        return mpi.get_port_id(index);
    }

    virtual std::string get_port_name (int index) const
    {
        const midi_port_info & mpi = nc_midi_port_info();
        return mpi.get_port_name(index);
    }

    virtual bool get_input (int index) const
    {
        const midi_port_info & mpi = nc_midi_port_info();
        return mpi.get_input(index);
    }

    virtual bool get_virtual (int index) const
    {
        const midi_port_info & mpi = nc_midi_port_info();
        return mpi.get_virtual(index);
    }

    virtual bool get_system (int index) const
    {
        const midi_port_info & mpi = nc_midi_port_info();
        return mpi.get_system(index);
    }

    virtual int queue_number (int index) const
    {
        const midi_port_info & mpi = nc_midi_port_info();
        return mpi.get_queue_number(index);
    }

    std::string connect_name (int index) const
    {
        const midi_port_info & mpi = nc_midi_port_info();
        return mpi.connect_name(index);
    }

    std::string port_list () const;

    int global_queue () const
    {
        return m_global_queue;
    }

    /**
     *  A basic error reporting function for midi_info classes.
     */

    void error (rterror::Type type, const std::string & errorstring);

    virtual int get_all_port_info () = 0;

protected:

    /**
     *  Adds the midibus to a quick list of all ports for use in the
     *  api_connect() call in mastermidibus.  We could add the midibus
     *  pointer to the midi_port_info structure, but that information
     *  is strictly for representing data obtained by querying the system
     *  via the selected API.
     */

    void add_bus (const midibus * m)
    {
        if (not_nullptr(m))
            m_bus_container.push_back(const_cast<midibus *>(m));
    }

    /**
     * \setter m_global_queue
     */

    void global_queue (int q)
    {
        m_global_queue = q;
    }

    /**
     * \setter m_midi_handle
     */

    void midi_handle (void * h)
    {
        m_midi_handle = h;
    }

    /**
     * \getter m_bus_container
     */

    std::vector<midibus *> & bus_container ()
    {
        return m_bus_container;
    }

private:

    /**
     * \getter m_input or m_output
     *      Used for retrieving values from the input or output containers.
     *      The caller must insure the proper container by calling the
     *      midi_mode() function with the value of true
     *      (SEQ64_MIDI_INPUT_PORT) or false (SEQ64_MIDI_OUTPUT_PORT) first.
     *      Ugly stuff.  I hate it.
     */

    const midi_port_info & nc_midi_port_info () const
    {
        return m_midi_mode_input ? m_input : m_output ;
    }

    /**
     * \getter m_input or m_output
     */

    midi_port_info & ref_midi_port_info ()
    {
        return m_midi_mode_input ? m_input : m_output ;
    }

};          // midi_info

}           // namespace seq64

#endif      // SEQ64_MIDI_INFO_HPP

/*
 * midi_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

