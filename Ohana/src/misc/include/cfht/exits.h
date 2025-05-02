/* Copyright (C) 1991, 94, 96    Canada-France-Hawaii Telescope Corp.    */
/* This program is distributed WITHOUT any warranty, and is under the    */
/* terms of the GNU General Public License, see the file COPYING    */
/*****************************************************************************
 *
 * file: exits.h
 * $Header: /cvsroot/pan-starrs/datasys/IPP/Ohana/src/misc/include/cfht/exits.h,v 1.1.1.1 2004-11-24 04:39:33 eugene Exp $
 * $Locker:  $
 *
 * This file defines the various exit codes for controllers and handlers.
 *
 * NOTE: please see ui/listexits.c also when you change this !!!!!!!!!
 *       The values of these exit codes are available symbolically in
 *       shell scripts by using ". setexits".
 *
 * HISTORY
 *
 * who       when           what
 * ------    -----------    ----------------------------------------------
 * jk        24 Mar 1988    Original
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/07/25 02:59:24  eugene
 * import Ohana
 *
 * Revision 1.4  96/08/12  15:30:00  15:30:00  thomas (Jim Thomas)
 * Added AOB exit codes and some comments
 * 
 * Revision 1.3  94/12/14  17:27:07  17:27:07  thomas (Jim Thomas)
 * Added pinger exit codes
 * 
 * Revision 1.2  93/06/26  17:59:06  17:59:06  steve (Steven Smith)
 * added define for environment var to pass exit code of data taking handlers
 * (ccdh, etc) for passing to all interested -E handlers.
 * 
 * Revision 1.1  91/09/24  17:38:54  17:38:54  steve (Steven Smith)
 * Original checkin
 * 
 *****************************************************************************/

#define EXIT_ENV_NAME	"CFHT_EXPOSURE_EXIT"
#define EXIT_PASS 0		  /* everything is OK */
#define EXIT_FAIL 1		  /* there was a real problem */
#define EXIT_EXP_IN_PROGRESS 2	  /* can't do request - exposure already on */
#define EXIT_EXP_ABORTED 3	  /* exposure trashed by user */
#define EXIT_EXP_STOPPED 4	  /* exposure stopped by user */
#define EXIT_CANCEL 5		  /* they hit the cancel button */

/* pinger exit codes */
#define PNG_EXIT_PASS EXIT_PASS	  /* service is healthy */
#define PNG_EXIT_BUSY 1		  /* service is busy */
#define PNG_EXIT_FAKE 2		  /* service is there but in FAKE mode */
#define PNG_EXIT_NOTTHERE 3	  /* service isn't there at all */
#define PNG_EXIT_NOTRAFFIC 4	  /* traffic isn't there */
#define PNG_EXIT_ERROR 5	  /* some program or argument error happened */
#define PNG_EXIT_HELP 6		  /* typed out the help message */

/* AOB exit codes */
#define AOB_EXIT_PASS EXIT_PASS	  /* things OK */
#define AOB_EXIT_FAIL EXIT_FAIL	  /* real problem */
#define AOB_EXIT_SEPARATION 2	  /* guide and object too far apart */
