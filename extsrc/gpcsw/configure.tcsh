#!/bin/csh -f

# this is a very low-tech version of configure, not built by autoconf.
# we check for the following libraries:

# we need to be able to list the required libraries for a given distribution

# evaluate command-line options
set vararch = 0
set optimize = 0
set pedantic = 1
set no_as_needed = 0
set debug_build = 0
set memcheck = 0
set use_tcmalloc = 0
set use_gnu99 = 0
set no_largefiles = 0

set prefix  = ""
set bindir  = ""
set libdir  = ""
set incdir  = ""
set mandir  = ""
set datadir  = ""
set sysconfdir  = ""
set exec_prefix = ""
set defines = ""
set profile = 0

set root    = ""
set args    = ""

while ("$1" != "") 
 switch ("$1")
  # options passed by build systems which we ignore
  case --enable-maintainer-mode
  case --no-create
  case --no-recursion
  case --disable-shared
  case --enable-shared
  case --disable-static
  case --enable-static
  # skipped by gpcsw, but not Ohana
  case --enable-optimize
  case --enable-profile
  case --pedantic
   breaksw;
  # key/value options passed by build systems which we ignore
  case --sbindir*
  case --libexecdir*
  case --sharedstatedir*
  case --localstatedir*
  case --oldincludedir*
  case --infodir*
  # skipped by gpcsw, but not Ohana
  case --exec-prefix*
  case --libdir*
  case --includedir*
  case --sysconfdir*
  case --datadir*
   # we need to strip the --opt word and --opt=word versions
   set word = `echo $1 | tr = ' '`
   if ($#word == 1) then
     if ($#argv > 1) then
      shift
     endif
   endif
   breaksw;
  case --enable-no-as-needed
   set no_as_needed = 1
   breaksw;
  case --enable-debug-build
   set debug_build = 1
   breaksw;
  case --prefix*
   if ("$1" == "--prefix") then
     shift
     set prefix = $1
   else
     set prefix = `echo $1 | tr = ' ' | awk '{print $2}'`
   endif
   breaksw;
  case --bindir*
   if ("$1" == "--bindir") then
     shift
     set bindir = $1
   else
     set bindir = `echo $1 | tr = ' ' | awk '{print $2}'`
   endif
   breaksw;
  case --mandir*
   if ("$1" == "--mandir") then
     shift
     set mandir = $1
   else
     set mandir = `echo $1 | tr = ' ' | awk '{print $2}'`
   endif
   breaksw;
  case --help:
   goto usage
  case -*: 
   echo ""
   echo "Unknown option: $1"
   goto usage
  default:
   set args=($args $1);
   breaksw;
 endsw
 shift
end
if ($#args != 1) goto usage

# gpc build ignores CC, CFLAGS, CPPFLAGS, LDFLAGS

# set up the basic directory names:
set root = `pwd`
if ($prefix == "") set prefix = $root

echo 
echo "install destinations:"
echo "ROOT: $root"
echo "PREFIX: $prefix"
echo 

# the config.tools fixconf operations below interpolate values in Makefile
if (-e Makefile) mv Makefile Makefile.bak
cp -f Makefile.in Makefile

# BINDIR holds the output binary files
if ("$bindir" == "") then
  set subdir = bin
  set bindir = $prefix/$subdir
endif
set bindir = `./config.tools fixpath $bindir`
./config.tools fixconf @BINDIR@ $bindir
echo BINDIR $bindir

# MANDIR (DESTMAN) holds the output man pages
if ("$mandir" == "") then
  set mandir = $prefix/man
endif
set mandir = `./config.tools fixpath $mandir`
./config.tools fixconf @MANDIR@ $mandir
echo DESTMAN $mandir

echo ""
echo "include $bindir in your path"

exit 0

usage:
cat <<EOF
USAGE: configure [OPTION]

echo remaining args: $args

set the installation directory root with --prefix
if you define the environment variable ARCH, you can set --vararch
 
Configuration:
  -h, --help              display this help and exit
  --enable-optimize       enable compiler optimization (-O2)
  --enable-memcheck       enable ohana memory tests
  --pedantic              include -Wall -Werror on compilation

Installation directories:
  --prefix=PREFIX         install architecture-independent files in PREFIX
  --vararch               install with ARCH suffixes for variable architectures

Fine tuning of the installation directories:
  --bindir=DIR           user executables [PREFIX/bin/$ARCH] 
  --libdir=DIR           object code libraries [PREFIX/lib/$ARCH]
  --includedir=DIR       C header files [PREFIX/include]
  --mandir=DIR           man documentation [PREFIX/man]
  --datadir=DIR          read-only architecture-independent data [PREFIX/share]
  --sysconfdir=DIR       read-only single-machine data [PREFIX/etc]

Makefile flags:
  CC=options
  CFLAGS=options
  CPPFLAGS=options
  LDFLAGS=options

The following options are silently ignored for compatibility:
  --enable-maintainer-mode
  --no-create
  --no-recursion
  --sbindir
  --libexecdir
  --sharedstatedir
  --localstatedir
  --oldincludedir
  --infodir

EOF
 exit 2;
