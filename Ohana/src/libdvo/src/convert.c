# include "convert.h"
# define _XOPEN_SOURCE /* glibc2 (strptime) needs this */
# include <time.h>

/** additional time / coordinate conversions not supplied by libohana **/

int hh_hms (double hh, int *hr, int *mn, double *sc) {

  int N, flag;

  flag = SIGN(hh);
  hh = fabs(hh);

  // rationalize hh to range -24.0 < hh < 24.0
  if (hh >= 24.0) {
    N = (int)(hh/24.0);
    hh -= 24.0*N;
  }

  *hr = (int) hh;
  *mn = (int) 60*(hh - *hr);
  *sc = 3600.0*(hh - *hr - *mn / 60.0);
  if (*sc > 59.99) {
    *sc = 0.0;
    *mn += 1.0;
  }
  *hr *= flag;
  return (TRUE);
}
 
int dd_dms (double dd, int *dg, int *mn, double *sc) {

  int flag;

  flag = SIGN(dd);
  dd = fabs (dd);
  *dg = (int) dd;
  *mn = (int) 60*(dd - *dg);
  *sc = 3600.0*(dd - *dg - *mn/60.0);
  if (*sc > 59.99) {
    *sc = 0;
    *mn += 1.0;
  }
  *dg *= flag;
  return (TRUE);
}
 
int hms_format (char *line, int length, double value) {

  int hr, mn;
  double sc;

  hh_hms (value, &hr, &mn, &sc);
  hr = (int) value;
  if (isnan (value))
    snprintf (line, length, "xx:xx:xx.xx");
  else {
    if (value < 0) {
      snprintf (line, length, "-%02d:%02d:%05.2f", abs(hr), mn, sc);
    } else {
      snprintf (line, length, "+%02d:%02d:%05.2f", hr, mn, sc);
    }
  }      
  return (TRUE);
}

int dms_format (char *line, int length, double value) {

  int dg, mn;
  double sc;

  dd_dms (value, &dg, &mn, &sc);
  if (value < 0) {
    snprintf (line, length, "-%02d:%02d:%05.2f", abs(dg), mn, sc);
  } else {
    snprintf (line, length, "+%02d:%02d:%05.2f", dg, mn, sc);
  }
  return (TRUE);
}

/***** convert 00:00:00 or 00:00 to 0 - 86400 ****/
int hms_to_sec (char *string, time_t *second) {
  
  char *p;
  struct tm time;

  time.tm_hour = 0;
  time.tm_min  = 0;
  time.tm_sec  = 0;

  p = strptime (string, "%H:%M:%S", &time);
  if (p != NULL) goto valid;

  p = strptime (string, "%H:%M", &time);
  if (p != NULL) goto valid;

  return (FALSE);
    
valid:
  if (*p) return (FALSE);
  *second = time.tm_hour*3600 + time.tm_min*60 + time.tm_sec;
  return (TRUE);
}

/***** convert Mon[@00:00:00] or 00:00 to 0 - 86400*7 ****/
int day_to_sec (char *string, time_t *second) {
  
  char *p;
  struct tm time;

  bzero (&time, sizeof(time));
  p = strptime (string, "%A@%H:%M:%S", &time);
  if (p != NULL) goto valid;

  p = strptime (string, "%A@%H:%M", &time);
  if (p != NULL) goto valid;

  p = strptime (string, "%A@%H", &time);
  if (p != NULL) goto valid;

  p = strptime (string, "%A", &time);
  if (p != NULL) goto valid;

  return (FALSE);

valid:
  if (*p) return (FALSE);
  *second = time.tm_wday*86400 + time.tm_hour*3600 + time.tm_min*60 + time.tm_sec;
  return (TRUE);
}

/***** convert seconds to HH:MM:SS ****/
char *ohana_sec_to_hms (time_t second) {
  
  struct tm *gmt;
  char *line;

  ALLOCATE (line, char, 64);
  gmt   = gmtime (&second);
  snprintf (line, 64, "%02d:%02d:%02d", gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
  return (line);
}

/***** convert seconds to Day@HH:MM:SS ****/
char *ohana_sec_to_day (time_t second) {
  
  struct tm *gmt;
  char *line;

  ALLOCATE (line, char, 64);
  gmt   = gmtime (&second);
  switch (gmt[0].tm_wday) {
    case 0:
      snprintf (line, 64, "Sun@%02d:%02d:%02d", gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
      break;
    case 1:
      snprintf (line, 64, "Mon@%02d:%02d:%02d", gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
      break;
    case 2:
      snprintf (line, 64, "Tue@%02d:%02d:%02d", gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
      break;
    case 3:
      snprintf (line, 64, "Wed@%02d:%02d:%02d", gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
      break;
    case 4:
      snprintf (line, 64, "Thu@%02d:%02d:%02d", gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
      break;
    case 5:
      snprintf (line, 64, "Fri@%02d:%02d:%02d", gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
      break;
    case 6:
      snprintf (line, 64, "Sat@%02d:%02d:%02d", gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
      break;
  }
  return (line);
}

int hh_hm (double hh, int *hr, double *mn) {

  int flag;

  flag = SIGN(hh);
  hh = fabs (hh);

  *mn = 60.0*(hh - (int)hh);
  *hr = (int) hh;
  *hr *= flag;
  return (TRUE);
}

char *meade_deg_to_str (double deg) {

  int hr;
  double mn;
  char *line;

  ALLOCATE (line, char, 16);

  hh_hm (deg, &hr, &mn);

  snprintf (line, 16, "%03d:%04.1f", abs(hr), mn);
  return (line);
}

char *meade_ra_to_str (double deg) {

  int hr;
  double mn;
  char *line;

  ALLOCATE (line, char, 16);

  hh_hm (deg/15.0, &hr, &mn);

  snprintf (line, 16, "%02d:%04.1f", abs(hr), mn);
  return (line);
}

char *meade_dec_to_str (double deg) {

  int hr;
  double mn;
  char *line;

  ALLOCATE (line, char, 16);

  hh_hm (deg, &hr, &mn);

  if (deg < 0) {
    snprintf_nowarn (line, 16, "-%02d:%04.1f", abs(hr), mn);
  } else {
    snprintf_nowarn (line, 16, "+%02d:%04.1f", hr, mn);
  }      
  return (line);
}

/* convert UNIX time to a value referenced to the TimeReference in the given unit */
double TimeValue (time_t time, time_t TimeReference, int TimeFormat) {

  double value, dt;

  dt = (time > TimeReference) ? (time - TimeReference) : -1 * (double)(TimeReference - time);
  switch (TimeFormat) {
  case TIME_JD:
    value = time / 86400.0 + 2440587.5;
    break;
  case TIME_MJD:
    value = time / 86400.0 + 40587.0;
    break;
  case TIME_DAYS:
    value = dt / 86400.0;
    break;
  case TIME_HOURS:
    value = dt / 3600.0;
    break;
  case TIME_MINUTES:
    value = dt / 60.0;
    break;
  case TIME_SECONDS:
  default:
    value = dt;
    break;
  }
  return (value);
}
  
/* convert UNIX time (sec) range to a time range in the given unit */
double GetTimeRange (time_t dt, int TimeFormat) {

  double value;

  switch (TimeFormat) {
  case TIME_JD:
  case TIME_MJD:
  case TIME_DAYS:
    value = dt / 86400.0;
    break;
  case TIME_HOURS:
    value = dt / 3600.0;
    break;
  case TIME_MINUTES:
    value = dt / 60.0;
    break;
  case TIME_SECONDS:
  default:
    value = dt;
    break;
  }
  return (value);
}
  
/* convert time value referenced to the TimeReference in the given unit to UNIX time */
time_t TimeRef (double value, time_t TimeReference, int TimeFormat) {

  int dt;
  time_t time;

  switch (TimeFormat) {
  case TIME_JD:
    time = (value -  2440587.5) * 86400.0;
    return (time);
    break;
  case TIME_MJD:
    time = (value -  40587.0) * 86400.0;
    return (time);
    break;
  case TIME_DAYS:
    dt = value * 86400.0;
    break;
  case TIME_HOURS:
    dt = value * 3600.0;
    break;
  case TIME_MINUTES:
    dt = value * 60.0;
    break;
  case TIME_SECONDS:
  default:
    dt = value;
    break;
  }

  time = TimeReference + dt;
  return (time);
}

/* times may be in forms as:
 * 20040200450s (N seconds since 1970.0)
 * 2440900.232j (julian date)
 * 99/02/23,03:22:18 (date string)
 * (separators may be anything except space, +, -)
 * 99:02:15:12:23:30
 * 99:02:15:12h23m30s
 */

