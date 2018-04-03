/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          lash.cpp
 *
 *  This module declares/defines the base class for LASH support.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-10-23
 * \license       GNU GPLv2 or above
 *
 *  Not totally sure that the LASH support is completely finished, at this
 *  time.  The version that ships with Debian Sid does not have it
 *  enabled.
 */

#include <string>

#include "lash.hpp"
#include "midifile.hpp"
#include "perform.hpp"
#include "settings.hpp"                 /* rc() and usr() functions     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{

/**
 *  The global pointer to the LASH driver instance.  It is actually hidden in
 *  this module now, so that a function can be used in its place.
 *
 *  Like the font renderer, this item was once created in the main module,
 *  sequencer64.cpp.  Now we make it a safer, more fool-proof, function.
 *  However, unlike the font-render, which always exists, the LASH driver is
 *  conditional, and might not be wanted.  Therefore, we cannot return a
 *  reference, because there's no such thing as a null reference in C++.  We
 *  have to return a pointer.
 */

static lash * s_global_lash_driver = nullptr;

/**
 *  Creates and starts a lash object.  Initializes the lash driver (strips
 *  lash-specific command line arguments), then connects to the LASH daemon
 *  and polls events.
 *
 *  This function will always be called from the main routine, and called only
 *  once.  Note that we don't need that darn SEQ64_LASH_SUPPORT macro in
 *  client code anymore.
 *
 * \param p
 *      The perform object that needs to implement LASH support.
 *
 * \param argc
 *      The number of command-line arguments.
 *
 * \param argv
 *      The command-line arguments.
 *
 * \return
 *      This function returns true if a lash object was created.  This
 *      function will not create one if not configured to, if the command-line
 *      options did not specify the creation of the LASH driver, or if the
 *      LASH driver was already created.
 */

bool
create_lash_driver (perform & p, int argc, char ** argv)
{
#ifdef SEQ64_LASH_SUPPORT
    bool result = is_nullptr(s_global_lash_driver);
    if (result)
    {
        result = rc().lash_support();
        if (result)
        {
            s_global_lash_driver = new lash(p, argc, argv);
            result = not_nullptr(s_global_lash_driver);
            if (result)
                s_global_lash_driver->start();
        }
    }
    return result;
#else
    return false;
#endif
}

/**
 *  Provides access to the lash object.
 *
 * \return
 *      Returns the pointer to the LASH driver if it exists.  Otherwise a null
 *      pointer is returned.  The caller <i> must always check </i> the return
 *      value.
 */

lash *
lash_driver ()
{
    return s_global_lash_driver;
}

/**
 *  Deletes the last object.  This function will always be called from the
 *  main routine, once.  The other lash-pointer functions will know if the
 *  pointer has been deleted.
 */

void
delete_lash_driver ()
{
    if (not_nullptr(s_global_lash_driver))
    {
        delete s_global_lash_driver;
        s_global_lash_driver = nullptr;
    }
}

/**
 *  This constructor calls lash_extract(), using the command-line
 *  arguments, if SEQ64_LASH_SUPPORT is enabled.  We fixed the crazy usage of
 *  argc and argv here and in the client code in the seq24 module.
 *
 * \param p
 *      The perform object that needs to implement LASH support.
 *
 * \param argc
 *      The number of command-line arguments.
 *
 * \param argv
 *      The command-line arguments.
 */

lash::lash (perform & p, int argc, char ** argv)
 :
    m_perform           (p),
#ifdef SEQ64_LASH_SUPPORT
    m_client            (nullptr),
    m_lash_args         (nullptr),
    m_is_lash_supported (true)
#else
    m_is_lash_supported (false)
#endif
{
#ifdef SEQ64_LASH_SUPPORT
    m_lash_args = lash_extract_args(&argc, &argv);
#endif
}

#ifdef SEQ64_LASH_SUPPORT

/**
 *  Initializes LASH support, if enabled.
 *
 * \return
 *      Returns true if the LASH subsystem was able to be initialized, and a
 *      LASH client representative (m_client) was allocated.
 */

bool
lash::init ()
{
    m_client = lash_init
    (
        m_lash_args, SEQ64_PACKAGE_NAME, LASH_Config_File, LASH_PROTOCOL(2, 0)
    );
    bool result = (m_client != NULL);
    if (result)
    {
        lash_event_t * event = lash_event_new_with_type(LASH_Client_Name);
        if (not_nullptr(event))
        {
            // lash_event_set_string(event, "Seq24");

            lash_event_set_string(event, "Sequencer64");
            lash_send_event(m_client, event);
            printf("[Connected to LASH]\n");
        }
        else
            fprintf(stderr, "Cannot communicate events with LASH.\n");
    }
    else
        fprintf(stderr, "Cannot connect to LASH; no session management.\n");

    return result;
}

#endif // SEQ64_LASH_SUPPORT

/**
 *  Make ourselves a LASH ALSA client.
 *
 * /param id
 *      The ALSA client ID to be set.
 */

void
lash::set_alsa_client_id (int id)
{
#ifdef SEQ64_LASH_SUPPORT
    lash_alsa_client_id(m_client, id);
#endif
}

/**
 *  Process any LASH events every 250 msec, which is an arbitrarily chosen
 *  interval.
 */

void
lash::start ()
{
#ifdef SEQ64_LASH_SUPPORT
    if (init())
    {
        /*
         * Let the perform object carry the GUI burden.
         *
         *  Glib::signal_timeout().connect
         *  (
         *      sigc::mem_fun(*this, &lash::process_events), 250
         *  );
         */

        m_perform.gui().lash_timeout_connect(this);
    }
#endif
}

#ifdef SEQ64_LASH_SUPPORT

/**
 *  Process LASH events.
 *
 * \return
 *      Always returns true.
 */

bool
lash::process_events ()
{
    lash_event_t * ev = nullptr;
    while (not_nullptr(ev = lash_get_event(m_client)))  /* process events */
    {
        handle_event(ev);
        lash_event_destroy(ev);
    }
    return true;
}

/**
 *  Handle a LASH event.
 *
 * \param ev
 *      Provides the event to be handled.
 */

void
lash::handle_event (lash_event_t * ev)
{
    LASH_Event_Type type = lash_event_get_type(ev);
    const char * cstring = lash_event_get_string(ev);
    std::string str = (cstring == nullptr) ? "" : cstring;
    if (not_nullptr(cstring))
    {
        str = cstring;
        str += "/sequencer64.midi";
    }
    else
        str = "~/sequencer64.midi";

    if (type == LASH_Save_File)
    {
        /*
         * Issue #49 made us discover that this call was out-of-date and thus
         * missing the PPQN parameter.
         */

        midifile f
        (
            str, m_perform.ppqn(), rc().legacy_format(),
            usr().global_seq_feature()
        );
        f.write(m_perform);
        lash_send_event(m_client, lash_event_new_with_type(LASH_Save_File));
    }
    else if (type == LASH_Restore_File)
    {
        midifile f(str);                /* flags don't apply to reading */
        f.parse(m_perform, 0);
        lash_send_event(m_client, lash_event_new_with_type(LASH_Restore_File));
    }
    else if (type == LASH_Quit)
    {
        m_client = NULL;
        m_perform.gui().quit();         /* Gtk::Main::quit();           */
    }
    else
    {
        errprint("Warning: Unhandled LASH event.");
    }
}

/*
 * Eliminate this annoying warning.
 */

#ifdef PLATFORM_GNU
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

/**
 *  Handle a LASH configuration item.
 *  Currently incomplete.
 *
 * \param conf
 *      Provides the configuration item to handle.
 */

void
lash::handle_config (lash_config_t * conf)
{
    const char * key = lash_config_get_key(conf);
    const void * val = lash_config_get_value(conf);
    size_t val_size = lash_config_get_value_size(conf);

    /*
     * Nothing is done with these, just gives warnings about "unused"
     */
}

#endif      // SEQ64_LASH_SUPPORT

}           // namespace seq64

/*
 * lash.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

