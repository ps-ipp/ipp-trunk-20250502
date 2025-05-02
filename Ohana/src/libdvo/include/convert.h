// # include "external.h"

#include <ohana.h>
#include <dvo.h>

# ifndef CONVERT_H
# define CONVERT_H

/*** time/coord conversion functions not supplied by libohana ***/
time_t        TimeRef               PROTO((double time, time_t TimeReference, int TimeFormat));
double        TimeValue             PROTO((time_t time, time_t TimeReference, int TimeFormat));
double        GetTimeRange          PROTO((time_t time, int TimeFormat));

int           hh_hms                PROTO((double hh, int *hr, int *mn, double *sc));
int           dd_dms                PROTO((double dd, int *dg, int *mn, double *sc));
int           hms_format            PROTO((char *line, int length, double value));
int           dms_format            PROTO((char *line, int length, double value));
int           hh_hm                 PROTO((double hh, int *hr, double *mn));
int           day_to_sec            PROTO((char *string, time_t *second));
int           hms_to_sec            PROTO((char *string, time_t *second));
char         *ohana_sec_to_hms      PROTO((time_t second));
char         *ohana_sec_to_day      PROTO((time_t second));

char         *meade_deg_to_str      PROTO((double deg));
char         *meade_ra_to_str       PROTO((double deg));
char         *meade_dec_to_str      PROTO((double deg));
char         *strptime              PROTO((const char *s, const char *format, struct tm *tm));

# endif
