# include "relphot.h"

// this function sets the NOCAL and AREA dbFlags bits for the MeasureTiny elements these
// are used elsewhere (StarOps.c, ImageOps.c, MosaicOps.c, GridOps.c, etc) to skip bad
// measurements.  The only exception is 'setMave' which is called by 'relphot_objects',
// and uses the bits read from disk as the test

int setExclusions (Catalog *catalog, int Ncatalog, int verbose) {

  off_t i, j, k, m, Narea, Nnocal, Ngood;
  Coords *coords;
  double r, d, x, y;

  Ngood = Nnocal = Narea = 0;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {
      m = catalog[i].averageT[j].measureOffset;
      for (k = 0; k < catalog[i].averageT[j].Nmeasure; k++, m++) {

	/* select measurements by photcode */
	int Ns = GetActivePhotcodeIndex (catalog[i].measureT[m].photcode);
	if (Ns < 0) goto mark_nocal;
	
	/* select measurements by time */
	if (TimeSelect) {
	  if (catalog[i].measureT[m].t < TSTART) goto mark_nocal;
	  if (catalog[i].measureT[m].t > TSTOP) goto mark_nocal;
	}

	/* select measurements by mag limit */
	if (AreaSelect) {
	  r = catalog[i].measureT[m].R;
	  d = catalog[i].measureT[m].D;
	  if ((coords = getCoords (m, i)) == NULL) goto markbad;
	  RD_to_XY (&x, &y, r, d, coords);
	  if (x < AreaXmin) goto markbad;
	  if (x > AreaXmax) goto markbad;
	  if (y < AreaYmin) goto markbad;
	  if (y > AreaYmax) goto markbad;
	}
	Ngood ++;
	continue;

      markbad:
	catalog[i].measureT[m].dbFlags |= ID_MEAS_AREA;
	Narea ++;
	continue;
	
      mark_nocal:
	catalog[i].measureT[m].dbFlags |= ID_MEAS_NOCAL;
	Nnocal ++;
	continue;
      }
    }
  }
  if (verbose) fprintf (stderr, OFF_T_FMT" measurements marked by area\n",    Narea);
  if (verbose) fprintf (stderr, OFF_T_FMT" measurements marked nocal\n",      Nnocal);
  if (verbose) fprintf (stderr, OFF_T_FMT" measurements kept for analysis\n", Ngood);
  return (TRUE);
}
