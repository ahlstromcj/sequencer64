#ifndef ABOUTDIALOG_HPP
#define ABOUTDIALOG_HPP

#include <QDialog>

namespace Ui
{
    class qsabout;
}

class qsabout : public QDialog
{
    Q_OBJECT

public:

    explicit qsabout(QWidget *parent = 0);
    ~qsabout();

private:

    Ui::qsabout *ui;
};

#endif // ABOUTDIALOG_HPP
