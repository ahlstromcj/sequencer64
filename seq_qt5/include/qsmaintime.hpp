#ifndef BEATINDICATOR_HPP
#define BEATINDICATOR_HPP

#include <QWidget>
#include <QPainter>
#include <QDebug>
#include <QTimer>

#include "perform.hpp"

 */
 */ \brief The qsmaintime class
 */
 */ A beat indicator widget

class qsmaintime : public QWidget
{
    Q_OBJECT

public:
    qsmaintime(QWidget *parent,
               perform *perf,
               int beats_per_measure,
               int beat_width);

    ~qsmaintime();

    int get_beats_per_measure() const;
    void set_beats_per_measure(int beats_per_measure);

    int get_beat_width() const;
    void setbeat_width(int beat_width);

    //start the redraw timer
    void startRedrawTimer();

    bool getPlaying() const;
    void setPlaying(bool mPlaying);

protected:
    //override painting event to draw on the frame
    void paintEvent(QPaintEvent *event);

    //override the sizehint to set our own defaults
    QSize sizeHint() const;

private:
    perform     * const m_main_perf;

    QPainter    *mPainter;
    QPen        *mPen;
    QBrush      *mBrush;
    QColor      *mColour;
    QFont        mFont;

    int         m_beats_per_measure;
    int         m_beat_width;
    int         lastMetro;
    int         alpha;

};

#endif // BEATINDICATOR_HPP
