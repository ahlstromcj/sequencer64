#include "qperftime.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

qperftime::qperftime(perform *a_perf,
                     QWidget *parent) :
    QWidget(parent),
    m_mainperf(a_perf),
    m_4bar_offset(0),
    m_snap(c_ppqn),
    m_measure_length(c_ppqn * 4),
    zoom(1)
{
    //start refresh timer to queue regular redraws
    mTimer = new QTimer(this);
    mTimer->setInterval(50);
    QObject::connect(mTimer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(update()));
    mTimer->start();

    setSizePolicy(QSizePolicy::Fixed,
                  QSizePolicy::Fixed);
}

void qperftime::paintEvent(QPaintEvent *)
{
    mPainter = new QPainter(this);
    mPen = new QPen(Qt::black);
    mBrush = new QBrush(Qt::lightGray, Qt::SolidPattern);
    mFont.setPointSize(6);
    mPainter->setPen(*mPen);
    mPainter->setBrush(*mBrush);
    mPainter->setFont(mFont);

    mPainter->drawRect(0,
                       0,
                       width(),
                       height());

    /* draw vert lines */
    long tick_offset = 0;
    long first_measure = 0;

    for (int i = first_measure;
            i < first_measure + (width() * c_perf_scale_x / (m_measure_length)) + 1; i++)
    {
        int x_pos = ((i * m_measure_length) - tick_offset) / (c_perf_scale_x * zoom);

        /* beat */
        mPen->setColor(Qt::black);
        mPainter->setPen(*mPen);
        mPainter->drawLine(x_pos,
                           0,
                           x_pos,
                           height());

        if (zoom <= 2) //only draw these numbers if they'll fit
        {
            QString bar(QString::number(i + 1));
            mPen->setColor(Qt::black);
            mPainter->setPen(*mPen);
            mPainter->drawText(x_pos + 2,
                               9,
                               bar);
        }
    }

    long left = m_mainperf->get_left_tick();
    long right = m_mainperf->get_right_tick();

    left -= (m_4bar_offset * 16 * c_ppqn);
    left /= c_perf_scale_x * zoom;
    right -= (m_4bar_offset * 16 * c_ppqn);
    right /= c_perf_scale_x * zoom;

    if (left >= 0 && left <= width())
    {

        mPen->setColor(Qt::black);
        mBrush->setColor(Qt::black);
        mPainter->setBrush(*mBrush);
        mPainter->setPen(*mPen);
        mPainter->drawRect(left,
                           height() - 9,
                           7,
                           10);

        mPen->setColor(Qt::white);
        mPainter->setPen(*mPen);
        mPainter->drawText(left + 1,
                           21,
                           "L");
    }

    if (right >= 0 && right <= width())
    {


        mPen->setColor(Qt::black);
        mBrush->setColor(Qt::black);
        mPainter->setBrush(*mBrush);
        mPainter->setPen(*mPen);
        mPainter->drawRect(right - 6,
                           height() - 9,
                           7,
                           10);

        mPen->setColor(Qt::white);
        mPainter->setPen(*mPen);
        mPainter->drawText(right - 6 + 1,
                           21,
                           "R");

    }
}

QSize qperftime::sizeHint() const
{
    return QSize(m_mainperf->get_max_trigger() / (zoom * c_perf_scale_x) + 2000, 22);
}

void qperftime::mousePressEvent(QMouseEvent *event)
{
    long tick = (long) event->x();
    tick *= c_perf_scale_x * zoom;
    tick += (m_4bar_offset * 16 * c_ppqn);

    tick = tick - (tick % m_snap);

    if (event->y() > height() * 0.5)
    {
        //move L/R markers

        if (event->button() == Qt::LeftButton)
        {
            m_mainperf->set_left_tick(tick);
        }
        if (event->button() == Qt::RightButton)
        {
            m_mainperf->set_right_tick(tick + m_snap);
        }
    }
    else
    {
        //reposition timecode
        m_mainperf->setTick(tick);
    }

}

void qperftime::mouseReleaseEvent(QMouseEvent *event)
{

}

void qperftime::mouseMoveEvent(QMouseEvent *event)
{

}

void qperftime::zoom_in()
{
    if (zoom > 1)
        zoom *= 0.5;
}

void qperftime::zoom_out()
{
    zoom *= 2;
}

void qperftime::set_guides(int a_snap, int a_measure)
{
    m_snap = a_snap;
    m_measure_length = a_measure;
}

}           // namespace seq64
