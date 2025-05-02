# include "addstar.h"

/* .asc files look like:
0         1         2         3
0123456789012345678901234567890123456789
187.498117^  2.659253^21.06$
187.498672^  2.713833^17.80$
(^ = tab char)
*/

# define NLINE_ASC 10000
# define NBYTE_ASC_TABLE 28
# define RA_INDEX_ASC 0
# define DEC_INDEX_ASC 11
# define MAG_INDEX_ASC 22

Catalog *LoadDataPMM (FILE *f, char *imagename, Image **images, off_t *nvalid) {

  off_t Nvalid, NVALID;
  char *buffer;
  int i, fd, Nbyte, Nline, code;
  double ra, dec, mag, airmass, az, ZeroPoint, ZeroPt;
  unsigned int Ninstars, NINSTARS;
  PhotCode *photcode;
  gzFile gz;

  double minR0, minR1, maxR0, maxR1, minD, maxD;

  if (images[0] == NULL) {
    Nvalid = 0;
    NVALID = 1;
    ALLOCATE (images[0], Image, NVALID);
  } else {
    Nvalid = *nvalid;
    NVALID = Nvalid ++;
    REALLOCATE (images[0], Image, NVALID);
  }    

  // there is only one PMM image per file
  if (VERBOSE) fprintf (stderr, "reading data for %s\n", imagename);

  ZeroPt = GetZeroPoint();

  // need to get the metadata from the PMM_CCD_TABLE
  photcode = LoadMetadataPMM (imagename, &images[0][Nvalid]);
  code = photcode[0].code;
  ZeroPoint = 0.001*photcode[0].C;
  // XXX NOTE : as of 2008.02.27, the zero point is still carried internally in millimags

  ALLOCATE (buffer, char, NLINE_ASC*NBYTE_ASC_TABLE);

  // use the following to get alt, az:
  // altaz (&alt, &az, 15.0*images[N].sidtime - images[N].coords.crval1, images[N].coords.crval2, Latitude);
  // these two can be calculated from HA and LATITUDE (need a table of observatory LAT)
  airmass = 1.0;
  az = 0.0;

  NINSTARS = 10000;
  Catalog *catalog = addstar_catalog_init (NINSTARS);
  ALLOCATE (catalog->average, Average, NINSTARS);
  for (i = 0; i < NINSTARS; i++) {
    dvo_measure_init (&catalog->measure[i]);
    dvo_average_init (&catalog->average[i]);
  }

  minR0 = minR1 = 360.0;
  maxR0 = maxR1 =   0.0;
  minD = +90.0;
  maxD = -90.0;

  fd = fileno (f);
  gz = gzdopen (dup(fd), "rb");

  // read in a big chunk at a time, parse the lines assuming fixes line sizes and fields
  Ninstars = 0;
  while (1) {

    Nbyte = gzread (gz, buffer, NLINE_ASC*NBYTE_ASC_TABLE);
    if (Nbyte == 0) break;

    assert (Nbyte % NBYTE_ASC_TABLE == 0);
    Nline = Nbyte / NBYTE_ASC_TABLE;

    for (i = 0; i < Nline; i++) {
      dparse (&ra,  1, &buffer[i*NBYTE_ASC_TABLE]);
      dparse (&dec, 2, &buffer[i*NBYTE_ASC_TABLE]);
      dparse (&mag, 3, &buffer[i*NBYTE_ASC_TABLE]);

      if (ra > 180) {
	minR1 = MIN(minR1, ra);
	maxR1 = MAX(maxR1, ra);
      } else {
	minR0 = MIN(minR0, ra);
	maxR0 = MAX(maxR0, ra);
      }
      minD = MIN(minD, dec);
      maxD = MAX(maxD, dec);

      catalog->average[Ninstars].R        = ra;
      catalog->average[Ninstars].D        = dec;
      catalog->average[Ninstars].Nmeasure = 1;
      catalog->average[Ninstars].measureOffset = Ninstars;

      catalog->measure[Ninstars].R        = ra;
      catalog->measure[Ninstars].D        = dec;
      catalog->measure[Ninstars].M        = mag - ZeroPoint + ZeroPt;
      catalog->measure[Ninstars].t        = images[0][0].tzero;
      catalog->measure[Ninstars].dt       = images[0][0].exptime;
      catalog->measure[Ninstars].photcode = code;
      catalog->measure[Ninstars].airmass  = airmass;
      catalog->measure[Ninstars].az       = az;

      // imageID, detID ?

      Ninstars++;
      if (Ninstars == NINSTARS) {
	NINSTARS += 10000;
	REALLOCATE (catalog->measure, Measure, NINSTARS);
	REALLOCATE (catalog->average, Average, NINSTARS);
	for (i = Ninstars; i < NINSTARS; i++) {
	  dvo_measure_init (&catalog->measure[i]);
	  dvo_average_init (&catalog->average[i]);
	}
      }
    }
  }
  catalog->Nmeasure = Ninstars;
  catalog->Naverage = Ninstars;

  fprintf (stderr, "ra ranges: %f - %f, %f - %f; dec ranges: %f - %f\n", minR0, maxR0, minR1, maxR1, minD, maxD);

  images[0][0].nstar = Ninstars;
  images[0][0].imageID = 0;

  *nvalid = Nvalid + 1;

  gzclose (gz);
  free (buffer);

  return (catalog);
}

# define NBYTE_PMM_TABLE 106
# define FILE_ID_INDEX  44
# define DATE_INDEX     52
# define TIME_INDEX     62
# define RA_INDEX       68
# define DEC_INDEX      75
# define EMULSION_INDEX 83
# define FILTER_INDEX   89
# define EXPTIME_INDEX  89

// these are a guess...
# define PLATE_NX 26500
# define PLATE_NY 26500
PhotCode *LoadMetadataPMM (char *datafile, Image *image) {

  PhotCode *photcode;
  char fileID[8], date[10], timestr[6], RA[7], DEC[8], emulsion[6], filter[7], EXPTIME[4];
  char line[NBYTE_PMM_TABLE+1];
  FILE *f;

  if (!PMM_CCD_TABLE) abort ();

  strncpy_nowarn (fileID, datafile, 7);

  f = fopen (PMM_CCD_TABLE, "r");
  if (f == NULL) {
    fprintf (stderr, "unable to open PMM table: %s\n", PMM_CCD_TABLE);
    exit (2);
  }
  
  while (fread (line, 1, NBYTE_PMM_TABLE, f) == NBYTE_PMM_TABLE) {
    line[NBYTE_PMM_TABLE] = 0;
    
    if (strncmp (fileID, &line[FILE_ID_INDEX], 7)) continue;
    
    strncpy_nowarn (date, &line[DATE_INDEX], 9);
    strncpy_nowarn (timestr, &line[TIME_INDEX], 5);
    strncpy_nowarn (RA, &line[RA_INDEX], 6);
    strncpy_nowarn (DEC, &line[DEC_INDEX], 7);
    strncpy_nowarn (emulsion, &line[EMULSION_INDEX], 5);
    strncpy_nowarn (filter, &line[FILTER_INDEX], 6);
    strncpy_nowarn (EXPTIME, &line[EXPTIME_INDEX], 3);

    image[0].tzero   = pmm_date_to_sec (date, timestr);
    image[0].exptime = atof(EXPTIME)*60.0;

    photcode = pmm_get_photcode (emulsion, filter);
    image[0].photcode = photcode[0].code;

    // XXX for now, we define a totally fake coordinate system centered on the plate center
    InitCoords (&image[0].coords, "DEC--TAN");
    
    image[0].coords.crval1  = pmm_get_ra (RA);
    image[0].coords.crval2  = pmm_get_dec (DEC);

    coords_precess (&image[0].coords.crval1, &image[0].coords.crval2, 1950.0, 2000.0);

    image[0].coords.crpix1 = 0.5*PLATE_NX;
    image[0].coords.crpix2 = 0.5*PLATE_NY;
    image[0].coords.cdelt1 = image[0].coords.cdelt2 = 0.9 / 3600.0;

    image[0].NX = PLATE_NX;
    image[0].NY = PLATE_NY;

    image[0].cerror = 0.0;
 
    image[0].apmifit = 0.0;
    image[0].dapmifit = 0.0;
    image[0].detection_limit = 0.0; 
    image[0].saturation_limit = 0.0;
    image[0].fwhm_x = 0.0;
    image[0].fwhm_y = 0.0;

    // XXX need to determine long & lat for observatories
    // jd = ohana_sec_to_jd (image[0].tzero);
    // image[0].sidtime  = ohana_lst (jd, Longitude);
    // image[0].latitude = Latitude;
    // altaz (&alt, &az, 15.0*image[0].sidtime - image[0].coords.crval1, image[0].coords.crval2, Latitude);

    // secz is in units of airmass
    image[0].trate = 0.0;
    image[0].secz = 1.0;
    image[0].ccdnum = 0;

    image[0].McalPSF   = 0.0;
    image[0].McalAPER  = 0.0;
    image[0].McalChiSq = NAN;
    image[0].dMcal     = NAN;
    image[0].flags     = 0;

    image[0].nstar = 0;
  
    image[0].imageID  = 0;
    image[0].externID = 0;
    image[0].sourceID = 0;

    // save the filename
    snprintf (image[0].name, DVO_IMAGE_NAME_LEN, "%s", datafile);
    return photcode;
  }
  fprintf (stderr, "failed to match image!\n");
  abort ();
}

/* emulsion / filter combinations:
   098 RED 
   098-0 RED70 
   098-0 RG630 
   103AD MULTI 
   103AD YEL3 
   103AD YEL8 
   103AE #12 
   103AE AMB2 
   103AE AMB3 
   103AE AMB4 
   103AE AMB5 
   103AE AMB6 
   103AE AMB7 
   103AE AMB8 
   103AE NONE 
   103AE RED66 
   103AE RED67 
   103AE RED68 
   103AE RED69 
   103AE RED70 
   103AE RED71 
   103AE RED73 
   103AE RG2444 
   103AE RP2444 
   103AO NONE 
   IIIAF OG590 
   IIIAF RG600 
   IIIAF RG610 
   IIIAF RG630 
   IIIAJ GG358 
IIIAJ GG385 
IIIAJ GG395 
IVN RG715 
IVN RG9 
IVN WR88A 
*/

// date in format DDMonYYYY
// time in format HH:MM
time_t pmm_date_to_sec (char *date, char *time) {
  
  time_t second;
  double jd;
  struct tm now;
  char *p1, *p2;
  
  bzero (&now, sizeof(now));

  p1 = date;
  now.tm_mday = strtod (p1, &p2);
  assert (p2 == p1 + 2);
  
  // month runs from 0 - 11
  p1 = date + 2;
  if (!strncasecmp (p1, "JAN", 3)) { now.tm_mon =  0; goto got_month; }
  if (!strncasecmp (p1, "FEB", 3)) { now.tm_mon =  1; goto got_month; }
  if (!strncasecmp (p1, "MAR", 3)) { now.tm_mon =  2; goto got_month; }
  if (!strncasecmp (p1, "APR", 3)) { now.tm_mon =  3; goto got_month; }
  if (!strncasecmp (p1, "MAY", 3)) { now.tm_mon =  4; goto got_month; }
  if (!strncasecmp (p1, "JUN", 3)) { now.tm_mon =  5; goto got_month; }
  if (!strncasecmp (p1, "JUL", 3)) { now.tm_mon =  6; goto got_month; }
  if (!strncasecmp (p1, "AUG", 3)) { now.tm_mon =  7; goto got_month; }
  if (!strncasecmp (p1, "SEP", 3)) { now.tm_mon =  8; goto got_month; }
  if (!strncasecmp (p1, "OCT", 3)) { now.tm_mon =  9; goto got_month; }
  if (!strncasecmp (p1, "NOV", 3)) { now.tm_mon = 10; goto got_month; }
  if (!strncasecmp (p1, "DEC", 3)) { now.tm_mon = 11; goto got_month; }
  fprintf (stderr, "error interpretting month: %s\n", date);
  abort ();

got_month:
  p1 = date + 5;
  now.tm_year = strtod (p1, &p2);
  assert (p2 == p1 + 4);

  p1 = time;
  now.tm_hour = strtod (p1, &p2);
  assert (p2 == p1 + 2);

  p1 = time + 3;
  now.tm_min = strtod (p1, &p2);
  assert (p2 == p1 + 2);

  jd = now.tm_mday - 32075 + (int)(1461*(now.tm_year + 4800 + (int)(((now.tm_mon+1)-14)/12))/4)
    + (int)(367*((now.tm_mon+1) - 2 - (int)(((now.tm_mon+1) - 14)/12)*12)/12)
    - (int)(3*(int)((1900 + now.tm_year + 4900 + (int)(((now.tm_mon+1) - 14)/12))/100)/4) - 0.5;
  
  second = (jd - 2440587.5)*86400 + 3600.0*now.tm_hour + now.tm_min*60.0 + now.tm_sec;

  return (second);
}

// RA in format HHMMSS
double pmm_get_ra (char *RA) {
  
  char tmp[3];
  double h, m, s, ra;
  
  strncpy_nowarn (tmp, &RA[0], 2); h = atof (tmp);
  strncpy_nowarn (tmp, &RA[2], 2); m = atof (tmp);
  strncpy_nowarn (tmp, &RA[4], 2); s = atof (tmp);

  ra = 15.0 * (h + m / 60.0 + s / 3600.0);
  return (ra);
}

// DEC in format sDDMMSS
double pmm_get_dec (char *DEC) {

  char tmp[3];
  double d, m, s, dec;

  strncpy_nowarn (tmp, &DEC[1], 2); d = atof (tmp);
  strncpy_nowarn (tmp, &DEC[3], 2); m = atof (tmp);
  strncpy_nowarn (tmp, &DEC[5], 2); s = atof (tmp);

  dec = d + m / 60.0 + s / 3600.0;

  if (tmp[0] == '-') dec *= -1.0;
  return (dec);
}

PhotCode *pmm_get_photcode (char *emulsion, char *filter) {

  PhotCode *photcode;
  char codename[32];

  /* emulsion / filter combinations */
  if (!strcmp(emulsion, "098  ") && !strcmp(filter, "RED   ")) { strcpy (codename, "USNO.098.RED");      goto got_photcode; }
  if (!strcmp(emulsion, "098-0") && !strcmp(filter, "RED70 ")) { strcpy (codename, "USNO.098-0.RED70");  goto got_photcode; }
  if (!strcmp(emulsion, "098-0") && !strcmp(filter, "RG630 ")) { strcpy (codename, "USNO.098-0.RG630");  goto got_photcode; }
  if (!strcmp(emulsion, "103AD") && !strcmp(filter, "MULTI ")) { strcpy (codename, "USNO.103AD.MULTI");  goto got_photcode; }
  if (!strcmp(emulsion, "103AD") && !strcmp(filter, "YEL3  ")) { strcpy (codename, "USNO.103AD.YEL3");   goto got_photcode; }
  if (!strcmp(emulsion, "103AD") && !strcmp(filter, "YEL8  ")) { strcpy (codename, "USNO.103AD.YEL8");   goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "#12   ")) { strcpy (codename, "USNO.103AE.#12");    goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "AMB2  ")) { strcpy (codename, "USNO.103AE.AMB2");   goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "AMB3  ")) { strcpy (codename, "USNO.103AE.AMB3");   goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "AMB4  ")) { strcpy (codename, "USNO.103AE.AMB4");   goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "AMB5  ")) { strcpy (codename, "USNO.103AE.AMB5");   goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "AMB6  ")) { strcpy (codename, "USNO.103AE.AMB6");   goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "AMB7  ")) { strcpy (codename, "USNO.103AE.AMB7");   goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "AMB8  ")) { strcpy (codename, "USNO.103AE.AMB8");   goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "NONE  ")) { strcpy (codename, "USNO.103AE.NONE");   goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "RED66 ")) { strcpy (codename, "USNO.103AE.RED66");  goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "RED67 ")) { strcpy (codename, "USNO.103AE.RED67");  goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "RED68 ")) { strcpy (codename, "USNO.103AE.RED68");  goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "RED69 ")) { strcpy (codename, "USNO.103AE.RED69");  goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "RED70 ")) { strcpy (codename, "USNO.103AE.RED70");  goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "RED71 ")) { strcpy (codename, "USNO.103AE.RED71");  goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "RED73 ")) { strcpy (codename, "USNO.103AE.RED73");  goto got_photcode; }
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "RG2444")) { strcpy (codename, "USNO.103AE.RG2444"); goto got_photcode; } 
  if (!strcmp(emulsion, "103AE") && !strcmp(filter, "RP2444")) { strcpy (codename, "USNO.103AE.RP2444"); goto got_photcode; } 
  if (!strcmp(emulsion, "103AO") && !strcmp(filter, "NONE  ")) { strcpy (codename, "USNO.103AO.NONE");   goto got_photcode; }
  if (!strcmp(emulsion, "IIIAF") && !strcmp(filter, "OG590 ")) { strcpy (codename, "USNO.IIIAF.OG590");  goto got_photcode; }
  if (!strcmp(emulsion, "IIIAF") && !strcmp(filter, "RG600 ")) { strcpy (codename, "USNO.IIIAF.RG600");  goto got_photcode; }
  if (!strcmp(emulsion, "IIIAF") && !strcmp(filter, "RG610 ")) { strcpy (codename, "USNO.IIIAF.RG610");  goto got_photcode; }
  if (!strcmp(emulsion, "IIIAF") && !strcmp(filter, "RG630 ")) { strcpy (codename, "USNO.IIIAF.RG630");  goto got_photcode; }
  if (!strcmp(emulsion, "IIIAJ") && !strcmp(filter, "GG358 ")) { strcpy (codename, "USNO.IIIAJ.GG358");  goto got_photcode; }
  if (!strcmp(emulsion, "IIIAJ") && !strcmp(filter, "GG385 ")) { strcpy (codename, "USNO.IIIAJ.GG385");  goto got_photcode; }
  if (!strcmp(emulsion, "IIIAJ") && !strcmp(filter, "GG395 ")) { strcpy (codename, "USNO.IIIAJ.GG395");  goto got_photcode; }
  if (!strcmp(emulsion, "IVN  ") && !strcmp(filter, "RG715 ")) { strcpy (codename, "USNO.IVN.RG715");    goto got_photcode; }
  if (!strcmp(emulsion, "IVN  ") && !strcmp(filter, "RG9   ")) { strcpy (codename, "USNO.IVN.RG9");      goto got_photcode; }
  if (!strcmp(emulsion, "IVN  ") && !strcmp(filter, "WR88A ")) { strcpy (codename, "USNO.IVN.WR88A");    goto got_photcode; }
  fprintf (stderr, "error interpretting emulsion and filter: %s, %s\n", emulsion, filter);
  abort ();

got_photcode:
  photcode = GetPhotcodebyName (codename);
  if (photcode == NULL) {
    fprintf (stderr, "unknown photcode %s\n", codename);
    abort ();
  }    

  return photcode;
}
