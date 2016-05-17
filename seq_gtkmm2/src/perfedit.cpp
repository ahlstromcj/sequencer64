/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          perfedit.cpp
 *
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       sequencer64 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2016-05-17
 * \license       GNU GPLv2 or above
 *
 */

#include <gtkmm/adjustment.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>
#include <gtkmm/table.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/combo.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/separator.h>
#include <gtkmm/tooltips.h>
#include <gtkmm/arrow.h>
#include <gtkmm/image.h>
#include <sigc++/bind.h>

#include "gdk_basic_keys.h"
#include "gtk_helpers.h"
#include "keystroke.hpp"
#include "perfedit.hpp"
#include "perfnames.hpp"
#include "perfroll.hpp"
#include "perftime.hpp"
#include "settings.hpp"                 /* seq64::choose_ppqn()         */

#include "pixmaps/pause.xpm"
#include "pixmaps/play2.xpm"
#include "pixmaps/snap.xpm"
#include "pixmaps/stop.xpm"
#include "pixmaps/expand.xpm"
#include "pixmaps/collapse.xpm"
#include "pixmaps/loop.xpm"
#include "pixmaps/copy.xpm"
#include "pixmaps/undo.xpm"
#include "pixmaps/down.xpm"
#include "pixmaps/perfedit.xpm"

using namespace Gtk::Menu_Helpers;

namespace seq64
{

/**
 *  Holds a pointer to the first instance of perfedit for the entire
 *  application, once it is created.
 */

static perfedit * gs_perfedit_pointer_0 = nullptr;

/**
 *  Holds a pointer to the second instance of perfedit for the entire
 *  application, once it is created.
 */

static perfedit * gs_perfedit_pointer_1 = nullptr;

/**
 *  This global function in the seq64 namespace calls perfedit ::
 *  draw_sequences(), if the global perfedit objects exist.  It is used by
 *  other objects (seqedit and eventedit) that can modify the currently-edited
 *  sequence shown in the perfedit (song window).
 */

void
update_perfedit_sequences ()
{
    if (not_nullptr(gs_perfedit_pointer_0))
        gs_perfedit_pointer_0->draw_sequences();

    if (not_nullptr(gs_perfedit_pointer_1))
        gs_perfedit_pointer_1->draw_sequences();
}

/**
 *  Principal constructor, has a reference to a perform object.  We've
 *  reordered the pointer members and put them in the initializer list to make
 *  the constructor a bit cleaner.
 *
 * \param p
 *      Refers to the main performance object.
 *
 * \todo
 *      Offload most of the work into an initialization function like
 *      options does.
 */

perfedit::perfedit
(
    perform & p,
    bool second_perfedit,
    int ppqn
) :
    gui_window_gtk2     (p, 750, 500),
    m_peer_perfedit     (nullptr),
    m_table             (manage(new Gtk::Table(6, 3, false))),
    m_vadjust           (manage(new Gtk::Adjustment(0, 0, 1, 1, 1, 1))),
    m_hadjust           (manage(new Gtk::Adjustment(0, 0, 1, 1, 1, 1))),
    m_vscroll           (manage(new Gtk::VScrollbar(*m_vadjust))),
    m_hscroll           (manage(new Gtk::HScrollbar(*m_hadjust))),
    m_perfnames         (manage(new perfnames(perf(), *this, *m_vadjust))),
    m_perfroll
    (
        manage(new perfroll(perf(), *this, *m_hadjust, *m_vadjust, ppqn))
    ),
    m_perftime          (manage(new perftime(perf(), *this, *m_hadjust))),
    m_menu_snap         (manage(new Gtk::Menu())),
    m_image_play        (manage(new PIXBUF_IMAGE(play2_xpm))),
    m_button_snap       (manage(new Gtk::Button())),
    m_entry_snap        (manage(new Gtk::Entry())),
    m_button_stop       (manage(new Gtk::Button())),
    m_button_play       (manage(new Gtk::Button())),
    m_button_loop       (manage(new Gtk::ToggleButton())),
    m_button_expand     (manage(new Gtk::Button())),
    m_button_collapse   (manage(new Gtk::Button())),
    m_button_copy       (manage(new Gtk::Button())),
    m_button_grow       (manage(new Gtk::Button())),
    m_button_undo       (manage(new Gtk::Button())),
    m_button_bpm        (manage(new Gtk::Button())),
    m_entry_bpm         (manage(new Gtk::Entry())),
    m_button_bw         (manage(new Gtk::Button())),
    m_entry_bw          (manage(new Gtk::Entry())),
    m_hbox              (manage(new Gtk::HBox(false, 2))),
    m_hlbox             (manage(new Gtk::HBox(false, 2))),
    m_tooltips          (manage(new Gtk::Tooltips())),  // valgrind complains!
    m_menu_bpm          (manage(new Gtk::Menu())),
    m_menu_bw           (manage(new Gtk::Menu())),
    m_snap              (0),
    m_bpm               (0),
    m_bw                (0),
    m_ppqn              (0),
#ifdef SEQ64_PAUSE_SUPPORT
    m_is_running        (false),
#endif
    m_standard_bpm      (SEQ64_DEFAULT_LINES_PER_MEASURE)   /* 4            */
{
    std::string title = "Sequencer64 - Song Editor";
    if (second_perfedit)
        title += " 2";

    m_ppqn = choose_ppqn(ppqn);
    set_icon(Gdk::Pixbuf::create_from_xpm_data(perfedit_xpm));
    set_title(title);                                       /* caption bar  */
    m_table->set_border_width(2);
    m_hlbox->set_border_width(2);
    m_button_grow->add
    (
        *manage(new Gtk::Arrow(Gtk::ARROW_RIGHT, Gtk::SHADOW_OUT))
    );
    m_button_grow->signal_clicked().connect(mem_fun(*this, &perfedit::grow));
    add_tooltip(m_button_grow, "Increase size of grid.");

    /*
     * Fill the table
     */

    m_table->attach(*m_hlbox, 0, 3, 0, 1,  Gtk::FILL, Gtk::SHRINK, 2, 0);
    m_table->attach(*m_perfnames, 0, 1, 2, 3, Gtk::SHRINK, Gtk::FILL);
    m_table->attach(*m_perftime, 1, 2, 1, 2, Gtk::FILL, Gtk::SHRINK);
    m_table->attach
    (
        *m_perfroll, 1, 2, 2, 3, Gtk::FILL | Gtk::SHRINK, Gtk::FILL | Gtk::SHRINK
    );
    m_table->attach(*m_vscroll, 2, 3, 2, 3, Gtk::SHRINK, Gtk::FILL | Gtk::EXPAND);
    m_table->attach(*m_hbox,  0, 1, 3, 4,  Gtk::FILL, Gtk::SHRINK, 0, 2);
    m_table->attach(*m_hscroll, 1, 2, 3, 4, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK);
    m_table->attach(*m_button_grow, 2, 3, 3, 4, Gtk::SHRINK, Gtk::SHRINK);

    /*
     * To reduce the amount of written code, we now use a static array to
     * initialize some of the menu entries.  We use the same list for the snap
     * menu and for the beat-width menu.  This adds a beat-width of 32 to the
     * beat-width menu.  A new feature!  :-D
     */

#define SET_SNAP    mem_fun(*this, &perfedit::set_snap)
#define SET_BW      mem_fun(*this, &perfedit::set_beat_width)

    static const int s_width_items [] = { 1, 2, 4, 8, 16, 32 };
    static const int s_width_count = sizeof(s_width_items) / sizeof(int);
    for (int si = 0; si < s_width_count; ++si)
    {
        int item = s_width_items[si];
        char fmt[8];
        if (item > 1)
            snprintf(fmt, sizeof fmt, "1/%d", item);
        else
            snprintf(fmt, sizeof fmt, "%d", item);

        m_menu_snap->items().push_back(MenuElem(fmt, sigc::bind(SET_SNAP, item)));
        snprintf(fmt, sizeof fmt, "%d", item);
        m_menu_bw->items().push_back(MenuElem(fmt, sigc::bind(SET_BW, item)));
    }

#define SET_POPUP   mem_fun(*this, &perfedit::popup_menu)

    m_button_snap->add(*manage(new PIXBUF_IMAGE(snap_xpm)));
    m_button_snap->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_snap)
    );
    add_tooltip(m_button_snap, "Grid snap (fraction of measure length).");
    m_entry_snap->set_size_request(40, -1);
    m_entry_snap->set_editable(false);

#define SET_BPB     mem_fun(*this, &perfedit::set_beats_per_bar)

    char b[4];
    for (int i = 0; i < 16; ++i)
    {
        snprintf(b, sizeof b, "%d", i + 1);
        m_menu_bpm->items().push_back(MenuElem(b, sigc::bind(SET_BPB, i + 1)));
    }
    m_button_bpm->add(*manage(new PIXBUF_IMAGE(down_xpm)));

    m_button_bpm->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_bpm)
    );
    add_tooltip
    (
        m_button_bpm, "Time signature: beats per measure, beats per bar."
    );
    m_entry_bpm->set_width_chars(2);
    m_entry_bpm->set_editable(false);

    m_button_bw->add(*manage(new PIXBUF_IMAGE(down_xpm)));  /* beat width */
    m_button_bw->signal_clicked().connect
    (
        sigc::bind<Gtk::Menu *>(SET_POPUP, m_menu_bw)
    );
    add_tooltip(m_button_bw, "Time signature: length of beat.");

    m_entry_bw->set_width_chars(2);
    m_entry_bw->set_editable(false);

    m_button_undo->add(*manage(new PIXBUF_IMAGE(undo_xpm)));
    m_button_undo->signal_clicked().connect(mem_fun(*this, &perfedit::undo));
    add_tooltip(m_button_undo, "Undo.");

    m_button_expand->add(*manage(new PIXBUF_IMAGE(expand_xpm)));
    m_button_expand->signal_clicked().connect(mem_fun(*this, &perfedit::expand));
    add_tooltip(m_button_expand, "Expand between the L and R markers.");

    m_button_collapse->add(*manage(new PIXBUF_IMAGE(collapse_xpm)));
    m_button_collapse->signal_clicked().connect
    (
        mem_fun(*this, &perfedit::collapse)
    );
    add_tooltip(m_button_collapse, "Collapse between the L and R markers.");

    m_button_copy->add(*manage(new PIXBUF_IMAGE(copy_xpm))); /* expand+copy */
    m_button_copy->signal_clicked().connect(mem_fun(*this, &perfedit::copy));
    add_tooltip(m_button_copy, "Expand and copy between the L and R markers.");

    m_button_loop->add(*manage(new PIXBUF_IMAGE(loop_xpm)));
    m_button_loop->signal_toggled().connect
    (
        mem_fun(*this, &perfedit::set_looped)
    );
    add_tooltip(m_button_loop, "Playback looped between the L and R markers.");

    m_button_stop->set_focus_on_click(false);
    m_button_stop->add(*manage(new PIXBUF_IMAGE(stop_xpm)));
    m_button_stop->signal_clicked().connect
    (
        mem_fun(*this, &perfedit::stop_playing)
    );
    add_tooltip(m_button_stop, "Stop playback.");
    m_button_stop->set_sensitive(true);

    m_button_play->set_focus_on_click(false);
    m_button_play->set_image(*m_image_play);
    m_button_play->signal_clicked().connect
    (
        mem_fun(*this, &perfedit::start_playing)
    );
    add_tooltip(m_button_play, "Begin playback at the L marker.");
    m_button_play->set_sensitive(true);

    m_hlbox->pack_end(*m_button_copy , false, false);
    m_hlbox->pack_end(*m_button_expand , false, false);
    m_hlbox->pack_end(*m_button_collapse , false, false);
    m_hlbox->pack_end(*m_button_undo , false, false);
    m_hlbox->pack_start(*m_button_stop , false, false);
    m_hlbox->pack_start(*m_button_play , false, false);
    m_hlbox->pack_start(*m_button_loop , false, false);
    m_hlbox->pack_start(*(manage(new Gtk::VSeparator())), false, false, 4);
    m_hlbox->pack_start(*m_button_bpm , false, false);
    m_hlbox->pack_start(*m_entry_bpm , false, false);
    m_hlbox->pack_start(*(manage(new Gtk::Label("/"))), false, false, 4);
    m_hlbox->pack_start(*m_button_bw , false, false);
    m_hlbox->pack_start(*m_entry_bw , false, false);
    m_hlbox->pack_start(*(manage(new Gtk::Label("x"))), false, false, 4);
    m_hlbox->pack_start(*m_button_snap , false, false);
    m_hlbox->pack_start(*m_entry_snap , false, false);
    add(*m_table);

    /*
     * Here, the set_snap call depends on the others being done first.  These
     * calls also depend upon the values being set to bogus (0) values in the
     * initializer list, otherwise no change will occur, and the items won't
     * be displayed.
     */

    set_beats_per_bar(SEQ64_DEFAULT_BEATS_PER_MEASURE); /* time-sig numerator   */
    set_beat_width(SEQ64_DEFAULT_BEAT_WIDTH);           /* time-sig denominator */
    set_snap(SEQ64_DEFAULT_PERFEDIT_SNAP);

    /*
     * Log the pointer to the appropriate perfedit object, if not already
     * done.
     */

    if (second_perfedit)
    {
        if (is_nullptr(gs_perfedit_pointer_1))
            gs_perfedit_pointer_1 = this;
    }
    else
    {
        if (is_nullptr(gs_perfedit_pointer_0))
            gs_perfedit_pointer_0 = this;
    }
}

/**
 *  This rote constructor does nothing.  We're going to have to run the
 *  application through valgrind to make sure that nothing is left behind.
 */

perfedit::~perfedit ()
{
    // Empty body
}

/**
 *  Helper wrapper for calling perfroll::queue_draw() for one or both
 *  perfedits.  Note that we call the children's queue_draw() functions, not
 *  enqueue_draw(), otherwise we'll get stack overflow.
 *
 * \param forward
 *      If true (the default), pass the call to the peer.  When passing this
 *      call to the peer, this parameter is set to false to prevent an
 *      infinite loop and the resultant stack overflow.
 */

void
perfedit::enqueue_draw (bool forward)
{
    m_perfroll->queue_draw();
    m_perfnames->queue_draw();
    m_perftime->queue_draw();
    if (forward && not_nullptr(m_peer_perfedit))
        m_peer_perfedit->enqueue_draw(false);
}

/**
 *  Implement the undo feature (Ctrl-Z).  We pop an Undo trigger, and then
 *  ask the perfroll to queue up a (re)drawing action.
 */

void
perfedit::undo ()
{
    perf().pop_trigger_undo();
    enqueue_draw();
}

/**
 *  Implement the collapse action.  This action removes all events between
 *  the L and R (left and right) markers.  This action is preceded by
 *  pushing an Undo operation in the perform object, not moving its
 *  triggers (they go away), and telling the perfroll to redraw.
 */

void
perfedit::collapse ()
{
    perf().collapse();
    enqueue_draw();
}

/**
 *  Implement the copy (actually, expand-and-copy) action.  This action
 *  opens up a space of events between the L and R (left and right) markers,
 *  and copies the information from the same amount of events that follow
 *  the R marker.  This action is preceded by pushing an Undo operation in
 *  the perform object, copying its triggers, and telling the perfroll to
 *  redraw.
 */

void
perfedit::copy ()
{
    perf().copy();
    enqueue_draw();
}

/**
 *  Implement the expand action.  This action opens up a space of events
 *  between the L and R (left and right) markers.  This action is preceded
 *  by pushing an Undo operation in the perform object, moving its
 *  triggers, and telling the perfroll to redraw.
 */

void
perfedit::expand ()
{
    perf().expand();
    enqueue_draw();
}

/**
 *  Set the looping in the perform object.
 */

void
perfedit::set_looped ()
{
    perf().set_looping(m_button_loop->get_active());
}

/**
 *  Opens the given popup menu.
 */

void
perfedit::popup_menu (Gtk::Menu * menu)
{
    menu->popup(0, 0);
}

/**
 *  Sets the guides, which are the L and R user-interface elements.
 *  See the set_snap() function.
 *
 *  It's a little confusing; I assigned the label "m_standard_bpm" to the
 *  value 4 in "measure_pulse = 192 * 4 * m_bpm / m_bw", but I am not sure I
 *  understand this equation... why the extra factor of 4?  That 4 appears
 *  in "c_ppqn * 4" a lot in the original code.
 */

void
perfedit::set_guides ()
{
    if (m_bw > 0 && m_snap > 0)
    {
        midipulse measure_pulses = m_ppqn * m_standard_bpm * m_bpm / m_bw;
        midipulse snap_pulses = measure_pulses / m_snap;
        midipulse beat_pulses = measure_pulses / m_bpm;
        m_perfroll->set_guides(snap_pulses, measure_pulses, beat_pulses);
        m_perftime->set_guides(snap_pulses, measure_pulses);
    }
}

/**
 *  Sets the snap text and values to the given value, and then calls
 *  set_guides().
 *
 * \param snap
 *      Provide the snap value to be set.  This value is basically the
 *      numerator of the expression "1 / snap".
 */

void
perfedit::set_snap (int snap)
{
    if (snap != m_snap && snap > 0)
    {
        char b[8];
        if (snap > 1)
            snprintf(b, sizeof b, "1/%d", snap);
        else
            snprintf(b, sizeof b, "%d", snap);

        m_entry_snap->set_text(b);
        m_snap = snap;
        set_guides();
    }
}

/**
 *  Sets the beats-per-measure text and value to the given value, and then
 *  calls set_guides().
 *
 *  The usage of is modified was faulty.  Offloaded it to the perform object
 *  to make it more foolproof.  See the perform::modify() function.
 *
 * \param bpm
 *      Provides the beats/measure or beats/bar value to be set.  This value
 *      is basically the numerator of the time signature.
 */

void
perfedit::set_beats_per_bar (int bpm)
{
    if (bpm != m_bpm && bpm > 0)
    {
        char b[8];
        snprintf(b, sizeof b, "%d", bpm);
        m_entry_bpm->set_text(b);
        if (m_bpm != 0)                     /* are we in construction?      */
            perf().modify();                /* no, it's a modification now  */

        m_bpm = bpm;
        set_guides();
    }
}

/**
 *  Sets the BW (beat width, or the denominator in the time signature)
 *  text and values to the given value, and then calls set_guides().
 *
 *  The usage of is modified was faulty.  Offloaded it to the perform object
 *  to make it more foolproof.  See the perform::modify() function.
 *
 * \param bw
 *      Provides the beat width to be set.  The beat width is basically the
 *      denominator of the time signature.
 */

void
perfedit::set_beat_width (int bw)
{
    if (bw != m_bw && bw > 0)
    {
        char b[8];
        snprintf(b, sizeof b, "%d", bw);
        m_entry_bw->set_text(b);
        if (m_bw != 0)                      /* are we in construction?      */
            perf().modify();                /* no, it's a modification now  */

        m_bw = bw;
        set_guides();
    }
}

/**
 *  Increments the size of the perfroll and perftime objects.  Make sure that
 *  setting the modified flag makes sense for this operation.  It doesn't seem
 *  to modify members.
 */

void
perfedit::grow ()
{
    m_perfroll->increment_size();
    m_perftime->increment_size();           /* a do-nothing function        */
}

/**
 *  This function forwards its call to the perfroll function of the same
 *  name.  It does not seem to need to also forward to the perftime
 *  function of the same name.
 */

void
perfedit::init_before_show ()
{
    m_perfroll->init_before_show();
}

/**
 *  Forces a redraw of the sequences, though currently just the perfnames
 *  part of each sequence in the performance editor.  This is meant to be
 *  called when the focus of an open seqedit or eventedit window changes.
 */

void
perfedit::draw_sequences ()
{
    if (is_realized())
        m_perfnames->draw_sequences();
}

/**
 *  Handles a drawing timeout.  It redraws "dirty" sequences in the perfroll
 *  and the perfnames objects, and shows draw progress on the perfroll.  It
 *  also changes the pause/play image if the status of running has changed.
 *  This function is called frequently and continuously.  It will work for
 *  both perfedit windows, if both are up.
 */

bool
perfedit::timeout ()
{
    m_perfroll->redraw_progress();
    m_perfnames->redraw_dirty_sequences();

#ifdef SEQ64_PAUSE_SUPPORT
    if (perf().is_running() != m_is_running)
    {
        m_is_running = perf().is_running();
        set_image(m_is_running);
    }
#endif

    return true;
}

/**
 *  Changes the image used for the pause/play button.
 *
 * \param isrunning
 *      If true, the image should be the pause image.  Otherwise, it should be
 *      the play image.
 */

void
perfedit::set_image (bool isrunning)
{
    delete m_image_play;
    if (isrunning)
    {
        m_image_play = manage(new PIXBUF_IMAGE(pause_xpm));
        add_tooltip(m_button_play, "Pause playback at the current location.");
    }
    else
    {
        m_image_play = manage(new PIXBUF_IMAGE(play2_xpm));
        add_tooltip(m_button_play, "Resume playback from the current location.");
    }
    m_button_play->set_image(*m_image_play);
}

/**
 *  Implement the playing.  JACK will be used if it is present and, in
 *  the application, enabled and working.
 */

void
perfedit::start_playing ()
{
#ifdef SEQ64_PAUSE_SUPPORT
    perf().pause_key();                 /* perf().start_key() */
#else
    perf().start_playing();             /* legacy behavior  */
#endif
}

/**
 *  Pauses the playing of the song, leaving the progress bar where it stopped.
 *  Currently, it is just the same as stop_playing(), but we will get it to
 *  work.  Keeps the stop button enabled as a kind of rewind for ALSA.
 */

void
perfedit::pause_playing ()                      /* Stop in place!   */
{
#ifdef SEQ64_PAUSE_SUPPORT
    perf().pause_key();
#else
    perf().pause_playing();
#endif
}

/**
 *  Stop the playing.
 *
 *  We need to make the progress line move back to the beginning right away
 *  here.
 */

void
perfedit::stop_playing ()
{
#ifdef SEQ64_PAUSE_SUPPORT
    perf().stop_key();
#else
    perf().stop_playing();
#endif
}

/**
 *  Implements the horizontal zoom feature.
 */

void
perfedit::set_zoom (int z)
{
    m_perfroll->set_zoom(z);
    m_perftime->set_zoom(z);
}

/**
 *  This callback function calls the base-class on_realize() function, and
 *  then connects the perfedit::timeout() function to the Glib
 *  signal-timeout, with a redraw timeout of redraw_period_ms().
 */

void
perfedit::on_realize ()
{
    gui_window_gtk2::on_realize();
    Glib::signal_timeout().connect
    (
        mem_fun(*this, &perfedit::timeout), redraw_period_ms()
    );
}

/**
 *  This function is the callback for a key-press event.  By default, the
 *  space-bar starts the playing, and the Escape key stops the playing.  The
 *  start/end key may be the same key (i.e. space-bar), allow toggling when
 *  the same key is mapped to both triggers.  Note that we now pass false in
 *  the call to perform::playback_key_event(), if SEQ64_PAUSE_SUPPORT is
 *  compiled in.  Song mode doesn't yield the pause effect we want.
 */

bool
perfedit::on_key_press_event (GdkEventKey * ev)
{
    if (CAST_EQUIVALENT(ev->type, SEQ64_KEY_PRESS))
    {
        keystroke k(ev->keyval, SEQ64_KEYSTROKE_PRESS, ev->state);

#ifdef SEQ64_USE_DEBUG_OUTPUT
        printf("key_press[%d] == %s\n", ev->keyval, gdk_keyval_name(ev->keyval));
#endif

#ifdef SEQ64_PAUSE_SUPPORT_XXX           // EXPERIMENTAL, disables song mode
        bool startstop = perf().playback_key_event(k, false);   // see notes
#else
        bool startstop = perf().playback_key_event(k, true);
#endif

        if (startstop)
            return true;                                        // event handled
    }
    (void) m_perftime->key_press_event(ev);
    return Gtk::Window::on_key_press_event(ev);
}

}           // namespace seq64

/*
 * perfedit.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

