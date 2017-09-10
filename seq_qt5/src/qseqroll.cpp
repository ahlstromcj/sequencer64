#include "qseqroll.hpp"

qseqroll::qseqroll(perform *a_perf,
                   sequence *a_seq,
                   QWidget *parent,
                   edit_mode_e mode):
    QWidget(parent),
    m_perform(a_perf),
    m_seq(a_seq),
    m_scale(0),
    m_key(0),
    m_zoom(1),
    m_note_length(c_ppqn * 4 / 16),
    m_selecting(false),
    m_adding(false),
    m_moving(false),
    m_moving_init(false),
    m_growing(false),
    m_painting(false),
    m_paste(false),
    m_is_drag_pasting(false),
    m_is_drag_pasting_start(false),
    m_justselected_one(false),
    m_old_progress_x(0),
    m_background_sequence(0),
    m_drawing_background_seq(false),
    editMode(mode),
    keyY(a_perf->getEditorKeyHeight()),
    keyAreaY(a_perf->getEditorKeyboardHeight())
{
    set_snap(m_seq->getSnap_tick());

    setSizePolicy(QSizePolicy::Fixed,
                  QSizePolicy::Fixed);

    setFocusPolicy(Qt::StrongFocus);

    //start refresh timer to queue regular redraws
    mTimer = new QTimer(this);
    mTimer->setInterval(20);
    QObject::connect(mTimer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(update()));
    mTimer->start();
}

void qseqroll::paintEvent(QPaintEvent *)
{
    mPainter = new QPainter(this);
    mBrush = new QBrush(Qt::NoBrush);
    mPen = new QPen(Qt::black);
    mFont.setPointSize(6);
    mPainter->setPen(*mPen);
    mPainter->setBrush(*mBrush);
    mPainter->setFont(mFont);

    //draw border
    //    m_painter->drawRect(0, 0, width(), height());

    mPen->setColor(Qt::lightGray);
    mPen->setStyle(Qt::DashLine);
    mPainter->setPen(*mPen);

    //for each note row in our grid
    for (int i = 1; i < c_num_keys; i++)
    {
        //set line colour dependent on the note row we're on
        //        if (0 == (((c_num_keys - i) + ( 12 - m_key )) % 12))
        //        {
        //            m_pen->setColor(Qt::darkGray);
        //            m_pen->setStyle(Qt::SolidLine);
        //            m_painter->setPen(*m_pen);
        //        }
        //        else if (11 == (((c_num_keys - i) + ( 12 - m_key )) % 12))
        //        {
        //            m_pen->setColor(Qt::darkGray);
        //            m_pen->setStyle(Qt::SolidLine);
        //            m_painter->setPen(*m_pen);
        //        }

        //draw horizontal grid lines
        //(differently depending on editing mode)
        switch (editMode)
        {
        case NOTE:
            mPainter->drawLine(0,
                               i * keyY,
                               width(),
                               i * keyY);
            break;

        case DRUM:
            mPainter->drawLine(0,
                               i * keyY - (0.5 * keyY),
                               width(),
                               i * keyY - (0.5 * keyY));
            break;
        }


        if (m_scale != c_scale_off)
        {
            if (!c_scales_policy[m_scale][((c_num_keys - i) - 1 + (12 - m_key)) % 12])
                /*m_painter->drawRect(0,
                                    i * keyY + 1,
                                    m_size_x,
                                    keyY - 1)*/;
        }
    }

    int measures_per_line = 1;
    int ticks_per_measure =  m_seq->get_beats_per_measure() * (4 * c_ppqn) / m_seq->get_beat_width();
    int ticks_per_beat = (4 * c_ppqn) / m_seq->get_beat_width();
    int ticks_per_step = 6 * m_zoom;
    int ticks_per_m_line =  ticks_per_measure * measures_per_line;

    //start drawing vertical grid lines
    for (int i = 0; i < width(); i += ticks_per_step)
    {
        int base_line = i + c_keyboard_padding_x;

        if (i % ticks_per_m_line == 0)
        {

            //solid line on every beat
            mPen->setColor(Qt::black);
            mPen->setStyle(Qt::SolidLine);

        }
        else if (i % ticks_per_beat == 0)
        {

            mPen->setColor(Qt::DashLine);
        }
        else
        {
            //faint step lines
            mPen->setColor(Qt::lightGray);
            mPen->setStyle(Qt::DotLine);

            int i_snap = i - (i % m_snap);

            if (i == i_snap)
            {
                //                m_pen->setColor(Qt::darkGray);
            }
            else
            {
                //                m_pen->setColor(Qt::gray);
            }
        }

        //draw vertical grid lines
        mPen->setStyle(Qt::SolidLine);
        mPainter->setPen(*mPen);
        mPainter->drawLine(base_line,
                           0,
                           base_line,
                           keyAreaY);
    }

    //draw the playhead
    mPen->setColor(Qt::red);
    mPen->setStyle(Qt::SolidLine);
    mPainter->setPen(*mPen);
    mPainter->drawLine(m_old_progress_x,
                       0,
                       m_old_progress_x,
                       height() * 8);

    m_old_progress_x = (m_seq->get_last_tick() / m_zoom + c_keyboard_padding_x);

    //draw notes
    long tick_s;
    long tick_f;
    int note;

    bool selected;

    int velocity;

    draw_type dt;

    int start_tick = 0;
    int end_tick = (width() * m_zoom);

    sequence *seq = NULL;

    for (int method = 0; method < 2; ++method)
    {

        if (method == 0 && m_drawing_background_seq)
        {

            if (m_perform->is_active(m_background_sequence))
            {
                seq = m_perform->get_sequence(m_background_sequence);
            }
            else
            {
                method++;
            }
        }

        else if (method == 0)
        {
            method++;
        }

        if (method == 1)
        {
            seq = m_seq;
        }

        /* draw boxes from sequence */
        mPen->setColor(Qt::black);
        mPen->setStyle(Qt::SolidLine);
        seq->reset_draw_marker();

        while ((dt = seq->get_next_note_event(&tick_s, &tick_f, &note, &selected, &velocity)) != DRAW_FIN)
        {

            if ((tick_s >= start_tick && tick_s <= end_tick) ||
                    ((dt == DRAW_NORMAL_LINKED) && (tick_f >= start_tick && tick_f <= end_tick))
               )
            {

                /* turn into screen corrids */
                note_x = tick_s / m_zoom + c_keyboard_padding_x;
                note_y = keyAreaY - (note * keyY) - keyY - 1 + 2;
                switch (editMode)
                {
                case NOTE:
                    note_height = keyY - 3;
                    break;
                case DRUM:
                    note_height = keyY;
                    break;
                }

                int in_shift = 0;
                int length_add = 0;

                if (dt == DRAW_NORMAL_LINKED)
                {
                    if (tick_f >= tick_s)
                    {
                        note_width = (tick_f - tick_s) / m_zoom;
                        if (note_width < 1)
                            note_width = 1;
                    }
                    else
                    {
                        note_width = (m_seq->getLength() - tick_s) / m_zoom;
                    }

                }
                else
                {
                    note_width = 16 / m_zoom;
                }

                if (dt == DRAW_NOTE_ON)
                {
                    in_shift = 0;
                    length_add = 2;
                }

                if (dt == DRAW_NOTE_OFF)
                {
                    in_shift = -1;
                    length_add = 1;
                }

                mPen->setColor(Qt::black);

                if (method == 0)
                    mPen->setColor(Qt::darkGray);

                mBrush->setStyle(Qt::SolidPattern);
                mBrush->setColor(Qt::black);
                mPainter->setBrush(*mBrush);
                mPainter->setPen(*mPen);

                switch (editMode)
                {
                case NOTE:
                    //draw outer note boundary (shadow)
                    mPainter->drawRect(note_x,
                                       note_y,
                                       note_width,
                                       note_height);
                    //draw shadow for notes starting before zero
                    if (tick_f < tick_s)
                    {
                        mPainter->setPen(*mPen);
                        mPainter->drawRect(c_keyboard_padding_x,
                                           note_y,
                                           tick_f / m_zoom,
                                           note_height);
                    }
                    break;
                case DRUM:
                    //draw polygon for drum hits
                    QPointF points[4] =
                    {
                        QPointF(note_x - note_height * 0.5,
                                note_y + note_height * 0.5),
                        QPointF(note_x,
                                note_y),
                        QPointF(note_x + note_height * 0.5,
                                note_y + note_height * 0.5),
                        QPointF(note_x,
                                note_y + note_height)
                    };
                    mPainter->drawPolygon(points, 4);
                    break;
                }

                //draw note highlight if there's room
                //always draw them in drum mode
                if (note_width > 3 || editMode == DRUM)
                {
                    //red noted selected, otherwise plain white
                    if (selected)
                        mBrush->setColor(Qt::red);
                    else
                        mBrush->setColor(Qt::white);

                    mPainter->setBrush(*mBrush);
                    if (method == 1)
                    {
                        switch (editMode)
                        {
                        case NOTE:
                            //if the note fits in the grid
                            if (tick_f >= tick_s)
                            {
                                //draw inner note (highlight)
                                mPainter->drawRect(note_x + in_shift,
                                                   note_y,
                                                   note_width - 1 + length_add,
                                                   note_height - 1);
                            }
                            else
                            {
                                mPainter->drawRect(note_x + in_shift,
                                                   note_y,
                                                   note_width,
                                                   note_height - 1);

                                mPainter->drawRect(c_keyboard_padding_x,
                                                   note_y,
                                                   (tick_f / m_zoom) - 3 + length_add,
                                                   note_height - 1);
                            }
                            break;
                        case DRUM:
                            //draw inner note (highlight)
                            QPointF points[4] =
                            {
                                QPointF(note_x - note_height * 0.5,
                                        note_y + note_height * 0.5),
                                QPointF(note_x,
                                        note_y),
                                QPointF(note_x + note_height * 0.5 - 1,
                                        note_y + note_height * 0.5),
                                QPointF(note_x,
                                        note_y + note_height - 1)
                            };
                            mPainter->drawPolygon(points, 4);
                            break;
                        }
                    }
                }
            }
        }
    }

    /* draw selections */
    int x, y, w, h;

    //painter reset
    mBrush->setStyle(Qt::NoBrush);
    mPainter->setBrush(*mBrush);

    if (m_selecting  ||  m_moving
            || m_paste ||  m_growing)
        mPen->setStyle(Qt::SolidLine);

    if (m_selecting)
    {

        xy_to_rect(m_drop_x,
                   m_drop_y,
                   m_current_x,
                   m_current_y,
                   &x, &y,
                   &w, &h);

        m_old.x = x;
        m_old.y = y;
        m_old.width = w;
        m_old.height = h + keyY;

        mPen->setColor(Qt::black);
        mPainter->setPen(*mPen);
        mPainter->drawRect(x + c_keyboard_padding_x,
                           y,
                           w,
                           h + keyY);
    }

    if (m_moving || m_paste)
    {
        int delta_x = m_current_x - m_drop_x;
        int delta_y = m_current_y - m_drop_y;

        x = m_selected.x + delta_x;
        y = m_selected.y + delta_y;

        mPen->setColor(Qt::black);
        mPainter->setPen(*mPen);
        switch (editMode)
        {
        case NOTE:
            mPainter->drawRect(x + c_keyboard_padding_x,
                               y,
                               m_selected.width,
                               m_selected.height);
            break;
        case DRUM:
            mPainter->drawRect(x - note_height * 0.5 + c_keyboard_padding_x,
                               y,
                               m_selected.width + note_height,
                               m_selected.height);
            break;
        }
        m_old.x = x;
        m_old.y = y;
        m_old.width = m_selected.width;
        m_old.height = m_selected.height;
    }

    if (m_growing)
    {
        int delta_x = m_current_x - m_drop_x;
        int width = delta_x + m_selected.width;

        if (width < 1)
            width = 1;

        x = m_selected.x;
        y = m_selected.y;

        mPen->setColor(Qt::black);
        mPainter->setPen(*mPen);
        mPainter->drawRect(x + c_keyboard_padding_x,
                           y,
                           width,
                           m_selected.height);

        m_old.x = x;
        m_old.y = y;
        m_old.width = width;
        m_old.height = m_selected.height;

    }
    delete mPainter;
    delete mBrush;
    delete mPen;
}

void qseqroll::mousePressEvent(QMouseEvent *event)
{
    int numsel;

    long tick_s;
    long tick_f;
    int note;
    int note_l;

    bool needs_update = false;

    int norm_x, norm_y, snapped_x, snapped_y;

    snapped_x = norm_x = event->x() - c_keyboard_padding_x;
    snapped_y = norm_y = event->y();

    snap_x(&snapped_x);
    snap_y(&snapped_y);

    /* y is always snapped */
    m_current_y = m_drop_y = snapped_y;

    if (m_paste)
    {
        convert_xy(snapped_x, snapped_y, &tick_s, &note);
        m_paste = false;
        m_seq->push_undo();
        m_seq->paste_selected(tick_s, note);

        needs_update = true;

    }
    else
    {
        if (event->button() == Qt::LeftButton)
        {
            //for selection, use non-snapped x
            m_current_x = m_drop_x = norm_x;

            //convert screen coords to ticks
            switch (editMode)
            {
            case NOTE:
                convert_xy(m_drop_x, m_drop_y, &tick_s, &note);
                tick_f = tick_s;
                break;
            case DRUM: //add padding for selecting drum hits
                convert_xy(m_drop_x - note_height * 0.5, m_drop_y, &tick_s, &note);
                convert_xy(m_drop_x + note_height * 0.5, m_drop_y, &tick_f, &note);
                break;
            }
            if (m_adding)   //painting new notes
            {
                /* start the paint job */
                m_painting = true;

                /* adding, snapped x */
                m_current_x = m_drop_x = snapped_x;
                convert_xy(m_drop_x, m_drop_y, &tick_s, &note);

                // test if a note is already there
                // fake select, if so, no add
                if (! m_seq->select_note_events(tick_s, note,
                                                tick_s, note,
                                                sequence::e_would_select))
                {

                    /* add note, length = little less than snap */
                    m_seq->push_undo();
                    m_seq->add_note(tick_s, m_note_length - 2, note, true);

                    needs_update = true;
                }

            }
            else // we're selecting
            {
                //if nothing's already selected in the range
                bool isSelected = false;
                switch (editMode)
                {
                case NOTE:
                    isSelected = m_seq->select_note_events(tick_s, note,
                                                           tick_f, note,
                                                           sequence::e_is_selected);
                    break;
                case DRUM:
                    isSelected = m_seq->select_note_events(tick_s, note,
                                                           tick_f, note,
                                                           sequence::e_is_selected_onset);
                    break;
                }

                if (!isSelected)
                {
                    if (!(event->modifiers() & Qt::ControlModifier))
                        m_seq->unselect();

                    /* on direct click select only one event */
                    switch (editMode)
                    {
                    case NOTE:
                        numsel = m_seq->select_note_events(tick_s, note, tick_f, note,
                                                           sequence::e_select_single);
                        break;
                    case DRUM:
                        numsel = m_seq->select_note_events(tick_s, note, tick_f, note,
                                                           sequence::e_select_onset_single);
                        break;

                    }
                    /* none selected, start selection box */
                    if (numsel == 0)
                    {
                        if (event->button() == Qt::LeftButton)
                            m_selecting = true;
                    }
                    else
                    {
                        needs_update = true;
                    }
                }

                isSelected = false;

                switch (editMode)
                {
                case NOTE:
                    isSelected = m_seq->select_note_events(tick_s, note,
                                                           tick_f, note,
                                                           sequence::e_is_selected);
                    break;
                case DRUM:
                    isSelected = m_seq->select_note_events(tick_s, note,
                                                           tick_f, note,
                                                           sequence::e_is_selected_onset);
                    break;
                }

                if (isSelected)
                {
                    // moving - left click only
                    if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::ControlModifier))
                    {
                        m_moving_init = true;
                        needs_update = true;

                        switch (editMode)
                        {
                        case NOTE: //take note lengths into account
                            m_seq->get_selected_box(&tick_s, &note,
                                                    &tick_f, &note_l);
                            break;
                        case DRUM: //ignore note lengths
                            m_seq->get_onsets_selected_box(&tick_s, &note,
                                                           &tick_f, &note_l);
                            break;
                        }

                        convert_tn_box_to_rect(tick_s, tick_f, note, note_l,
                                               &m_selected.x,
                                               &m_selected.y,
                                               &m_selected.width,
                                               &m_selected.height);

                        /* save offset that we get from the snap above */
                        int adjusted_selected_x = m_selected.x;
                        snap_x(&adjusted_selected_x);
                        m_move_snap_offset_x = (m_selected.x - adjusted_selected_x);

                        m_current_x = m_drop_x = snapped_x;
                    }

                    /* middle mouse button, or left-ctrl click (for 2button mice) */
                    if ((event->button() == Qt::MiddleButton ||
                            (event->button() == Qt::LeftButton &&
                             (event->modifiers() & Qt::ControlModifier)))
                            && editMode == NOTE)
                    {

                        m_growing = true;

                        /* get the box that selected elements are in */
                        m_seq->get_selected_box(&tick_s, &note,
                                                &tick_f, &note_l);

                        convert_tn_box_to_rect(tick_s, tick_f, note, note_l,
                                               &m_selected.x,
                                               &m_selected.y,
                                               &m_selected.width,
                                               &m_selected.height);

                    }
                }
            }
        }

        /*     right mouse button      */
        if (event->button() == Qt::RightButton)
        {
            set_adding(true);
        }

    }

    //set seq dirty if something's changed
    if (needs_update)
    {
        m_seq->set_dirty();
    }

}

void qseqroll::mouseReleaseEvent(QMouseEvent *event)
{
    long tick_s; //start of tick window
    long tick_f; //end of tick window
    int note_h;  //highest note in window
    int note_l;  //lowest note in window
    int x, y, w, h; //window dimensions

    bool needs_update = false;

    m_current_x = event->x() - c_keyboard_padding_x;
    m_current_y = event->y();

    snap_y(&m_current_y);

    if (m_moving)
        snap_x(&m_current_x);

    int delta_x = m_current_x - m_drop_x;
    int delta_y = m_current_y - m_drop_y;

    long delta_tick;
    int delta_note;

    if (event->button() == Qt::LeftButton)
    {

        if (m_selecting)
        {

            xy_to_rect(m_drop_x,
                       m_drop_y,
                       m_current_x,
                       m_current_y,
                       &x, &y,
                       &w, &h);

            switch (editMode)
            {
            case NOTE:

                convert_xy(x,     y, &tick_s, &note_h);
                convert_xy(x + w, y + h, &tick_f, &note_l);
                m_seq->select_note_events(tick_s, note_h,
                                          tick_f, note_l,
                                          sequence::e_select);
                break;
            case DRUM:
                convert_xy(x,     y, &tick_s, &note_h);
                convert_xy(x + w, y + h, &tick_f, &note_l);
                m_seq->select_note_events(tick_s, note_h,
                                          tick_f, note_l,
                                          sequence::e_select_onset);
                break;
            }

            needs_update = true;
        }

        if (m_moving)
        {

            /* adjust for snap */
            delta_x -= m_move_snap_offset_x;

            /* convert deltas into screen corridinates */
            convert_xy(delta_x, delta_y, &delta_tick, &delta_note);

            /* since delta_note was from delta_y, it will be filpped
               ( delta_y[0] = note[127], etc.,so we have to adjust */
            delta_note = delta_note - (c_num_keys - 1);

            m_seq->push_undo();
            m_seq->move_selected_notes(delta_tick, delta_note);
            needs_update = true;
        }

    }

    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton)
    {

        if (m_growing)
        {

            /* convert deltas into screen corridinates */
            convert_xy(delta_x, delta_y, &delta_tick, &delta_note);
            m_seq->push_undo();

            if (event->modifiers() & Qt::ShiftModifier)
            {
                m_seq->stretch_selected(delta_tick);
            }
            else
            {
                m_seq->grow_selected(delta_tick);
            }

            needs_update = true;
        }
    }

    if (event->button() == Qt::RightButton)
    {
        set_adding(false);
    }

    /* turn off */
    m_selecting = false;
    m_moving = false;
    m_growing = false;
    m_paste = false;
    m_moving_init = false;
    m_painting = false;

    m_seq->unpaint_all();

    /* if they clicked, something changed */
    if (needs_update)
    {
        m_seq->set_dirty();
    }
}

void qseqroll::mouseMoveEvent(QMouseEvent *event)
{
    m_current_x = event->x() - c_keyboard_padding_x;
    m_current_y = event->y();

    int note;
    long tick;

    if (m_moving_init)
    {
        m_moving_init = false;
        m_moving = true;
    }


    snap_y(&m_current_y);
    convert_xy(0, m_current_y, &tick, &note);

    if (m_selecting || m_moving || m_growing || m_paste)
    {

        if (m_moving || m_paste)
        {
            snap_x(&m_current_x);
        }

    }

    if (m_painting)
    {
        snap_x(&m_current_x);
        convert_xy(m_current_x, m_current_y, &tick, &note);

        m_seq->add_note(tick, m_note_length - 2, note, true);
    }
}

void qseqroll::keyPressEvent(QKeyEvent *event)
{

    if (event->key() == Qt::Key_Delete ||
            event->key() == Qt::Key_Backspace)
    {
        //delete selected notes
        m_seq->push_undo();
        m_seq->mark_selected();
        m_seq->remove_marked();
        return;
    }

    if (!is_pattern_playing)
    {
        if (event->key() == Qt::Key_Home)
        {
            m_seq->set_orig_tick(0);
            return;
        }
        if (event->key() == Qt::Key_Left)
        {
            m_seq->set_orig_tick(m_seq->get_last_tick() - m_snap);
            return;
        }
        if (event->key() == Qt::Key_Right)
        {
            m_seq->set_orig_tick(m_seq->get_last_tick() + m_snap);
            return;
        }
    }

    //Ctrl + ... events
    if (event->modifiers() & Qt::ControlModifier)
    {
        switch (event->key())
        {
        case Qt::Key_X:
            m_seq->push_undo();
            m_seq->copy_selected();
            m_seq->mark_selected();
            m_seq->remove_marked();
            return;
            break;

        case Qt::Key_C:
            m_seq->copy_selected();
            return;
            break;

        case Qt::Key_V:
            start_paste();
            return;
            break;

        case Qt::Key_Z:
            if (event->modifiers() & Qt::ShiftModifier)
            {
                m_seq->pop_redo();
                return;
            }
            else
                m_seq->pop_undo();
            return;
            break;

        case Qt::Key_A:
            m_seq->select_all();
            return;
            break;
        }
    }

    //if we reach this point, the key isn't relevant to us
    //ignore it so the event is passed to the parent
    event->ignore();

}
void qseqroll::keyReleaseEvent(QKeyEvent *event)
{

}

QSize qseqroll::sizeHint() const
{
    return QSize(m_seq->getLength() / m_zoom + 100 + c_keyboard_padding_x, keyAreaY + 1);
}

void qseqroll::snap_y(int *a_y)
{
    *a_y = *a_y - (*a_y % keyY);
}

void qseqroll::snap_x(int *a_x)
{
    //snap = number pulses to snap to
    //m_zoom = number of pulses per pixel
    //so snap / m_zoom  = number pixels to snap to
    int mod = (m_snap / m_zoom);
    if (mod <= 0)
        mod = 1;

    *a_x = *a_x - (*a_x % mod);
}

void qseqroll::convert_xy(int a_x, int a_y, long *a_tick, int *a_note)
{
    *a_tick = a_x * m_zoom;
    *a_note = (keyAreaY - a_y - 2) / keyY;
}

void qseqroll::convert_tn(long a_ticks, int a_note, int *a_x, int *a_y)
{
    *a_x = a_ticks /  m_zoom;
    *a_y = keyAreaY - ((a_note + 1) * keyY) - 1;
}

void qseqroll::xy_to_rect(int a_x1, int a_y1, int a_x2, int a_y2,
                          int *a_x, int *a_y, int *a_w, int *a_h)
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

void qseqroll::convert_tn_box_to_rect(long a_tick_s, long a_tick_f,
                                      int a_note_h,  int a_note_l,
                                      int *a_x,      int *a_y,
                                      int *a_w,      int *a_h)
{
    int x1, y1, x2, y2;

    /* convert box to X,Y values */
    convert_tn(a_tick_s, a_note_h, &x1, &y1);
    convert_tn(a_tick_f, a_note_l, &x2, &y2);

    xy_to_rect(x1, y1, x2, y2, a_x, a_y, a_w, a_h);

    *a_h += keyY;
}

void qseqroll::set_adding(bool a_adding)
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

int qseqroll::length() const
{
    return m_note_length;
}

void qseqroll::set_note_length(int length)
{
    m_note_length = length;
}

void qseqroll::set_snap(int snap)
{
    m_snap = snap;
}

void qseqroll::start_paste()
{
    long tick_s;
    long tick_f;
    int note_h;
    int note_l;

    snap_x(&m_current_x);
    snap_y(&m_current_x);

    m_drop_x = m_current_x;
    m_drop_y = m_current_y;

    m_paste = true;

    /* get the box that selected elements are in */
    m_seq->get_clipboard_box(&tick_s, &note_h,
                             &tick_f, &note_l);

    convert_tn_box_to_rect(tick_s, tick_f, note_h, note_l,
                           &m_selected.x,
                           &m_selected.y,
                           &m_selected.width,
                           &m_selected.height);

    /* adjust for clipboard being shifted to tick 0 */
    m_selected.x += m_drop_x;
    m_selected.y += (m_drop_y - m_selected.y);
}

void qseqroll::zoom_in()
{
    if (m_zoom > 1)
        m_zoom *= 0.5;
}

void qseqroll::zoom_out()
{
    if (m_zoom < 32)
        m_zoom *= 2;
}

void qseqroll::updateEditMode(edit_mode_e mode)
{
    editMode = mode;
}
