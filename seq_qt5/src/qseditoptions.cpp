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
 * \file          qseditoptions.cpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-03-03
 * \license       GNU GPLv2 or above
 *
 */

#include "perform.hpp"
#include "qseditoptions.hpp"
#include "forms/qseditoptions.ui.h"
#include "settings.hpp"                 /* seq64::rc() and seq64::usr()     */

/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace seq64
{

/**
 *
 */

qseditoptions::qseditoptions (perform & p, QWidget * parent)
 :
    QDialog             (parent),
    ui                  (new Ui::qseditoptions),
    mPerf               (p),
    backupJackTransport (false),
    backupTimeMaster    (false),
    backupMasterCond    (false),
    backupNoteResume    (false),
    backupKeyHeight     (0)
{
    ui->setupUi(this);
    backup();
    syncWithInternals();
    connect
    (
        ui->btnJackConnect, SIGNAL(clicked(bool)),
        this, SLOT(jackConnect())
    );
    connect
    (
        ui->btnJackDisconnect, SIGNAL(clicked(bool)),
        this, SLOT(jackDisconnect())
    );
    connect
    (
        ui->chkJackTransport, SIGNAL(stateChanged(int)),
        this, SLOT(updateTransportSupport())
    );
    connect
    (
        ui->chkJackConditional, SIGNAL(stateChanged(int)),
        this, SLOT(updateMasterCond())
    );
    connect
    (
        ui->chkJackMaster, SIGNAL(stateChanged(int)),
        this, SLOT(updateTimeMaster())
    );
    connect
    (
        ui->buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked(bool)),
        this, SLOT(okay())
    );
    connect
    (
        ui->buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked(bool)),
        this, SLOT(cancel())
    );
    connect
    (
        ui->chkNoteResume, SIGNAL(stateChanged(int)),
        this, SLOT(updateNoteResume())
    );
    connect
    (
        ui->spinKeyHeight, SIGNAL(valueChanged(int)),
        this, SLOT(updateKeyHeight())
    );
}

/**
 *  Why is this not virtual?
 */

qseditoptions::~qseditoptions ()
{
    delete ui;
}

/**
 *  \todo
 *      We already have a better facility.

void
qseditoptions::addRecentFile(QString path)
{
    // start shifting from the last element if file not already recent

    int path_found_index = 9;

    // check if path is already present

    for (int c = 0; c < 10; c++)
    {
        if (recent_files[c] == path)
            path_found_index = c;
    }

    // shift the recent files along by one, thus dropping the oldest
    for (int c = path_found_index; c > 0; c--)
        recent_files[c] = recent_files[c - 1];

    recent_files[0] = path; // add the new path to the first slot
}
 */

/**
 *
 */

void
qseditoptions::jackConnect ()
{
    ///// perf().init_jack();
    perf().set_jack_mode(true);
}

/**
 *
 */

void
qseditoptions::jackDisconnect ()
{
    ///// perf().deinit_jack();
    perf().set_jack_mode(false);
}

/**
 *
 */

void
qseditoptions::updateMasterCond ()
{
    rc().with_jack_master_cond(ui->chkJackConditional->isChecked());
    syncWithInternals();
}

/**
 *
 */

void
qseditoptions::updateTimeMaster ()
{
    rc().with_jack_master(ui->chkJackMaster->isChecked());
    syncWithInternals();
}

/**
 *
 */

void
qseditoptions::updateTransportSupport ()
{
    rc().with_jack_transport(ui->chkJackTransport->isChecked());
    syncWithInternals();
}

/**
 * restore settings from the backup
 */

void
qseditoptions::cancel ()
{
    rc().with_jack_transport(backupJackTransport);
    rc().with_jack_master_cond(backupMasterCond);
    rc().with_jack_master(backupTimeMaster);
    //////// perf().setEditorKeyHeight(backupKeyHeight);
    perf().resume_note_ons(backupNoteResume);
    syncWithInternals();
    close();
}

/**
 * sync with preferences
 */

void
qseditoptions::syncWithInternals ()
{
    ui->chkJackTransport->setChecked(rc().with_jack_transport());
    ui->chkJackMaster->setChecked(rc().with_jack_master());
    ui->chkJackConditional->setChecked(rc().with_jack_master_cond());
    if (! rc().with_jack_transport())
    {
        //these options are meaningless if jack transport is disabled
        ui->chkJackMaster->setDisabled(true);
        ui->chkJackConditional->setDisabled(true);
    }
    else
    {
        ui->chkJackMaster->setDisabled(false);
        ui->chkJackConditional->setDisabled(false);
    }

    ui->chkNoteResume->setChecked(perf().resume_note_ons());
    ///// ui->spinKeyHeight->setValue(perf().getEditorKeyHeight());
}

/**
 * backup settings incase user cancels
 *
 */

void
qseditoptions::backup ()
{
    backupJackTransport = rc().with_jack_transport();
    backupMasterCond = rc().with_jack_master_cond();
    backupTimeMaster = rc().with_jack_master();
    ///// backupKeyHeight = perf().getEditorKeyHeight();
    backupNoteResume = perf().resume_note_ons();
}

/**
 *
 */

void
qseditoptions::okay ()
{
    backup();
    close();
}

/**
 *
 */

void
qseditoptions::updateNoteResume ()
{
    perf().resume_note_ons(ui->chkNoteResume->isChecked());
    syncWithInternals();
}

/**
 *
 */

void
qseditoptions::updateKeyHeight ()
{
    ////// perf().setEditorKeyHeight(ui->spinKeyHeight->value());
    syncWithInternals();
}

}           // namespace seq64

/*
 * qseditoptions.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
