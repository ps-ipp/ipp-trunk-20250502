/* Copyright (C) 1991-1999  Canada-France-Hawaii Telescope Corp.         */
/* This program is distributed WITHOUT any warranty, and is under the    */
/* terms of the GNU General Public License, see the file COPYING         */
/*****************************************************************************
 *
 * file: cfht.h
 * $Id: cfht.h,v 1.1.1.1 2004-11-24 04:39:33 eugene Exp $
 * $Locker:  $
 *
 * This file is the main include file for the generic library.
 *
 *
 * HISTORY
 *
 * who       when          what
 * -------   -----------   ----------------------------------------------
 * jab       27 Jan 1988   Original coding
 * jab        1 Mar 1988   Added config file stuff
 * jab       12 Mar 1988   Added logging stuff
 * jk        22 Mar 1988   Added cfht_od()
 * jk/jab    24 Mar 1988   Added CFHT_TAPE and CFHT_DISPLAY
 * jab        6 Apr 1988   Added puma link device
 * jab       29 Apr 1988   Added already included check
 * sss       08 May 1988   Added CFHT_NPIPE
 * jk        17 May 1988   Modified tape and display because of collision
 * jk        06 Oct 1988   added cfht_resetetimer as a define
 * sss       17 Mar 1989   added cfht_log() support stuffs
 * sss       11 Apr 1989   added real time defines for detectors,etc.
 * sss       24 Oct 1989   added CFHT_RELEASE define for .,config access
 * sss        3 Jan 1990   added CFHT_*SESSIONHOST defines for env. vars
 * jrw       01 Oct 1990   added stuff for cfht_number
 * jrw       02 Nov 1990   added CFHT_STR_* defines
 * jk        27 Feb 1991   cfht_exec unions not used anymore
 * sss       27 Mar 1991   added cfht_basename()
 * jrw       09 May 1991   added function definitions (ANSI and crufty flavors)
 * jk        29 Aug 1991   changed cfht_errno from BOOLEAN to int
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/07/25 02:59:23  eugene
 * import Ohana
 *
 * Revision 1.44  2000/04/10  11:28:40  isani
 * Added prototype for cfht_log_proxy.
 * 
 * Revision 1.43  2000/02/10 13:19:08  isani
 * Added include of sys/types.h whenever pid_t is needed.
 *
 * Revision 1.42  1999/12/02 21:40:30  thomas
 * Added PFVint
 *
 * Revision 1.41  1999/07/25 06:29:57  thomas
 * Added UTDATE/TIME and PFDATE defs
 *
 * Revision 1.40  1999/04/29 17:18:00  thomas
 * Added @ ( # ) to RCSID string for what(1) to find
 * Added non-gcc but STDC version of above
 * Added changes for switch to /cfht
 *
 * Revision 1.39  99/02/17  13:55:15  13:55:15  thomas (Jim Thomas)
 * Added cfht_cfhthome
 * 
 * Revision 1.38  98/12/11  18:23:20  18:23:20  thomas (Jim Thomas)
 * Added CFHT_*OBSERVERPATH
 * 
 * Revision 1.37  98/10/22  17:52:44  17:52:44  thomas (Jim Thomas)
 * Added CFHT_ESESSION
 * 
 * Revision 1.36  98/07/16  15:53:32  15:53:32  thomas (Jim Thomas)
 * Added cfht_system
 * 
 * Revision 1.35  98/07/07  12:52:47  12:52:47  thomas (Jim Thomas)
 * Added CFHTPATH , IOCONFDIR , and CONFDIR definitions
 * 
 * Revision 1.34  98/04/09  17:52:20  17:52:20  thomas (Jim Thomas)
 * Added cfht_argsToString
 * 
 * Revision 1.33  98/02/25  16:35:00  16:35:00  thomas (Jim Thomas)
 * Added prototype for cfht_logpv
 * 
 * Revision 1.32  97/12/09  00:11:48  00:11:48  healey (SueAnn Healey)
 * added CFHT_LIBSSX
 * 
 * Revision 1.31  1997/12/01 16:03:29  isani
 * Moved missing protos to separate file.
 * Added extern "C" declarations for C++ projects.
 *
 * Revision 1.30  1997/09/17 13:48:31  thomas
 * Added CFHT_EXEC_NOT_EXITED and CFHT_EXEC_FAILED
 * Documented bogus use of FAIL in exp_controller_t
 *
 * Revision 1.29  97/06/29  15:41:07  15:41:07  isani (Sidik Isani)
 * Had to fiddle with some of the includes to get to compile with GCC
 *   on the sparc engines.
 * 
 * Revision 1.28  97/05/28  02:51:35  isani
 * added CFHT_SIG_ defines for handler argument of cfht_signal()
 * 
 * Revision 1.27  97/04/07  20:10:26  thomas
 * Added LIBOMS, changed cfht_signal event prototypes
 * 
 * Revision 1.26  96/11/22  02:36:44  02:36:44  isani (Sidik Isani)
 * added macro for TEMP_FAILURE_RETRY, in case anyone wants to use it
 * changed cfht_delay(n) into a macro that calls cfht_sleep(n,1)
 * 
 * Revision 1.25  96/11/19  22:24:19  isani
 * moved P() and Q() macros closer to top of file
 * added RCSID() macro
 * define volatile to nothing on compilers where everything is volatile
 * define UNUSED for use with gcc, or nothing on compilers that don't care
 * added a few more missing libcfht prototypes
 * concentrated most of the old-sun hacks here rather than in each file
 *   (protos for getenv, putenv, different name for mktime, etc.)
 * fixed order of arguments in cfht_fromjulian prototype
 * 
 * Revision 1.24  96/08/14  13:01:17  13:01:17  thomas (Jim Thomas)
 * Added TCS_ConsolePrompt error return code
 * 
 * Revision 1.23  96/05/20  16:01:16  16:01:16  thomas (Jim Thomas)
 * Added LIBCX, TCS_CommandFailed,
 * Fixed F4 cfht_errno definitions
 * 
 * Revision 1.22  96/02/21  11:16:07  11:16:07  isani (Sidik Isani)
 * cfht_errno codes for gecko and f4 server
 * 
 * Revision 1.21  1995/07/19 12:48:00  thomas
 * Added AOB cfht_errno codes
 *
 * Revision 1.20  95/06/29  11:38:38  11:38:38  thomas (Jim Thomas)
 * Added cfht_errno codes for IDS
 * 
 * Revision 1.19  95/06/13  12:03:06  12:03:06  thomas (Jim Thomas)
 * Added environment variables and cfht_log_who entries for CS, DUCK, HELP,
 * IDS, PIXD, TCS, and UI libraries
 * 
 * Revision 1.18  95/06/07  17:22:30  17:22:30  thomas (Jim Thomas)
 * Added typedef for cfht_errno codes
 * 
 * Revision 1.17  94/12/28  17:06:51  17:06:51  thomas (Jim Thomas)
 * Moved strerror definition for suns here from cfp.h (for cfht_logv)
 * 
 * Revision 1.16  94/12/21  10:35:46  10:35:46  john (John Kerr)
 * add pid_t.
 * 
 * Revision 1.15  94/12/15  10:46:42  10:46:42  john (John Kerr)
 * modified pid in cfht_exec_data to be a pid_t type.
 * 
 * Revision 1.14  94/12/14  10:50:01  10:50:01  thomas (Jim Thomas)
 * Added CFHT_LIBMUSIC, CFHT_VALUE_SIZE, Q macro
 * Deleted BYTE
 * 
 * Revision 1.13  94/09/20  18:40:45  18:40:45  thomas (Jim Thomas)
 * added CFHT_LOGONLY, CFHT_LIBOCS
 * reworked symbols related to cfht_log
 * 
 * Revision 1.12  94/08/14  10:27:17  10:27:17  jwright (Jim Wright)
 * add warning log messages and color messages
 * 
 * Revision 1.11  94/07/11  15:48:53  15:48:53  veran (Jean-Pierre Veran)
 * AAdded net logging capabilities
 * 
 * Revision 1.10  94/02/02  14:08:00  14:08:00  jwright (Jim Wright)
 * remove typedef of FILE_ID form cfp.h and put it in cfht.h.  reason is
 * that cfht_exec() has an argument of type FILE_ID and thus needs to have
 * this type visible so that it can correctly declare an ANSI prototype.
 * it is presumed that any file including cfp.h will first have included
 * cfht.h.
 * 
 * Revision 1.9  94/01/20  20:35:13  20:35:13  jwright (Jim Wright)
 * fix enum
 * 
 * Revision 1.8  94/01/20  04:51:31  04:51:31  jwright (Jim Wright)
 * add enumeration for controller types
 * 
 * Revision 1.7  93/11/26  10:14:09  10:14:09  jwright (Jim Wright)
 * added definitions to retrieve info from net.par
 * 
 * Revision 1.6  93/05/19  08:45:49  08:45:49  steve (Steven Smith)
 * added cfht_logv TIMING define
 * 
 * Revision 1.5  93/02/16  16:21:15  16:21:15  john (John Kerr)
 * added a CFHT_INSTRUMENT_TYPE
 * 
 * Revision 1.4  92/12/01  19:21:47  19:21:47  steve (Steven Smith)
 * added CFHT_ELOGHOST for cfht_log() net based
 * 
 * Revision 1.3  92/11/09  13:11:03  13:11:03  john (John Kerr)
 * added void cfht_goDaemon()
 * 
 * Revision 1.2  91/11/15  10:07:52  10:07:52  jwright (Jim Wright)
 * add macro to handle both ansi and k&r style parameters
 * in function declarations; convert declarations to use macro
 * 
 * Revision 1.1  91/09/24  15:59:31  15:59:31  steve (Steven S Smith)
 * Initial revision
 * 
 ****************************************************************************/

#ifndef CFHTDOTH
#define CFHTDOTH

#ifdef __cplusplus
extern "C" {
#endif

#ifndef pid_t
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif /* pid_t */

/******************************************************************
 * macro to declare prototypes for ansi, but still works with k&r *
 *                                                                *
 * define prototype like                                          *
 *        int foo P((int a, char c))                              *
 *                                                                *
 * note that the space before the P and the doubled parentheses   *
 *      are needed                                                *
 ******************************************************************/

#ifndef P
#ifdef __STDC__
#define P(args) args
#else
#define P(args) ()
#endif
#endif

/********************************************************************
 * macro to declare routine parameters for ansi that works with k&r *
 *                                                                  *
 * define actual routine like                                       *
 *        int foo Q((int a, char c), (a, c), int a; char c;) {      *
 *                                                                  *
 * note that inner parentheses are needed on the ansi parameters    *
 *                                 needed on the K&R parameters     *
 *                             and must not be there on the K&R     *
 *                                          type definition         *
 *           there must be a space before the Q                     *
 *           the arguments to Q can be on separate lines like       *
 *                         int foo Q((int a, char c),               *
 *                                   (a, c),                        *
 *                                   int a; char c;)                *
 *                         {                                        *
 ********************************************************************/

#ifndef Q
#ifdef __STDC__
#define Q(ansi, karparm, kartype) ansi
#else
#define Q(ansi, karparm, kartype) karparm kartype
#endif
#endif

/*
 * UNUSED attribute to tell Gnu compiler not to warn
 * and define volatile to nothing for compilers where everything
 * is volatile (We hope!)
 */
#ifdef __STDC__
#ifdef __GNUC__
#define UNUSED __attribute__ ((__unused__))
#else
#define UNUSED
#endif
#else
#define UNUSED
#define volatile
#endif

/*
 * Macro to use for inserting rcs id's 
 */

#ifdef __LINT__
#define RCSID(string) extern int lint_rcs_id_dummy
#else
#if defined(__GNUC__) && defined(__STDC__)
#define RCSID(string) \
  static const char rcs_id[] __attribute__ ((__unused__)) = "@(#) " string
#else
#if defined(__STDC__)
#define RCSID(string) static char rcs_id[] = "@(#) " string
#else
#define RCSID(string) static char rcs_id[] = string
#endif
#endif
#endif

/*********************
 * type definitions *
 ********************/

typedef int BOOLEAN;           /* should only contain TRUE, or FALSE */
typedef double ANGLE;          /* floating pt. arc seconds */
typedef double TIME;           /* floating pt. seconds */
typedef double JDATE;          /* julian day number */
typedef char *DATE;            /* ascii string date */
typedef int PASSFAIL;          /* for functions that return PASS, or FAIL */
typedef void (*PFV) P((void));   /* pointer to function that returns void */
typedef void (*PFVint) P((int));   /*  ditto, with int parameter */
typedef void (*PFVui) P((int,int,void*));  /* used for ui callbacks */
typedef void (*PFVexec) P((pid_t,int));    /* used by cfht_exec() */

enum exp_controller_t {       /* valid detector controller types */
    FAIL = -1,			  /* NOTE - depends on define below :-( */
    CONTROLLER_GENIII,
    CONTROLLER_CC200,
    CONTROLLER_FTS,
    CONTROLLER_RETICON
};

typedef int FILE_ID;              /* used for parse file instance handles */

/*************
 * constants *
 *************/

#define PASS  0           /* function returning with no error */
#define FAIL  -1          /* function returning an error in errno */
			  /* lots of things depend on FAIL being negative!! */

#define TRUE  1           /* 'C' boolean */
#define FALSE 0           /* 'C' boolean */

#ifndef NULL              /* make sure it's not already defined */
#define NULL  0           /* used mostly for null string pointers */
#endif  /* NULL */

#define CFHTLOCKS  "/tmp/cfhtlocks"

/********************
 * real time defs   *
 ********************/

#define CFHT_RT_DETECTOR 65    /* about 1/2 */
#define CFHT_RT_MOMMA    75    /* not as fast */
#define CFHT_RT_HANDLER  85    /* slower yet */

/********************
 * exec definitions *
 ********************/

#define CFHT_EXEC_NUM    10    /* number of pending deaths allowed */
#define CFHT_EXEC         1    /* straight exec */
#define CFHT_EXECFG       2    /* wait and return exit code */
#define CFHT_EXECBG       3    /* dont wait and dont ever bother me */
#define CFHT_EXECBGC      4    /* dont wait but deposit exit code when ... */
#define CFHT_EXECBGH      5    /* dont wait but call handler when ... */
                               /* (handler is called with pid and exit code */
#define CFHT_EXEC_NOT_EXITED -1 /* initial value of exit code */
#define CFHT_EXEC_FAILED -2    /*  exit code if ran but failed to exit */
/* #define CFHT_EXEC_EXITED (8 bits of system exit code) */ /* normal exit */

#define CFHT_ARGV_NUM   100    /* size of cfht_argv() arg list */
#define CFHT_ARGS_SIZE 1024    /* size of string to make args out of */
#define CFHT_VALUE_SIZE 256    /* max length of a value (as in name=value) */

union cfht_exec_addr {
    volatile int *code;        /* exit code (-1 -> not yet exited) */
    PFV func;                  /* handler to call */
};

struct cfht_exec_data {
    volatile pid_t pid;        /* process id (0 -> unused) */
    volatile int type;         /* CFHT_EXECBGC, or CFHT_EXECBGH (0 -> ignore) */
    volatile void *addr;       /* union not used anymore */
};

extern int cfht_exec_rtprio;

/*
 * paths and file names
 * CFHT_E* is the name in the environment, without the E is the actual value
 * at the moment - so try not to use these (unfortunately that means programs
 * don't work outside the session, sigh, so they appear in cfp_file.c and
 * roll.c at least :-(
 */
/*
 * The original path to everything started at /usr/local/cfht .  To get things
 * out from under /usr/local , we created an automount collection for /cfht .
 * To make things more consistent with other packages, we changed from
 * /usr/local/cfht/dev to /cfht/src for the top of the source tree.  While
 * both paths still exist, handle both possibilities here (and nowhere else in
 * any c code please :-)
 */
#if defined(HPUX10) || defined(Solaris)
#define CFHT_ECFHTPATH   "cfhtdir"			/* top of our tree */
#define CFHT_CFHTPATH    "/cfht"			/*   which is */
#define CFHT_CONFDIR     "src/conf"			/* dir for par files */
#define CFHT_IOCONFDIR   "conf"				/*   except ioconfig */
#define CFHT_ECONFPATH   "CFHTCONFPATH"			/* path to par files */
#define CFHT_CONFPATH    "/cfht/src/conf"		/*   which is */
#else
#define CFHT_ECFHTPATH   "cfhtdir"			/* top of our tree */
#define CFHT_CFHTPATH    "/usr/local/cfht"		/*   which is */
#define CFHT_CONFDIR     "dev/conf"			/* dir for par files */
#define CFHT_IOCONFDIR   "conf"				/*   except ioconfig */
#define CFHT_ECONFPATH   "CFHTCONFPATH"                 /* path to par files */
#define CFHT_CONFPATH    "/usr/local/cfht/dev/conf"     /*   which is */
#endif /* HPUX10 or Solaris */
#define CFHT_ECONFNAME   "CFHTCONFNAME"                 /* name for .,config */
#define CFHT_CONFNAME    "config"                       /*   which is */
#define CFHT_ESESSION    "Session"			/* master session name */
#define CFHT_EOBSERVERPATH "observer"                   /* path to top of homes */
#define CFHT_OBSERVERPATH "/users/observer"             /*   which is */

/* identifiers within config file */
#define CFHT_DETECTOR    "detector"                     /* e.g. th1 */
#define CFHT_RELEASE     "release"                      /* e.g. 901231 */
#define CFHT_INSTRUMENT  "instrument"                   /* 'who am i' */
#define CFHT_INSTRUMENT_TYPE  "instrument_type"         /* e.g. FTS */
#define CFHT_HANDLERS    "handlers"                     /* e.g. tcsh, ccdh */
#define CFHT_FOCUS       "focus"                        /* e.g. prime */
#define CFHT_DISPLAY     "display_guy"                  /* display handler */
#define CFHT_TAPE        "tape_guy"                     /* tape handler */
#define CFHT_SESSION     "session"                      /* e.g. ccd, focam */

/*****************
 * net.par stuff *
 *****************/

#define CFHT_CCDSERVER	"ccdserver"
#define CFHT_F4SERVER	"f4server"
#define CFHT_FOCAMSERVER "focamserver"
#define CFHT_GENSERVER	"genserver"
#define CFHT_LOGSERVER	"logserver"
#define CFHT_RFSERVER	"rfserver"
#define CFHT_TCSSERVER	"tcsserver"
#define CFHT_TRAFFIC	"traffic"

/**********************
 * session host stuff *
 **********************/

#define CFHT_ESESSIONHOST    "SESSIONHOST"    /* used in xstart files */
#define CFHT_SESSIONHOST     "moe"            /* perhaps hostname()? */

/*****************
 * logging stuff *
 *****************/

/* environment variables */

/*
 * The following are viewed as boolean flags.  For each, the indicated setting
 * has the described effect (e.g., "setenv CFHTERROR off" will cause error
 * messages not to appear in the feedback window), and the inverse setting or
 * no setting has the opposite effect (e.g., otherwise error messages appear
 * in the feedback window).
 */
#define CFHT_EERROR     "CFHTERROR"     /* Off -> no error messages to
					 * feedback window */
#define CFHT_EWARN      "CFHTWARN"      /* Off -> no warning messages to
					 * feedback window */
#define CFHT_ENODISP    "CFHTNODISP"    /* On -> no START/STATUS/DONE messages
					 * to feedback window */
#define CFHT_EDEBUG     "CFHTDEBUG"     /* On -> CFHT_MAIN debug messages to
					 * log file */
#define CFHT_ELIB       "CFHTLIB"       /* On -> unknown lib debug messages to
					 * log file */
#define CFHT_ELIBCAMAC  "CFHTLIBCAMAC"  /* On -> libcamac debug to log file */
#define CFHT_ELIBCCD    "CFHTLIBCCD"    /* On -> libccd debug to log file */
#define CFHT_ELIBCFHT   "CFHTLIBCFHT"   /* On -> libcfht debug to log file */
#define CFHT_ELIBCFP    "CFHTLIBCFP"    /* On -> libcfp debug to log file */
#define CFHT_ELIBCS     "CFHTLIBCS"     /* On -> libcs debug to log file */
#define CFHT_ELIBCX     "CFHTLIBCX"     /* On -> libcx debug to log file */
#define CFHT_ELIBDUCK   "CFHTLIBDUCK"   /* On -> libduck debug to log file */
#define CFHT_ELIBFF     "CFHTLIBFF"     /* On -> libff debug to log file */
#define CFHT_ELIBHELP   "CFHTLIBHELP"   /* On -> libhelp debug to log file */
#define CFHT_ELIBHH     "CFHTLIBHH"     /* On -> libhh debug to log file */
#define CFHT_ELIBIDS    "CFHTLIBIDS"    /* On -> libids debug to log file */
#define CFHT_ELIBMUSIC  "CFHTLIBMUSIC"  /* On -> libmusic debug to log file */
#define CFHT_ELIBOCS    "CFHTLIBOCS"    /* On -> libocs debug to log file */
#define CFHT_ELIBOMS    "CFHTLIBOMS"    /* On -> liboms debug to log file */
#define CFHT_ELIBPIXD   "CFHTLIBPIXD"   /* On -> libpixd debug to log file */
#define CFHT_ELIBRET    "CFHTLIBRET"    /* On -> libret debug to log file */
#define CFHT_ELIBRET    "CFHTLIBRET"    /* On -> libret debug to log file */
#define CFHT_ELIBSSX    "CFHTLIBSSX"    /* On -> libssx debug to log file */
#define CFHT_ELIBTCS    "CFHTLIBTCS"    /* On -> libtcs debug to log file */
#define CFHT_ELIBUI     "CFHTLIBUI"     /* On -> libui debug to log file */

#define CFHT_ECOLORLOG	"CFHTCOLORLOG" 	/* Off -> no color on user log */

#define CFHT_LOGU       "CFHTLOGU"      /* user log messages file name */
#define CFHT_LOGS       "CFHTLOGS"      /* system log messages file name */
#define CFHT_ELOGHOST	"CFHTLOGHOST" 	/* host for netbased logging */

/* who  (NOTE, should be alphabetized only at release time) */
typedef enum {
    CFHT_MAIN = 0,		  /* caller is end user */
    CFHT_LIB,			  /* caller is undefined library */
    CFHT_LIBCAMAC,		  /* caller is CAMAC library */
    CFHT_LIBCCD,		  /* caller is ccd library */
    CFHT_LIBCFHT,		  /* caller is CFHT library */
    CFHT_LIBCFP,		  /* caller is CFP library */
    CFHT_LIBFF,			  /* caller is FITS library */
    CFHT_LIBHH,			  /* caller is high level HPIB library */
    CFHT_LIBMUSIC,		  /* caller is music library */
    CFHT_LIBOCS,		  /* caller is OCS library */
    CFHT_LIBRET,		  /* caller is reticon library */
    CFHT_LIBCS,			  /* caller is client/server library */
    CFHT_LIBDUCK,		  /* caller is DUCK library */
    CFHT_LIBHELP,		  /* caller is Pegasus help library */
    CFHT_LIBIDS,		  /* caller is IDS communication library */
    CFHT_LIBPIXD,		  /* caller is pixel library */
    CFHT_LIBTCS,		  /* caller is Telescope Control System */
    CFHT_LIBUI,			  /* caller is user interface library */
    CFHT_LIBCX,			  /* caller is coordinate transform library */
    CFHT_LIBOMS,		  /* caller is OMS library */
    CFHT_LIBSSX			  /* caller is ssx controller library */
} cfht_log_who;

/* type */
typedef enum {
    CFHT_LOG_ID = 0,		  /* client side initialization msg type */
    CFHT_START,			  /* program is starting */
    CFHT_STATUS,		  /* something interesting to the user */
    CFHT_ERROR,			  /* error */
    CFHT_FATAL = CFHT_ERROR,	  /* obsolete - about to exit due to error */
    CFHT_ERRNO,			  /* do not use - error with errno message */
    CFHT_WARN,			  /* warning - something's wrong */
    CFHT_DONE,			  /* about to exit normally */
    CFHT_LOGONLY,		  /* operational diagnositc info */
    CFHT_DEBUG,			  /* program trace info */
    CFHT_TIMING,		  /* print elapsed + delta times */
    CFHT_FEEDINIT,		  /* init a feedback type output sink */
    CFHT_FEEDQUIT,		  /* free up fd's, etc for feedback sinks */
    CFHT_ROLLINIT,		  /* start up another roll type output sink */
    CFHT_ROLLQUIT,		  /* shut down roll type output sinks */
    CFHT_SHOWSINKS,		  /* used by developer to debug...  */
    CFHT_SERVINIT		  /* re-init da logserver without killing it */
} cfht_log_type;

#define CFHT_DATE_SIZE 29	  /* size needed for cfht_date result */
#define CFHT_UTDATE_SIZE 11	  /* size needed for cfht_UTdate result */
#define CFHT_PFDATE_SIZE 11	  /* size needed for cfht_PFdate result */
#define CFHT_TIME_SIZE 12	  /* size needed for cfht_time result */
#define CFHT_UTTIME_SIZE 12	  /* size needed for cfhtUTtime result */

/***********************
 * parsing definitions *
 ***********************/

#define CFHT_STR_MATCH     1  /* check if input matches one of set of strings */
#define CFHT_STR_INCLUDE   2  /* only pass through characters mentioned */
#define CFHT_STR_EXCLUDE   3  /* only pass through characters not mentioned */
#define CFHT_STR_NOWHITE   4  /* remove all white space from string */
#define CFHT_STR_NOTRAIL   5  /* remove all trailing white space */
#define CFHT_STR_ESCAPE    6  /* convert non-printing chars to escape codes */
#define CFHT_STR_UNESCAPE  7  /* convert escape codes to chars */

/**************************
 * cfht_errno definitions *
 **************************/

typedef enum {
    /* general error numbers */
    CHECK_ERRNO = -1,		  /* check unix(tm) errno variable for reason */

    /* unfortunately, some code stuffs errno into cfht_errno (e.g., old libids) */
    /* so we need to use 0 but skip all low positive values */

    NO_ERROR = 0,		  /* success */
    NOT_A_TTY = 1000,		  /* historical uses, and general laziness */
    PARSE_FAILED,		  /* unable to cope with input */
    TOO_BIG,			  /* input exceeds hardware limit */
    OUT_OF_RANGE,		  /* input exceeds software restriction */
    TYPE_ERROR,			  /* incompatible or unexpected type */
    UNIMPLEMENTED,		  /* feature not yet implemented */
    BAD_OPTION,			  /* option passed in to routine was invalid */
    INVALID_RANGE,		  /* range structure malformed or illegal */

    /* OCS error codes */

    OCS_FAILURE = 1100,		  /* internal OCS problem, e.g., buffers*/
    OCS_NoSuchServer,		  /* requested non-existent server name */
    OCS_SERVER_BUSY,		  /* couldn't get server's attention */
    OCS_SERVER_T_REJECTED,	  /* task rejected by the server */
    OCS_SERVER_T_FAILED,	  /* task didn't complete successfully */
    OCS_NOTIFYCLOSED,		  /* server quit during transactions */
    OCS_SERVER_TIMEOUT,		  /* server time-out on request */
    OCS_ERROR,			  /* catchall */

    /* OCS stuffs music's merrno in, so those should be here too */

    M_NOSTART = 1200,		  /* did not call mstart() */
    M_BODYTOOBIG,		  /* body overflow */
    M_WRITERR,			  /* system error during write */
    M_PARTWRITE,		  /* wrote partial message */
    M_READERR,			  /* system error during read */
    M_READCLOSED,		  /* read a closed fd */
    M_BODYTOOSHORT,		  /* not enough in body for mget() */
    M_BADMAGIC,			  /* bad magic numbers in header */
    M_TIMEOUT,			  /* mread_until() timed-out */
    M_NOQMSG,			  /* nothing to read on msg queue */

    /* ids error codes */
    IDS_openError = 1300,	  /* problem opening terminal */
    IDS_termioError,		  /* problem doing termio ioctl */
    IDS_FlushError,		  /* problem flushing buffer */
    IDS_writeError,		  /* problem on write */
    IDS_TimeOut,		  /* no response */
    IDS_read0Bytes,		  /* read got nothing */
    IDS_readError,		  /* read failed - not time out */

    /* Client/Server codes */
    CS_FakeMode = 1400,		  /* server is in fake mode */
    CS_Disabled,		  /* server is disabled */

    /* TCS error return codes */
    TCS_InvalidCommand = 1500,	  /* invalid command sent to TCS */
    TCS_InvalidArgument,	  /* invalid command argument sent to TCS */
    TCS_NotTracking,		  /* TCS ignored command 'cause it's not on */
    TCS_CommandFailed,		  /* TCS did command, but it did not work */
    TCS_ConsolePrompt,		  /* TCS command prompt is probably up */
    TCS_UnknownStatus,		  /* we don't understand returned code */

    /* AOB error return codes */
    AOB_UnknownCommand = 1600,	  /* we do not understand the command */
    AOB_UnlockNotLocked,	  /* unlock when not locked */
    AOB_NotLockOwner,		  /* unlock attempt by someone else? */
    AOB_CurrentlyLocked,	  /* OMBA locked by another process */
    AOB_BadArgValue,		  /* some argument was bad */

    /* Coude f/4 and Gecko server conditions */
    F4_UnknownCommand = 1700,	  /* empty / badly formed command */
    F4_UnlockNotLocked,		  /* (NYI) already unlocked */
    F4_NotLockOwner,		  /* (NYI) not server lock owner */
    F4_CurrentlyLocked,		  /* (NYI) server locked by another process */
    F4_BadArgValue,		  /* (NYI) server got bad argument */
    F4_NotInitialized,		  /* no answer from the instrument hardware */
    F4_TrafficError,		  /* server had trouble with ocs */
    F4_DuckCommandError,	  /* problem sending to duck */
    F4_DuckResponseError,	  /* problem with duck response */

    cfht_errno_codes_filler	  /* ending dummy entry with no comma */
} cfht_errno_CodesType;

/*************************
 * structure definitions *
 *************************/

struct cfht_rng_double {
    double min;
    double max;
    BOOLEAN min_inclusive;
    BOOLEAN max_inclusive;
};
struct cfht_rng_int {
    int min;
    int max;
    BOOLEAN min_inclusive;
    BOOLEAN max_inclusive;
};
struct cfht_rng_string {
    char **string_array;
};
struct cfht_rng_char {
    char *list_of_chars;
};
union cfht_ranges {
    struct cfht_rng_double cfht_double_range;
    struct cfht_rng_int cfht_int_range;
    struct cfht_rng_string cfht_string_range;
    struct cfht_rng_char cfht_char_range;
};

typedef union cfht_ranges *cfht_range_t;

/*********************
 * global references *
 *********************/

extern BOOLEAN cfht_debug;
extern BOOLEAN cfht_error;
extern BOOLEAN cfht_lib;
extern BOOLEAN cfht_nodisp;
extern int cfht_errno;

extern union cfht_ranges *CFHT_RNG_RA;
extern union cfht_ranges *CFHT_RNG_DEC;

#define cfht_resetetimer() cfht_etimer(TRUE)
#define cfht_delay(millisec) (void)cfht_sleep((millisec),1)

/*
 * Macro to put around system calls that might be interrupted by a signal.
 * This is ripped from glibc.  It will keep re-trying systems calls until
 * at least a partial success, or fatal error is encountered.  Note that
 * read might still return FEWER BYTES than you told it to read if it is
 * interrupted!
 */
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)               \
({ long int __result;                                \
     do __result = (long int) (expression);          \
     while (__result == -1L && errno == EINTR);      \
     __result; })
#endif

/*
 * Special values that can be passed to cfht_signal()'s arg2
 */

#include <signal.h> /* for SIG_DFL and SIG_IGN */

#define CFHT_SIG_TRAP ((PFV)cfht_signal_handler)
  /* %%% The protos for CFHT_SIG_DFL et al are defined in terms of
   *     SIG_DFL et al defined in signal.h, but unfortunately these
   *     are not properly prototyped either :-(  Overriding them here
   *     is kind of dangerous, and should be checked carefully.
   */
#undef  CFHT_SIG_DFL
#undef  CFHT_SIG_IGN
#define CFHT_SIG_DFL ((PFV)(0))
#define CFHT_SIG_IGN ((PFV)(1))
/*
#define CFHT_SIG_DFL  ((PFV)SIG_DFL)
#define CFHT_SIG_IGN  ((PFV)SIG_IGN)
*/

/*********************************************
 * complete (?) list of all cfht_* functions *
 *********************************************/

char    *cfht_argsToString P((int argc, char *argv[]));
void	cfht_awake P((double when_from_now, double how_often));
char	*cfht_basename P((char *dest, char *filename, char *suffix));
char	*cfht_cfhthome P((void));
char	*cfht_date P((long *clockp));
char	*cfht_PFdate P((int  days));
char	*cfht_UTdate P((long *clockp));
char	*cfht_UTtime P((long *clockp));
char	*cfht_doupper P((char *str));
double	cfht_dtime P((void));
char	*cfht_fre P((unsigned char *src, unsigned char *dest, int *dsize));
char	*cfht_toe P((unsigned char *src, unsigned char *dest, int ssize));
double	cfht_etimer P((BOOLEAN reset));
void	cfht_signal P((int signo, PFV handler));
void	cfht_signal_block P((int signo));
void	cfht_signal_unblock P((int signo));
void    cfht_signal_handler P((int));
BOOLEAN	cfht_signal_event P((int signo));
BOOLEAN	cfht_signal_peek P((int signo));
int	cfht_exec P((FILE_ID id, char *string, int type, volatile void *addr));
int	cfht_system P((char *string));
char	*cfht_frs P((char *dest, double val, int scale, int prec));
void	cfht_genh P((int argc, char **argv, char **envp));
void	usage P((char **msg));
void	getcurdeath P((char *what));
double	cfht_julian P((int year, int month, int day, int hour, int minute, double sec));
void    cfht_fromjulian P((double jd, int *year, int *month, int *day, int *hour, int *minute, double *sec));
char	*cfht_Mjulian P((long *clockp));
char	*cfht_LocalSidTime P((long *clockp));
BOOLEAN	cfht_checksem P((int semid));
int	cfht_initsem P((char *path, char id));
PASSFAIL cfht_setsem P((int semid));
PASSFAIL cfht_setsemwait P((int semid, BOOLEAN sigign));
PASSFAIL cfht_relsem P((int semid));
PASSFAIL cfht_waitsemrel P((int semid, BOOLEAN sigign));
PASSFAIL cfht_waitsemset P((int semid, BOOLEAN sigign));
BOOLEAN cfht_checklock P((int fd));
int	cfht_initlock P((char *path, char id));
PASSFAIL cfht_setlock P((int fd));
PASSFAIL cfht_setlockwait P((int fd, BOOLEAN sigign));
PASSFAIL cfht_rellock P((int fd));
PASSFAIL cfht_waitlockrel P((int fd, BOOLEAN sigign));
PASSFAIL cfht_waitlockset P((int fd, BOOLEAN sigign));
void	cfht_logv P((int who, int type, char *fmt, ...));
void	cfht_logpv P((int who, int type, char *fmt, ...));
void	cfht_log P((int who, int type, char *mess));
void	cfht_log_proxy P((int who, int type, char *mess, char* id, int pid));
PASSFAIL cfht_int P((char *in_str, int *out_addr, cfht_range_t rangeinfo));
PASSFAIL cfht_double P((char *in_str, double *out_addr, cfht_range_t rangeinfo));
long	cfht_od P((void));
int	cfht_sleep P((int msec, int igsig));
char	*cfht_string P((char *in_str, int operation, cfht_range_t rangeinfo));
char	*cfht_time P((long *clockp));
BOOLEAN	cfht_tob P((char *string));
char	*cfht_tok P((char *string, char *sep, char **tail));
void	cfht_goDaemon P((void));

#ifdef __cplusplus
}
#endif

/*
 * See comments in missing_protos.h
 */
#include <missing_protos.h>

#endif /* CFHTDOTH */
