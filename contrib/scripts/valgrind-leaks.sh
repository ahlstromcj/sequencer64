#!/bin/sh

valgrind --leak-check=full --suppressions=contrib/valgrind/seq64.supp \
 --leak-resolution=high --track-origins=yes --log-file=v.log \
 --show-leak-kinds=all ./Seq64rtmidi/seq64 contrib/midi/b4uacuse-GM-format.midi
