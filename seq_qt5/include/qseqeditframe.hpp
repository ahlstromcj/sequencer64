#ifndef EDITFRAME_HPP
#define EDITFRAME_HPP

#include "perform.hpp"
#include "sequence.hpp"
#include "qseqkeys.hpp"
#include "qseqtime.hpp"
#include "qseqroll.hpp"
#include "qseqdata.hpp"
#include "qstriggereditor.hpp"

#include <QFrame>
#include <QLayout>
#include <qmath.h>
#include <QScrollBar>
#include <QScrollArea>
#include <QPalette>
#include <QMenu>

namespace Ui
{
class qseqeditframe;
}

 */
 */ \brief The qseqeditframe class
 */
 */ Holds tools for editing an individual MIDI sequence

class qseqeditframe : public QFrame
{
    Q_OBJECT

public:
    explicit qseqeditframe(QWidget *parent,
                           perform *perf,
                           int mSeqId);
    ~qseqeditframe();

    void updateDrawGeometry();

    //set a new editing mode
    void setEditorMode(edit_mode_e mode);

private:
    Ui::qseqeditframe   *ui;

    QGridLayout     *m_layout_grid;
    QScrollArea     *m_scroll_area;
    QWidget         *mContainer;
    QPalette        *m_palette;
    QMenu           *mPopup;

    sequence    * const mSeq;
    perform * const mPerformance;

    qseqkeys          *mKeyboard;
    qseqtime       *mTimeBar;
    qseqroll      *mNoteGrid;
    qseqdata   *mEventValues;
    qstriggereditor *mEventTriggers;

    /* set snap to in pulses, off = 1 */
    int         mSnap;
    edit_mode_e editMode;
    int         mSeqId;

private slots:
    void updateSeqName();
    void updateGridSnap(int snapIndex);
    void updatemidibus(int newIndex);
    void updateMidiChannel(int newIndex);
    void undo();
    void redo();
    void showTools();
    void updateNoteLength(int newIndex);
    void zoom_in();
    void zoom_out();
    void updateKey(int newIndex);
    void updateSeqLength();
    void updateScale(int newIndex);
    void updateBackgroundSeq(int newIndex);
    void toggleEditorMode();
    void updateRecVol();
    void toggleMidiPlay(bool newVal);
    void toggleMidiThru(bool newVal);
    void toggleMidiQRec(bool newVal);
    void toggleMidiRec(bool newVal);
    void selectAllNotes();
    void inverseNoteSelection();
    void quantizeNotes();
    void tightenNotes();
    void transposeNotes();
};

#endif // EDITFRAME_HPP
