FreeBSD console screensaver
===========================

This new console screen saver for FreeBSD improved on a classic by doing
anti-aliased line drawing and traversing the z-buffer in reverse order so
that erasures never overwrote existing lines, an aesthetic flaw in all the
other originals I've seen.

Source code in C: [`lines_saver.c`](https://github.com/jloughry/FreeBSD/blob/master/lines_saver.c).

Submitted as a patch 30th May 2000.

Since this is a BSD project, the licence is the two-clause BSD licence.

