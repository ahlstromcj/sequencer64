#include "qstriggereditor.hpp"

qstriggereditor::qstriggereditor(sequence *a_seq,
                                 qseqdata *a_seqdata_wid,
                                 QWidget *parent, int keyHeight):
    QWidget(parent),
    m_seq(a_seq),
    m_seqdata_wid(a_seqdata_wid),
    m_zoom(1),
    m_snap(1),
    m_selecting(false),
    m_moving_init(false),
    m_moving(false),
    m_growing(false),
    m_painting(false),
    m_paste(false),
    m_adding(false),
    m_status(EVENT_NOTE_ON),
    keyY(keyHeight)

{
    m_old = new QRect();
    m_selected = new QRect();

    m_snap = m_seq->getSnap_tick();

    setSizePolicy(QSizePolicy::Fixed,
                  QSizePolicy::Fixed);

    //start refresh timer to queue regular redraws
    mTimer = new QTimer(this);
    mTimer->setInterval(20);
    QObject::connect(mTimer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(update()));
    mTimer->start();
}

void qstriggereditor::zoom_in()
{
    if (m_zoom > 1)
        m_zoom *= 0.5;
}

void qstriggereditor::zoom_out()
{
    if (m_zoom < 32)
        m_zoom *= 2;
}

QSize qstriggereditor::sizeHint() const
{
    return QSize(m_seq->getLength() / m_zoom + 100 + c_keyboard_padding_x, c_eventarea_y + 1);
}

void qstriggereditor::paintEvent(QPaintEvent *)
{
    mPainter = new QPainter(this);
    mPen = new QPen(Qt::black);
    mBrush = new QBrush(Qt::darkGray, Qt::SolidPattern);
    mFont.setPointSize(6);
    mPainter->setPen(*mPen);
    mPainter->setBrush(*mBrush);
    mPainter->setFont(mFont);

    /* draw background */
    mPainter->drawRect(c_keyboard_padding_x,
                       0,
                       width(),
                       height());

    int measures_per_line = 1;
    int ticks_per_measure =  m_seq->get_beats_per_measure() * (4 * c_ppqn) / m_seq->get_beat_width();
    int ticks_per_beat = (4 * c_ppqn) / m_seq->get_beat_width();
    int ticks_per_step = 6 * m_zoom;
    int ticks_per_m_line =  ticks_per_measure * measures_per_line;
    int start_tick = 0;
    int end_tick = width() * m_zoom;

    //printf ( "ticks_per_step[%d] start_tick[%d] end_tick[%d]\n",
    //         ticks_per_step, start_tick, end_tick );

    for (int i = start_tick; i < width(); i += ticks_per_step)
    {
        int base_line = i + c_keyboard_padding_x;

        if (i % ticks_per_m_line == 0)
        {
            /* solid line on every beat */
            mPen->setColor(Qt::black);
            mPen->setStyle(Qt::SolidLine);

        }
        else if (i % ticks_per_beat == 0)
        {
            mPen->setStyle(Qt::SolidLine);
            mPen->setColor(Qt::black);
        }
        else
        {
            mPen->setColor(Qt::lightGray);
            mPen->setStyle(Qt::DashLine);
        }

        mPainter->setPen(*mPen);
        mPainter->drawLine(base_line,
                           1,
                           base_line,
                           c_eventarea_y);
    }

    //draw event boxes
    long tick;
    int x;
    midibyte d0, d1;
    bool selected;

    /* draw boxes from sequence */
    mPen->setColor(Qt::black);
    mPen->setStyle(Qt::SolidLine);

    m_seq->reset_draw_marker();
    while (m_seq->get_next_event(m_status,
                                 m_cc,
                                 &tick, &d0, &d1,
                                 &selected) == true)
    {
        if ((tick >= start_tick && tick <= end_tick))
        {


            /* turn into screen corrids */
            x = tick / m_zoom + c_keyboard_padding_x;

            //draw outer note border
            mPen->setColor(Qt::black);
            mBrush->setStyle(Qt::SolidPattern);
            mBrush->setColor(Qt::black);
            mPainter->setBrush(*mBrush);
            mPainter->setPen(*mPen);
            mPainter->drawRect(x,
                               (c_eventarea_y - c_eventevent_y) / 2,
                               c_eventevent_x,
                               c_eventevent_y);

            if (selected)
                mBrush->setColor(Qt::red);
            else
                mBrush->setColor(Qt::white);

            //draw note highlight
            mPainter->setBrush(*mBrush);
            mPainter->drawRect(x,
                               (c_eventarea_y - c_eventevent_y) / 2,
                               c_eventevent_x - 1,
                               c_eventevent_y - 1);
        }
    }

    //draw selection

    int w;

    int y = (c_eventarea_y - c_eventevent_y) / 2;
    int h =  c_eventevent_y;

    //painter reset
    mBrush->setStyle(Qt::NoBrush);
    mPainter->setBrush(*mBrush);

    if (m_selecting)
    {

        x_to_w(m_drop_x, m_current_x, &x, &w);

        m_old->setX(x);
        m_old->setWidth(w);

        mPen->setColor(Qt::black);
        mPainter->setPen(*mPen);
        mPainter->drawRect(x + c_keyboard_padding_x,
                           y,
                           w,
                           h);
    }

    if (m_moving || m_paste)
    {

        int delta_x = m_current_x - m_drop_x;

        x = m_selected->x() + delta_x;

        mPen->setColor(Qt::black);
        mPainter->setPen(*mPen);
        mPainter->drawRect(x + c_keyboard_padding_x,
                           y,
                           m_selected->width(),
                           h);
        m_old->setX(x);
        m_old->setWidth(m_selected->width());
    }

    delete mPainter;
    delete mBrush;
    delete mPen;
}

void qstriggereditor::mousePressEvent(QMouseEvent *event)
{
    int x, w, numsel;

    long tick_s;
    long tick_f;
    long tick_w;

    convert_x(c_eventevent_x, &tick_w);

    /* if it was a button press */

    /* set values for dragging */
    m_drop_x = m_current_x = (int) event->x() - c_keyboard_padding_x;

    /* reset box that holds dirty redraw spot */
    m_old->setX(0);
    m_old->setY(0);
    m_old->setWidth(0);
    m_old->setHeight(0);

    if (m_paste)
    {
        snap_x(&m_current_x);
        convert_x(m_current_x, &tick_s);
        m_paste = false;
        m_seq->push_undo();
        m_seq->paste_selected(tick_s, 0);

    }
    else
    {
        /*      left mouse button     */
        if (event->button() == Qt::LeftButton)
        {
            /* turn x,y in to tick/note */
            convert_x(m_drop_x, &tick_s);

            /* shift back a few ticks */
            tick_f = tick_s + (m_zoom);
            tick_s -= (tick_w);

            if (tick_s < 0)
                tick_s = 0;

            if (m_adding)
            {
                m_painting = true;

                snap_x(&m_drop_x);
                /* turn x,y in to tick/note */
                convert_x(m_drop_x, &tick_s);
                /* add note, length = little less than snap */

                if (! m_seq->select_events(tick_s, tick_f,
                                           m_status, m_cc, sequence::e_would_select))
                {
                    m_seq->push_undo();
                    drop_event(tick_s);
                }

            }
            else /* selecting */
            {
                if (! m_seq->select_events(tick_s, tick_f,
                                           m_status, m_cc, sequence::e_is_selected))
                {
                    if (!(event->modifiers() & Qt::ControlModifier))
                    {
                        m_seq->unselect();
                    }

                    numsel = m_seq->select_events(tick_s, tick_f,
                                                  m_status,
                                                  m_cc, sequence::e_select_single);

                    /* if we didnt select anyhing (user clicked empty space)
                       unselect all notes, and start selecting */

                    /* none selected, start selection box */
                    if (numsel == 0)
                    {
                        m_selecting = true;
                    }
                    else
                    {
                        /// needs update
                    }
                }

                if (m_seq->select_events(tick_s, tick_f,
                                         m_status, m_cc, sequence::e_is_selected))
                {

                    m_moving_init = true;
                    int note;

                    /* get the box that selected elements are in */
                    m_seq->get_selected_box(&tick_s, &note,
                                            &tick_f, &note);

                    tick_f += tick_w;

                    /* convert box to X,Y values */
                    convert_t(tick_s, &x);
                    convert_t(tick_f, &w);

                    /* w is actually corrids now, so we have to change */
                    w = w - x;

                    /* set the m_selected rectangle to hold the
                       x,y,w,h of our selected events */

                    m_selected->setX(x);
                    m_selected->setWidth(w);

                    m_selected->setY((c_eventarea_y - c_eventevent_y) / 2);
                    m_selected->setHeight(c_eventevent_y);


                    /* save offset that we get from the snap above */
                    int adjusted_selected_x = m_selected->x();
                    snap_x(&adjusted_selected_x);
                    m_move_snap_offset_x = (m_selected->x() - adjusted_selected_x);

                    /* align selection for drawing */
                    //save X as a variable so we can use the snap function
                    int tempSelectedX = m_selected->x();
                    snap_x(&tempSelectedX);
                    m_selected->setX(tempSelectedX);
                    snap_x(&m_current_x);
                    snap_x(&m_drop_x);

                }
            }

        } /* end if button == 1 */

        if (event->button() == Qt::RightButton)
        {

            set_adding(true);
        }
    }
}

void qstriggereditor::mouseReleaseEvent(QMouseEvent *event)
{
    long tick_s;
    long tick_f;

    int x, w;

    m_current_x = (int) event->x() - c_keyboard_padding_x;

    if (m_moving)
        snap_x(&m_current_x);

    int delta_x = m_current_x - m_drop_x;

    long delta_tick;

    if (event->button() == Qt::LeftButton)
    {
        if (m_selecting)
        {
            x_to_w(m_drop_x, m_current_x, &x, &w);

            convert_x(x,   &tick_s);
            convert_x(x + w, &tick_f);

            m_seq->select_events(tick_s, tick_f,
                                 m_status,
                                 m_cc, sequence::e_select);
        }

        if (m_moving)
        {

            /* adjust for snap */
            delta_x -= m_move_snap_offset_x;

            /* convert deltas into screen corridinates */
            convert_x(delta_x, &delta_tick);

            /* not really notes, but still moves events */
            m_seq->push_undo();
            m_seq->move_selected_notes(delta_tick, 0);
        }

        set_adding(m_adding);
    }

    if (event->button() == Qt::RightButton)
    {
        set_adding(false);
    }

    /* turn off */
    m_selecting = false;
    m_moving = false;
    m_growing = false;
    m_moving_init = false;
    m_painting = false;

    m_seq->unpaint_all();
}

void qstriggereditor::mouseMoveEvent(QMouseEvent *event)
{
    long tick = 0;

    if (m_moving_init)
    {
        m_moving_init = false;
        m_moving = true;
    }

    if (m_selecting || m_moving || m_paste)
    {

        m_current_x = (int) event->x() - c_keyboard_padding_x;

        if (m_moving || m_paste)
            snap_x(&m_current_x);
    }


    if (m_painting)
    {
        m_current_x = (int) event->x();
        snap_x(&m_current_x);
        convert_x(m_current_x, &tick);
        drop_event(tick);
    }
}

void qstriggereditor::keyPressEvent(QKeyEvent *event)
{
    bool ret = false;

    if (event->key() == Qt::Key_Delete ||
            event->key() == Qt::Key_Backspace)
    {
        m_seq->push_undo();
        m_seq->mark_selected();
        m_seq->remove_marked();
        ret = true;
    }

    if (event->modifiers() & Qt::ControlModifier)
    {

        switch (event->key())
        {
        /* cut */
        case Qt::Key_X:
            m_seq->copy_selected();
            m_seq->mark_selected();
            m_seq->remove_marked();
            ret = true;
            break;
        /* copy */
        case Qt::Key_C:
            m_seq->copy_selected();
            ret = true;
            break;
        /* paste */
        case Qt::Key_V:
            start_paste();
            ret = true;
            break;
        /* Undo */
        case Qt::Key_Z:
            if (event->modifiers() & Qt::ShiftModifier)
                m_seq->pop_redo();
            else
                m_seq->pop_undo();
            ret = true;
            break;
        }
    }

    if (ret == true)
        m_seq->set_dirty();
}

void qstriggereditor::keyReleaseEvent(QKeyEvent *event)
{

}

void qstriggereditor::x_to_w(int a_x1, int a_x2, int *a_x, int *a_w)
{
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
}

void qstriggereditor::start_paste()
{
    long tick_s;
    long tick_f;
    int note_h;
    int note_l;
    int x, w;

    snap_x(&m_current_x);
    snap_y(&m_current_x);

    m_drop_x = m_current_x;
    m_drop_y = m_current_y;

    m_paste = true;

    /* get the box that selected elements are in */
    m_seq->get_clipboard_box(&tick_s, &note_h,
                             &tick_f, &note_l);

    /* convert box to X,Y values */
    convert_t(tick_s, &x);
    convert_t(tick_f, &w);

    /* w is actually corrids now, so we have to change */
    w = w - x;

    /* set the m_selected rectangle to hold the
    x,y,w,h of our selected events */

    m_selected->setX(x);
    m_selected->setWidth(w);
    m_selected->setY((c_eventarea_y - c_eventevent_y) / 2);
    m_selected->setHeight(c_eventevent_y);

    /* adjust for clipboard being shifted to tick 0 */
    m_selected->setX(m_selected->x() + m_drop_x);
}

void qstriggereditor::convert_x(int a_x, long *a_tick)
{
    *a_tick = a_x * m_zoom;
}

void qstriggereditor::convert_t(long a_ticks, int *a_x)
{
    *a_x = a_ticks /  m_zoom;
}

void qstriggereditor::drop_event(long a_tick)
{
    midibyte status = m_status;
    midibyte d0 = m_cc;
    midibyte d1 = 0x40;

    if (m_status == EVENT_AFTERTOUCH)
        d0 = 0;

    if (m_status == EVENT_PROGRAM_CHANGE)
        d0 = 0; /* d0 == new patch */

    if (m_status == EVENT_CHANNEL_PRESSURE)
        d0 = 0x40; /* d0 == pressure */

    if (m_status == EVENT_PITCH_WHEEL)
        d0 = 0;

    m_seq->add_event(a_tick,
                     status,
                     d0,
                     d1, true);
}

/* performs a 'snap' on y */
void qstriggereditor::snap_y(int *a_y)
{
    *a_y = *a_y - (*a_y % keyY);
}

/* performs a 'snap' on x */
void qstriggereditor::snap_x(int *a_x)
{
    //snap = number pulses to snap to
    //m_zoom = number of pulses per pixel
    //so snap / m_zoom  = number pixels to snap to
    int mod = (m_snap / m_zoom);
    if (mod <= 0)
        mod = 1;

    *a_x = *a_x - (*a_x % mod);

}

void qstriggereditor::set_adding(bool a_adding)
{
    if (a_adding)
    {
        setCursor(Qt::PointingHandCursor);
        m_adding = true;
    }
    else
    {
        setCursor(Qt::ArrowCursor);
        m_adding = false;
    }
}

void qstriggereditor::set_data_type(midibyte a_status,
                                    midibyte a_control = 0)
{
    m_status = a_status;
    m_cc = a_control;
}
