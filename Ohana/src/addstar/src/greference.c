# include "addstar.h"
# define LOAD_ALLSKY 0
# define LOAD_DR2    1

Catalog *greference (char *Refcat, SkyRegion *region, int photcode) {
  OHANA_UNUSED_PARAM(photcode);

  Catalog *catalog = NULL;

  if (VERBOSE) fprintf (stderr, "loading reference catalog data from %s\n", Refcat); 
  if (VERBOSE) fprintf (stderr, "full region: %f - %f, %f - %f\n", region[0].Rmin, region[0].Rmax, region[0].Dmin, region[0].Dmax);

  /* get stars from HST GSC for the given region */
  if (!strcasecmp (Refcat, "GSC")) {
    catalog = getgsc (region);
  }
  
  /* get stars from USNO for the given region */
  if (!strcasecmp (Refcat, "USNO")) {
    catalog = getusno (region);
  }

  /* get stars from the USNO B catalog for the given region */
  if (!strcasecmp (Refcat, "USNOB")) {
    catalog = getusnob (region);
  }

  /* get stars from the Tycho catalog for the given region */
  if (!strcasecmp (Refcat, "TYCHO")) {
    fprintf (stderr, "Tycho load via addstar is deprecated: use loadtycho\n");
    exit (2);
  }

  /* get stars from the Tycho catalog for the given region (old ingest) */
  if (!strcasecmp (Refcat, "TYCHO_OLD")) {
    fprintf (stderr, "Tycho load via addstar is deprecated: use loadtycho\n");
    exit (2);
  }

  /* get stars from 2MASS for the given region */
  if (!strcasecmp (Refcat, "2MASS")) {
    fprintf (stderr, "2MASS load via addstar is deprecated: use load2mass\n");
    exit (2);
  }
  
  /* get stars from 2MASS for the given region */
  if (!strcasecmp (Refcat, "2MASS-ALLSKY")) {
    fprintf (stderr, "2MASS load via addstar is deprecated: use load2mass\n");
    exit (2);
  }
  
  /* get stars from 2MASS for the given region */
  if (!strcasecmp (Refcat, "2MASS-DR2")) {
    fprintf (stderr, "2MASS load via addstar is deprecated: use load2mass\n");
    exit (2);
  }
  
  if (VERBOSE && (catalog->Naverage == 0)) fprintf (stderr, "no ref objects in region %s\n", region[0].name);

  return (catalog);
}
