# include "mosastro.h"
# define NBYTE   4
# define NELEM  20

# define MY_MAX_PATH 256

StarData *getusnob (CatStats *catstats, int *Nstars) {

  off_t offset, nitems, Nitems;
  int i, bin, first, last, Nbins;
  float hours[100];
  int start[100], number[100], *buffer, *buf;
  char filename[MY_MAX_PATH];
  FILE *f;
  double DEC1;
  double uR, uD;
  float mB1, mB2, mR1, mR2, mB, mR;
  int iDEC0, iDEC1, iRA0, iRA1;
  int spd, spd_start, spd_end;
  int NUSNO, Nusno, nstars;
  StarData *stars;

  /* identify ra & dec range of interest */
  iRA0 = catstats[0].RA[0] * 360000.0;
  iRA1 = catstats[0].RA[1] * 360000.0;
  iDEC0 = (catstats[0].DEC[0] + 90.0) * 360000.0;
  iDEC1 = (catstats[0].DEC[1] + 90.0) * 360000.0;
  /* note that DEC is in SPD, while both have units to 0.01 degrees */
  
  /* data is organized in south-pole distance zones, 1 deg per direction, 0.1 deg per file */
  spd_start = (int)(10*(catstats[0].DEC[0] + 90));
  DEC1 = 10*(catstats[0].DEC[1] + 90);
  if (DEC1 > (int)(DEC1)) {
    spd_end =   (int)(1 + 10*(catstats[0].DEC[1] + 90));
  } else {
    spd_end =   (int)(0 + 10*(catstats[0].DEC[1] + 90));
  }

  Nusno = 0;
  NUSNO = 5000;
  ALLOCATE (stars, StarData, NUSNO);

  for (spd = spd_start; spd < spd_end; spd ++) {
    
    /* load accelerator file */
    snprintf_nowarn (filename, MY_MAX_PATH, "%s/%03d/b%04d.acc", USNO_B_DIR, (int)(spd/10), spd); 
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
    snprintf_nowarn (filename, MY_MAX_PATH, "%s/%03d/b%04d.cat", USNO_B_DIR, (int)(spd/10), spd); 
    fprintf (stderr, "reading from %s\n", filename);
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't open file %s\n", filename);
      exit (1);
    }

    /** USNO-B consists of 20 x 4byte (int) records **/
    /* advance file pointer to first slice */
    offset = NELEM*NBYTE*(start[first] - 1);
    fseeko (f, offset, SEEK_SET);

    /* sum the number of stars in data segment of interest */
    nstars = 0;
    for (bin = first; bin < last; bin++) {
      nstars += number[bin];
    }
    Nitems = NELEM*nstars;  /* number of integer blocks; need to use Fread for byte-swapping read */

    /* allocate space for stars in segment */
    ALLOCATE (buffer, int, Nitems);
    // data has the WRONG byte order?
    // nitems = Fread (buffer, sizeof(int), Nitems, f, "int");
    nitems = fread (buffer, sizeof(int), Nitems, f);
    if (nitems != Nitems) {
      fprintf (stderr, "ERROR: failure reading data from file %s\n", filename);
      exit (1);
    }

    buf = buffer;
    /* print out data from slice within RA and DEC range */
    for (i = 0; i < nstars; i++, buf+=NELEM) {
      if (buf[0] < iRA0) continue;
      if (buf[0] > iRA1) continue;
      if (buf[1] < iDEC0) continue;
      if (buf[1] > iDEC1) continue;
      
      bzero (&stars[Nusno], sizeof(StarData));
      stars[Nusno].R = buf[0]/360000.0;
      stars[Nusno].D = buf[1]/360000.0 - 90.0;
      
      uR = (buf[2] % 10000);
      uR = (uR - 5000.0) * 0.002 / 3600.0;
      uD = ((buf[2] / 10000) % 10000);
      uD = (uD - 5000.0) * 0.002 / 3600.0;

      /* 1st blue mag */
      mB1 = 0.01 * (buf[5] % 10000);
      /* 1st blue mag */
      mB2 = 0.01 * (buf[6] % 10000);
      /* 1st blue mag */
      mR1 = 0.01 * (buf[7] % 10000);
      /* 1st blue mag */
      mR2 = 0.01 * (buf[8] % 10000);

      if (mB1 && mB2) {
	mB = 0.5*(mB1 + mB2);
      } else {
	mB = (mB1) ? mB1 : mB2;
      }

      if (mR1 && mR2) {
	mR = 0.5*(mR1 + mR2);
      } else {
	mR = (mR1) ? mR1 : mR2;
      }
      
      stars[Nusno].M = (TRUE) ? mB : mR;
      stars[Nusno].R += uR*(Year - 2000.0);
      stars[Nusno].D += uD*(Year - 2000.0);
      Nusno ++;
      CHECK_REALLOCATE (stars, StarData, NUSNO, Nusno, 5000);
    }
    free (buffer);
    fclose (f);
  }
  
  *Nstars = Nusno;
  if (VERBOSE) fprintf (stderr, "%d stars from USNO B1.0\n", Nusno);
  return (stars);
}
