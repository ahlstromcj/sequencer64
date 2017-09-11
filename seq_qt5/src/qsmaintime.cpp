#include "qsmaintime.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

qsmaintime::qsmaintime(QWidget *parent,
                       perform *perf,
                       int beats_per_measure,
                       int beat_width):
    QWidget(parent),
    m_main_perf(perf),
    m_beats_per_measure(beats_per_measure),
    m_beat_width(beat_width)
{
    setSizePolicy(QSizePolicy::MinimumExpanding,
                  QSizePolicy::Fixed);

    mFont.setPointSize(9);
    mFont.setBold(true);

    mColour     = new QColor(Qt::red);
    alpha = 0;
    lastMetro = 0;
}
void qsmaintime::paintEvent(QPaintEvent *)
{
    mPainter    = new QPainter(this);
    mBrush      = new QBrush(Qt::NoBrush);
    mPen        = new QPen(Qt::darkGray);

    mPainter->setPen(*mPen);
    mPainter->setFont(mFont);
    mPainter->setBrush(*mBrush);

    long tick = m_main_perf->get_tick();
    int metro = (tick / (c_ppqn / 4 * m_beat_width)) % m_beats_per_measure;
    int divX = (width() - 1) / m_beats_per_measure;

    //flash on beats
    //(i.e. if metro has changed or we've just started playing)
    if (metro != lastMetro || (tick < 50 && tick > 0))
    {
        alpha = 230;
        if (metro == 0)
        {
            //red on first beat in bar
            mColour->setRgb(255, 50, 50);
        }
        else
        {
            //white on others
            mColour->setRgb(255, 255, 255);
        }
    }

    //draw beat blocks
    for (int i = 0; i < m_beats_per_measure; i++)
    {
        int offsetX = divX * i;

        //with flash if on current beat
        if (i == metro && m_main_perf->is_running())
        {
            mBrush->setStyle(Qt::SolidPattern);
            mPen->setColor(Qt::black);
        }
        else
        {
            mBrush->setStyle(Qt::NoBrush);
            mPen->setColor(Qt::darkGray);

        }

        mColour->setAlpha(alpha);
        mBrush->setColor(*mColour);
        mPainter->setPen(*mPen);
        mPainter->setBrush(*mBrush);
        mPainter->drawRect(offsetX + mPen->width() - 1,
                           mPen->width() - 1,
                           divX - mPen->width(),
                           height() - mPen->width());
    }

    //draw beat number (if there's space)
    if (m_beats_per_measure < 10)
    {
        mPen->setColor(Qt::black);
        mPen->setStyle(Qt::SolidLine);
        mPainter->setPen(*mPen);
        mPainter->drawText((metro + 1) * divX - (mFont.pointSize() + 2),
                           height() * 0.3 + mFont.pointSize(),
                           QString::number(metro + 1));
    }

    //lessen alpha on each redraw to have smooth fading
    //done as a factor of the bpm to get useful fades
    alpha *= 0.7 - m_main_perf->get_bpm() / 300;

    lastMetro = metro;

    delete mPainter, mBrush, mPen;
}

int qsmaintime::get_beat_width() const
{
    return m_beat_width;
}

void qsmaintime::setbeat_width(int beat_width)
{
    m_beat_width = beat_width;
}

int qsmaintime::get_beats_per_measure() const
{
    return m_beats_per_measure;
}

void qsmaintime::set_beats_per_measure(int beats_per_measure)
{
    m_beats_per_measure = beats_per_measure;
}

qsmaintime::~qsmaintime()
{

}

QSize qsmaintime::sizeHint() const
{
    return QSize(150, mFont.pointSize() * 2.4);
}

}           // namespace seq64
