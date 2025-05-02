# include "addstar.h"
# include "2mass.h"

static int FilterSkip;
static int TimeSkip;
static int Qentry;
static int Photcode;

int get2mass_setup (int photcode) {

  NAMED_PHOTCODE (TM_J, "2MASS_J");
  NAMED_PHOTCODE (TM_H, "2MASS_H");
  NAMED_PHOTCODE (TM_K, "2MASS_K");

  NAMED_PHOTCODE (TM_B, "TYCHO_B_2MASS");
  NAMED_PHOTCODE (TM_V, "TYCHO_V_2MASS");

  FilterSkip = TimeSkip = Qentry = 0;
  Photcode = photcode;
  if (photcode == -1) return TRUE;

  if (photcode == TM_J) {
      FilterSkip = 6;
      TimeSkip = 28;
      Qentry   = 0;
  }
  if (photcode == TM_H) {
      FilterSkip = 10;
      TimeSkip = 24;
      Qentry   = 1;
  }
  if (photcode == TM_K) {
      FilterSkip = 14;
      TimeSkip = 20;
      Qentry   = 2;
  }
  if (!FilterSkip) Shutdown ("invalid photcode %s", GetPhotcodeNamebyCode(photcode));
  return TRUE;
}

# if (0)
// fill in the data for a single star.  takes a pointer to the start of the line
int get2mass_star (Stars *star, char *line, int Nmax) {

  char *ptr, qc;
  double M, dM;
  e_time time;

  InitStar (star);

  ptr = skipNbounds (line, '|', FilterSkip, Nmax);
  if (ptr == NULL) Shutdown ("format error in 2mass");
  M  = strtod (ptr, NULL);
  ptr = skipNbounds (ptr, '|', 1, Nmax - (ptr - line));
  dM = strtod (ptr, NULL);
  time = get2mass_time (ptr, TimeSkip, Nmax - (ptr - line));

  /* filter on the ph_qual flag for this filter (field 19) */
  if (SELECT_2MASS_QUALITY != NULL) {
    ptr = skipNbounds (line, '|', 18, Nmax);
    qc  = ptr[Qentry];
    if (strchr (SELECT_2MASS_QUALITY, qc) == NULL) return (FALSE);
  }

  star[0].measure.M        = M;
  star[0].measure.dM       = dM;
  star[0].measure.photcode = Photcode;
  star[0].measure.t        = time;
  star[0].measure.detID    = 0;
  star[0].measure.imageID  = 0;

  return TRUE;
}

// fill in the data for a JHK triplet star.  takes a pointer to the start of the line
int get2mass_3star (Stars *star, char *line, int Nmax) {

  char *ptr;
  double J, dJ, H, dH, K, dK;
  e_time time;

  ptr = line;
  if (ptr == NULL) Shutdown ("format error in 2mass");

  ptr = skipNbounds (ptr, '|', 6, Nmax - (ptr - line));
  J  = strtod (ptr, NULL);
  ptr = skipNbounds (ptr, '|', 1, Nmax - (ptr - line));
  dJ = strtod (ptr, NULL);

  ptr = skipNbounds (ptr, '|', 3, Nmax - (ptr - line));
  H  = strtod (ptr, NULL);
  ptr = skipNbounds (ptr, '|', 1, Nmax - (ptr - line));
  dH = strtod (ptr, NULL);

  ptr = skipNbounds (ptr, '|', 3, Nmax - (ptr - line));
  K  = strtod (ptr, NULL);
  ptr = skipNbounds (ptr, '|', 1, Nmax - (ptr - line));
  dK = strtod (ptr, NULL);

  // get the time
  time = get2mass_time (ptr, 20, Nmax - (ptr - line));

# if (0)
  char Jquality, Hquality, Kquality;
  /* old code to filter on the ph_qual flag for this filter (field 19) */
  if (SELECT_2MASS_QUALITY != NULL) {
    ptr = skipNbounds (ptr, '|', 3, Nmax - (ptr - line));
    Jquality = (strchr (SELECT_2MASS_QUALITY, ptr[0]) != NULL);
    Hquality = (strchr (SELECT_2MASS_QUALITY, ptr[1]) != NULL);
    Kquality = (strchr (SELECT_2MASS_QUALITY, ptr[2]) != NULL);
    time = get2mass_time (ptr, 18, Nmax - (ptr - line));
  } else {
    time = get2mass_time (ptr, 20, Nmax - (ptr - line));
  }
# endif

  // how many bits are being used for the 2mass flags; can we just set photFlags based on them?

  star[0].measure.M         = J;
  star[0].measure.dM        = dJ;
  star[0].measure.photcode  = TM_J;
  star[0].measure.t         = time;
  star[0].measure.detID     = 0;
  star[0].measure.imageID   = 0;

  star[1].measure.M         = H;
  star[1].measure.dM        = dH;
  star[1].measure.photcode  = TM_H;
  star[1].measure.t         = time;
  star[1].measure.detID     = 0;
  star[1].measure.imageID   = 0;

  star[2].measure.M         = K;
  star[2].measure.dM        = dK;
  star[2].measure.photcode  = TM_K;
  star[2].measure.t         = time;
  star[2].measure.detID     = 0;
  star[2].measure.imageID   = 0;

  return TRUE;
}
# endif

// fill in the coords for a single star.  takes a pointer to the start of the line
int get2mass_coords (char *line, double *R, double *D, int Nmax) {

  char *ptr;

  *R = strtod (line, NULL);
  ptr = skipNbounds (line, '|', 1, Nmax);
  *D = strtod (ptr, NULL);
  if (*D > 90) Shutdown ("weird DEC value: something is wrong");

  return TRUE;
}

// this function retrieves the time from the DATE field
e_time get2mass_date (char *ptr, int Nbound, int Nmax) {

  e_time time;
  char *p, *end;

  p = skipNbounds (ptr, '|', Nbound, Nmax);
  if (p == NULL) Shutdown ("format error in 2mass");
  end = memchr (p, '|', Nmax - (p - ptr));
  if (end == NULL) Shutdown ("format error in 2mass");
  *end = 0;
  time = ohana_date_to_sec (ptr);
  *end = '|';

  return (time);
}

// this function retrieves the time from the JDATE field (%12.4f)
e_time get2mass_time (char *ptr, int Nbound, int Nmax) {

  e_time time;
  double jd;
  char *p, *end;

  p = skipNbounds (ptr, '|', Nbound, Nmax);
  if (p == NULL) Shutdown ("format error in 2mass");
  end = memchr (p, '|', Nmax - (p - ptr));
  if (end == NULL) Shutdown ("format error in 2mass");
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
  
/* watch for patches which cross 0,360 boundary */
SkyTable *load2mass_acc (char *path, char *accel) {

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
    regions[Nregions].childE = Nrec; // a cheat since 2MASS only has one depth

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

int get2mass_sortStars (TMStars *tstars, int Ntstars) {

# define SWAPFUNC(A,B){ TMStars temp = tstars[A]; tstars[A] = tstars[B]; tstars[B] = temp; }
# define COMPARE(A,B)(tstars[A].R < tstars[B].R)

  OHANA_SORT (Ntstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}
