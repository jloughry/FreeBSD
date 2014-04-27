FreeBSD console screensaver
===========================

This new console screen saver for FreeBSD improved on a classic by doing
anti-aliased line drawing *and* traversing the z-buffer in reverse order so
that erasures never overwrote existing lines, an aesthetic flaw common to
all the other originals of this programme I've ever seen.

It uses Xiaolin Wu's fast antialiasing algorithm, described by Michael Abrash
in *Dr Dobb's* in 1992.[<sup>*</sup>](#ref1)

Source code in C: [`lines_saver.c`](https://github.com/jloughry/FreeBSD/blob/master/lines_saver.c).

Submitted as a patch to the [FreeBSD Project](http://freebsd.org) 30th May 2000.

Since this is a BSD project, the licence is the two-clause BSD licence.

Makefile
--------

When installing on FreeBSD, the file `rename_this_file_to_Makefile` is the
real Makefile that goes into the FreeBSD distribution.

TODO
----

Need a screenshot of the programme running here.

References
----------

1. <a name="ref1"/>Michael Abrash. &ldquo;Fast Antialiasing&rdquo;. *Dr Dobb's Journal*
**17**(6), pp. 139&ndash;45 (June 1992).

