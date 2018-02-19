#include "qseditoptions.hpp"
#include "forms/qseditoptions.ui.h"

qseditoptions::qseditoptions(perform *perf,
                             QWidget *parent):
    QDialog(parent),
    ui(new Ui::qseditoptions),
    mPerf(perf)
{
    ui->setupUi(this);

    backup();

    syncWithInternals();

    connect(ui->btnJackConnect,
            SIGNAL(clicked(bool)),
            this,
            SLOT(jackConnect()));

    connect(ui->btnJackDisconnect,
            SIGNAL(clicked(bool)),
            this,
            SLOT(jackDisconnect()));

    connect(ui->chkJackTransport,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(updateTransportSupport()));

    connect(ui->chkJackConditional,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(updateMasterCond()));

    connect(ui->chkJackMaster,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(updateTimeMaster()));

    connect(ui->buttonBox->button(QDialogButtonBox::Ok),
            SIGNAL(clicked(bool)),
            this,
            SLOT(okay()));

    connect(ui->buttonBox->button(QDialogButtonBox::Cancel),
            SIGNAL(clicked(bool)),
            this,
            SLOT(cancel()));

    connect(ui->chkNoteResume,
            SIGNAL(stateChanged(int)),
            this,
            SLOT(updateNoteResume()));

    connect(ui->spinKeyHeight,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(updateKeyHeight()));
}

qseditoptions::~qseditoptions()
{
    delete ui;
}

void qseditoptions::addRecentFile(QString path)
{
    //start shifting from the last element if file not already recent
    int path_found_index = 9;

    //check if path is already present
    for (int c = 0; c < 10; c++)
    {
        if (recent_files[c] == path)
        {
            path_found_index = c;
        }
    }

    //shift the recent files along by one, thus dropping the oldest
    for (int c = path_found_index; c > 0; c--)
    {
        recent_files[c] = recent_files[c - 1];
    }

    //add the new path to the first slot
    recent_files[0] = path;
}

void qseditoptions::jackConnect()
{
    mPerf->init_jack();
}

void qseditoptions::jackDisconnect()
{
    mPerf->deinit_jack();
}

void qseditoptions::updateMasterCond()
{
    global_with_jack_master_cond = ui->chkJackConditional->isChecked();
    syncWithInternals();
}

void qseditoptions::updateTimeMaster()
{
    global_with_jack_master = ui->chkJackMaster->isChecked();
    syncWithInternals();
}

void qseditoptions::updateTransportSupport()
{
    global_with_jack_transport = ui->chkJackTransport->isChecked();
    syncWithInternals();
}

void qseditoptions::cancel()
{
    //restore settings from the backup
    global_with_jack_transport = backupJackTransport;
    global_with_jack_master_cond = backupMasterCond;
    global_with_jack_master = backupTimeMaster;
    mPerf->setEditorKeyHeight(backupKeyHeight);
    mPerf->setResumeNoteOns(backupNoteResume);

    syncWithInternals();

    close();
}

void qseditoptions::syncWithInternals()
{
    //sync with preferences
    ui->chkJackTransport->setChecked(global_with_jack_transport);
    ui->chkJackMaster->setChecked(global_with_jack_master);
    ui->chkJackConditional->setChecked(global_with_jack_master_cond);

    if (!global_with_jack_transport)
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

void qseditoptions::backup()
{
    //backup settings incase user cancels
    backupJackTransport = global_with_jack_transport;
    backupMasterCond = global_with_jack_master_cond;
    backupTimeMaster = global_with_jack_master;
    backupKeyHeight = mPerf->getEditorKeyHeight();
    backupNoteResume = mPerf->getResumeNoteOns();
}

void qseditoptions::okay()
{
    backup();
    close();
}

void qseditoptions::updateNoteResume()
{
    mPerf->setResumeNoteOns(ui->chkNoteResume->isChecked());
    qDebug() << "Note resume status" << mPerf->getResumeNoteOns() << endl;
    syncWithInternals();
}

void qseditoptions::updateKeyHeight()
{
    mPerf->setEditorKeyHeight(ui->spinKeyHeight->value());
    syncWithInternals();
}
