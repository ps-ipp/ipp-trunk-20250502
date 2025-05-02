# include "addstar.h"
# include "WISE.h"

int getWISE_setup () {

  NAMED_PHOTCODE (WISE_W1, "WISE_W1");
  NAMED_PHOTCODE (WISE_W2, "WISE_W2");
  NAMED_PHOTCODE (WISE_W3, "WISE_W3");
  NAMED_PHOTCODE (WISE_W4, "WISE_W4");

  return TRUE;
}

// fill in the coords for a single star.  takes a pointer to the start of the line
int getWISE_coords (char *line, double *R, double *D, int Nmax) {

  char *ptr = line;

  if (MODE == MODE_CATWISE) {
      // copy the string ranges and then use strtod (not necessary?)
      // *R = strtod (&ptr[47], NULL); // safe to use strtod on the line because it is white-space separated
      // *D = strtod (&ptr[59], NULL);
      // note we use ra_pm and dec_pm which are different from ra,dec when the object is moving
      *R = strtod (&ptr[1139], NULL); // safe to use strtod on the line because it is white-space separated
      *D = strtod (&ptr[1151], NULL);
  } else {
      ptr = skipNbounds (ptr, '|', 1, Nmax);
      *R = strtod (ptr, NULL);
      ptr = skipNbounds (ptr, '|', 1, Nmax);
      *D = strtod (ptr, NULL);
  }
  if (*D > 90) Shutdown ("weird DEC value: something is wrong");

  return TRUE;
}

int getWISE_sortStars (WISE_Stars *tstars, int Ntstars) {

# define SWAPFUNC(A,B){ WISE_Stars temp = tstars[A]; tstars[A] = tstars[B]; tstars[B] = temp; }
# define COMPARE(A,B)(tstars[A].R < tstars[B].R)

  OHANA_SORT (Ntstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}

// this function retrieves the time from the DATE field
e_time getWISE_date (char *ptr, int Nbound, int Nmax) {

  e_time time;
  char *p, *end;

  p = skipNbounds (ptr, '|', Nbound, Nmax);
  if (p == NULL) Shutdown ("format error in WISE");
  end = memchr (p, '|', Nmax - (p - ptr));
  if (end == NULL) Shutdown ("format error in WISE");
  *end = 0;
  time = ohana_date_to_sec (ptr);
  *end = '|';

  return (time);
}

// this function retrieves the time from the JDATE field (%12.4f)
e_time getWISE_time (char *ptr, int Nbound, int Nmax) {

  e_time time;
  double jd;
  char *p, *end;

  p = skipNbounds (ptr, '|', Nbound, Nmax);
  if (p == NULL) Shutdown ("format error in WISE");
  end = memchr (p, '|', Nmax - (p - ptr));
  if (end == NULL) Shutdown ("format error in WISE");
  *end = 0;
  jd = strtod (p, NULL);
  time = ohana_jd_to_sec (jd);
  *end = '|';

  return (time);
}

/* return a pointer to the first char after Nbound of value bound */
char *skipNbounds (char *line, char bound, int Nbound, int Nbyte) {

  int i;
  char *p, *q;

  p = line;
  for (i = 0; i < Nbound; i++) {
    q = memchr (p, bound, Nbyte - (p - line));
    if (q == NULL) return (NULL);
    p = q + 1;
    if (p - line == Nbyte) return (NULL);
  }
  return (p);
}
  
/* return a pointer to the first char after Nbound of value bound */
char *getLineSegment (char *line, int start, int end) {

  // do NOT damage the buffer by seting the end byte to NULL
  if (0) { line[end] = 0; }
  char *ptr = &line[start];
  return (ptr);
}
  
double getDoubleRAW (char *line, int start, int end) {
    char *ptr = getLineSegment(line, start, end);
    double value = strtod (ptr, NULL);
    return value;
}

double getDoubleNAN (char *line, int start, int end) {
    char *endpoint = NULL;
    char *ptr = getLineSegment(line, start, end);
    double value = strtod (ptr, &endpoint);
    if (endpoint == ptr) { value = NAN; }
    return value;
}

/* watch for patches which cross 0,360 boundary */
SkyTable *loadWISE_acc (char *path, char *accel) {

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

    // don't restrict by RA, but limit by DEC
    if (De < UserPatch.Dmin) continue;
    if (Ds > UserPatch.Dmax) continue;

    regions[Nregions].Rmin = Rs;
    regions[Nregions].Rmax = Re;
    regions[Nregions].Dmin = Ds;
    regions[Nregions].Dmax = De;
    regions[Nregions].childE = Nrec; // a cheat since WISE only has one depth

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

