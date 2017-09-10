#include "qperfeditframe.hpp"
#include "ui_qperfeditframe.h"

qperfeditframe::qperfeditframe(perform *a_perf,
                               QWidget *parent):
    QFrame(parent),
    ui(new Ui::qperfeditframe),
    m_mainperf(a_perf)
{
    ui->setupUi(this);

    setSizePolicy(QSizePolicy::Expanding,
                  QSizePolicy::Expanding);

    // fill options for grid snap combo box and set default
    for (int i = 0; i < 6; i++)
    {
        QString combo_text = "1/" + QString::number(pow(2, i));
        ui->cmbGridSnap->insertItem(i, combo_text);
    }
    ui->cmbGridSnap->setCurrentIndex(3);

    m_scroll_area = new QScrollArea(this);
    ui->vbox_centre->addWidget(m_scroll_area);

    mContainer = new QWidget(m_scroll_area);
    m_layout_grid = new QGridLayout(mContainer);
    mContainer->setLayout(m_layout_grid);

    m_palette = new QPalette();
    m_palette->setColor(QPalette::Background, Qt::darkGray);
    mContainer->setPalette(*m_palette);

    m_perfnames = new qperfnames(m_mainperf, mContainer);
    m_perfroll = new qperfroll(m_mainperf, mContainer);
    m_perftime = new qperftime(m_mainperf, mContainer);

    m_layout_grid->setSpacing(0);
    m_layout_grid->addWidget(m_perfnames, 1, 0, 1, 1);
    m_layout_grid->addWidget(m_perftime, 0, 1, 1, 1);
    m_layout_grid->addWidget(m_perfroll, 1, 1, 1, 1);
    m_layout_grid->setAlignment(m_perfroll, Qt::AlignTop);

    m_scroll_area->setWidget(mContainer);

    connect(ui->cmbGridSnap,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateGridSnap(int)));

    connect(ui->btnUndo,
            SIGNAL(clicked(bool)),
            m_perfroll,
            SLOT(undo()));

    connect(ui->btnRedo,
            SIGNAL(clicked(bool)),
            m_perfroll,
            SLOT(redo()));

    connect(ui->cmbGridSnap,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(updateGridSnap(int)));

    connect(ui->btnZoomIn,
            SIGNAL(clicked(bool)),
            this,
            SLOT(zoom_in()));

    connect(ui->btnZoomOut,
            SIGNAL(clicked(bool)),
            this,
            SLOT(zoom_out()));

    connect(ui->btnCollapse,
            SIGNAL(clicked(bool)),
            this,
            SLOT(markerCollapse()));

    connect(ui->btnExpand,
            SIGNAL(clicked(bool)),
            this,
            SLOT(markerExpand()));

    connect(ui->btnExpandCopy,
            SIGNAL(clicked(bool)),
            this,
            SLOT(markerExpandCopy()));

    connect(ui->btnLoop,
            SIGNAL(clicked(bool)),
            this,
            SLOT(markerLoop(bool)));

    m_snap = 8;
    mbeats_per_measure = 4;
    mbeat_width = 4;

    set_snap(8);
    set_beats_per_measure(4);
    set_beat_width(4);
}

qperfeditframe::~qperfeditframe()
{
    delete ui;
}

int qperfeditframe::get_beat_width() const
{
    return mbeat_width;
}

void qperfeditframe::updateGridSnap(int snapIndex)
{
    int snap;
    switch (snapIndex)
    {
    case 0:
        snap = 1;
        break;
    case 1:
        snap = 2;
        break;
    case 2:
        snap = 4;
        break;
    case 3:
        snap = 8;
        break;
    case 4:
        snap = 16;
        break;
    case 5:
        snap = 32;
        break;
    }

    m_snap = snap;
    set_guides();
}

void qperfeditframe::set_snap(int a_snap)
{
    char b[10];
    snprintf(b, sizeof(b), "1/%d", a_snap);
    ui->cmbGridSnap->setCurrentText(b);

    m_snap = a_snap;
    set_guides();
}

void qperfeditframe::set_beats_per_measure(int a_beats_per_measure)
{
    mbeats_per_measure = a_beats_per_measure;
    set_guides();
}

int qperfeditframe::get_beats_per_measure() const
{
    return mbeats_per_measure;
}


void qperfeditframe::set_beat_width(int a_beat_width)
{
    mbeat_width = a_beat_width;
    set_guides();
}

void qperfeditframe::set_guides()
{
    long measure_ticks = (c_ppqn * 4) * mbeats_per_measure / mbeat_width;
    long snap_ticks =  measure_ticks / m_snap;
    long beat_ticks = (c_ppqn * 4) / mbeat_width;
    m_perfroll->set_guides(snap_ticks, measure_ticks, beat_ticks);
    m_perftime->set_guides(snap_ticks, measure_ticks);
}

void qperfeditframe::zoom_in()
{
    m_perftime->zoom_in();
    m_perfroll->zoom_in();
}

void qperfeditframe::zoom_out()
{
    m_perftime->zoom_out();
    m_perfroll->zoom_out();
}

void qperfeditframe::update_sizes()
{
    m_perfroll->updateGeometry();
    m_perftime->updateGeometry();
    mContainer->adjustSize();
}

void qperfeditframe::markerCollapse()
{
    m_mainperf->push_trigger_undo();
    m_mainperf->move_triggers(false);
}

void qperfeditframe::markerExpand()
{
    m_mainperf->push_trigger_undo();
    m_mainperf->move_triggers(true);
}

void qperfeditframe::markerExpandCopy()
{
    m_mainperf->push_trigger_undo();
    m_mainperf->copy_triggers();
}

void qperfeditframe::markerLoop(bool loop)
{
    m_mainperf->set_looping(loop);
}
