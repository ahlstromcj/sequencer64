readme.txt for Sequencer64
Chris Ahlstrom
2015-09-10 to 2018-06-03

Sequencer64 is a reboot of seq24, extending it with new features and bug fixes.
It is a "live performance" sequencer, with the musician creating and
controlling a number of patter loops.

An extensive manual is found at:

    https://github.com/ahlstromcj/sequencer64-doc.git
    
Prebuilt Debian packages, Windows installers, and source tarballs are
available here:

    https://github.com/ahlstromcj/sequencer64-packages.git

Windows support:

    This version uses a Qt 5 user-interface based on Kepler34, but using the
    standard Sequencer64 libraries.  The user-interface works, and Windows
    built-in MIDI devices are detected, inaccessible devices are ignored, and
    playback (e.g. to the built-in wavetable synthesizer) work.

    However, the Qt 5 GUI is well behind the Gtkmm 2.4 GUI for many features.
    That will be fixed bit-by-bit (help wanted!).  In the meantime,
    some configuration can be done manually in the "rc" and "usr" files.
    See README.windows for more information.

See the INSTALL file for build-from-source instructions or using a
conventional source tarball.  This file is part of:

    https://github.com/ahlstromcj/sequencer64.git

# vim: sw=4 ts=4 wm=4 et ft=sh fileformat=dos
