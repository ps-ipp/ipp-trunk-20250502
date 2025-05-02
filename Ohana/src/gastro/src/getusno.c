# include "gastro.h"
# define NZONE 24

int SPDzone[] = {
  0, 75, 450, 375, 1500, 1650, 300, 1425, 1725, 525, 1275, 225, 
  675, 150, 600, 1575, 750, 975, 900, 1050, 1125, 1200, 825, 1350};

int USNOdisk[] = {
  1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10};

# define MY_MAX_PATH 256

USNOdata *getusno (USNOstats *usnostats, CatStats *catstats, int *Nusno) {

  off_t offset;
  int i, bin, first, last, nitems, Nitems, Nbins;
  float hours[100];
  int start[100], number[100], *buffer, *buf;
  char filename[MY_MAX_PATH], c;
  FILE *f;
  double DEC1;
  int iDEC0, iDEC1, iRA0, iRA1;
  int spd, spd_start, spd_end, disk;
  int NUSNO, nusno;
  USNOdata *usno;

  iRA0 = catstats[0].RA[0] * 360000.0;
  iRA1 = catstats[0].RA[1] * 360000.0;
  iDEC0 = (catstats[0].DEC[0] + 90.0) * 360000.0;
  iDEC1 = (catstats[0].DEC[1] + 90.0) * 360000.0;
  
  spd_start = (int)(    (catstats[0].DEC[0] + 90) / 7.5) * 75.0;
  DEC1 = (catstats[0].DEC[1] + 90) / 7.5;
  if (DEC1 > (int)(DEC1)) {
    spd_end =   (int)(1 + (catstats[0].DEC[1] + 90) / 7.5) * 75.0;
  } else {
    spd_end =   (int)(0 + (catstats[0].DEC[1] + 90) / 7.5) * 75.0;
  }

  NUSNO = 5000;
  ALLOCATE (usno, USNOdata, NUSNO);
  nusno = 0;

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
    snprintf_nowarn (filename, MY_MAX_PATH, "%s/zone%04d.acc", CDROM, spd); 
    fprintf (stderr, "reading from %s\n", filename);
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "can't open file %s, is cdrom %d in drive?\n", filename, disk);
      fprintf (stderr, "press return when ready to continue: ");
      if (fscanf (stdin, "%c", &c) != 1) fprintf (stderr, "\n");
      fprintf (stderr, "trying again...\n");
      f = fopen (filename, "r");
      if (f == (FILE *) NULL) {
	fprintf (stderr, "still can't open file %s, is cdrom %d in drive?\n", filename, disk);
	exit (0);  
      }
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
      fprintf (stderr, "RA out of range\n");
      exit (0);
    }
    
    /* open data file */
    snprintf_nowarn (filename, MY_MAX_PATH, "%s/zone%04d.cat", CDROM, spd);
    fprintf (stderr, "reading from %s\n", filename);
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "can't open file %s\n", filename);
      exit (0);
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
	fprintf (stderr, "error reading data from file %s (%d, %d, %d)\n", 
		 filename, start[bin], number[bin], nitems);
	exit (0);
      }
      buf = buffer;
      /* print out data from slice within RA and DEC range */
      for (i = 0; i < number[bin]; i++, buf+=3) {
	if ((buf[0] > iRA0) && (buf[0] < iRA1) &&
	    (buf[1] > iDEC0) && (buf[1] < iDEC1)) {
	  usno[nusno].R = buf[0]/360000.0;
	  usno[nusno].D = buf[1]/360000.0 - 90.0;
	  usno[nusno].r = 0.1*(buf[2] - 1000*((int)(buf[2]/1000)));
	  usno[nusno].b = 0.1*((int)(buf[2] - 1000000*((int)(buf[2]/1000000))) / 1000);
	  nusno ++;
	  if (nusno == NUSNO - 1) {
	    NUSNO += 5000;
	    REALLOCATE (usno, USNOdata, NUSNO);
	  }	  
	}
      }
      free (buffer);
    }
    fclose (f);
  }

  REALLOCATE (usno, USNOdata, MAX (nusno, 1));
  *Nusno = nusno;
  return (usno);

}


