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
set use_gnu89 = 0
set no_largefiles = 0
set extra_safety_checks = 0

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
set sanitize_address = 0

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
   breaksw;
  # key/value options passed by build systems which we ignore
  case --sbindir*
  case --libexecdir*
  case --sharedstatedir*
  case --localstatedir*
  case --oldincludedir*
  case --infodir*
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
   set pedantic = 0
   echo "disabling pedantic build"
   endif    
   breaksw;
  # options used by Ohana, but not gpcsw
  case --vararch
   set vararch = 1
   breaksw;
  case --enable-optimize
   set optimize = 1
   breaksw;
  case --enable-profile
   set profile = 1
   breaksw;
  case --enable-sanitize-address
   set sanitize_address = 1
   breaksw;
  case --extra-safety-checks
   set extra_safety_checks = 1
   breaksw;
  case --enable-memcheck
   set memcheck = 1
   breaksw;
  case --use-tcmalloc
   set use_tcmalloc = 1
   breaksw;
  case --use-gnu89
   set use_gnu89 = 1
   echo "---- warning : gnu89 is now deprecated"
   breaksw;
  case --no-largefiles
   set no_largefiles = 1
   breaksw;
  case --pedantic
   set pedantic = 1
   if ($debug_build) then
    echo "--pedantic and --enable-debug-build are mutually exclusive"
    exit 2;
   endif    
   breaksw;
  case --no-pedantic
   set pedantic = 0
   breaksw;
  case --prefix*
   if ("$1" == "--prefix") then
     shift
     set prefix = $1
   else
     set prefix = `echo $1 | tr = ' ' | awk '{print $2}'`
   endif
   breaksw;
  case --exec-prefix*
   if ("$1" == "--exec-prefix") then
     shift
     set exec_prefix = $1
   else
     set exec_prefix = `echo $1 | tr = ' ' | awk '{print $2}'`
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
  case --libdir*
   if ("$1" == "--libdir") then
     shift
     set libdir = $1
   else
     set libdir = `echo $1 | tr = ' ' | awk '{print $2}'`
   endif
   breaksw;
  case --includedir*
   if ("$1" == "--includedir") then
     shift
     set incdir = $1
   else
     set incdir = `echo $1 | tr = ' ' | awk '{print $2}'`
   endif
   breaksw;
  case --sysconfdir*
   if ("$1" == "--sysconfdir") then
     shift
     set sysconfdir = $1
   else
     set sysconfdir = `echo $1 | tr = ' ' | awk '{print $2}'`
   endif
   breaksw;
  case --datadir*
   if ("$1" == "--datadir") then
     shift
     set datadir = $1
   else
     set datadir = `echo $1 | tr = ' ' | awk '{print $2}'`
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

if ("$exec_prefix" == "") then
  set exec_prefix = $prefix
endif

# replace $exec_prefix in variables
#echo "setting libdir ($libdir)"
#set libdir = `echo $libdir | sed 's|\$exec_prefix|$exec_prefix|'`

# set values for CC, CFLAGS, CPPFLAGS, LDFLAGS
if (! $?CC) then
  set CC = gcc
endif  

if (! $?CFLAGS) then
  set CFLAGS = "-g -O0"
endif  

# optimize overrides user-supplied CFLAGS
if ($optimize) set CFLAGS = "-O2"

# other temp flags:
# profiler code
if ($profile) set CFLAGS = "$CFLAGS -pg"

# sanitize-address code
if ($sanitize_address) set CFLAGS = "$CFLAGS -fsanitize=address"

# gnu c99 fails due to missing flockfile prototype
# we are forced to used gnu99 
# -std=gnu89 fails with -pedantic tests because we use VA_ARGS in macros (allowed by gnu but not C89)

# use_gnu99
if ($use_gnu89 && ($extra_safety_checks == 0)) then
    set CFLAGS = "$CFLAGS -std=gnu89"
else
    set CFLAGS = "$CFLAGS -std=gnu99"
endif

# no_largefiles
if ($no_largefiles) then
  echo "skipping explicit large-file support"
else
  set addflags = `getconf LFS_CFLAGS`
  set CFLAGS = "$CFLAGS $addflags"
endif

if (! $?CPPFLAGS) then
  set CPPFLAGS = ""
endif  

# other temp flags
if ($extra_safety_checks) then
  set CPPFLAGS = "$CPPFLAGS -fstack-check -fstack-protector-all -D_FORTIFY_SOURCE=2 -Wstrict-aliasing=2 -fno-strict-aliasing -pedantic"
  set CPPFLAGS = "$CPPFLAGS -Wclobbered -Wempty-body -Wignored-qualifiers -Wmissing-field-initializers -Wmissing-parameter-type -Wold-style-declaration -Woverride-init -Wtype-limits -Wuninitialized -Wunused-parameter -Wunused-but-set-parameter"
  set CPPFLAGS = "$CPPFLAGS -Wsign-compare"
endif

if ($pedantic) set CPPFLAGS = "$CPPFLAGS -Wall -Werror"
if ($debug_build) set CPPFLAGS = "$CPPFLAGS -Wall"
if ($memcheck) set CPPFLAGS = "$CPPFLAGS -DOHANA_MEMORY"

if (! $?LDFLAGS) then
  set LDFLAGS = 
endif  
if ($profile) set LDFLAGS = "$LDFLAGS -Wl,--start-group -Wl,-Bstatic -Wl,-Bdynamic"
if ($profile) set LDFLAGS = "$LDFLAGS -fsanitizer=address"

if ($no_as_needed) set LDFLAGS = "$LDFLAGS -Wl,--no-as-needed"

set syslibpath = "/lib /usr/lib /usr/X11R6/lib /usr/local/lib"
set xtrlibpath = `./checkpaths.pl lib`
set syslibpath = "$syslibpath $xtrlibpath"

set needlibs   = ""
set needlibs   = "$needlibs png"
set needlibs   = "$needlibs z"
set needlibs   = "$needlibs jpeg"
set needlibs   = "$needlibs readline"
set needlibs   = "$needlibs X11"
set needlibs   = "$needlibs pthread"
set needlibs   = "$needlibs gsl"
set needlibs   = "$needlibs gslcblas"
set needlibs   = "$needlibs m"

set optlibs    = ""
set optlibs    = "$optlibs mysqlclient"
if ($use_tcmalloc) set optlibs = "$optlibs tcmalloc"

set sysincpath = "/usr/include /usr/local/include /usr/X11R6/include"
set xtrincpath = `./checkpaths.pl include`
set sysincpath = "$sysincpath $xtrincpath"

set needincs = ""
set needincs = "$needincs X11/Xatom.h"
set needincs = "$needincs X11/Xlib.h"
set needincs = "$needincs X11/Xresource.h"
set needincs = "$needincs X11/Xutil.h"
set needincs = "$needincs X11/cursorfont.h"
set needincs = "$needincs X11/keysym.h"
set needincs = "$needincs X11/keysymdef.h"
set needincs = "$needincs arpa/inet.h"
set needincs = "$needincs ctype.h"
set needincs = "$needincs errno.h"
set needincs = "$needincs fcntl.h"
set needincs = "$needincs glob.h"
set needincs = "$needincs jpeglib.h"
set needincs = "$needincs math.h"
set needincs = "$needincs netdb.h"
set needincs = "$needincs netinet/ip.h"
set needincs = "$needincs png.h"
set needincs = "$needincs pthread.h"
set needincs = "$needincs gsl/gsl_sf_legendre.h"
set needincs = "$needincs readline/history.h"
set needincs = "$needincs readline/readline.h"
set needincs = "$needincs signal.h"
set needincs = "$needincs stdio.h"
set needincs = "$needincs stdlib.h"
set needincs = "$needincs string.h"
set needincs = "$needincs sys/ipc.h"
set needincs = "$needincs sys/resource.h"
set needincs = "$needincs sys/sem.h"
set needincs = "$needincs sys/socket.h"
set needincs = "$needincs sys/stat.h"
set needincs = "$needincs sys/time.h"
set needincs = "$needincs sys/types.h"
set needincs = "$needincs sys/uio.h"
set needincs = "$needincs sys/un.h"
set needincs = "$needincs sys/wait.h"
set needincs = "$needincs time.h"
set needincs = "$needincs unistd.h"
set needincs = "$needincs zlib.h"

set optincs  = ""
set optincs = "$optincs mysql.h"

# XXX need to have options for non-ANSI includes? (ie, varargs.h)
# set needincs = "$needincs cfuncs.h" - from non-ANSI option in ohana.h
# set needincs = "$needincs float.h" - is from missing_proto (CFHT)
# set needincs = "$needincs floatingpoint.h" - is from missing_proto (CFHT)
# set needincs = "$needincs stdarg.h" - from std includes (in gcc path)
# set needincs = "$needincs varargs.h" - from std includes (in gcc path)

# XXX I was probing for these before, but RHL claims we don't need them.
# I suspect they may be needed on older systems a la CFHT.
# set needincs = "$needincs malloc.h"
# set needincs = "$needincs memory.h"
# set needincs = "$needincs values.h"

# check the hardware architecture:
set sys=`uname -s` 
set ranlib = "ranlib"
set dlltype = "so"
switch ($sys)
 case IRIX64:
   set arch="irix";
   breaksw;
 case SunOS:
   set ver=`uname -r | awk '{print substr($1,1,1)}'`;
   if ($ver == 5) then
     set arch="sol";
   else 
     set arch="sun4";
   endif
   # sun (at least) seems to need the socket library (linux does not)
   set syslibpath = "$syslibpath /usr/openwin/lib"
   set sysincpath = "$sysincpath /usr/openwin/include"
   set needlibs = "$needlibs libsocket libnsl"
   set ranlib = "touch"
   breaksw;
 case Linux:
   set arch="linux";
   if (-e /etc/sidious.config) set arch="sid";
   set mach=`uname -m`
   if ("$mach" == "x86_64") then
    set arch="lin64";
    set syslibpath = "/lib64 /usr/lib64 /usr/X11R6/lib64 $syslibpath"
   endif
   breaksw;
 case Darwin:
   set arch="darwin";
   set mach=`uname -m`
   if ("$mach" == "i386") then
    set arch="darwin_x86";
   endif
   set syslibpath = "$syslibpath /sw/lib"
   set sysincpath = "$sysincpath /sw/include"
   set dlltype = dylib
   # set CFLAGS = "$CFLAGS -D_DARWIN_C_SOURCE"
   # set defines = "-D_DARWIN_C_SOURCE"
   breaksw;
 case HP-UX:
    set arch="hpux";
    breaksw;
 default:
   echo "unknown architecture";
   exit 1;
   breaksw;
endsw
echo "setting architecture to: $arch" 

# add 

# set up the basic directory names:
set root = `pwd`
if ($prefix == "") set prefix = $root

# set the install include directory
if ($incdir == "") then
  if ($vararch) then
    set incdir = $prefix/include/$arch
  else
    set incdir = $prefix/include
  endif
endif

# set the install lib directory
if ($libdir == "") then
  if ($vararch) then
    set libdir = $prefix/lib/$arch
  else
    set libdir = $prefix/lib
  endif
endif

if ($?LIBRARY_PATH) then 
  set libpath = `echo $LIBRARY_PATH | tr ':' ' '`
else
  set libpath = ""
endif

# check for basic libraries
echo ""
echo "searching for required external libraries..."
set faillibs = ""
set libflags = ""
set libdirs  = ""
set nonomatch
foreach f ( $needlibs )
  foreach g ( $libpath $libdir $syslibpath )
    set name = $g/lib$f.a
    if (-e $name[1]) goto got_lib;
    set name = $g/lib$f.$dlltype
    if (-e $name[1]) goto got_lib;
  end
  echo "missing lib$f"
  set faillibs = "$faillibs lib$f"
  continue
got_lib:
  echo "found lib$f ($name[1])"
  set gotlibdir = `dirname $name[1]`
  echo "$libdirs" | grep -- "-L$gotlibdir " > /dev/null
  if ($status) then
    set libdirs  = "$libdirs-L$gotlibdir "
  endif
  echo "$libflags" | grep -- "-l$f " > /dev/null
  if ($status) then
    set libflags = "$libflags-l$f "
  endif
end

# we need a curses library; can choose one of the following:
# check for termcap, curses, etc
foreach f ( ncurses curses termcap )
    foreach g ( $libpath $libdir $syslibpath )
        # echo "trying $g"
        set name = $g/lib$f.a
        # echo "trying $name"
        if (-e $name[1]) goto got_curses;
        set name = $g/lib$f.$dlltype
        # echo "trying $name"
        if (-e $name[1]) goto got_curses;
    end
    # try versioned libraries such as .so.N
    foreach g ( $libpath $libdir $syslibpath )
        # echo "trying $g"
        set name = $g/lib$f.$dlltype.*
	# echo "$#name : $name[1]"
        if ($#name < 2) continue
        # echo "trying $name[1]"
        if (-e $name[1]) goto got_curses;
    end
end
set faillibs = "$faillibs (ncurses | curses | termcap)"
echo "missing a valid curses library"
echo "missing: $faillibs"
echo "please find one of them and install them in $libpath"
exit 1

got_curses:
  echo "found $f ($name[$#name])"
  echo "$libdirs" | grep -- "-L$g " > /dev/null
  if ($status) then
    set libdirs  = "$libdirs-L$g "
  endif
  echo "$libflags" | grep -- "-l$f " > /dev/null
  if ($status) then
    set libflags = "$libflags-l$f "
  endif

if ("$faillibs" != "") then
  echo "your installation is missing some important libraries"
  echo "missing: $faillibs"
  echo "please find them and install them in $libdir"
  exit 1
endif    

# check for optional libraries
echo ""
echo "searching for optional external libraries..."
foreach f ( $optlibs )
  foreach g ( $libpath $libdir $syslibpath )
    set name = $g/lib$f.a
    if (-e $name[1]) goto got_optlib;
    set name = $g/lib$f.$dlltype
    if (-e $name[1]) goto got_optlib;
    set name = $g/*/lib$f.a
    if (-e $name[1]) goto got_optlib;
    set name = $g/*/lib$f.$dlltype
    if (-e $name[1]) goto got_optlib;
  end
  echo "missing lib$f; skipping"
  continue
got_optlib:
  echo "found lib$f ($name[1])"
  set gotlibdir = `dirname $name[1]`
  echo "$libdirs" | grep -- "-L$gotlibdir " > /dev/null
  if ($status) then
    set libdirs  = "$libdirs-L$gotlibdir "
  endif
  echo "$libflags" | grep -- "-l$f " > /dev/null
  if ($status) then
    set libflags = "$libflags-l$f "
  endif
end

# add the CPATH
if ($?CPATH) then 
  set incpath = `echo $CPATH | tr ':' ' '`
else
  set incpath = ""
endif

# check for required headers (including in subdirectories)
echo ""
echo "searching for required external header files..."
set failincs = ""
set incdirs = ""
foreach f ( $needincs )
  foreach g ( $incdir $incpath $sysincpath )
    set name = "$g/$f"
    if (-e $name) goto got_inc;
    set nonomatch
    set name = $g/*/$f
    echo "$name" | grep "*" > /dev/null
    if (! $status) continue
    unset nonomatch
    if (-e $name[1]) goto got_inc;
  end
  echo "missing $f"
  set failincs = "$failincs $f"
  continue
got_inc:
  set gotinc = $name[1]
  echo "found $f ($gotinc)"
  set gotincdir = `dirname $name[1]`
  if (`dirname $f` != ".") then
    set gotincdir = `dirname $gotincdir`
  endif
  echo "$incdirs" | grep -- "-I$gotincdir " > /dev/null
  if ($status) then
    set incdirs = "$incdirs-I$gotincdir "
  endif
end

if ("$failincs" != "") then
  echo "your installation is missing some important library headers"
  echo "please find them and install them in $inc"
  exit 1
endif    

# check for optional headers (including in subdirectories)
echo ""
echo "searching for optional external header files..."
foreach f ( $optincs )
  foreach g ( $incdir $incpath $sysincpath )
    set name = "$g/$f"
    if (-e $name) goto got_optinc;
    set nonomatch
    set name = $g/*/$f
    echo "$name" | grep "*" > /dev/null
    if (! $status) continue
    unset nonomatch
    if (-e $name[1]) goto got_optinc;
  end
  echo "missing $f; skipping"
  continue
got_optinc:
  set gotinc = $name[1]
  echo "found $f ($gotinc)"
  set gotincdir = `dirname $name[1]`
  if (`dirname $f` != ".") then
    set gotincdir = `dirname $gotincdir`
  endif
  echo "$incdirs" | grep -- "-I$gotincdir " > /dev/null
  if ($status) then
    set incdirs = "$incdirs-I$gotincdir "
  endif
  set haveflag = `echo $f | tr '[:lower:]' '[:upper:]' | tr '.' '_'`
  set CPPFLAGS = "$CPPFLAGS -DHAVE_$haveflag"
end

echo 
echo "Compiler options:"
echo "CC: $CC"
echo "CFLAGS: $CFLAGS"
echo "CPPFLAGS: $CPPFLAGS"
echo "LDFLAGS: $LDFLAGS"

echo
echo "Additional compiler flags:"
echo "INCDIRS: $incdirs"
echo "LIBDIRS: $libdirs"
echo "LIBFLAGS: $libflags"

echo 
echo "ARCH: $arch"
echo "ROOT: $root"
echo "PREFIX: $prefix"
echo 

# the config.tools fixconf operations below interpolate values in Makefile.System
if (-e Makefile.System) mv Makefile.System Makefile.System.bak
cp Makefile.System.in Makefile.System

# we don't currently need to modify the Makefile but since configure
# should create a new Makefile, we need to do this:
cp -f Makefile.in Makefile

# the ROOTDIR defines the location of the source tree
./config.tools fixconf @ROOTDIR@  "$root"

# the following entries define the target installation locations 

# BINDIR (DESTBIN) holds the output binary files
if ("$bindir" == "") then
  set subdir = bin
  set subpath = bin
  if ($vararch) then 
    set subdir = 'bin/$(ARCH)'
    set subpath = "bin/$arch"
  endif
  set bindir = $prefix/$subdir
  set binpath = $prefix/$subpath
endif
set bindir = `./config.tools fixpath $bindir`
./config.tools fixconf @BINDIR@ $bindir
echo DESTBIN $bindir

# INCDIR (DESTINC) holds the output header files
if ("$incdir" == "") then
  set subdir = include
  if ($vararch) set subdir = 'include/$(ARCH)'
  set incdir = $prefix/$subdir
endif
set incdir = `./config.tools fixpath $incdir`
./config.tools fixconf @INCDIR@ $incdir
echo DESTINC $incdir

# LIBDIR (DESTLIB) holds the output library files
if ("$libdir" == "") then
  set subdir = lib
  if ($vararch) set subdir = 'lib/$(ARCH)'
  set libdir = $prefix/$subdir
endif
set libdir = `./config.tools fixpath $libdir`
./config.tools fixconf @LIBDIR@ $libdir
echo DESTLIB $libdir

# MANDIR (DESTMAN) holds the output man pages
if ("$mandir" == "") then
  set mandir = $prefix/man
endif
set mandir = `./config.tools fixpath $mandir`
./config.tools fixconf @MANDIR@ $mandir
echo DESTMAN $mandir

# DATADIR (DESTDATA) holds the general non-binary files
if ("$datadir" == "") then
  set datadir = $prefix/share
endif
set datadir = `./config.tools fixpath $datadir`
./config.tools fixconf @DATADIR@ $datadir
echo DESTDATA $datadir

# the vararch option defines an automatic arch-dependent directory 
# tree for DESTBIN, DESTLIB, DESTINC
if ($vararch) then
  ./config.tools fixconf "^\s*ARCH" "# ARCH"
else 
  ./config.tools fixconf @ARCHVAL@ $arch
endif 

# INCDIRS, LIBDIRS, LIBFLAGS define include and library flags needed
# by the externally-supplied libraries
./config.tools fixconf @INCDIRS@  "$incdirs"
./config.tools fixconf @LIBDIRS@  "$libdirs"
./config.tools fixconf @LIBFLAGS@ "$libflags"

# these are the compiler options
./config.tools fixconf @CC@ "$CC"
./config.tools fixconf @CFLAGS@ "$CFLAGS"
./config.tools fixconf @CPPFLAGS@ "$CPPFLAGS"
./config.tools fixconf @LDFLAGS@ "$LDFLAGS"
./config.tools fixconf @DLLTYPE@  "$dlltype"

# other architecture dependent options
./config.tools fixconf @RANLIB@ "$ranlib"

cat ohana-config.in | sed "s|@INCDIR@|$incdir|" | sed "s|@LIBDIR@|$libdir|" | sed "s|(ARCH)|ARCH|" | sed "s|@DEFINES@|$defines|" > ohana-config

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
  -h, --help                display this help and exit
  --enable-optimize         enable compiler optimization (-O2)
  --enable-profile          enable 
  --enable-sanitize-address enable 
  --extra-safety-checks     enable extra gcc safety checks
  --enable-memcheck         enable ohana memory tests
  --enable-debug-build      enable debug build
  --pedantic                include -Wall -Werror on compilation
  --use-tcmalloc            use the alternate tcmalloc from Google
  --use-gnu89               use gnu89 for possible backwards compatibility (probably will not work)
  --no-largefiles           skip large file compatibility
  --enable-no-as-needed     pass --no-as-needed flag to linker

Installation directories:
  --prefix=PREFIX           install architecture-independent files in PREFIX
  --vararch                 install with ARCH suffixes for variable architectures

Fine tuning of the installation directories:
  --bindir=DIR             user executables [PREFIX/bin/$ARCH] 
  --libdir=DIR             object code libraries [PREFIX/lib/$ARCH]
  --includedir=DIR         C header files [PREFIX/include]
  --mandir=DIR             man documentation [PREFIX/man]
  --datadir=DIR            read-only architecture-independent data [PREFIX/share]
  --sysconfdir=DIR         read-only single-machine data [PREFIX/etc]

Makefile flags:
  CC=options
  CFLAGS=options
  CPPFLAGS=options
  LDFLAGS=options

The following options are silently ignored for compatibility:
  --enable-maintainer-mode
  --no-create
  --no-recursion
  --disable-shared
  --enable-shared
  --disable-static
  --enable-static
  --sbindir
  --libexecdir
  --sharedstatedir
  --localstatedir
  --oldincludedir
  --infodir

EOF
 exit 2;

