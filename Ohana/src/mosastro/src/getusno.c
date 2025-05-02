# include "mosastro.h"
# define NZONE 24

int SPDzone[] = {
  0, 75, 450, 375, 1500, 1650, 300, 1425, 1725, 525, 1275, 225, 
  675, 150, 600, 1575, 750, 975, 900, 1050, 1125, 1200, 825, 1350};

int USNOdisk[] = {
  1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10};

# define MY_MAX_PATH 256

StarData *getusno (CatStats *catstats, int *Nstars) {

  off_t offset, nitems, Nitems;
  int i, bin, first, last, Nbins;
  float hours[100];
  int start[100], number[100], *buffer, *buf;
  char filename[MY_MAX_PATH];
  FILE *f;
  int iRA0, iRA1, iDEC0, iDEC1;
  double RA0, RA1, DEC0, DEC1, dec;
  int spd, spd_start, spd_end, disk;
  int NUSNO, Nusno;
  StarData *stars;

  RA0  = catstats[0].RA[0]; 
  RA1  = catstats[0].RA[1]; 
  DEC0 = catstats[0].DEC[0];
  DEC1 = catstats[0].DEC[1];

  /* identify ra & dec range of interest */
  iRA0 = RA0 * 360000.0;
  iRA1 = RA1 * 360000.0;
  iDEC0 = (DEC0 + 90.0) * 360000.0;
  iDEC1 = (DEC1 + 90.0) * 360000.0;
  
  /* data is organized in south-pole distance zones */
  spd_start = (int)((DEC0 + 90) / 7.5) * 75.0;
  dec = (DEC1 + 90) / 7.5;
  if (dec > (int)(dec)) {
    spd_end =   (int)(1 + (DEC1 + 90) / 7.5) * 75.0;
  } else {
    spd_end =   (int)(0 + (DEC1 + 90) / 7.5) * 75.0;
  }

  Nusno = 0;
  NUSNO = 5000;
  ALLOCATE (stars, StarData, NUSNO);

  for (spd = spd_start; spd < spd_end; spd += 75) {
    disk = -1;
    for (i = 0; i < NZONE; i++) {
      if (spd == SPDzone[i]) 
	disk = USNOdisk[i];
    }
    if (disk < 0) {
      fprintf (stderr, "ERROR: can't find cdrom for spd %d\n",  spd);
      exit (0);
    }
    
    /* load accelerator file */
    snprintf_nowarn (filename, MY_MAX_PATH, "%s/zone%04d.acc", USNO_A_DIR, spd); 
    if (VERBOSE) fprintf (stderr, "reading from %s\n", filename);
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't open file %s, is cdrom %d in drive?\n", filename, disk);
      exit (1);  
    }
    for (i = 0; fscanf (f, "%f %d %d", &hours[i], &start[i], &number[i]) != EOF; i++);
    Nbins = i;
    fclose (f);
    
    first = RA0 / 3.75;
    if ((RA1 / 3.75) == (int) (RA1 / 3.75)) 
      last  = RA1 / 3.75;
    else 
      last  = 1 + RA1 / 3.75;

    if ((first > Nbins) || (last > Nbins)) {
      fprintf (stderr, "ERROR: RA out of range\n");
      exit (1);
    }
    
    /* open data file */
    snprintf_nowarn (filename, MY_MAX_PATH, "%s/zone%04d.cat", USNO_A_DIR, spd);
    if (VERBOSE) fprintf (stderr, "reading from %s\n", filename);
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't open file %s\n", filename);
      exit (1);
    }
    /* advance file pointer to first slice */
    offset = 3*sizeof(int)*(start[first] - 1);
    fseeko (f, offset, SEEK_SET);
    /* on each loop, load data from an RA slice of the catalog */
    for (bin = first; bin < last; bin++) {
      Nitems = 3*number[bin];
      ALLOCATE (buffer, int, Nitems);
      nitems = Fread (buffer, sizeof(int), Nitems, f, "int");
      if (nitems != Nitems) {
	fprintf (stderr, "ERROR: failure reading data from file %s\n", filename);
	exit (1);
      }
      buf = buffer;
      /* print out data from slice within RA and DEC range */
      for (i = 0; i < number[bin]; i++, buf+=3) {
	if ((buf[0] > iRA0) && (buf[0] < iRA1) &&
	    (buf[1] > iDEC0) && (buf[1] < iDEC1)) {
	  bzero (&stars[Nusno], sizeof(StarData));
	  stars[Nusno].R = buf[0]/360000.0;
	  stars[Nusno].D = buf[1]/360000.0 - 90.0;
	  stars[Nusno].Mag = fabs (0.1*(buf[2] - 1000*((int)(buf[2]/1000))));
	  /* b = 0.1*((int)(buf[2] - 1000000*((int)(buf[2]/1000000))) / 1000); */
	  Nusno ++;
	  if (Nusno == NUSNO) {
	    NUSNO += 5000;
	    REALLOCATE (stars, StarData, NUSNO);
	  }	  
	}
      }
      free (buffer);
    }
    fclose (f);
  }

  *Nstars = Nusno;
  if (VERBOSE) fprintf (stderr, "%d stars from USNO 1.0\n", Nusno);
  return (stars);
}


