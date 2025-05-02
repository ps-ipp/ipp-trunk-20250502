dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)

AC_INIT([psastro], [0.9.0], [ipp-support@ifa.hawaii.edu])
AC_CONFIG_SRCDIR([src])

AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2])
AM_CONFIG_HEADER([src/config.h])
AM_MAINTAINER_MODE

IPP_STDLDFLAGS

AC_LANG(C)
AC_GNU_SOURCE
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_SYS_LARGEFILE

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
   AC_MSG_NOTICE([libkapa ldflags info supplied by ohana-config])
   AC_MSG_CHECKING([libkapa ldflags])
   KAPA_LIBS="`${KAPA_CONFIG} --libs` -lX11"
   AC_MSG_RESULT([${KAPA_LIBS}])
  fi
fi

if test "$HAVE_KAPA" = "true" ; then
 AC_MSG_NOTICE([libkapa supplied])
 PSASTRO_CFLAGS="${PSASTRO_CFLAGS} ${KAPA_CFLAGS}"
 PSASTRO_LIBS="${PSASTRO_LIBS} ${KAPA_LIBS}"
else
 AC_MSG_NOTICE([libkapa ignored])
fi

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
  [PSASTRO_CFLAGS="$PSASTRO_CFLAGS $JPEG_CFLAGS" AC_SUBST(JPEG_CFLAGS)],
  [HAVE_KAPA=false; AC_MSG_WARN([libjpeg headers not found: output plots disabled.  Obtain libjpeg from http://www.ijg.org/ or use --with-jpeg to specify location.])]
)

AC_CHECK_LIB(jpeg,jpeg_CreateCompress,
  [PSASTRO_LIBS="$PSASTRO_LIBS $JPEG_LDFLAGS -ljpeg"],
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
  [PSASTRO_CFLAGS="$PSASTRO_CFLAGS $PNG_CFLAGS" AC_SUBST(PNG_CFLAGS)],
  [HAVE_KAPA=false; AC_MSG_WARN([libpng headers not found: output plots disabled.  Obtain libpng from http://www.ijg.org/ or use --with-png to specify location.])]
)

AC_CHECK_LIB(png,png_init_io,
  [PSASTRO_LIBS="$PSASTRO_LIBS $PNG_LDFLAGS -lpng"],
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

dnl ------------- psLib, psModules ---------------
PKG_CHECK_MODULES([PSLIB],    [pslib >= 1.0.0])
PKG_CHECK_MODULES([PSMODULE], [psmodules >= 1.0.0])
PKG_CHECK_MODULES([PPSTATS],  [ppStats >= 1.0.0]) 

dnl Set CFLAGS for build
IPP_STDOPTS
IPP_STDCFLAGS

echo "PSASTRO_CFLAGS: $PSASTRO_CFLAGS"
echo "PSASTRO_LIBS: $PSASTRO_LIBS"

IPP_VERSION

AC_SUBST([PSASTRO_CFLAGS])
AC_SUBST([PSASTRO_LIBS])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  psastro.pc
  Doxyfile
])

AC_OUTPUT
