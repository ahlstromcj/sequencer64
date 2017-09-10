#include "qsabout.hpp"
#include "ui_qsabout.h"

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
