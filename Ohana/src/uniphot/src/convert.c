# include "uniphot.h"

time_t GetTimeReference (char *reference) {

  time_t TimeReference;
  
  if (!ohana_str_to_time (reference, &TimeReference)) {
    fprintf (stderr, "error in time reference %s\n", reference);
    exit (2);
  }

  return (TimeReference);
}

int GetTimeUnits (char *name) {

  int Units;

  Units = FALSE;
  if (!strcasecmp (name, "JD")) Units     = TIME_JD;
  if (!strcasecmp (name, "MJD")) Units    = TIME_MJD;
  if (!strcasecmp (name, "date")) Units   = TIME_DATE;
  if (!strcasecmp (name, "days")) Units   = TIME_DAYS;
  if (!strcasecmp (name, "hours")) Units  = TIME_HOURS;
  if (!strcasecmp (name, "min")) Units    = TIME_MINUTES;
  if (!strcasecmp (name, "sec")) Units    = TIME_SECONDS;
  if (!Units) {
    fprintf (stderr, "error in time units %s\n", name);
    exit (2);
  }

  return (Units);
}
