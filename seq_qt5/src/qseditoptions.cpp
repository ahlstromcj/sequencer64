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
 * \updates       2018-05-27
 * \license       GNU GPLv2 or above
 *
 */

#include "perform.hpp"
#include "qclocklayout.hpp"
#include "qinputcheckbox.hpp"
#include "qseditoptions.hpp"
#include "settings.hpp"                 /* seq64::rc() and seq64::usr()     */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#ifdef SEQ64_QMAKE_RULES
#include "forms/ui_qseditoptions.h"
#else
#include "forms/qseditoptions.ui.h"
#endif

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
    backupKeyHeight     (usr().key_height())
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
        ui->buttonBoxOptionsDialog->button(QDialogButtonBox::Ok),
        SIGNAL(clicked(bool)), this, SLOT(okay())
    );
    connect
    (
        ui->buttonBoxOptionsDialog->button(QDialogButtonBox::Cancel),
        SIGNAL(clicked(bool)), this, SLOT(cancel())
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

    /*
     * Set up the MIDI Clock tab.  We use the new qclocklayout class to hold
     * most of the complex code needed to handle the list of output ports and
     * clock radio-buttons.
     */

    QVBoxLayout * vboxclocks = new QVBoxLayout;
    int buses = perf().master_bus().get_num_out_buses();
    for (int bus = 0; bus < buses; ++bus)
    {
        qclocklayout * tempqc = new qclocklayout(this, perf(), bus);
        vboxclocks->addLayout(tempqc->layout());
    }

    QSpacerItem * spacer = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding
    );

    vboxclocks->addItem(spacer);
    ui->groupBoxClocks->setLayout(vboxclocks);

    /*
     * Set up the MIDI Input tab.  It is simpler, just a list of check-boxes
     * in the groupBoxInputs widget.  No need for a separate class.
     */

    QVBoxLayout * vboxinputs = new QVBoxLayout;
    buses = perf().master_bus().get_num_in_buses();
    for (int bus = 0; bus < buses; ++bus)
    {
        qinputcheckbox * tempqi = new qinputcheckbox(this, perf(), bus);
        vboxinputs->addWidget(tempqi->input_checkbox());
    }

    QSpacerItem * spacer2 = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding
    );
    vboxinputs->addItem(spacer2);
    ui->groupBoxInputs->setLayout(vboxinputs);
}

/**
 *  Why is this not virtual?
 */

qseditoptions::~qseditoptions ()
{
    delete ui;
}

/**
 *
 */

void
qseditoptions::jackConnect ()
{
    perf().set_jack_mode(true);             // perf().init_jack();
}

/**
 *
 */

void
qseditoptions::jackDisconnect ()
{
    perf().set_jack_mode(false);            // perf().deinit_jack();
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
 *  Backs up the current settings logged into the various settings object into
 *  the "backup" members, then calls close().
 */

void
qseditoptions::okay ()
{
    backup();
    close();
}

/**
 *  Restores the settings from the "backup" variables, then calls
 *  syncWithInternals()
 */

void
qseditoptions::cancel ()
{
    rc().with_jack_transport(backupJackTransport);
    rc().with_jack_master_cond(backupMasterCond);
    rc().with_jack_master(backupTimeMaster);
    usr().key_height(backupKeyHeight);
    perf().resume_note_ons(backupNoteResume);
    syncWithInternals();
    close();
}

/**
 *  Backs up the JACK, Time, Key-height, and Note-Resume settings in case the
 *  user cancels. In that case, the cancel() function will put these settings
 *  back into the various settings objects.
 */

void
qseditoptions::backup ()
{
    backupJackTransport = rc().with_jack_transport();
    backupMasterCond = rc().with_jack_master_cond();
    backupTimeMaster = rc().with_jack_master();
    backupKeyHeight = usr().key_height();
    backupNoteResume = perf().resume_note_ons();
}

/**
 *  Sync with preferences.  In other words, the current values in the various
 *  settings objects are used to set the user-interface elements in this
 *  dialog.
 */

void
qseditoptions::syncWithInternals ()
{
    ui->chkJackTransport->setChecked(rc().with_jack_transport());
    ui->chkJackMaster->setChecked(rc().with_jack_master());
    ui->chkJackConditional->setChecked(rc().with_jack_master_cond());

    /*
     * These JACK options are meaningless if JACK Transport is disabled.
     */

    ui->chkJackMaster->setDisabled(! rc().with_jack_transport());
    ui->chkJackConditional->setDisabled(! rc().with_jack_transport());

    ui->chkNoteResume->setChecked(perf().resume_note_ons());
    ui->spinKeyHeight->setValue(usr().key_height());
}

/**
 *  Updates the perform::result_note_ons() setting in accord with the
 *  user-interface, and then calls syncWithInternals(), perhaps needlessly, to
 *  make sure the user-interface items correctly represent the settings.
 */

void
qseditoptions::updateNoteResume ()
{
    perf().resume_note_ons(ui->chkNoteResume->isChecked());
    syncWithInternals();
}

/**
 *  Updates the user_settings::key_height() setting in accord with the
 *  user-interface, and then calls syncWithInternals(), perhaps needlessly, to
 *  make sure the user-interface items correctly represent the settings.
 */

void
qseditoptions::updateKeyHeight ()
{
    usr().key_height(ui->spinKeyHeight->value());
    syncWithInternals();
}

/**
 *  Added for Sequencer64.  Not yet filled with functionality.
 */

void
qseditoptions::on_spinBoxClockStartModulo_valueChanged(int arg1)
{

}

/**
 *  Added for Sequencer64.  Not yet filled with functionality.
 */

void
qseditoptions::on_plainTextEditTempoTrack_textChanged()
{

}

/**
 *  Added for Sequencer64.  Not yet filled with functionality.
 */

void
qseditoptions::on_pushButtonTempoTrack_clicked()
{

}

/**
 *  Added for Sequencer64.  Not yet filled with functionality.
 */

void
qseditoptions::on_checkBoxRecordByChannel_clicked(bool checked)
{

}

/**
 *  Added for Sequencer64.  Not yet filled with functionality.
 */

void
qseditoptions::on_chkJackConditional_stateChanged(int arg1)
{

}

}           // namespace seq64

/*
 * qseditoptions.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
