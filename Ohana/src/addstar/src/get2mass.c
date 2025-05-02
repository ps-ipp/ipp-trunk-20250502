# include "addstar.h"
# include "2mass.h"

Stars *get2mass (SkyRegion *patch, int photcode, int mode, unsigned int *NSTARS) {
  
  char *path;
  char gzname[1024];
  int i, status, Nstars, Nrefcat; 
  Stars    *stars;
  Stars    *refcat;
  SkyTable *sky;
  struct stat filestat;

  fprintf (stderr, "please use the load2mass program instead\n");
  exit (1);

  NAMED_PHOTCODE (TM_J, "2MASS_J");
  NAMED_PHOTCODE (TM_H, "2MASS_H");
  NAMED_PHOTCODE (TM_K, "2MASS_K");
  if (photcode == TM_J) goto good_code;
  if (photcode == TM_H) goto good_code;
  if (photcode == TM_K) goto good_code;
  Shutdown ("2MASS photcode not specified");

good_code:
  path = TWO_MASS_DIR_AS;
  if (mode == 1) path = TWO_MASS_DIR_DR2;

  // the accel.dat file has the raw filenames
  // test if the file exists, or else try the .gz version
  sky = get2mass_acc (patch, path, "accel.dat");
  
  Nstars = 0;
  ALLOCATE (stars, Stars, 1);

  for (i = 0; i < sky[0].Nregions; i++) {
    refcat = NULL;
    switch (mode) {
      case 0:
	// XXX put filename from table here
	status = stat (sky[0].filename[i], &filestat);
	if ((status == -1) && (errno == ENOENT)) {
	  sprintf (gzname, "%s.gz", sky[0].filename[i]);
	  refcat = get2mass_AS_data (&sky[0].regions[i], gzname, patch, photcode, &Nrefcat);
	} else {
	  refcat = get2mass_AS_rawdata (&sky[0].regions[i], sky[0].filename[i], patch, photcode, &Nrefcat);
	}
	if (VERBOSE) fprintf (stderr, "loaded %d stars from 2MASS (allsky) : %s\n", Nrefcat, sky[0].filename[i]);
	break;
      case 1:
	refcat = get2mass_2DR_data (&sky[0].regions[i], sky[0].filename[i], patch, photcode, &Nrefcat);
	if (VERBOSE) fprintf (stderr, "loaded %d stars from 2MASS (dr2)\n", Nrefcat);
	break;
    }

    REALLOCATE (stars, Stars, MAX (1, Nstars + Nrefcat));
    memcpy (&stars[Nstars], refcat, Nrefcat*sizeof(Stars));
    Nstars += Nrefcat;

    free (refcat);
  }
  
  if (VERBOSE) fprintf (stderr, "loaded total %d stars from 2MASS\n", Nstars);
  *NSTARS = Nstars;
  return (stars);
}  

/* watch for patches which cross 0,360 boundary */
SkyTable *get2mass_acc (SkyRegion *patch, char *path, char *accel) {

  int Nregions, NREGIONS, Nrec;
  char accelfile[1024], line[256], filename[128], datafile[256], **filenames;
  FILE *f;
  double Rs, Re, Ds, De;

  SkyTable *sky;
  SkyRegion *regions;

  sprintf (accelfile, "%s/%s", path, accel);
  f = fopen (accelfile, "r");
  if (f == NULL) Shutdown ("can't read data from accelerator %s", accelfile);

  Nregions = 0;
  NREGIONS = 200;
  ALLOCATE (regions, SkyRegion, NREGIONS);
  ALLOCATE (filenames, char *, NREGIONS);

  /* read in stars line-by-line */
  while (scan_line (f, line) != EOF) {
    stripwhite (line);
    if (line[0] == 0) continue;
    if (line[0] == '#') continue;
    sscanf (line, "%s %lf %lf %lf %lf %d", filename, &Rs, &Re, &Ds, &De, &Nrec);
    Rs *= 15.0;
    Re *= 15.0;
    if (Rs > patch[0].Rmax) continue;
    if (Re < patch[0].Rmin) continue;
    if (Ds > patch[0].Dmax) continue;
    if (De < patch[0].Dmin) continue;
    regions[Nregions].Rmin = Rs;
    regions[Nregions].Rmax = Re;
    regions[Nregions].Dmin = Ds;
    regions[Nregions].Dmax = De;
    regions[Nregions].childE = Nrec; // a cheat since 2MASS only has one depth
    fprintf (stderr, "choosing: %10.6f - %10.6f, %10.6f - %10.6f\n", Rs, Re, Ds, De);

    sprintf (datafile, "%s/%s", path, filename);
    filenames[Nregions] = strcreate (datafile);

    Nregions ++;
    if (Nregions >= NREGIONS) {
	NREGIONS += 20;
	REALLOCATE (regions, SkyRegion, NREGIONS);
	REALLOCATE (filenames, char *, NREGIONS);
    }
  }    
  fclose (f);

  ALLOCATE (sky, SkyTable, 1);
  sky[0].regions = regions;
  sky[0].filename = filenames;
  sky[0].Nregions = Nregions;
  return (sky);
}
