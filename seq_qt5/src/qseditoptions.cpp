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
 * \updates       2018-02-19
 * \license       GNU GPLv2 or above
 *
 */

#include "perform.hpp"
#include "qseditoptions.hpp"
#include "forms/qseditoptions.ui.h"

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
    mPerf               (p)
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
 */

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

/**
 *
 */

void
qseditoptions::jackConnect ()
{
    mPerf->init_jack();
}

/**
 *
 */

void
qseditoptions::jackDisconnect ()
{
    mPerf->deinit_jack();
}

/**
 *
 */

void
qseditoptions::updateMasterCond ()
{
    global_with_jack_master_cond = ui->chkJackConditional->isChecked();
    syncWithInternals();
}

/**
 *
 */

void
qseditoptions::updateTimeMaster ()
{
    global_with_jack_master = ui->chkJackMaster->isChecked();
    syncWithInternals();
}

/**
 *
 */

void
qseditoptions::updateTransportSupport ()
{
    global_with_jack_transport = ui->chkJackTransport->isChecked();
    syncWithInternals();
}

/**
 * restore settings from the backup
 */

void
qseditoptions::cancel ()
{
    global_with_jack_transport = backupJackTransport;
    global_with_jack_master_cond = backupMasterCond;
    global_with_jack_master = backupTimeMaster;
    mPerf->setEditorKeyHeight(backupKeyHeight);
    mPerf->setResumeNoteOns(backupNoteResume);
    syncWithInternals();
    close();
}

/**
 * sync with preferences
 */

void
qseditoptions::syncWithInternals ()
{
    ui->chkJackTransport->setChecked(global_with_jack_transport);
    ui->chkJackMaster->setChecked(global_with_jack_master);
    ui->chkJackConditional->setChecked(global_with_jack_master_cond);
    if (! global_with_jack_transport)
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

    ui->chkNoteResume->setChecked(mPerf->getResumeNoteOns());
    ui->spinKeyHeight->setValue(mPerf->getEditorKeyHeight());
}

/**
 * backup settings incase user cancels
 *
 */

void
qseditoptions::backup ()
{
    backupJackTransport = global_with_jack_transport;
    backupMasterCond = global_with_jack_master_cond;
    backupTimeMaster = global_with_jack_master;
    backupKeyHeight = mPerf->getEditorKeyHeight();
    backupNoteResume = mPerf->getResumeNoteOns();
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
    mPerf->setResumeNoteOns(ui->chkNoteResume->isChecked());
    qDebug() << "Note resume status" << mPerf->getResumeNoteOns() << endl;
    syncWithInternals();
}

/**
 *
 */

void
qseditoptions::updateKeyHeight ()
{
    mPerf->setEditorKeyHeight(ui->spinKeyHeight->value());
    syncWithInternals();
}

/*
 * qseditoptions.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

