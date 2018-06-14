#ifndef QSEQEDITEX_H
#define QSEQEDITEX_H

#include <QWidget>

namespace Ui {
class qseqeditex;
}

class qseqeditex : public QWidget
{
    Q_OBJECT

public:
    explicit qseqeditex(QWidget *parent = 0);
    ~qseqeditex();

private:
    Ui::qseqeditex *ui;
};

#endif // QSEQEDITEX_H
