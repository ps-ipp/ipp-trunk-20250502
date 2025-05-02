# include "pantasks.h"

/* the tested time is saved by CheckTimeRanges for a following BumpTimeRanges
   otherwise we could have an inconsistency between valid ranges and Nrun */
static time_t daytime, weektime, abstime;

/* test if we meet all time range qualifications */
int CheckTimeRanges (TimeRange *ranges, int Nranges) {

  int i, intime, Ninclude, include, exclude, valid;
  time_t testtime;
  struct timeval now;
  struct tm Now;

  /* get the current time */
  gettimeofday (&now, NULL);
  gmtime_r (&now.tv_sec, &Now);
  daytime  = Now.tm_sec + Now.tm_min*60 + Now.tm_hour*3600;
  weektime = Now.tm_sec + Now.tm_min*60 + Now.tm_hour*3600 + Now.tm_wday*86400;
  abstime  = now.tv_sec;

  Ninclude = 0;
  include = FALSE;
  exclude = FALSE;
  
  for (i = 0; i < Nranges; i++) {
    if (ranges[i].include) Ninclude ++;

    switch (ranges[i].type) {
      /* set the testtime */
      case RANGE_ABS:
	testtime = abstime;
	break;
      case RANGE_DAY:
	testtime = daytime;
	break;
      case RANGE_WEEK:
	testtime = weektime;
	break;
      default:
	abort ();
    }
    intime = (testtime >= ranges[i].start) && (testtime <= ranges[i].stop);

    /* check for more than max runs in time range */
    if (ranges[i].include && intime && ranges[i].Nmax) {
      if (ranges[i].Nrun >= ranges[i].Nmax) return (FALSE);
    }
    /* reset Nrun if we are outside of intime */
    if (ranges[i].include && !intime && ranges[i].Nmax && ranges[i].Nrun) {
      ranges[i].Nrun = 0;
    }

    /* is this a valid time? */
    if ( ranges[i].include &&  intime) include = TRUE;
    if (!ranges[i].include && !intime) exclude = TRUE;
  }

  if (Ninclude == 0) include = TRUE;
  valid = include && !exclude;

  return (valid);
}  

/* increment the number of runs for all inclusive time ranges with Nmax > 0
   (only call when we execute a task -- after CheckTimeRanges) */
int BumpTimeRanges (TimeRange *ranges, int Nranges) {

  int i, intime;
  time_t testtime;

  /* only increment the counter for ranges which are valid */
  for (i = 0; i < Nranges; i++) {
    if (!ranges[i].Nmax) continue;
    if (!ranges[i].include) continue;

    switch (ranges[i].type) {
      /* set the testtime */
      case RANGE_ABS:
	testtime = abstime;
	break;
      case RANGE_DAY:
	testtime = daytime;
	break;
      case RANGE_WEEK:
	testtime = weektime;
	break;
      default:
	abort ();
    }
    intime = (testtime >= ranges[i].start) && (testtime <= ranges[i].stop);

    /* reset Nrun if we are outside of intime */
    if (intime) ranges[i].Nrun ++;
  }
  return (TRUE);
}  
