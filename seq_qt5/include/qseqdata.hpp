#ifndef EDITEVENTVALUES_HPP
#define EDITEVENTVALUES_HPP

#include <QWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

#include "sequence.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 * Displays the values for MIDI events
 * e.g. Modwheel, pitchbend etc
 */

class qseqdata : public QWidget
{
    Q_OBJECT

public:
    explicit qseqdata(sequence *a_seq,
                      QWidget *parent = 0);

    void zoom_in();
    void zoom_out();
    void set_data_type(midibyte a_status, midibyte a_control);

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

    // Takes two points, returns a Xwin rectangle
    void xy_to_rect(int a_x1,  int a_y1,
                    int a_x2,  int a_y2,
                    int *a_x,  int *a_y,
                    int *a_w,  int *a_h);

    //convert the given X coordinate into a tick via a pointer
    void convert_x(int a_x, long *a_tick);

    sequence *m_seq;

    QPen        *mPen;
    QBrush      *mBrush;
    QPainter    *mPainter;
    QFont        mFont;
    QRect       *mOld;
    QString      mNumbers;
    QTimer      *mTimer;

    int m_zoom;

    int mDropX, mDropY;
    int mCurrentX, mCurrentY;

    /* what is the data window currently editing ? */
    midibyte m_status;
    midibyte m_cc;

    //interaction states
    bool mLineAdjust; //dragging a new-level adjustment slope
    bool mRelativeAdjust; //relative adjusting notes by dragging

    friend class qseqroll;
    friend class qstriggereditor;
};

}           // namespace seq64

#endif // EDITEVENTVALUES_HPP
