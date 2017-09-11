#ifndef SONGSEQUENCEGRID_HPP
#define SONGSEQUENCEGRID_HPP

#include <QWidget>
#include <QTimer>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QMouseEvent>

#include "Globals.hpp"

#include "globals.h"
#include "perform.hpp"
#include "seq24Rect.hpp"

const int c_perfroll_background_x = (c_ppqn * 4 * 16) / c_perf_scale_x;
const int c_perfroll_size_box_w = 3;
const int c_perfroll_size_box_click_w = c_perfroll_size_box_w + 1 ;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq64
{
    class perform;

/**
 * The grid in the song editor
 * for setting out sequences
 */

class qperfroll : public QWidget
{
    Q_OBJECT

public:
    explicit qperfroll(perform *a_perf,
                       QWidget *parent);

    int getSnap() const;
    void set_snap(int getSnap);

    void set_guides(int a_snap, int a_measure, int a_beat);
    void update_sizes();
    void increment_size();

    void zoom_in();
    void zoom_out();

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

public slots:
    void undo();
    void redo();

private:

    void xy_to_rect(int a_x1, int a_y1, int a_x2, int a_y2,
                    int *a_x, int *a_y, int *a_w, int *a_h);
    void convert_xy(int a_x, int a_y, long *a_ticks, int *a_seq);
    void convert_x(int a_x, long *a_ticks);
    void snap_x(int *a_x);
    void snap_y(int *a_y);
    void half_split_trigger(int a_sequence, long a_tick);
    void set_adding(bool a_adding);

    perform *mPerf;

    QPen        *mPen;
    QBrush      *mBrush;
    QPainter    *mPainter;
    QFont        mFont;
    QTimer      *mTimer;

    seq24Rect m_old;

    int     m_snap;
    int     m_measure_length;
    int     m_beat_length;
    int     m_roll_length_ticks;
    int     m_drop_x, m_drop_y;
    int     m_current_x, m_current_y;
    int     m_drop_sequence;
    int     zoom;

    //sequence selection
    long    tick_s; //start of tick window
    long    tick_f; //end of tick window
    int     seq_h;  //highest seq in window
    int     seq_l;  //lowest seq in window

    long    m_drop_tick;
    long    m_drop_tick_trigger_offset; //how far in ticks we clicked from
    //the start of this trigger
    long    mLastTick; //tick we we're using at last mouse event
    bool    m_sequence_active[qc_total_seqs];
    bool    m_moving;
    bool    mBoxSelect;
    bool    m_growing;
    bool    m_grow_direction;
    bool    m_adding;
    bool    m_adding_pressed;

};

}           // namespace seq64

#endif // SONGSEQUENCEGRID_HPP
