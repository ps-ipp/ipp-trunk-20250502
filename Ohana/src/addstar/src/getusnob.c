# include "addstar.h"
# define NBYTE   4
# define NELEM  20

Catalog *getusnob (SkyRegion *catstats) {

  off_t offset;
  int i, j, bin, first, last, nitems, Nitems, Nbins, Nitemsum;
  float hours[100];
  int start[100], number[100], *buffer, *buf;
  char filename[300];
  FILE *f;
  double dec;
  float m1, m2, mag;
  int iDEC0, iDEC1, iRA0, iRA1;
  int spd, spd_start, spd_end;
  short int USNO_RED, USNO_BLUE, USNO_N;
  e_time USNOepoch;

  m1 = m2 = 0.0;

  /* require photcode */
  NAMED_PHOTCODE (USNO_RED,  "USNO_RED");
  NAMED_PHOTCODE (USNO_BLUE, "USNO_BLUE");
  NAMED_PHOTCODE (USNO_N,    "USNO_N");

  fprintf (stderr, "loading USNO-B 1.0\n");

  /* identify ra & dec range of interest */
  /* note: the use of UserPatch to restrict here limits general utility of function */
  iRA0  =  MAX (catstats[0].Rmin, UserPatch.Rmin) * 360000.0;
  iRA1  =  MIN (catstats[0].Rmax, UserPatch.Rmax) * 360000.0;
  iDEC0 = (MAX (catstats[0].Dmin, UserPatch.Dmin) + 90.0) * 360000.0;
  iDEC1 = (MIN (catstats[0].Dmax, UserPatch.Dmax) + 90.0) * 360000.0;
  /* note that iDECn is in SPD, while both have units of 0.01 degrees */
  
  /* data is organized in south-pole distance zones, 1 deg per direction, 0.1 deg per file */
  spd_start = (int)(10*(catstats[0].Dmin + 90));
  dec = 10*(catstats[0].Dmax + 90);
  if (dec > (int)(dec)) {
    spd_end =   (int)(1 + 10*(catstats[0].Dmax + 90));
  } else {
    spd_end =   (int)(0 + 10*(catstats[0].Dmax + 90));
  }

  Nitemsum = 0;

  int Nave = 0;
  int Nmeas = 0;
  int NAVE = 1000;
  int NMEAS = 1000;

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);
  ALLOCATE (catalog->average, Average, NAVE);
  ALLOCATE (catalog->measure, Measure, NMEAS);

  for (spd = spd_start; spd < spd_end; spd ++) {
    
    /* load accelerator file */
    sprintf (filename, "%s/%03d/b%04d.acc", USNO_B_DIR, (int)(spd/10), spd); 
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
    sprintf (filename, "%s/%03d/b%04d.cat", USNO_B_DIR, (int)(spd/10), spd); 
    if (VERBOSE) fprintf (stderr, "reading from %s\n", filename);
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
    int Nstars = 0;
    for (bin = first; bin < last; bin++) {
      Nstars += number[bin];
    }
    Nitems = NELEM*Nstars;  
    /* number of blocks to read -- we need to use Fread for byte-swapping read */

    /* allocate space for stars in segment and read */
    ALLOCATE (buffer, int, Nitems);
    // data has the WRONG byte order?
    // nitems = Fread (buffer, sizeof(int), Nitems, f, "int");
    nitems = fread (buffer, NBYTE, Nitems, f);
    if (nitems != Nitems) {
      fprintf (stderr, "ERROR: failure reading data from file %s\n", filename);
      exit (1);
    }

    USNOepoch = ohana_date_to_sec ("2000/01/01,00:00:00");

    buf = buffer;
    /* print out data from slice within RA and DEC range */
    for (i = 0; i < Nstars; i++, buf += NELEM) {
      if (buf[0] < iRA0) continue;
      if (buf[0] > iRA1) continue;
      if (buf[1] < iDEC0) continue;
      if (buf[1] > iDEC1) continue;
      
      /* USNO coords are reported for J2000 / epoch 2000.0 */
      /* extract the basic stellar data */
      dvo_average_init (&catalog->average[Nave]);
      dvo_measure_init (&catalog->measure[Nmeas+0]);
      dvo_measure_init (&catalog->measure[Nmeas+1]);

      catalog->average[Nave].R   = buf[0]/360000.0;
      catalog->average[Nave].D   = buf[1]/360000.0 - 90.0;

      /* XXX uR cos(D) or just uR ??? */
      catalog->average[Nave].uR  = 2.0 * ((buf[2]       % 10000) - 5000);
      catalog->average[Nave].uD  = 2.0 * ((buf[2]/10000 % 10000) - 5000);

      catalog->average[Nave].duR = (buf[3]      % 1000);
      catalog->average[Nave].duD = (buf[3]/1000 % 1000);
      catalog->average[Nave].dR  = 0.001 * (buf[4]      % 1000);
      catalog->average[Nave].dD  = 0.001 * (buf[4]/1000 % 1000);
      catalog->average[Nave].P   = 0;
      catalog->average[Nave].dP  = 0;

      /* USNO-B uses J2000 equinox and 2000.0 epoch for coordinates */
      /* the magnitudes have no temporal information */ 

      for (j = 0; j < 3; j++) {
	catalog->measure[Nmeas+j].R  = catalog->average[Nave].R;
	catalog->measure[Nmeas+j].D  = catalog->average[Nave].D;
	catalog->measure[Nmeas+j].dM = 0.3; /* USNO magnitude errors are reported as a fixed 0.3 mag */
	catalog->measure[Nmeas+j].t  = USNOepoch;
      }

      catalog->measure[Nmeas+0].photcode = USNO_BLUE;
      m1 = fabs(0.01 * (buf[5] % 10000)); /* 1st blue mag */
      m2 = fabs(0.01 * (buf[7] % 10000)); /* 2nd blue mag */
      if (m1 && m2) {
	mag = 0.5*(m1 + m2);
      } else {
	mag = (m1) ? m1 : m2;
      }
      catalog->measure[Nmeas+0].M = (mag == 0.0) ? 32.0 : mag;

      catalog->measure[Nmeas+1].photcode = USNO_RED;
      m1 = fabs(0.01 * (buf[6] % 10000)); /* 1st red mag */
      m2 = fabs(0.01 * (buf[8] % 10000)); /* 2nd red mag */
      if (m1 && m2) {
	mag = 0.5*(m1 + m2);
      } else {
	mag = (m1) ? m1 : m2;
      }
      catalog->measure[Nmeas+1].M = (mag == 0.0) ? 32.0 : mag;
      
      catalog->measure[Nmeas+2].photcode = USNO_N;
      catalog->measure[Nmeas+2].M = fabs(0.01 * (buf[9] % 10000)); /* N (IR) survey */

      catalog->average[Nave].Nmeasure = 3;
      catalog->average[Nave].measureOffset = Nmeas;

      Nave ++;
      Nmeas += 3;

      CHECK_REALLOCATE (catalog->average, Average, NAVE,  Nave,  1000);
      CHECK_REALLOCATE (catalog->measure, Measure, NMEAS, Nmeas, 1000);
    }
    free (buffer);
    Nitemsum += Nitems;
    if (VERBOSE) fprintf (stderr, "Naverage: %d, Nitems: %d, Nitemsum : %d\n", Nave, Nitems, Nitemsum);
    fclose (f);
  }
  catalog->Naverage = Nave;
  catalog->Nmeasure = Nmeas;

  if (VERBOSE) fprintf (stderr, "%d stars from USNO-B 1.0\n", Nave);
  return (catalog);
}
