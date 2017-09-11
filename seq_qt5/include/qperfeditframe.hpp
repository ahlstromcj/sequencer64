#ifndef SONGFRAME_HPP
#define SONGFRAME_HPP

#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include <qmath.h>

#include "perform.hpp"
#include "qperfroll.hpp"
#include "qperfnames.hpp"
#include "qperftime.hpp"

namespace Ui
{
    class qperfeditframe;
}


class qperfeditframe : public QFrame
{
    Q_OBJECT

public:
    explicit qperfeditframe(seq64::perform *a_perf,
                            QWidget *parent);
    ~qperfeditframe();

    int get_beat_width() const;
    void set_beat_width(int a_beat_width);

    int get_beats_per_measure() const;
    void set_beats_per_measure(int a_beats_per_measure);

    //calls update geometry on elements to react to changes
    //in MIDI file sizes
    void update_sizes();

private:
    /* set snap to in pulses */
    int m_snap;

    int mbeats_per_measure;
    int mbeat_width;

    void set_snap(int a_snap);

    void set_guides();

    void grow();

    Ui::qperfeditframe *ui;

    QGridLayout     *m_layout_grid;
    QScrollArea     *m_scroll_area;
    QWidget         *mContainer;
    QPalette        *m_palette;

    seq64::perform   *m_mainperf;
    seq64::qperfroll  *m_perfroll;
    seq64::qperfnames *m_perfnames;
    seq64::qperftime       *m_perftime;

private slots:
    void updateGridSnap(int snapIndex);
    void zoom_in();
    void zoom_out();
    void markerCollapse();
    void markerExpand();
    void markerExpandCopy();
    void markerLoop(bool loop);
};

#endif // SONGFRAME_HPP
