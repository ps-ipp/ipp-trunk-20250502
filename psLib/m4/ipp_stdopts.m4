
dnl IPP_STDCFLAGS must go *after* external probes (to avoid Werror confusing things)
AC_DEFUN([IPP_STDCFLAGS],
[
    dnl this section currently overrides a user-defined CFLAGS
    AC_ARG_ENABLE(optimize,
      [AS_HELP_STRING(--enable-optimize,enable compiler optimization)],
      [AC_MSG_RESULT(compile optimization enabled)
          CFLAGS="-pipe -O2 -g -DPS_NO_TRACE"],
      [AC_MSG_RESULT([compile optimization disabled])
          CFLAGS="-pipe -O0 -g"
      ]
    )
    dnl this section accepts a user-defined CFLAGS, and sets CFLAGS to the empty string if not present
    AC_ARG_ENABLE(debug-build,
      [AS_HELP_STRING(--enable-debug-build,enable debug build: ie disable Werror)],
      [AC_MSG_RESULT(debug build enabled)
        if test x"${CFLAGS}" == x; then
          CFLAGS="-Wall"
	else
          CFLAGS="${CFLAGS=} -Wall"
        fi
      ],
      [AC_MSG_RESULT([debug build disabled])
        if test x"${CFLAGS}" == x; then
          CFLAGS="-Wall -Werror"
	else
          CFLAGS="${CFLAGS=} -Wall -Werror"
        fi
      ]
    )
])

dnl IPP_STDLDFLAGS must go *before* external probes (so linking is done with --no-as-needed if needed)
AC_DEFUN([IPP_STDLDFLAGS],
[
    dnl this section accepts a user-defined LDFLAGS, and sets LDFLAGS to the empty string if not present
    AC_ARG_ENABLE(no-as-needed, 
      [AS_HELP_STRING(--enable-no-as-needed, prevent as-needed option sometimes supplied to gcc such as Ubuntu after 11.11)],
      [AC_MSG_RESULT(no-as-needed passed to linker) 
      	if test x"${LDFLAGS}" == x; then
          LDFLAGS="-Wl,--no-as-needed"
	else
          LDFLAGS="${LDFLAGS=} -Wl,--no-as-needed"
	fi
      ],
      [AC_MSG_RESULT(as-needed linker flags accepted) 
      	if test x"${LDFLAGS}" == x; then
          LDFLAGS=""
	fi
      ]
    )
])
dnl

AC_DEFUN([IPP_STDOPTS],
[
    dnl handle profiler building
    AC_ARG_ENABLE(profile, [AS_HELP_STRING(--enable-profile,enable compiler profiler information inclusion)],
      [AC_MSG_RESULT([profiling enabled, check that you also did --disable-shared])
       CFLAGS="${CFLAGS=} -pg -g"
       LDFLAGS="${LDFLAGS=} -pg"]
    )

    dnl handle path coverage checking
    AC_ARG_ENABLE(coverage,
      [AS_HELP_STRING(--enable-coverage,enable path coverage checking)],
      [AC_MSG_RESULT(path coverage enabled)
       CFLAGS="${CFLAGS=} -lgcov -fprofile-arcs -ftest-coverage -pg"]
    )

    dnl turn off trace messages
    AC_ARG_ENABLE(trace,
      [AS_HELP_STRING(--disable-trace,disable psTrace functionality)],
      [AC_MSG_RESULT(psTrace disabled)
       CFLAGS="${CFLAGS=} -DPS_NO_TRACE"]
    )
])


AC_DEFUN([IPP_VERSION],
[
	AC_ARG_ENABLE(version,
		[AS_HELP_STRING(--disable-version,Disable dynamic version information)],
		[case "${enableval}" in
		      yes) enable_version=true ;;
		      no)  enable_version=false ;;
		      *)   AC_MSG_ERROR(bad value ${enableval} for --disable-version) ;;
	         esac], [enable_version=true]
	)
	
	AS_IF([test "x$enable_version" = xtrue],
		[AC_PATH_PROG([SVNVERSION], [svnversion])
		AC_PATH_PROG([SVN], [svn])]
	)
	AC_PROG_SED
	AM_CONDITIONAL([HAVE_SVNVERSION], [test "x$SVNVERSION" != x])
	AM_CONDITIONAL([HAVE_SVN], [test "x$SVN" != x])
])
