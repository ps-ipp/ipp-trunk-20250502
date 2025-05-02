# include "delstar.h"

/* drop all MISSED values for the given catalog */

delete_missed (Catalog *catalog) {

  off_t i, Nave, Nmeas, Nmiss;

  Nave = catalog[0].Naverage;
  Nmeas = catalog[0].Nmeasure;
  Nmiss = catalog[0].Nmissing;
  
  if (VERBOSE) fprintf (stderr, "starting with Nave, Nmeas, Nmiss: "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT"\n",  Nave,  Nmeas,  Nmiss);

  /* set up references for missing to average */
  for (i = 0; i < Nave; i++) {
    catalog[0].average[i].Nn = 0;
  }
  REALLOCATE (catalog[0].missing, Missing, 1);
  catalog[0].Nmissing = 0;
  if (VERBOSE) fprintf (stderr, "  ending with Nave, Nmeas, Nmiss: "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT"\n",  Nave,  Nmeas,  Nmiss);
}

