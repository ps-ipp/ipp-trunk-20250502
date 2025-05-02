AC_PREREQ(2.61)

AC_INIT([pslib],[1.1.1],[http://pan-starrs.ifa.hawaii.edu/bugzilla])
AC_CONFIG_SRCDIR([pslib.pc.in])
AC_CANONICAL_TARGET

dnl this enables the building of libtap
AC_CONFIG_SUBDIRS([test/tap])
dnl AC_CONFIG_SUBDIRS([test/pstap])

AM_INIT_AUTOMAKE([1.7 foreign dist-bzip2])
AC_CONFIG_HEADERS([src/config.h])
AM_MAINTAINER_MODE

dnl So, libtool library versions are described by three integers:
dnl 
dnl current
dnl     The most recent interface number that this library implements.
dnl revision
dnl     The implementation number of the current interface.
dnl age
dnl     The difference between the newest and oldest interfaces that this
dnl library implements. In other words, the library implements all the
dnl interface numbers in the range from number current - age to current

PSLIB_LT_VERSION="1:1:0"
AC_SUBST(PSLIB_LT_VERSION,$PSLIB_LT_VERSION)

IPP_STDLDFLAGS

dnl ------------------- PERL options ---------------------
  AC_ARG_WITH(perl,
    [AS_HELP_STRING(--with-perl=FILE,Specify location of PERL executable.)],
    [AC_CHECK_PROG(PERL, $withval, $withval)],
    [AC_CHECK_PROG(PERL, perl, `which perl`)])
    if test "$PERL" == ""
    then
      AC_MSG_ERROR([PERL is required.  Use --with-perl to specify its install location.])
    fi
    AC_SUBST(PERL,$PERL)

SUBDIR="etc src share test utils" dnl don't include 'swig', as it is optional
dnl SUBDIR="etc src share utils" dnl don't include 'swig', as it is optional

SRCPATH='${top_srcdir}/src'
SRCDIRS="sys astro db fft fits imageops jpeg math mathtypes types"   dnl xml
# escape two escapes at this level so \\ gets passed to the shell and \ to perl
SRCINC=`echo "${SRCDIRS}" | ${PERL} -pe "s|(\w+)|-I\\\\${SRCPATH}/\1|g"`
SRCINC="-I${SRCPATH} ${SRCINC}"
SRCSUBLIBS=`echo "${SRCDIRS}" | ${PERL} -pe "s|(\w+)|\1/libpslib\1.la|g"`
AC_SUBST(SRCSUBLIBS,${SRCSUBLIBS})
AC_SUBST(SRCINC,${SRCINC})
AC_SUBST(ABS_SRCDIR,`pwd`) dnl assuming configure is run in top directory -- is there something better?
AC_PROG_MAKE_SET

dnl build tests at the same time as the source code
AC_ARG_ENABLE(tests,
  [AS_HELP_STRING(--enable-tests,build tests at same time as source)],
  [AC_MSG_RESULT(test building enabled)
   tests=true],
   [tests=false])
AM_CONDITIONAL(BUILD_TESTS, test x$tests = xtrue)

dnl turn on mem leak backtracing 
AC_ARG_ENABLE(backtrace,
  [AS_HELP_STRING(--enable-backtrace, enable memory allocation backtracing)],
  [AC_MSG_RESULT(memory allocation backtracing enabled)
    AC_DEFINE([PS_MEM_BACKTRACE], 1, [Define to 1 if you want memory backtracing])
    CFLAGS="${CFLAGS} -rdynamic"
  ]
)
AC_CHECK_FUNCS([backtrace])

AC_LANG(C)
AC_PROG_CC_C99
dnl _GNU_SOURCE is needed for atleast strcasestr() with glibc
AC_GNU_SOURCE
AC_C_INLINE

dnl enable largefile supports
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl check the systems endianness
AC_C_BIGENDIAN

AC_PREFIX_DEFAULT([/usr/local])
test "$prefix" = NONE && prefix=/usr/local

dnl ----------------- pthread support --------------------

dnl Check for how to link against pthreads
dnl RHEL 4.0 was upset about the first arg to ACX_PTHREAD being NULL ([]) so
dnl we've added in a worthless message to keep it happy
dnl PTHREAD_CFLAGS /must/ also be used in the linking of an exe.  Without
dnl adding PTHREAD_CFLAGS into PSLIB_LIBS we can't link exes on Linux.
dnl Basically this means the entire method of only linking psLib's deps in the
dnl final exe or library is flawed.

ACX_PTHREAD(
  [PSLIB_CFLAGS="${PSLIB_CFLAGS} ${PTHREAD_CFLAGS}"
    PSLIB_LIBS="${PSLIB_LIBS} ${PTHREAD_CFLAGS} ${PTHREAD_LIBS}"],
  [AC_MSG_ERROR([pthread support is required to build psLib])]
)

dnl ----------------- MYSQL options --------------------

AX_LIB_MYSQL([4.1.2],
  [PSLIB_CFLAGS="${PSLIB_CFLAGS} ${MYSQL_CFLAGS} -DHAVE_PSDB"
    PSLIB_LIBS="${PSLIB_LIBS} ${MYSQL_LDFLAGS}"],
  [PSLIB_CFLAGS="${PSLIB_CFLAGS}"]
)

dnl ----------------- CFITSIO options --------------------

dnl save LIBS/CFLAGS/LDFLAGS
TMP_LIBS=${LIBS}
TMP_CFLAGS=${CFLAGS}
TMP_LDFLAGS=${LDFLAGS}
TMP_CPPFLAGS=${CPPFLAGS}

PKG_CHECK_MODULES([CFITSIO], [cfitsio], [], AC_MSG_ERROR([CFITSIO package not found.  Obtain CFITSIO at  http://heasarc.gsfc.nasa.gov/docs/software/fitsio]))

PSLIB_CFLAGS="${PSLIB_CFLAGS} ${CFITSIO_CFLAGS}"
PSLIB_LIBS="${PSLIB_LIBS} ${CFITSIO_LIBS}"

CFLAGS="${CFLAGS} ${CFITSIO_CFLAGS}"
LDFLAGS="${LDFLAGS} ${CFITSIO_LIBS}"

dnl Now check if CFITSIO supports fits_open_diskfile, i.e., is at least version 2.501
AC_CHECK_FUNC(ffdkopn,
  [CFITSIO_DISKFILE=1],
  [AC_MSG_WARN([The CFITSIO library version is rather old.  Suggested version is 2.501 or greater.])
   CFITSIO_DISKFILE=0]
)

AC_DEFINE_UNQUOTED([CFITSIO_DISKFILE],${CFITSIO_DISKFILE},[Define to 1 if you have fits_open_diskfile in CFITSIO])

AC_SUBST([CFITSIO_CFLAGS])
AC_SUBST([CFITSIO_LIBS])

dnl restore the LIBS/CFLAGS/LDFLAGS
LIBS=${TMP_LIBS}
CFLAGS=${TMP_CFLAGS}
LDFLAGS=${TMP_LDFLAGS}
CPPFLAGS=${TMP_CPPFLAGS}

dnl ------------------ FFTW3 options ---------------------

dnl save LIBS/CFLAGS/LDFLAGS
TMP_LIBS=${LIBS}
TMP_CFLAGS=${CFLAGS}
TMP_LDFLAGS=${LDFLAGS}
TMP_CPPFLAGS=${CPPFLAGS}

PKG_CHECK_MODULES([FFTW3], [fftw3f], [], AC_MSG_ERROR([FFTW version 3 (--enable-float) library not found.  Obtain it at http://www.fftw.org/]))

PSLIB_CFLAGS="${PSLIB_CFLAGS} ${FFTW3_CFLAGS}"
PSLIB_LIBS="${PSLIB_LIBS} ${FFTW3_LIBS}"

CFLAGS="${CFLAGS} ${FFTW3_CFLAGS}"
CPPFLAGS=${CFLAGS}
LDFLAGS="${LDFLAGS} ${FFTW3_LIBS}"

AC_CHECK_FUNC(fftwf_plan_dft_2d,[],
  [AC_MSG_ERROR([FFTW version 3 (--enable-float) library not found.  Obtain it at http://www.fftw.org/])])
AC_CHECK_FUNC(fftwf_init_threads,[FFTW_THREADS=1],[FFTW_THREADS=0])
AC_DEFINE_UNQUOTED([HAVE_FFTW_THREADS],${FFTW_THREADS},[Define to 1 if you have FFTW compiled with thread support])

AC_SUBST([FFTW3_CFLAGS])
AC_SUBST([FFTW3_LIBS])

dnl restore the CFLAGS/LDFLAGS
LIBS=${TMP_LIBS}
CFLAGS=${TMP_CFLAGS}
LDFLAGS=${TMP_LDFLAGS}
CPPFLAGS=${TMP_CPPFLAGS}

dnl -------------------- GSL options ---------------------

dnl save LIBS/CFLAGS/LDFLAGS
TMP_LIBS=${LIBS}
TMP_CFLAGS=${CFLAGS}
TMP_LDFLAGS=${LDFLAGS}
TMP_CPPFLAGS=${CPPFLAGS}

PKG_CHECK_MODULES([GSL], [gsl], [], AC_MSG_ERROR([GSL is required.  Obtain it at http://www.gnu.org/software/gsl]))

PSLIB_CFLAGS="${PSLIB_CFLAGS} ${GSL_CFLAGS}"
PSLIB_LIBS="${PSLIB_LIBS} ${GSL_LIBS}"

AC_SUBST([GSL_CFLAGS])
AC_SUBST([GSL_LIBS])

dnl restore the CFLAGS/LDFLAGS
LIBS=${TMP_LIBS}
CFLAGS=${TMP_CFLAGS}
LDFLAGS=${TMP_LDFLAGS}
CPPFLAGS=${TMP_CPPFLAGS}

dnl ------------------ kapa,libkapa options -------------------------
dnl -- libkapa implies the requirement for libpng, libjpeg as well --

dnl save LIBS/CFLAGS/LDFLAGS
TMP_LIBS=${LIBS}
TMP_CFLAGS=${CFLAGS}
TMP_LDFLAGS=${LDFLAGS}
TMP_CPPFLAGS=${CPPFLAGS}

dnl test for command-line options: use ohana-config if not supplied
KAPA_CFLAGS_CONFIG="true"
KAPA_LIBS_CONFIG="true"
AC_ARG_WITH(kapa,
[AS_HELP_STRING(--with-kapa=DIR,Specify location of libkapa)],
[KAPA_CFLAGS="-I$withval/include" KAPA_LIBS="-L$withval/lib" 
 KAPA_CFLAGS_CONFIG="false"       KAPA_LIBS_CONFIG="false"])
AC_ARG_WITH(kapa-include,
[AS_HELP_STRING(--with-kapa-include=DIR,Specify libkapa include directory.)],
[KAPA_CFLAGS="-I$withval" KAPA_CFLAGS_CONFIG="false"])
AC_ARG_WITH(kapa-lib,
[AS_HELP_STRING(--with-kapa-lib=DIR,Specify libkapa library directory.)],
[KAPA_LIBS="-L$withval" KAPA_LIBS_CONFIG="false"])

echo "KAPA_CFLAGS_CONFIG: $KAPA_CFLAGS_CONFIG"
echo "KAPA_LIBS_CONFIG: $KAPA_LIBS_CONFIG"
echo "KAPA_CFLAGS: $KAPA_CFLAGS"
echo "KAPA_LIBS: $KAPA_LIBS"

dnl HAVE_KAPA is set to false if any of the tests fail
HAVE_KAPA="true"
AC_MSG_NOTICE([checking for libkapa])
if test "$KAPA_CFLAGS_CONFIG" = "true" -o "$KAPA_LIBS_CONFIG" = "true"; then
  AC_MSG_NOTICE([kapa info supplied by ohana-config])
  KAPA_CONFIG=`which ohana-config`
  AC_CHECK_FILE($KAPA_CONFIG,[],
    [HAVE_KAPA="false"; AC_MSG_WARN([libkapa is not found: output plots disabled.  Obtain libkapa at http://kiawe.ifa.hawaii.edu/Elixir/Ohana or use --with-kapa to specify location])])
  
  echo "HAVE_KAPA: $HAVE_KAPA"
  echo "KAPA_CFLAGS_CONFIG: $KAPA_CFLAGS_CONFIG"

  if test "$HAVE_KAPA" = "true" -a "$KAPA_CFLAGS_CONFIG" = "true" ; then
   AC_MSG_NOTICE([libkapa cflags info supplied by ohana-config])
   AC_MSG_CHECKING([libkapa cflags])
   KAPA_CFLAGS="`${KAPA_CONFIG} --cflags`"
   AC_MSG_RESULT([${KAPA_CFLAGS}])
  fi

  if test "$HAVE_KAPA" = "true" -a "$KAPA_LIBS_CONFIG" = "true" ; then
   AC_PATH_X
   if test "$no_x" = "yes" ; then
      AC_MSG_WARN([X11 not found: output plots using kapa disabled.  Use --x-includes and --x-libraries if required.])
      HAVE_KAPA="false"
   else
      AC_MSG_NOTICE([libkapa ldflags info supplied by ohana-config])
      AC_MSG_CHECKING([libkapa ldflags])
      if test -n "$x_libraries" ; then
            KAPA_LIBS="`${KAPA_CONFIG} --libs` -L$x_libraries -lX11"
      else
            KAPA_LIBS="`${KAPA_CONFIG} --libs` -lX11"
      fi
      if test -n "$x_includes" ; then
            KAPA_CFLAGS="${KAPA_CFLAGS} -I$x_includes"
      else
            KAPA_CFLAGS="${KAPA_CFLAGS}"
      fi
      AC_MSG_RESULT([${KAPA_LIBS}])
   fi
  fi
fi

if test "$HAVE_KAPA" = "true" ; then
 AC_MSG_NOTICE([libkapa supplied])
 PSLIB_CFLAGS="${PSLIB_CFLAGS} ${KAPA_CFLAGS}"
 PSLIB_LIBS="${PSLIB_LIBS} ${KAPA_LIBS}"
else
 AC_MSG_NOTICE([libkapa ignored])
fi

dnl ------------------ libjpeg options ---------------------

dnl save LIBS/CFLAGS/LDFLAGS
TMP_LIBS=${LIBS}
TMP_CFLAGS=${CFLAGS}
TMP_LDFLAGS=${LDFLAGS}
TMP_CPPFLAGS=${CPPFLAGS}

AC_ARG_WITH(jpeg,
[AS_HELP_STRING(--with-jpeg=DIR,Specify location of libjpeg.)],
[JPEG_CFLAGS="-I$withval/include"
 JPEG_LDFLAGS="-L$withval/lib"])
AC_ARG_WITH(jpeg-include,
[AS_HELP_STRING(--with-jpeg-include=DIR,Specify libjpeg include directory.)],
[JPEG_CFLAGS="-I$withval"])
AC_ARG_WITH(jpeg-lib,
[AS_HELP_STRING(--with-jpeg-lib=DIR,Specify libjpeg library directory.)],
[JPEG_LDFLAGS="-L$withval"])

CFLAGS="${CFLAGS} ${JPEG_CFLAGS}"
CPPFLAGS=${CFLAGS}
LDFLAGS="${LDFLAGS} ${JPEG_LDFLAGS}"

AC_CHECK_HEADERS([jpeglib.h],[PSLIB_CFLAGS="${PSLIB_CFLAGS} ${JPEG_CFLAGS}"
			      AC_SUBST(JPEG_CFLAGS)],
  [AC_MSG_ERROR([libjpeg headers not found.  Obtain libjpeg from http://www.ijg.org/ or use --with-jpeg to specify location.])]
)

AC_CHECK_LIB(jpeg,jpeg_CreateCompress,[PSLIB_LIBS="${PSLIB_LIBS} ${JPEG_LDFLAGS} -ljpeg"],
  [AC_MSG_ERROR([libjpeg library not found.  Obtain libjpeg from http://www.ijg.org/ or use --with-jpeg to specify location.])]
)

dnl restore the CFLAGS/LDFLAGS
LIBS=${TMP_LIBS}
CFLAGS=${TMP_CFLAGS}
LDFLAGS=${TMP_LDFLAGS}
CPPFLAGS=${TMP_CPPFLAGS}

dnl ------------------ libpng options ---------------------

dnl save LIBS/CFLAGS/LDFLAGS
TMP_LIBS=${LIBS}
TMP_CFLAGS=${CFLAGS}
TMP_LDFLAGS=${LDFLAGS}
TMP_CPPFLAGS=${CPPFLAGS}

AC_ARG_WITH(png,
[AS_HELP_STRING(--with-png=DIR,Specify location of libpng.)],
[PNG_CFLAGS="-I$withval/include"
 PNG_LDFLAGS="-L$withval/lib"])
AC_ARG_WITH(png-include,
[AS_HELP_STRING(--with-png-include=DIR,Specify libpng include directory.)],
[PNG_CFLAGS="-I$withval"])
AC_ARG_WITH(png-lib,
[AS_HELP_STRING(--with-png-lib=DIR,Specify libpng library directory.)],
[PNG_LDFLAGS="-L$withval"])

CFLAGS="${CFLAGS} ${PNG_CFLAGS}"
CPPFLAGS=${CFLAGS}
LDFLAGS="${LDFLAGS} ${PNG_LDFLAGS}"

AC_CHECK_HEADERS([png.h],
  [PSLIB_CFLAGS="$PSLIB_CFLAGS $PNG_CFLAGS" AC_SUBST(PNG_CFLAGS)],
  [HAVE_KAPA=false; AC_MSG_WARN([libpng headers not found: output plots disabled.  Obtain libpng from http://www.ijg.org/ or use --with-png to specify location.])]
)

AC_CHECK_LIB(png,png_init_io,
  [PSLIB_LIBS="$PSLIB_LIBS $PNG_LDFLAGS -lpng"],
  [HAVE_KAPA=false; AC_MSG_WARN([libpng library not found: output plots disabled.  Obtain libpng from http://www.ijg.org/ or use --with-png to specify location.])]
)

dnl restore the CFLAGS/LDFLAGS
LIBS=${TMP_LIBS}
CFLAGS=${TMP_CFLAGS}
LDFLAGS=${TMP_LDFLAGS}
CPPFLAGS=${TMP_CPPFLAGS}

dnl ------------------ use kapa or not? ---------------------

if test "$HAVE_KAPA" == "true" ; then
  AC_MSG_RESULT([including plotting functions])
  AC_DEFINE([HAVE_KAPA],[1],[enable use of libkapa])
else
  AC_MSG_RESULT([skipping plotting functions])
  AC_DEFINE([HAVE_KAPA],[0],[disable use of libkapa])
fi

dnl ------------------- SWIG options ---------------------

dnl save LIBS/CFLAGS/LDFLAGS
TMP_LIBS=${LIBS}
TMP_CFLAGS=${CFLAGS}
TMP_LDFLAGS=${LDFLAGS}
TMP_CPPFLAGS=${CPPFLAGS}

AC_ARG_ENABLE(perlmodule,
 [AS_HELP_STRING(--enable-perlmodule,Enables creation of perl module)],
 [SWIG_REQ="$enableval"],
 [SWIG_REQ="no"])

if test "$SWIG_REQ" == "yes"
then
  AC_MSG_RESULT([enable building perl module])
  AC_ARG_WITH(swig,
  [AS_HELP_STRING(--with-swig=FILE,Specify location of SWIG executable.)],
  [SWIG=$withval],
  [SWIG=`which swig`])
  AC_SUBST(SWIG,$SWIG)

  AC_CHECK_FILE($SWIG,[],
     [AC_MSG_ERROR([SWIG is required.  Obtain it at www.swig.org or use --with-swig to specify its install location.])])
  SUBDIR="${SUBDIR} swig"

dnl restore the CFLAGS/LDFLAGS
LIBS=${TMP_LIBS}
CFLAGS=${TMP_CFLAGS}
LDFLAGS=${TMP_LDFLAGS}
CPPFLAGS=${TMP_CPPFLAGS}

dnl ---------------- PERLPREFIX option -------------------

dnl save LIBS/CFLAGS/LDFLAGS
TMP_LIBS=${LIBS}
TMP_CFLAGS=${CFLAGS}
TMP_LDFLAGS=${LDFLAGS}
TMP_CPPFLAGS=${CPPFLAGS}

  AC_ARG_WITH(perlprefix,
    [AS_HELP_STRING(--with-perlprefix=DIR,Specify prefix of Perl Module's installation. (default=PREFIX))],
    [PERL_PREFIX="$withval"],
    [PERL_PREFIX="$prefix"])
    AC_SUBST(PERL_PREFIX,$PERL_PREFIX)
    AC_SUBST(PERL_INSTALLSYTLE,[`perl '-V:installstyle'`])

else
  AC_MSG_RESULT([disable building perl module])
fi

dnl restore the CFLAGS/LDFLAGS
LIBS=${TMP_LIBS}
CFLAGS=${TMP_CFLAGS}
LDFLAGS=${TMP_LDFLAGS}
CPPFLAGS=${TMP_CPPFLAGS}

dnl for MacOSX suppport -- check if sqrtf is in mx library.

dnl save LIBS/CFLAGS/LDFLAGS
TMP_LIBS=${LIBS}
TMP_CFLAGS=${CFLAGS}
TMP_LDFLAGS=${LDFLAGS}
TMP_CPPFLAGS=${CPPFLAGS}

AC_CHECK_LIB(mx,sqrtf,
  [PSLIB_LIBS="${PSLIB_LIBS} -lmx"])
LIBS=${TMP_LIBS}
AC_CHECK_FUNC(atoll, [],
  [AC_MSG_ERROR([The C99 function, atoll, is required; update your compiler version?])])

dnl restore the CFLAGS/LDFLAGS
LIBS=${TMP_LIBS}
CFLAGS=${TMP_CFLAGS}
LDFLAGS=${TMP_LDFLAGS}
CPPFLAGS=${TMP_CPPFLAGS}

dnl doxygen doc generation is very, very slow so we're turing it off by default

dnl save LIBS/CFLAGS/LDFLAGS
TMP_LIBS=${LIBS}
TMP_CFLAGS=${CFLAGS}
TMP_LDFLAGS=${LDFLAGS}
TMP_CPPFLAGS=${CPPFLAGS}

AC_ARG_ENABLE(doxygen,
  [AS_HELP_STRING(--enable-doxygen ,enable manpage generation)],
  [AC_MSG_RESULT(doxygen enabled)
    AC_PATH_PROG([DOXYGEN], [doxygen], [])
  ],
  [AC_MSG_RESULT([doxygen disabled])
    doxygen=off
  ]
)
AM_CONDITIONAL([HAVE_DOXYGEN], test -n "$DOXYGEN" -a "x$doxygen" != "xoff")

dnl restore the CFLAGS/LDFLAGS
LIBS=${TMP_LIBS}
CFLAGS=${TMP_CFLAGS}
LDFLAGS=${TMP_LDFLAGS}
CPPFLAGS=${TMP_CPPFLAGS}

IPP_VERSION

dnl ------- enable -Werror after all of the probes have run ---------
IPP_STDOPTS
IPP_STDCFLAGS
CFLAGS="${CFLAGS} -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200112L"

dnl ---------------- export variables --------------------

AC_SUBST(SUBDIR,[$SUBDIR])
AC_SUBST(SRCDIRS,[$SRCDIRS])
AC_SUBST(PSLIB_LIBS,[$PSLIB_LIBS])
AC_SUBST(PSLIB_CFLAGS,[$PSLIB_CFLAGS])

AC_SUBST(PS_LANG,[en])

AC_CONFIG_FILES([
  Doxyfile
  Makefile
  etc/Makefile
  m4/Makefile
  pslib-config
  pslib.pc
  share/Makefile
  src/Makefile
  src/astro/Makefile
  src/db/Makefile
  src/fft/Makefile
  src/fits/Makefile
  src/imageops/Makefile
  src/jpeg/Makefile
  src/math/Makefile
  src/mathtypes/Makefile
  src/sys/Makefile
  src/types/Makefile
  test/Makefile
  test/astro/Makefile
  test/db/Makefile
  test/fft/Makefile
  test/fits/Makefile
  test/imageops/Makefile
  test/jpeg/Makefile
  test/math/Makefile
  test/mathtypes/Makefile
  test/pstap/Makefile
  test/pstap/src/Makefile
  test/sys/Makefile
  test/types/Makefile
  test/optime/Makefile
  utils/Makefile
])

#if test "$SWIG_REQ" == "yes"
#then
#  AC_CONFIG_FILES([ swig/Makefile ])
#fi

AC_OUTPUT
