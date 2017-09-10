#ifndef SONGSEQUENCENAMES_HPP
#define SONGSEQUENCENAMES_HPP

#include <perform.hpp>
#include <sequence.hpp>
#include <globals.h>

#include <QWidget>
#include <QPainter>
#include <QPen>

 */
 */ \brief The qperfnames class
 */
 */ Sequence labels for the side of the song editor

class qperfnames : public QWidget
{
    Q_OBJECT
public:
    explicit qperfnames(perform *a_perf,
                        QWidget *parent);
    ~qperfnames();

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
    perform *mPerf;

    QTimer      *mTimer;
    QPen        *mPen;
    QBrush      *mBrush;
    QPainter    *mPainter;
    QFont        mFont;

    bool         m_sequence_active[c_total_seqs];
};

#endif // SONGSEQUENCENAMES_HPP
