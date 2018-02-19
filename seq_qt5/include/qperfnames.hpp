#ifndef SONGSEQUENCENAMES_HPP
#define SONGSEQUENCENAMES_HPP

#include <QWidget>
#include <QPainter>
#include <QPen>

#include "Globals.hpp"

#include "perform.hpp"
#include "sequence.hpp"
#include "globals.h"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 * Sequence labels for the side of the song editor
 */

class qperfnames : public QWidget
{
    Q_OBJECT

public:

    explicit qperfnames(perform *a_perf,
                        QWidget *parent);
    ~qperfnames();

protected:
    //override painting event to draw on the frame
    void paintEvent(QPaintEvent *);

    //override mouse events for interaction
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

    //override the sizehint to set our own defaults
    QSize sizeHint() const;

signals:

public slots:

private:
    perform *mPerf;

    QTimer      *mTimer;
    QPen        *mPen;
    QBrush      *mBrush;
    QPainter    *mPainter;
    QFont        mFont;

    bool         m_sequence_active[qc_total_seqs];
};

}           // namespace seq64

#endif // SONGSEQUENCENAMES_HPP
