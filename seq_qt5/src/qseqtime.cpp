#include "qseqtime.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

qseqtime::qseqtime(sequence *a_seq, QWidget *parent):
    QWidget(parent),
    m_seq(a_seq)
{
    //start refresh timer to queue regular redraws
    m_timer = new QTimer(this);
    m_timer->setInterval(50);
    QObject::connect(m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(update()));
    m_timer->start();

    m_zoom = 1;

    setSizePolicy(QSizePolicy::Fixed,
                  QSizePolicy::Fixed);
}

void qseqtime::paintEvent(QPaintEvent *)
{
    m_painter = new QPainter(this);
    m_pen = new QPen(Qt::black);
    m_brush = new QBrush(Qt::lightGray, Qt::SolidPattern);
    m_font.setPointSize(6);
    m_painter->setPen(*m_pen);
    m_painter->setBrush(*m_brush);
    m_painter->setFont(m_font);

    //draw time bar border
    m_painter->drawRect(c_keyboard_padding_x,
                        0,
                        size().width(),
                        size().height() - 1);

    int measure_length_32nds =  m_seq->get_beats_per_measure() * 32 /
                                m_seq->get_beat_width();

    int measures_per_line = (128 / measure_length_32nds) / (32 / m_zoom);
    if (measures_per_line <= 0)
        measures_per_line = 1;

    int ticks_per_measure =  m_seq->get_beats_per_measure() * (4 * c_ppqn) / m_seq->get_beat_width();
    int ticks_per_beat =  ticks_per_measure * measures_per_line;
    int start_tick = 0;
    int end_tick = (m_seq->getLength());

    m_pen->setColor(Qt::black);
    m_painter->setPen(*m_pen);
    for (int i = start_tick; i <= end_tick; i += ticks_per_beat)
    {
        int zoomedX = i / m_zoom + c_keyboard_padding_x;

        //vertical line at each beat
        m_painter->drawLine(zoomedX,
                            0,
                            zoomedX,
                            size().height());

        char bar[5];
        snprintf(bar, sizeof(bar), "%d", (i / ticks_per_measure) + 1);

        //number each beat
        m_pen->setColor(Qt::black);
        m_painter->setPen(*m_pen);
        m_painter->drawText(zoomedX + 3,
                            10,
                            bar);

    }

    long end_x = m_seq->getLength() / m_zoom + c_keyboard_padding_x;

    //draw end of seq label
    //label background
    m_pen->setColor(Qt::white);
    m_brush->setColor(Qt::white);
    m_brush->setStyle(Qt::SolidPattern);
    m_painter->setBrush(*m_brush);
    m_painter->setPen(*m_pen);
    m_painter->drawRect(end_x + 1,
                        13,
                        15,
                        8);
    //label text
    m_pen->setColor(Qt::black);
    m_painter->setPen(*m_pen);
    m_painter->drawText(end_x + 1, 21,
                        tr("END"));

    delete m_painter;
    delete m_brush;
    delete m_pen;
}

void qseqtime::mousePressEvent(QMouseEvent *event)
{

}

void qseqtime::mouseReleaseEvent(QMouseEvent *event)
{

}

void qseqtime::mouseMoveEvent(QMouseEvent *event)
{

}

QSize qseqtime::sizeHint() const
{
    return QSize(m_seq->getLength() / m_zoom + 100 + c_keyboard_padding_x, 22);
}

void qseqtime::zoom_in()
{
    if (m_zoom > 1)
        m_zoom *= 0.5;
}

void qseqtime::zoom_out()
{
    if (m_zoom < 32)
        m_zoom *= 2;
}

}           // namespace seq64
