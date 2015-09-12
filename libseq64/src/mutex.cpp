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
 * \file          mutex.cpp
 *
 *  This module declares/defines the base class for mutexes.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-09-11
 * \license       GNU GPLv2 or above
 *
 */

#include "mutex.hpp"

/*
 *  Define the static enabler for the locking mutex.  This does not solve
 *  the problem of the long MIDI-file parsing.
 *
 *      bool mutex::sm_mutex_enabled = true;
 */

/**
 *  Define the static recursive mutex and its condition variable.
 */

const pthread_mutex_t mutex::sm_recursive_mutex =
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/**
 *  Define the static condition variable used by all mutex locks.
 */

const pthread_cond_t condition_var::cond = PTHREAD_COND_INITIALIZER;

/**
 *  The constructor assigns the recursive mutex to the local locking
 *  mutex.
 */

mutex::mutex ()
 :
    m_mutex_lock    (sm_recursive_mutex)
{
    // empty body
}

/**
 *  Lock the mutex.
 */

void
mutex::lock () const
{
    pthread_mutex_lock(&m_mutex_lock);
}

/**
 *  Unlock the mutex.
 */

void
mutex::unlock () const
{
    pthread_mutex_unlock(&m_mutex_lock);
}

/**
 *  Initialize the condition variable with the global variable.
 */

condition_var::condition_var ()
 :
    m_cond  (cond)
{
    // Empty body
}

/**
 *  Signals the confition variable.
 */

void
condition_var::signal ()
{
    pthread_cond_signal(&m_cond);
}

/**
 *  Waits for the confition variable.
 */

void
condition_var::wait ()
{
    pthread_cond_wait(&m_cond, &m_mutex_lock);
}

/*
 * mutex.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */
