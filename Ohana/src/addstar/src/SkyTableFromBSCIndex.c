# include "dvo.h"
# define NBANDS 24
# define NDIV 4

static double DecBands[] = {0.0, +7.5, +15.0, +22.5, +30.0, +37.5, +45.0, +52.5, +60.0, +67.5, +75.0, +82.5, +90.0,
			    0.0, -7.5, -15.0, -22.5, -30.0, -37.5, -45.0, -52.5, -60.0, -67.5, -75.0, -82.5, -90.0};

static char *DecNames[] = {"n0000", "n0730", "n1500", "n2230", "n3000", "n3730", "n4500", "n5230", "n6000", "n6730", "n7500", "n8230", "none",
			   "s0000", "s0730", "s1500", "s2230", "s3000", "s3730", "s4500", "s5230", "s6000", "s6730", "s7500", "s8230", "none"};

SkyTable *SkyTableFromTychoIndex (char *filename, int VERBOSE) {

  int i, No, Nr, NR, Ntycho;
  double Dmin, Dmax;
  char line[256];
  FILE *f;
  SkyTable *skytable;
  SkyRegion *regions;
  SkyRegion *tycho;
  
  f = fopen (filename, "r");
  if (f == NULL) {
    if (VERBOSE) fprintf (stderr, "can't find Tycho Index file %s\n", filename);
    return (NULL);
  }

  /* load in table data */
  ALLOCATE (tycho, SkyRegion, 10000);
  for (i = 0; i < 10000; i++) {
    if (scan_line (f, line) == EOF) break;
    tycho[i].Rmin = atof (&line[15]);
    tycho[i].Rmax = atof (&line[22]);
    tycho[i].Dmin = atof (&line[29]);
    tycho[i].Dmax = atof (&line[36]);

    memset (tycho[i].name, 0, 21);
    strncpy_nowarn (tycho[i].name, line, 7);
  }
  Ntycho = i;
  fclose (f);

  /* build supporting level 0 and 1 regions */
  Nr = 0;
  NR = 100;
  ALLOCATE (regions, SkyRegion, 100);
  
  /* level 0 : full sky */
  regions[Nr].Rmin 	=   0;
  regions[Nr].Rmax 	= 360;
  regions[Nr].Dmin 	= -90;
  regions[Nr].Dmax 	= +90;
  regions[Nr].index  	=  0;
  regions[Nr].depth  	=  0;
  regions[Nr].parent 	= -1;
  regions[Nr].child  	=  TRUE;
  regions[Nr].table  	=  FALSE;
  memset (regions[Nr].name, 0, 21);
  strcpy (regions[Nr].name, "fullsky");
  
  No = Nr;
  Nr ++;

  /* level 1 : add the dec bands */
  regions[No].childS = Nr;
  /* first north */
  for (i = 0; i < 12; i++, Nr++) {
    regions[Nr].Rmin   	  =   0;
    regions[Nr].Rmax   	  = 360;
    regions[Nr].Dmin   	  = DecBands[i];
    regions[Nr].Dmax   	  = DecBands[i+1];
    regions[Nr].index  	  =  i+1;
    regions[Nr].depth  	  =  1;
    regions[Nr].parent 	  =  0;
    regions[Nr].child  	  =  TRUE;
    regions[Nr].table  	  =  FALSE;
    memset (regions[Nr].name, 0, 21);
    strcpy (regions[Nr].name, DecNames[i]);
  }
  /* now south */
  for (i = 0; i < 12; i++, Nr++) {
    regions[Nr].Rmin   	  =   0;
    regions[Nr].Rmax   	  = 360;
    regions[Nr].Dmin   	  = DecBands[i+14];
    regions[Nr].Dmax   	  = DecBands[i+13];
    regions[Nr].index  	  =  i+1;
    regions[Nr].depth  	  =  1;
    regions[Nr].parent 	  =  0;
    regions[Nr].child  	  =  TRUE;
    regions[Nr].table  	  =  FALSE;
    memset (regions[Nr].name, 0, 21);
    strcpy (regions[Nr].name, DecNames[i+13]);
  }
  regions[No].childE = Nr;

  CHECK_REALLOCATE (regions, SkyRegion, NR, Nr, 100);

  /* level 2 : copy the data from the GSC Region files */
  No = 1;
  Dmin = regions[No].Dmin - 0.2;
  Dmax = regions[No].Dmax + 0.2;
  regions[No].childS = Nr;

  for (i = 0; i < Ntycho; i++) {
    /* if we are outside of current region, go to the next one */
    if ((tycho[i].Dmin < Dmin) || (tycho[i].Dmax > Dmax)) {
      regions[No].childE = Nr;

      No++;
      Dmin = regions[No].Dmin - 0.2;
      Dmax = regions[No].Dmax + 0.2;
      regions[No].childS = Nr;

      if ((tycho[i].Dmin < Dmin) || (tycho[i].Dmax > Dmax)) {
	fprintf (stderr, "ERROR: tycho index is not in order!\n");
	exit (1);
      }
    }

    /* set the values for this region */
    regions[Nr].Rmin = tycho[i].Rmin;
    regions[Nr].Rmax = tycho[i].Rmax;
    regions[Nr].Dmin = tycho[i].Dmin;
    regions[Nr].Dmax = tycho[i].Dmax;
    strcpy (regions[Nr].name, tycho[i].name);

    regions[Nr].index    =  Nr;
    regions[Nr].depth    =  2;
    regions[Nr].parent   =  No;
    regions[Nr].child    =  FALSE;
    regions[Nr].table    =  FALSE;
    regions[Nr].childS   =  0;
    regions[Nr].childE   =  0;

    Nr ++;
    CHECK_REALLOCATE (regions, SkyRegion, NR, Nr, 100);
  }

  free (tycho);

  ALLOCATE (skytable, SkyTable, 1);
  skytable[0].regions = regions;
  skytable[0].Nregions = Nr;

  ALLOCATE (skytable[0].filename, char *, skytable[0].Nregions);
  for (i = 0; i < skytable[0].Nregions; i++) {
    skytable[0].filename[i] = NULL;
  }
  if (VERBOSE) fprintf (stderr, "loaded "OFF_T_FMT" tables from tycho index\n",  skytable[0].Nregions);

  return (skytable);
}
