#!/bin/sh

set -x
aclocal || exit 1
autoheader || autoheader || exit 1
libtoolize -c -f || libtoolize -c -f || glibtoolize -c -f || exit 1
automake -a -c || automake -a -c || exit 1
autoconf || autoconf || exit 1
