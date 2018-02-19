#include "qsabout.hpp"
#include "forms/qsabout.ui.h"

qsabout::qsabout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::qsabout)
{
    ui->setupUi(this);
}

qsabout::~qsabout()
{
    delete ui;
}
