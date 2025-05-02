# include "dvo.h"
# include "get_graphdata.h"

// This file contains some functions used by the libdvo db lookup functions
// that are really specific to dvo.
//
// These are declared weak so they can be overridden by the program.

// overridden for opihi in opihi/lib.data/open_kapa.c
// by default, no graphdata 
int GetGraphdata(Graphdata *graph, int *kapa, char *name) {
  OHANA_UNUSED_PARAM(graph);
  OHANA_UNUSED_PARAM(kapa);
  OHANA_UNUSED_PARAM(name);
    return FALSE;
}

// overridden for opihi programs in opihi/lib.shell/timeformat.c
int
GetTimeFormat(time_t *TimeReference, int *TimeFormat) {
    *TimeReference = 0;
    *TimeFormat = TIME_SECONDS;
    return (TRUE);
}

// overridden for dvo in opihi/dvo/photometry.c
int GetTimeSelection (time_t *tz, time_t *te) {

  *tz = 0;
  *te = 0;
  return (TRUE);
}
