# README for Sequencer64 0.96.8 (Native JACK, CLI, tempo, Linux/Windows, play-lists)
Chris Ahlstrom
2015-09-10 to 2020-07-05

Sequencer64 is a reboot of seq24, extending it greatly over the last four years.
The heart of seq24 remains intact.  It is an old friend with a whole lot of
added equipment.  It has an extensive manual and Windows installers:

    https://github.com/ahlstromcj/sequencer64-doc.git
    https://github.com/ahlstromcj/sequencer64-packages.git

Sequencer64 has build options for ALSA, PortMidi, JACK, Gtkmm 2.4, Qt 5, Windows,
and a command-line/daemon.

## Native JACK support: Seq64rtmidi/seq64

    Seq64 has native JACK MIDI/Transport, with virtual/manual ports and
    auto-connect like ALSA, based on RtMidi massively refactored. It falls
    back to ALSA support if JACK is not running.  See README.jack for basic
    instructions on native JACK.

## GUI-less application: Seq64rtmidi/seq64cli

    The RtMidi/JACK version without a GUI.  Controlled via MIDI control events
    (start/stop events must be set up), it relies on a good working
    configuration generated via the GUI or edited by hand.  MIDI files are
    loaded via play-lists (see data/nanomap.rc and data/seq64cli.rc).  Seq64cli
    supports a "daemonize" option and log-files.

## Windows support derived from PortMidi: Seq64qt5/qpseq64.exe

    Qpseq64 uses a Qt 5 user-interface based on Kepler34 and the Sequencer64
    PortMidi engine.  Windows built-in MIDI devices are detected, inaccessible
    devices are ignored, and playback (e.g. to the Windows wavetable
    synthesizer) work. It is built easily via Qt Creator or qmake, using
    MingW.  The Qt 5 GUI still has a few features to be added, but will
    be the official GUI of Seq66 (in the near future).  See README.windows for
    more information.

## See the INSTALL file for build-from-source instructions for Linux or Window,
and using a conventional source tarball.

## Recent changes:

    -   Version 0.96.8:
        -   Reduced the size of the pattern editor to 800x480 to fit on small
            screens.
        -   Many many tweaks and optimizations.
        -   Fixed bug in event_list::link_new().
        -   Sped up the MIDI record code to try to avoid issues recording
            chords from a MIDI keyboard.
        -   Fixed issue #206 MIDI Start/Stop/Continue; but still an issue
            with events not detected until the next event comes in.
    -   Version 0.96.7:
        -   Fixed issue #181 by adding more beats/bar menu values up to 20.
            Note that the fix to issue #77 allows numbers up to 128 to be
            typed in.
        -   In Seq66, found an issue handling running status in the 2rock.mid
            file.  Backported the fix here.
        -   Feature request #188: in the "usr" file, hold the desired state of
            muting, MIDI thru, record, quantized record, and recording style,
            for new patterns when opened.  An assist for live recording.
            Run with the --user-save option to save with the new feature.
            Currently, the criterion for "new pattern" is a name of "Untitled".
        -   Along with #188, made it possible for MIDI control to bring up the
            pattern editor.
        -   Added a work-around for crashing when opening menus with horizontal
            separators on certain themes (e.g. Breeze-Gtk and its dark version).
            See issues #126, #154, and #185.  Currently, all separators are
            removed by default, until we can detect themes.  A slight
            degradation in the quality of the user interface.  Not bad, though.
        -   Added signal handling (e.g. for nsm-proxy) from user falkTX.
            Used in the CLI, GTK, and Qt versions to trigger file save and
            application exit.
        -   Added code to define the engine used and the exact "uname a-" of the
            operating system on which the build was done, for the --version
            option.
    -   For earlier version information, see the NEWS and ChangeLog files.

This package is oriented to developers and users who do not mind building from
source, with a little help.  It is organized and well documented.
Initial work/thought/documentation started in July of 2015, when I was laid
up after some old-man surgery :-(.
