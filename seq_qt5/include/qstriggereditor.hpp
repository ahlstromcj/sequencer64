#ifndef EDITEVENTTRIGGERS_HPP
#define EDITEVENTTRIGGERS_HPP

#include "sequence.hpp"
#include "qseqdata.hpp"

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QPen>

 */
 */ \brief The qstriggereditor class
 */
 */ Displays the triggers for MIDI events
 */ e.g. Modwheel, pitchbend etc

class qstriggereditor : public QWidget
{
    Q_OBJECT

public:
    explicit qstriggereditor(sequence *a_seq,
                             qseqdata *a_seqdata_wid,
                             QWidget *parent = 0,
                             int keyHeight = 12);
    void zoom_in();
    void zoom_out();
    void set_data_type(midibyte a_status,
                       midibyte a_control);

protected:
    //override painting event to draw on the frame
    void paintEvent(QPaintEvent *);

    //override mouse events for interaction
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);

    //override keyboard events for interaction
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);

    //override the sizehint to set our own defaults
    QSize sizeHint() const;

signals:

public slots:

private:
    /* checks mins / maxes..  the fills in x,y
       and width and height */
    void x_to_w(int a_x1, int a_x2,
                int *a_x, int *a_w);
    void start_paste();
    void convert_x(int a_x, long *a_tick);
    void convert_t(long a_ticks, int *a_x);
    void drop_event(long a_tick);
    void snap_y(int *a_y);
    void snap_x(int *a_x);
    void set_adding(bool a_adding);

    sequence    *m_seq;
    qseqdata *m_seqdata_wid;

    QPen        *mPen;
    QBrush      *mBrush;
    QPainter    *mPainter;
    QFont        mFont;
    QRect       *m_old;
    QRect       *m_selected;
    QTimer      *mTimer;

    /* one pixel == m_zoom ticks */
    int          m_zoom;
    int          m_snap;

    int m_window_x, m_window_y;
    int keyY;

    /* when highlighting a bunch of events */
    bool m_selecting;
    bool m_moving_init;
    bool m_moving;
    bool m_growing;
    bool m_painting;
    bool m_paste;
    bool m_adding;

    /* where the dragging started */
    int m_drop_x;
    int m_drop_y;
    int m_current_x;
    int m_current_y;

    int m_move_snap_offset_x;

    /* what is the data window currently editing ? */
    midibyte m_status;
    midibyte m_cc;
};

#endif // EDITEVENTTRIGGERS_HPP
