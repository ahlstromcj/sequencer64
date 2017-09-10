#include "qseqdata.hpp"

qseqdata::qseqdata(sequence *a_seq,
                   QWidget *parent):
    QWidget(parent),
    m_seq(a_seq),
    m_zoom(1),
    mLineAdjust(false)
{
    setSizePolicy(QSizePolicy::Fixed,
                  QSizePolicy::Fixed);

    //edit note velocity for now
    m_status = EVENT_NOTE_ON;
    m_cc = 1;

    mOld = new QRect();

    //start refresh timer to queue regular redraws
    mTimer = new QTimer(this);
    mTimer->setInterval(20);
    QObject::connect(mTimer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(update()));
    mTimer->start();
}

void qseqdata::zoom_in()
{
    if (m_zoom > 1)
        m_zoom *= 0.5;
}

void qseqdata::zoom_out()
{
    if (m_zoom < 32)
        m_zoom *= 2;
}

QSize qseqdata::sizeHint() const
{
    return QSize(m_seq->getLength() / m_zoom + 100 + c_keyboard_padding_x, c_dataarea_y);
}

void qseqdata::paintEvent(QPaintEvent *)
{
    mPainter = new QPainter(this);
    mPen = new QPen(Qt::black);
    mBrush = new QBrush(Qt::lightGray, Qt::SolidPattern);
    mFont.setPointSize(6);
    mPainter->setPen(*mPen);
    mPainter->setBrush(*mBrush);
    mPainter->setFont(mFont);

    //this is perhaps the background?

    long tick;

    midibyte d0, d1;

    int event_x;
    int event_height;

    bool selected;

    int start_tick = 0 ;
    int end_tick = (width() * m_zoom);

    mPainter->drawRect(0,
                       0,
                       width() - 1,
                       height() - 1);

    m_seq->reset_draw_marker();
    while (m_seq->get_next_event(m_status,
                                 m_cc,
                                 &tick, &d0, &d1,
                                 &selected) == true)
    {

        if (tick >= start_tick && tick <= end_tick)
        {

            /* turn into screen corrids */

            event_x = tick / m_zoom + c_keyboard_padding_x;

            /* generate the value */
            event_height = d1;

            if (m_status == EVENT_PROGRAM_CHANGE ||
                    m_status == EVENT_CHANNEL_PRESSURE)
            {

                event_height = d0;
            }

            /* draw vert lines */
            mPen->setWidth(2);
            mPainter->setPen(*mPen);
            mPainter->drawLine(event_x + 1,
                               height() - event_height,
                               event_x + 1,
                               height());

            //draw numbers
            QString val = QString::number(d1);

            mPen->setColor(Qt::black);
            mPen->setWidth(1);
            mPainter->setPen(*mPen);
            if (val.length() >= 1)
                mPainter->drawText(event_x + 3,
                                   c_dataarea_y - 25,
                                   val.at(0));
            if (val.length() >= 2)
                mPainter->drawText(event_x + 3,
                                   c_dataarea_y - 25 + 8, \
                                   val.at(1));
            if (val.length() >= 3)
                mPainter->drawText(event_x + 3,
                                   c_dataarea_y - 25 + 16,
                                   val.at(2));
        }
    }

    //draw edit line

    if (mLineAdjust)
    {
        int x, y, w, h;
        mPen->setColor(Qt::black);
        mPen->setStyle(Qt::DashLine);
        mPainter->setPen(*mPen);

        xy_to_rect(mDropX,
                   mDropY,
                   mCurrentX,
                   mCurrentY,
                   &x, &y,
                   &w, &h);

        mOld->setX(x);
        mOld->setY(y);
        mOld->setWidth(w);
        mOld->setHeight(h);

        mPainter->drawLine(mCurrentX + c_keyboard_padding_x,
                           mCurrentY,
                           mDropX + c_keyboard_padding_x,
                           mDropY);
    }

    delete mPainter;
    delete mBrush;
    delete mPen;
}

void qseqdata::mousePressEvent(QMouseEvent *event)
{
    m_seq->push_undo();

    int mouseX = event->x() - c_keyboard_padding_x;
    int mouseY = event->y();

    //if we're near an event (4px), do relative adjustment
    long tick_start, tick_finish;
    convert_x(mouseX - 2, &tick_start);
    convert_x(mouseX + 2, &tick_finish);

    //check if these ticks would select an event
    if (m_seq->select_events(tick_start, tick_finish, m_status, m_cc, sequence::e_would_select))
    {
        mRelativeAdjust = true;
    }
    else //else set new values for seqs under a line
    {
        mLineAdjust = true;
    }

    /* set values for line */
    mDropX = mouseX;
    mDropY = mouseY;

    /* reset box that holds dirty redraw spot */
    mOld->setX(0);
    mOld->setY(0);
    mOld->setWidth(0);
    mOld->setHeight(0);
}

void qseqdata::mouseReleaseEvent(QMouseEvent *event)
{
    mCurrentX = (int) event->x() - c_keyboard_padding_x;
    mCurrentY = (int) event->y();
    long tick_s, tick_f;

    if (mLineAdjust)
    {
        if (mCurrentX < mDropX)
        {
            swap(mCurrentX, mDropX);
            swap(mCurrentY, mDropY);
        }

        convert_x(mDropX, &tick_s);
        convert_x(mCurrentX, &tick_f);

        m_seq->change_event_data_range(tick_s, tick_f,
                                       m_status,
                                       m_cc,
                                       c_dataarea_y - mDropY - 1,
                                       c_dataarea_y - mCurrentY - 1);

        /* convert x,y to ticks, then set events in range */
        mLineAdjust = false;

    }
    else if (mRelativeAdjust)
    {
        mRelativeAdjust = false;
    }


}

void qseqdata::mouseMoveEvent(QMouseEvent *event)
{
    mCurrentX = event->x() - c_keyboard_padding_x;
    mCurrentY = event->y();
    long tick_s, tick_f;


    if (mLineAdjust)
    {
        int adj_x_min, adj_x_max,
            adj_y_min, adj_y_max;
        if (mCurrentX < mDropX)
        {
            adj_x_min = mCurrentX;
            adj_y_min = mCurrentY;
            adj_x_max = mDropX;
            adj_y_max = mDropY;
        }
        else
        {
            adj_x_max = mCurrentX;
            adj_y_max = mCurrentY;
            adj_x_min = mDropX;
            adj_y_min = mDropY;
        }

        convert_x(adj_x_min, &tick_s);
        convert_x(adj_x_max, &tick_f);

        m_seq->change_event_data_range(tick_s, tick_f,
                                       m_status,
                                       m_cc,
                                       c_dataarea_y - adj_y_min - 1,
                                       c_dataarea_y - adj_y_max - 1);
    }
    else if (mRelativeAdjust)
    {
        int adjY = mDropY - mCurrentY;

        convert_x(mDropX - 2, &tick_s);
        convert_x(mDropX + 2, &tick_f);

        m_seq->change_event_data_relative(tick_s, tick_f,
                                          m_status,
                                          m_cc,
                                          adjY);

        //move the drop location so we increment properly on next mouse move
        mDropY = mCurrentY;
    }
}

void qseqdata::xy_to_rect(int a_x1,  int a_y1,
                          int a_x2,  int a_y2,
                          int *a_x,  int *a_y,
                          int *a_w,  int *a_h)
{
    /* checks mins / maxes..  the fills in x,y
       and width and height */

    if (a_x1 < a_x2)
    {
        *a_x = a_x1;
        *a_w = a_x2 - a_x1;
    }
    else
    {
        *a_x = a_x2;
        *a_w = a_x1 - a_x2;
    }

    if (a_y1 < a_y2)
    {
        *a_y = a_y1;
        *a_h = a_y2 - a_y1;
    }
    else
    {
        *a_y = a_y2;
        *a_h = a_y1 - a_y2;
    }
}

void qseqdata::set_data_type(midibyte a_status, midibyte a_control = 0)
{
    m_status = a_status;
    m_cc = a_control;
}

void qseqdata::convert_x(int a_x, long *a_tick)
{
    *a_tick = a_x * m_zoom;
}

