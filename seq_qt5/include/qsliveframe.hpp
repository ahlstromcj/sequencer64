#ifndef LIVEFRAME_HPP
#define LIVEFRAME_HPP

#include <QFrame>
#include <QPainter>
#include <QDebug>
#include <QMenu>
#include <QTimer>
#include <QMessageBox>

#include "perform.hpp"
#include "sequence.hpp"

namespace Ui
{
    class qsliveframe;
}

class qsliveframe : public QFrame
{
    Q_OBJECT

public:
    explicit qsliveframe(QWidget *parent = 0, perform *perf = 0);

    ~qsliveframe();

    //redraw frame contents
    void redraw();

    //set bank of sequences displayed
    void setBank(int newBank);

protected:
    //override painting event to draw on the frame
    void paintEvent(QPaintEvent *event);

    //override mouse events for interaction
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void mouseDoubleClickEvent(QMouseEvent *);

    //override keyboard events for interaction
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);

private:
    //draw a single sequence thumbnail at index
    void drawSequence(int a_seq);

    //draw all sequences
    void drawAllSequences();

    //used to grab std::string bank name and
    //convert it to QString for display
    void updateInternalBankName();

    //converts the XY coordinates of a click into a seq ID
    int seqIDFromClickXY(int click_x, int click_y);

    /* set/unset sequences with key presses */
    void sequence_key(int a_seq);

    Ui::qsliveframe *ui;

    seq64::perform *mPerf;
    seq64::sequence    m_moving_seq;
    seq64::sequence    mSeqClipboard;

    QPainter    *mPainter;
    QBrush      *mBrush;
    QPen        *mPen;
    QMenu       *mPopup;
    QFont        mFont;
    QTimer      *mRedrawTimer;
    QMessageBox *mMsgBoxNewSeqCheck;

    int     m_bank_id;

    //thumbnail dimensions
    int     thumbW, thumbH;

    //internal seq MIDI preview dimensions
    int     previewW, previewH;

    //beat pulsing
    int         lastMetro;
    int         alpha;

    //mouse interaction
    int     mCurrentSeq;
    int     mOldSeq;
    bool    mButtonDown;
    bool    mMoving; //are we moving bewteen slots
    bool    mAddingNew; /*we can add a new seq here,
                          wait for double click*/
    long    m_last_tick_x[c_max_sequence];
    bool    m_last_playing[c_max_sequence];
    bool    mCanPaste;

private slots:
    void updateBank(int newBank);
    void updateBankName();
    void newSeq();
    void editSeq();
    void copySeq();
    void cutSeq();
    void pasteSeq();
    void deleteSeq();
    void setColourWhite();
    void setColourRed();
    void setColourGreen();
    void setColourBlue();
    void setColourYellow();
    void setColourPurple();
    void setColourPink();
    void setColourOrange();

signals:
    void callEditor(int seqId); //call the editor tab on the given seq
};

#endif // LIVEFRAME_HPP
