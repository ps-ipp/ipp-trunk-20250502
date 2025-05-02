# include "gastro2.h"
# define NZONE 24

int SPDzone[] = {
  0, 75, 450, 375, 1500, 1650, 300, 1425, 1725, 525, 1275, 225, 
  675, 150, 600, 1575, 750, 975, 900, 1050, 1125, 1200, 825, 1350};

int USNOdisk[] = {
  1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10};

# define MY_MAX_PATH 256

int getusno (CatStats *catstats, RefCatalog *Ref) {

  off_t offset;
  int i, bin, first, last, nitems, Nitems, Nbins;
  float hours[100];
  int start[100], number[100], *buffer, *buf;
  char filename[MY_MAX_PATH];
  FILE *f;
  double DEC1;
  int iDEC0, iDEC1, iRA0, iRA1;
  int spd, spd_start, spd_end, disk;
  int NUSNO, Nusno;
  StarData *stars;

  /* identify ra & dec range of interest */
  iRA0 = catstats[0].RA[0] * 360000.0;
  iRA1 = catstats[0].RA[1] * 360000.0;
  iDEC0 = (catstats[0].DEC[0] + 90.0) * 360000.0;
  iDEC1 = (catstats[0].DEC[1] + 90.0) * 360000.0;
  
  /* data is organized in south-pole distance zones */
  spd_start = (int)((catstats[0].DEC[0] + 90) / 7.5) * 75.0;
  DEC1 = (catstats[0].DEC[1] + 90) / 7.5;
  if (DEC1 > (int)(DEC1)) {
    spd_end =   (int)(1 + (catstats[0].DEC[1] + 90) / 7.5) * 75.0;
  } else {
    spd_end =   (int)(0 + (catstats[0].DEC[1] + 90) / 7.5) * 75.0;
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
      fprintf (stderr, "ERROR: can't find USNO zone for spd %d\n",  spd);
      exit (0);
    }
    
    /* load accelerator file */
    snprintf_nowarn (filename, MY_MAX_PATH, "%s/zone%04d.acc", USNO_A_DIR, spd); 
    fprintf (stderr, "reading from %s\n", filename);
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't open accelerator file %s\n", filename);
      exit (1);  
    }
    for (i = 0; fscanf (f, "%f %d %d", &hours[i], &start[i], &number[i]) != EOF; i++);
    Nbins = i;
    fclose (f);
    
    first = catstats[0].RA[0] / 3.75;
    if ((catstats[0].RA[1] / 3.75) == (int) (catstats[0].RA[1] / 3.75)) 
      last  = catstats[0].RA[1] / 3.75;
    else 
      last  = 1 + catstats[0].RA[1] / 3.75;

    if ((first > Nbins) || (last > Nbins)) {
      fprintf (stderr, "ERROR: RA out of range\n");
      exit (1);
    }
    
    /* open data file */
    snprintf_nowarn (filename, MY_MAX_PATH, "%s/zone%04d.cat", USNO_A_DIR, spd);
    fprintf (stderr, "reading from %s\n", filename);
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
	  stars[Nusno].R = buf[0]/360000.0;
	  stars[Nusno].D = buf[1]/360000.0 - 90.0;
	  /* note that this is the RED mag */
	  stars[Nusno].M = fabs (0.1*(buf[2] - 1000*((int)(buf[2]/1000))));
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

  area_of_region (catstats);

  REALLOCATE (stars, StarData, MAX (1, Nusno));

  Ref[0].stars = stars;
  Ref[0].N     = Nusno;
  Ref[0].R0    = catstats[0].RA[0];
  Ref[0].R1    = catstats[0].RA[1];
  Ref[0].D0    = catstats[0].DEC[0];
  Ref[0].D1    = catstats[0].DEC[1];
  Ref[0].Area  = catstats[0].Area;

  get_luminosity_func (Ref[0].stars, Ref[0].N, &Ref[0].lum);
  
  if (VERBOSE) fprintf (stderr, "%d stars from USNO 1.0\n", Nusno);
  return (TRUE);
}


