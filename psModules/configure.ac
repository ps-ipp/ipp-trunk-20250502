AC_PREREQ(2.61)

AC_INIT([psmodules],[1.1.0],[http://pan-starrs.ifa.hawaii.edu/bugzilla])
AC_CONFIG_SRCDIR([psmodules.pc.in])

dnl this enables the building of libtap
AC_CONFIG_SUBDIRS([test/tap])

AM_INIT_AUTOMAKE([1.7 foreign dist-bzip2 subdir-objects])
AM_CONFIG_HEADER([src/config.h])
AM_MAINTAINER_MODE

PSMODULES_LT_VERSION="1:1:0"
AC_SUBST(PSMODULES_LT_VERSION,$PSMODULES_LT_VERSION)

IPP_STDLDFLAGS

AC_LANG(C)
AC_PROG_CC_C99
AC_GNU_SOURCE
AC_C_INLINE
AC_C_CONST
AC_PROG_INSTALL
AM_PROG_LIBTOOL
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

AC_PREFIX_DEFAULT([`pwd`])

dnl build tests at the same time as the source code
AC_ARG_ENABLE(tests,
  [AS_HELP_STRING(--enable-tests,build tests at same time as source)],
  [AC_MSG_RESULT(test building enabled)
   tests=true],
   [tests=false])
AM_CONDITIONAL(BUILD_TESTS, test x$tests = xtrue)

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

SRCPATH='${top_srcdir}/src'
SRCDIRS="extras config concepts camera astrom detrend imcombine objects"
# escape two escapes at this level so \\ gets passed to the shell and \ to perl
SRCINC=`echo "${SRCDIRS=}" | ${PERL} -pe "s|(\w+)|-I\\\\${SRCPATH=}/\1|g"`
SRCINC="-I${SRCPATH=} ${SRCINC=}"
SRCSUBLIBS=`echo "${SRCDIRS=}" | ${PERL} -pe "s|(\w+)|\1/libpsmodules\1.la|g"`
AC_SUBST(SRCSUBLIBS,${SRCSUBLIBS=})
AC_SUBST(SRCINC,${SRCINC=})
AC_SUBST([SRCDIRS],${SRCDIRS=})

dnl doxygen -------------------------------------------------------------------
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

dnl ------------------------------------------------------------

AC_PATH_PROG([ERRORCODES], [psParseErrorCodes], [missing])
if test "$ERRORCODES" = "missing" ; then
  AC_MSG_ERROR([psParseErrorCodes is required])
fi

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
 PSMODULES_CFLAGS="${PSMODULES_CFLAGS} ${KAPA_CFLAGS}"
 PSMODULES_LIBS="${PSMODULES_LIBS} ${KAPA_LIBS}"
else
 AC_MSG_NOTICE([libkapa ignored])
fi

dnl HAVE_KAPA is set to false if any of the tests fail
dnl HAVE_KAPA=true
dnl AC_CHECK_HEADERS([kapa.h],
dnl  [PSMODULES_CFLAGS="$PSMODULES_CFLAGS $KAPA_CFLAGS" AC_SUBST(KAPA_CFLAGS)],
dnl  [HAVE_KAPA=false; AC_MSG_WARN([libkapa headers not found: output plots disabled.  Obtain libkapa at http://kiawe.ifa.hawaii.edu/Elixir/Ohana or use --with-kapa to specify location.])]
dnl )
dnl AC_CHECK_LIB(kapa,KapaInitGraph,
dnl  [PSMODULES_LIBS="$PSMODULES_LIBS $JPEG_LDFLAGS -ljpeg"],  
dnl  [HAVE_KAPA=false; AC_MSG_WARN([libkapa headers not found: output plots disabled.  Obtain libkapa at http://kiawe.ifa.hawaii.edu/Elixir/Ohana or use --with-kapa to specify location.])],[-lm]
dnl )

dnl restore the CFLAGS/LDFLAGS
LIBS=${TMP_LIBS}
CFLAGS=${TMP_CFLAGS}
LDFLAGS=${TMP_LDFLAGS}
CPPFLAGS=${TMP_CPPFLAGS}

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

AC_CHECK_HEADERS([jpeglib.h],
  [PSMODULES_CFLAGS="$PSMODULES_CFLAGS $JPEG_CFLAGS" AC_SUBST(JPEG_CFLAGS)],
  [HAVE_KAPA=false; AC_MSG_WARN([libjpeg headers not found: output plots disabled.  Obtain libjpeg from http://www.ijg.org/ or use --with-jpeg to specify location.])]
)

AC_CHECK_LIB(jpeg,jpeg_CreateCompress,
  [PSMODULES_LIBS="$PSMODULES_LIBS $JPEG_LDFLAGS -ljpeg"],
  [HAVE_KAPA=false; AC_MSG_WARN([libjpeg library not found: output plots disabled.  Obtain libjpeg from http://www.ijg.org/ or use --with-jpeg to specify location.])]
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
  [PSMODULES_CFLAGS="$PSMODULES_CFLAGS $PNG_CFLAGS" AC_SUBST(PNG_CFLAGS)],
  [HAVE_KAPA=false; AC_MSG_WARN([libpng headers not found: output plots disabled.  Obtain libpng from http://www.ijg.org/ or use --with-png to specify location.])]
)

AC_CHECK_LIB(png,png_init_io,
  [PSMODULES_LIBS="$PSMODULES_LIBS $PNG_LDFLAGS -lpng"],
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

dnl pslib ---------------------------------------------------------------------
AC_ARG_WITH(pslib-config,
[  --with-pslib-config=FILE  Specify location of psLib-config script],
[PSLIB_CONFIG=$withval])

if test -z ${PSLIB_CONFIG} ; then
  PKG_CHECK_MODULES([PSLIB], [pslib >= 0.12.0])
else
  AC_CHECK_FILE($PSLIB_CONFIG,[],
    [AC_MSG_ERROR([psLib is required.  If not in path, use --with-pslib-config to specify pslib-config script location.])])
  AC_MSG_CHECKING([PSLIB_CFLAGS])
  PSLIB_CFLAGS="`${PSLIB_CONFIG} --cflags`"
  AC_MSG_RESULT([${PSLIB_CFLAGS}])
  AC_MSG_CHECKING([PSLIB_LIBS])
  PSLIB_LIBS="`${PSLIB_CONFIG} --libs`"
  AC_MSG_RESULT([${PSLIB_LIBS}])
fi

PSMODULES_CFLAGS="${PSMODULES_CFLAGS=} ${PSLIB_CFLAGS}"
PSMODULES_LIBS="${PSMODULES_LIBS=} ${PSLIB_LIBS}"

dnl nebclient -----------------------------------------------------------------

PKG_CHECK_MODULES([NEBCLIENT], [nebclient >= 0.0.2],
    [AC_DEFINE([HAVE_NEBCLIENT], 1, [Define to 1 if libnebclient is avaiable])],    [AC_MSG_RESULT([no])]
)

PSMODULES_CFLAGS="${PSMODULES_CFLAGS=} ${NEBCLIENT_CFLAGS}"
PSMODULES_LIBS="${PSMODULES_LIBS=} ${NEBCLIENT_LIBS}"

echo "PSMODULES_CFLAGS: $PSMODULES_CFLAGS"
echo "PSMODULE_LIBS: $PSMODULES_LIBS"

IPP_VERSION(PSMODULES)
AM_CONDITIONAL([HAVE_SVNVERSION], [test "x$SVNVERSION" != x])
AM_CONDITIONAL([HAVE_SVN], [test "x$SVN" != x])

dnl ------- enable -Werror after all of the probes have run ---------
IPP_STDOPTS
IPP_STDCFLAGS

dnl enable POSIX/XSI and C99 compliance
CFLAGS="${CFLAGS=} -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200112L"

dnl ---------------- export variables --------------------


AC_SUBST([PSMODULES_CFLAGS])
AC_SUBST([PSMODULES_LIBS])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/astrom/Makefile
  src/camera/Makefile
  src/config/Makefile
  src/concepts/Makefile
  src/detrend/Makefile
  src/imcombine/Makefile
  src/objects/Makefile
  src/extras/Makefile
  test/Makefile
  test/astrom/Makefile
  test/config/Makefile
  test/camera/Makefile
  test/concepts/Makefile
  test/detrend/Makefile
  test/extras/Makefile
  test/imcombine/Makefile
  test/objects/Makefile
  test/pstap/Makefile
  test/pstap/src/Makefile
  Doxyfile
  psmodules-config
  psmodules.pc
])

AC_OUTPUT
