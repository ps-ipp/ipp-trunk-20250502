# include "addstar.h"
# define NZONE 24
# define NBYTE  4
# define NELEM  3

Catalog *getusno (SkyRegion *catstats) {

  off_t offset;
  int i, bin, first, last, nitems, Nitems, Nbins, Nstars;
  float hours[100];
  int start[100], number[100], *buffer, *buf;
  char filename[300];
  FILE *f;
  int iRA0, iRA1, iDEC0, iDEC1;
  double dec;
  int spd, spd_start, spd_end;
  short int USNO_RED, USNO_BLUE;

  /* require photcode */
  NAMED_PHOTCODE (USNO_RED, "USNO_RED");
  NAMED_PHOTCODE (USNO_BLUE, "USNO_BLUE");

  /* identify ra & dec range of interest */
  /* note: the use of UserPatch to restrict here limits general utility of function */
  iRA0  =  MAX (catstats[0].Rmin, UserPatch.Rmin) * 360000.0;
  iRA1  =  MIN (catstats[0].Rmax, UserPatch.Rmax) * 360000.0;
  iDEC0 = (MAX (catstats[0].Dmin, UserPatch.Dmin) + 90.0) * 360000.0;
  iDEC1 = (MIN (catstats[0].Dmax, UserPatch.Dmax) + 90.0) * 360000.0;
  
  /* data is organized in south-pole distance zones */
  spd_start = (int)((catstats[0].Dmin + 90) / 7.5) * 75.0;
  dec = (catstats[0].Dmax + 90) / 7.5;
  if (dec > (int)(dec)) {
    spd_end =   (int)(1 + (catstats[0].Dmax + 90) / 7.5) * 75.0;
  } else {
    spd_end =   (int)(0 + (catstats[0].Dmax + 90) / 7.5) * 75.0;
  }

  int Nave = 0;
  int Nmeas = 0;
  int NAVE = 1000;
  int NMEAS = 1000;

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);
  ALLOCATE (catalog->average, Average, NAVE);
  ALLOCATE (catalog->measure, Measure, NMEAS);

  for (spd = spd_start; spd < spd_end; spd += 75) {
    
    /* load accelerator file */
    sprintf (filename, "%s/zone%04d.acc", USNO_A_DIR, spd); 
    if (VERBOSE) fprintf (stderr, "reading from %s\n", filename);
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't open accelerator file %s\n", filename);
      exit (1);  
    }
    for (i = 0; fscanf (f, "%f %d %d", &hours[i], &start[i], &number[i]) != EOF; i++);
    Nbins = i;
    fclose (f);
    
    first = catstats[0].Rmin / 3.75;
    if ((catstats[0].Rmax / 3.75) == (int) (catstats[0].Rmax / 3.75)) 
      last  = catstats[0].Rmax / 3.75;
    else 
      last  = 1 + catstats[0].Rmax / 3.75;

    if ((first > Nbins) || (last > Nbins)) {
      fprintf (stderr, "ERROR: RA out of range\n");
      exit (1);
    }
    
    /* open data file */
    sprintf (filename, "%s/zone%04d.cat", USNO_A_DIR, spd);
    if (VERBOSE) fprintf (stderr, "reading from %s\n", filename);
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't open file %s\n", filename);
      exit (1);
    }

    /** USNO-A consists of 3 x 4byte (int) records **/
    /* advance file pointer to first slice */
    offset = NELEM*NBYTE*(start[first] - 1);
    fseeko (f, offset, SEEK_SET);

    /* sum the number of stars in data segment of interest */
    Nstars = 0;
    for (bin = first; bin < last; bin++) {
      Nstars += number[bin];
    }
    Nitems = NELEM*Nstars;  
    /* number of blocks to read -- we need to use Fread for byte-swapping read */

    /* read stars from catalog */
    ALLOCATE (buffer, int, Nitems);
    nitems = Fread (buffer, NBYTE, Nitems, f, "int");
    if (nitems != Nitems) {
      fprintf (stderr, "ERROR: failure reading data from file %s\n", filename);
      exit (1);
    }

    /* extract the data of interest from segment (in RA and DEC range) */
    buf = buffer;
    for (i = 0; i < Nstars; i++, buf += NELEM) {
      if (buf[0] < iRA0) continue;
      if (buf[0] > iRA1) continue;
      if (buf[1] < iDEC0) continue;
      if (buf[1] > iDEC1) continue;

      dvo_average_init (&catalog->average[Nave]);
      dvo_measure_init (&catalog->measure[Nmeas+0]);
      dvo_measure_init (&catalog->measure[Nmeas+1]);

      catalog->average[Nave].R     = buf[0]/360000.0;
      catalog->average[Nave].D     = buf[1]/360000.0 - 90.0;

      catalog->measure[Nmeas+0].R  = catalog->average[Nave].R;
      catalog->measure[Nmeas+0].D  = catalog->average[Nave].D;
      catalog->measure[Nmeas+0].dM = NAN;
      catalog->measure[Nmeas+1].R  = catalog->average[Nave].R;
      catalog->measure[Nmeas+1].D  = catalog->average[Nave].D;
      catalog->measure[Nmeas+1].dM = NAN;

      catalog->measure[Nmeas+0].photcode = USNO_RED;
      catalog->measure[Nmeas+0].M        = fabs (0.1*(buf[2] - 1000*((int)(buf[2]/1000))));
      catalog->measure[Nmeas+1].photcode = USNO_BLUE;
      catalog->measure[Nmeas+1].M        = fabs (0.1*((int)(buf[2] - 1000000*((int)(buf[2]/1000000))) / 1000));

      catalog->average[Nave].Nmeasure = 2;
      catalog->average[Nave].measureOffset = Nmeas;

      Nave ++;
      Nmeas += 2;

      CHECK_REALLOCATE (catalog->average, Average, NAVE,  Nave,  1000);
      CHECK_REALLOCATE (catalog->measure, Measure, NMEAS, Nmeas, 1000);
    }
    free (buffer);
    fclose (f);
  }
  catalog->Naverage = Nave;
  catalog->Nmeasure = Nmeas;

  if (VERBOSE) fprintf (stderr, "%d stars from USNO 1.0\n", Nave);
  return (catalog);
}




/* these entries are legacy code incase you want to read from one of the USNO CDRoms
int SPDzone[]  = {0, 75, 450, 375, 1500, 1650, 300, 1425, 1725, 525, 1275, 225, 675, 150, 600, 1575, 750, 975, 900, 1050, 1125, 1200, 825, 1350};
int USNOdisk[] = {1,  1,   1,   2,    2,    2,   3,    3,    3,   4,    4,   5,   5,   6,   6,    6,   7,   7,   8,    8,    9,    9,  10,   10};

    disk = -1;
    for (i = 0; i < NZONE; i++) {
      if (spd == SPDzone[i]) 
	disk = USNOdisk[i];
    }
    if (disk < 0) {
      fprintf (stderr, "ERROR: can't find cdrom for spd %d\n",  spd);
      exit (1);
    }

*/

