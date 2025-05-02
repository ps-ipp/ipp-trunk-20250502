# include "opihi.h"

int GetTimeFormat (time_t *TimeReference, int *TimeFormat) {

  char *p;

  *TimeReference = 0;
  if ((p = get_variable ("TIMEREF")) != (char *) NULL) {
    if (!ohana_str_to_time (p, TimeReference)) {
      gprint (GP_ERR, "error in TIME_REF format\n");
      return (FALSE);
    }
    free (p);
  }
  *TimeFormat = FALSE;
  if ((p = get_variable ("TIMEFORMAT")) != (char *) NULL) {
    if (!strcasecmp (p, "JD")) *TimeFormat     = TIME_JD;
    if (!strcasecmp (p, "MJD")) *TimeFormat    = TIME_MJD;
    if (!strcasecmp (p, "date")) *TimeFormat   = TIME_DATE;
    if (!strcasecmp (p, "days")) *TimeFormat   = TIME_DAYS;
    if (!strcasecmp (p, "hours")) *TimeFormat  = TIME_HOURS;
    if (!strcasecmp (p, "min")) *TimeFormat    = TIME_MINUTES;
    if (!strcasecmp (p, "sec")) *TimeFormat    = TIME_SECONDS;
    if (!*TimeFormat) gprint (GP_ERR, "unknown TIME_FORMAT\n");
    free (p);
    return (FALSE);
  }
  if (!*TimeFormat) *TimeFormat = TIME_SECONDS;
  return (TRUE);
}
