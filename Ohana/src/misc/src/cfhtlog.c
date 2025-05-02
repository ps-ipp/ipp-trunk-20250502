/****************************************************************************
    Copyright (C) 1991, 1995    Canada-France-Hawaii Telescope Corp.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Contact information:
    CFHT, PO Box 1597, Kamuela, HI 96743, USA
    daprog@cfht.hawaii.edu

****************************************************************************/
/*!****************************************************************************
 * FILE
 *
 * $Header: /cvsroot/pan-starrs/datasys/IPP/Ohana/src/misc/src/cfhtlog.c,v 1.1 2003-01-17 18:42:35 eugene Exp $
 * $Locker:  $
 *
 * ROUTINES
 *
 *
 * HISTORY
 * who          when            what
 * ---------    ------------    ----------------------------------------------
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/07/25 02:59:22  eugene
 * import Ohana
 *
 * Revision 1.4  2001/01/25 21:21:06  bernt
 * nothing changed
 *
 * Revision 1.3  1999/07/06 14:14:07  bernt
 * *** empty log message ***
 *
 * Revision 1.2  98/06/09  22:50:01  22:50:01  bernt (Bernt Grundseth)
 * clean some unused vars
 * 
 * Revision 1.1  97/11/21  11:28:53  11:28:53  bernt (Bernt Grundseth)
 * Initial revision
 * 
 * Revision 1.5  96/11/17  22:59:00  22:59:00  bernt (Bernt Grundseth)
 * *** empty log message ***
 * 
 * Revision 1.4  96/05/29  15:27:03  15:27:03  bernt (Bernt Grundseth)
 * removed reading from current.dat file, because the last data file is 
 * copied to uwila_data every ten minutes instead of the temporary kludge
 * with the current.dat file.
 * 
 * Revision 1.3  96/05/29  07:23:29  07:23:29  bernt (Bernt Grundseth)
 * *** empty log message ***
 * 
 * Revision 1.2  96/02/20  21:58:38  21:58:38  bernt (Bernt Grundseth)
 * basic operations working still need improvements, range checking
 * 
 * Revision 1.1  96/02/19  22:31:43  22:31:43  bernt (Bernt Grundseth)
 * Initial revision
 * 
 * 
 ***************************************************************************!*/

#include <stdio.h>
#include <string.h>		  /* for string functions */
#include <stdlib.h>		  /* for strtoul */
#ifdef sun
#define strtoul strtol		  /*   but sun doesn't have one */
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>		  /* for time functions */
#ifdef sun
#define mktime timelocal	  /*   but sun has a different name */
#endif
#include <errno.h>		  /* for ERANGE */

#include <cfht/cfht.h>		  /* for P macro */
#include <cfht/exits.h>           /* for standard exit codes */


void dp_usage P((char *s));       	  /* usage exit */
PASSFAIL TimeStringToTime P((char *String, time_t *Time));
double cfht_julian();
void cfht_fromjulian();
double atof();

time_t TimeNow;			  /* current time */
static char path[] = "/data/logger/";
#define NUMITEMS 915

int main(argc, argv)
    int argc;
    char *argv[];
{
#ifndef __LINT__
    static char rcs_id[] =
      "$Header: /cvsroot/pan-starrs/datasys/IPP/Ohana/src/misc/src/cfhtlog.c,v 1.1 2003-01-17 18:42:35 eugene Exp $";
#endif
    /* command line argument values */
    char *OurName;		  /* name of executing program */
    char *Input = (char *) NULL;  /* name of [pipe] to read */
    char *SessionName = (char *) NULL; /* name of session to match */
    char *FromString = (char *) NULL; /* time to start outputting */
    char *ToString = (char *) NULL; /* time to stop outputting */
    char *ProbeString = (char *) NULL; /* probes  */
    BOOLEAN Verbose = FALSE;	  /* want lots of debug output? */
    BOOLEAN GoBackwards = TRUE;	  /* want LIFO or FIFO?  TRUE = LIFO */
    BOOLEAN Dosuid = FALSE;	  /* do suid?  TRUE = yes */
    unsigned int HoursBack = 0;	  /* start output this far back */
    time_t FromTime;		  /* from time in internal form */
    time_t ToTime;		  /* to time in internal form */

    int Probes[100], nprobes;
    double JDFrom, JDTo;
    struct tm *tmd;



    int i;			  /* index into argv */
    int nlen;			  /* temporary  */

    
    OurName = cfht_basename((char *) NULL, argv[0], (char *) NULL);
    if (argc < 2) {
        dp_usage(argv[0]); /* else error */
        exit(EXIT_FAIL);
    }


    time(&TimeNow);		  /* get current time */
    tmd = localtime(&TimeNow);
    printf("# DATE/TIME now %d %d %d %d %d %d\n",
		   tmd->tm_year+1900, tmd->tm_mon+1, tmd->tm_mday,
		   tmd->tm_hour, tmd->tm_min, tmd->tm_sec);
    
    for (i = 1 ; i < argc ; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {

	      case 'f':
		FromString = argv[++i]; /* get from time from command line */
		printf("# from date %s \n",FromString);
                if (get_JDdate(FromString, &JDFrom)) {
		    printf("# malformed date: %s \n",FromString);
                    exit(EXIT_FAIL);
                }
		printf("#  julian day = %lf\n", JDFrom);
		break;

	      case 't':
		ToString = argv[++i]; /* get to time from command line */
		printf("# to date %s \n",ToString);
                if (get_JDdate(ToString, &JDTo)) {
		    printf("# malformed date: %s \n",ToString);
                    exit(EXIT_FAIL);
                }
		printf("#  julian day = %lf\n", JDTo);
		break;

	      case 'p':
		ProbeString = argv[++i]; /* get probes from command line */
		break;

	      case 'h':
		/* one hour */

                JDTo = cfht_julian(tmd->tm_year + 1900, tmd->tm_mon + 1,
		  		  tmd->tm_mday, tmd->tm_hour,
				  tmd->tm_min, (double)tmd->tm_sec);
                JDTo = JDTo + 10./24.;
                JDFrom = JDTo - 1.0/24.0;
		printf("# INTERVAL: hour\n");
		break;

	      case 'd':
		/* one day */

                JDTo = cfht_julian(tmd->tm_year + 1900, tmd->tm_mon + 1,
		  		  tmd->tm_mday, tmd->tm_hour,
				  tmd->tm_min, (double)tmd->tm_sec);
                JDTo = JDTo + 10./24.;
                JDFrom = JDTo - 1.0;
		printf("# INTERVAL: day\n");
		break;

	      case 'w':
		/* one week */

                JDTo = cfht_julian(tmd->tm_year + 1900, tmd->tm_mon + 1,
		  		  tmd->tm_mday, tmd->tm_hour,
				  tmd->tm_min, (double)tmd->tm_sec);
                JDTo = JDTo + 10./24.;
                JDFrom = JDTo - 7.0;
		printf("# INTERVAL: week\n");
		break;

	      case 'm':
		/* one month */

                JDTo = cfht_julian(tmd->tm_year + 1900, tmd->tm_mon + 1,
		  		  tmd->tm_mday, tmd->tm_hour,
				  tmd->tm_min, (double)tmd->tm_sec);
                JDTo = JDTo + 10./24.;
                JDFrom = JDTo - 30.0;
		printf("# INTERVAL: month\n");
		break;

	      default:
		dp_usage(argv[0]); /* else error */
	    }
	} 

    }

    /* parse the probes numbers requested, and total # of probes */
    if (ProbeString == NULL) { printf("# no probes selected\n");
        exit(EXIT_FAIL);
	}

    get_tokens(ProbeString, " ,",Probes, &nprobes);
    printf("# PROBES: %d ", nprobes); 
    for (i = 0; i < nprobes; i++) {
	if (Probes[i] < 0 || Probes[i] > 99 ) {
	    printf("# error probe # out of range\n");
            exit(EXIT_FAIL);
        }
    printf(" %d ", Probes[i]); 
    }
    printf("\n"); 
    get_data(JDFrom, JDTo, Probes, nprobes); 


    exit(EXIT_PASS);
    return(0);			  /* dummy */
}

PASSFAIL
get_data(JDstart, JDend, probes, nprobes)
double JDstart, JDend;
int probes[], nprobes;
{
    FILE *infile;
    double julind, jd, jd0;
    int year, month, day, hour, min, dayno;
    double sec;
    char fullname[64];
    char filename[32];

    char string[512];
    double timestamp;
    char data[915];
    char bufstr[12];
    long line = 0;
    int nchar,j;

    /* make JDstart to be in the 10 minutes boundary */
    cfht_fromjulian(JDstart, &year, &month, &day, &hour, &min, &sec);
    min = min/10*10;
    sec = 0.;
    JDstart = cfht_julian(year, month , day, hour, min, sec);


    /* At this point it is assumed that the dates are OK.  */
    /* get get year and day number at start  */
    for (julind = JDstart -1; julind <= JDend; julind=julind+1.) {
	jd = julind -10./24.;
        cfht_fromjulian(jd, &year, &month, &day, &hour, &min, &sec);
        jd0 = cfht_julian(year, 1 , 1, 0, 0, 0.);
	dayno = jd -jd0 + 1;
	strcpy (fullname, path);
	sprintf(filename, "%d/%d%.3d.dat",year,(year-1900),dayno);
	strcat (fullname, filename);

	infile = fopen(fullname, "r");
	if (infile == NULL) {
	    if ( (int)julind < (int)JDend ) { /* last one ??? */
	        sprintf (string, "# unable to open input file '%s'", fullname);
	        perror (string);
	    }

	} else {
        /*********************
         * process each line *
         *********************/
            while ((nchar = fread(data, sizeof(char), NUMITEMS, infile)) > 0 ) {
	        timestamp = atof(data) + 0.00001;

	        if ((timestamp >= JDstart) && (timestamp <= JDend)) {
                    cfht_fromjulian((timestamp-10./24.), &year, &month, &day,
				    &hour, &min, &sec);
		    printf("%lf ", timestamp - JDstart);
		    printf("%lf ", (timestamp - JDstart)*24.0);
		    printf("%.2d/%.2d/%.2d %.2d:%.2d ",
		           year, month,day,hour,min);
		    for (j = 0; j < nprobes; j++) {
		        strncpy_nowarn(bufstr, data + (14 + probes[j]*9), 9);
		        printf("%s ", bufstr);
		    }
		    printf("\n");
                }
            }
	    fclose(infile);
        }

    }
    return(PASS);
}


PASSFAIL
get_JDdate(datestring, JD)
char *datestring;
double *JD;
{
    int vect[10];
    int n;
    int year, month, day, hour, min, sec;
    get_tokens(datestring, " /:", vect, &n);
    if (n < 3 ) {
	return(FAIL);
    }
/*    year = vect[0] + 1900; */
    year = vect[0];
    if (year < 1993 || year > 2010 )
	return(FAIL);
    month = vect[1];
    if (month < 1 || month > 12 )
	return(FAIL);
    day = vect[2];
    if (day < 1 || day > 31 )
	return(FAIL);
    hour = 0;
    min  = 0;
    sec  = 0;
    if ( n > 3 ) {
       hour = vect[3];
       if (hour < 0 || hour > 23 )
           return(FAIL);
    }
    if ( n > 4 ) {
       min = vect[4];
       if (min < 0 || min > 59 )
           return(FAIL);
    }
    if ( n > 5 ) {
       sec = vect[5];
       if (sec < 0 || sec > 59 )
           return(FAIL);
    }
    *JD = cfht_julian(year, month, day, hour, min, (double)sec);
    *JD = *JD + 10.0 / 24.0;
    return(PASS);
}

PASSFAIL
get_tokens(buf, sep, vect, nitems)
char buf[];
char *sep;
int vect[];
int *nitems;
{
    char *cp;
    int i;

    cp = strtok(buf, sep);
    vect[0] = atoi(cp);


    i = 1;
    while( 1 ) {

        if( (cp = strtok(0, sep) ) == 0 ) {
        break;
        }
        vect[i] = atoi(cp);
        i++;
    }
    *nitems = i;

    return(PASS);
}

/* usage: prints default error message and then exits */
/* s is program name */
void dp_usage(s)
    char *s;
{
    fprintf(stderr, "Usage: %s\n", s);
    fprintf(stderr, " [-h ] [-d] [-w] [-m]\n");
    fprintf(stderr, " 1 hour, 1 day, 1 week, 1 month  \n");
    fprintf(stderr, " [-f timefrom] [-t timeto] -p probeNumber\n");
    fprintf(stderr, " Ex. -f \"2000/02/23 10:00:00\" -t \"2000/02/24\" -p 1,2,13,45\n");
    exit(EXIT_FAIL);
}

