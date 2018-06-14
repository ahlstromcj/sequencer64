#include "qseqeditex.hpp"
#include "ui_qseqeditex.h"

qseqeditex::qseqeditex(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::qseqeditex)
{
    ui->setupUi(this);
}

qseqeditex::~qseqeditex()
{
    delete ui;
}
