#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

PROJECT=ippTasks
TEST_TYPE=-f
# change this to be a unique filename in the top level dir
FILE=autogen.sh

DIE=0

LIBTOOLIZE=libtoolize
ACLOCAL="aclocal $ACLOCAL_FLAGS"
AUTOHEADER=autoheader
AUTOMAKE=automake
AUTOCONF=autoconf

#($LIBTOOLIZE --version) < /dev/null > /dev/null 2>&1 || {
#        echo
#        echo "You must have $LIBTOOLIZE installed to compile $PROJECT."
#        echo "Download the appropriate package for your distribution,"
#        echo "or get the source tarball at http://ftp.gnu.org/gnu/libtool/"
#        DIE=1
#}

($ACLOCAL --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have $ACLOCAL installed to compile $PROJECT."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
        DIE=1
}

#($AUTOHEADER --version) < /dev/null > /dev/null 2>&1 || {
#        echo
#        echo "You must have $AUTOHEADER installed to compile $PROJECT."
#        echo "Download the appropriate package for your distribution,"
#        echo "or get the source tarball at http://ftp.gnu.org/gnu/autoconf/"
#        DIE=1
#}

($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have $AUTOMAKE installed to compile $PROJECT."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
        DIE=1
}

($AUTOCONF --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have $AUTOCONF installed to compile $PROJECT."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at http://ftp.gnu.org/gnu/autoconf/"
        DIE=1
}

if test "$DIE" -eq 1; then
        exit 1
fi

test $TEST_TYPE $FILE || {
        echo "You must run this script in the top-level $PROJECT directory"
        exit 1
}

if test -z "$*"; then
        echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

#$LIBTOOLIZE --copy --force || echo "$LIBTOOLIZE failed"
$ACLOCAL || echo "$ACLOCAL failed"
#$AUTOHEADER || echo "$AUTOHEADER failed"
$AUTOMAKE --add-missing --force-missing --copy || echo "$AUTOMAKE failed"
$AUTOCONF || echo "$AUTOCONF failed"

cd $ORIGDIR

run_configure=true
for arg in $*; do
    case $arg in
        --no-configure)
            run_configure=false
            ;;
        *)
            ;;
    esac
done

if $run_configure; then
    $srcdir/configure --enable-maintainer-mode "$@"
    echo
    echo "Now type 'make' to compile $PROJECT."
else
    echo
    echo "Now run 'configure' and 'make' to compile $PROJECT."
fi
