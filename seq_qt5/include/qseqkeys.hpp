#ifndef EDITKEYS_HPP
#define EDITKEYS_HPP

#include "sequence.hpp"

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QPen>
#include <QSizePolicy>
#include <QMouseEvent>

 *draws the piano keys in the sequence editor

class qseqkeys : public QWidget
{
    Q_OBJECT

public:
    explicit qseqkeys(sequence *a_seq,
                      QWidget *parent = 0,
                      int keyHeight = 12,
                      int keyAreaHeight = 12 * c_num_keys + 1);

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

    //takes a y coordinate and converts it to a note value
    void convert_y(int a_y, int *a_note);

    sequence *m_seq;

    QTimer      *m_timer;
    QPen        *m_pen;
    QBrush      *m_brush;
    QPainter    *m_painter;
    QFont        m_font;

    int m_key;
    int keyY;
    int keyAreaY;

    bool mPreviewing;
    int  mPreviewKey;

};

#endif // EDITKEYS_HPP
