#ifndef SONGTIMEBAR_HPP
#define SONGTIMEBAR_HPP

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QObject>
#include <QPen>
#include <QMouseEvent>

#include "globals.h"
#include "perform.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 * The time bar for the song editor
 */

class qperftime : public QWidget
{
    Q_OBJECT

public:
    explicit qperftime(perform *a_perf,
                       QWidget *parent);

    void zoom_in();
    void zoom_out();

    void set_guides(int a_snap, int a_measure);

protected:
    //override painting event to draw on the frame
    void paintEvent(QPaintEvent *);

    //override mouse events for interaction
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

    //override the sizehint to set our own defaults
    QSize sizeHint() const;

private:
    perform *m_mainperf;

    QTimer      *mTimer;
    QPen        *mPen;
    QBrush      *mBrush;
    QPainter    *mPainter;
    QFont        mFont;

    int m_4bar_offset;

    int m_snap, m_measure_length;

    int zoom;

signals:

public slots:
};

}           // namespace seq64

#endif // SONGTIMEBAR_HPP
