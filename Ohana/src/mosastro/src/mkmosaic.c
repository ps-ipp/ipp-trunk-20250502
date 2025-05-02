# include "mosastro.h"

Header *mkmosaic (int Nx, int Ny, int Nstars, Coords *coords) {

  int Nastro;
  double Xmin, Xmax, Ymin, Ymax, tmp;
  double DL, DM, Cerror, Cprecise;
  Header *header;
  char line[80];

  /* mosaic header has info to define the outline of the mosaic */
  RD_to_XY (&Xmin, &Ymin, field.Rmin, field.Dmin, coords);
  RD_to_XY (&Xmax, &Ymax, field.Rmax, field.Dmax, coords);

  ALLOCATE (header, Header, 1);

  gfits_init_header (header);

  header[0].bitpix = -32;
  header[0].Naxes = 2;
  header[0].Naxis[0] = Xmax - Xmin;
  header[0].Naxis[1] = Ymax - Ymin;

  gfits_create_header (header);

  /* calculate image-wide astrometry scatter */
  Cerror   = GetScatter (&Nastro, &DL, &DM, FALSE);
  Cprecise = Cerror / sqrt (Nastro);
  gfits_modify (header, "CERROR",   "%lf", 1, Cerror);
  gfits_modify (header, "CERR_L",   "%lf", 1, DL*3600.0 * field.project.cdelt1);
  gfits_modify (header, "CERR_M",   "%lf", 1, DM*3600.0 * field.project.cdelt1);

  gfits_modify (header, "CPRECISE", "%lf", 1, Cprecise);
  gfits_modify (header, "NSTARS",   "%d",  1, 0);  /*** modify addstar to allow NSTARS = 0 ***/
  gfits_modify (header, "NASTRO",   "%d",  1, Nastro);

  GetScatter (&Nastro, &DL, &DM, TRUE);
  gfits_modify (header, "CERR_LB",  "%lf", 1, DL*3600.0 * field.project.cdelt1);
  gfits_modify (header, "CERR_MB",  "%lf", 1, DM*3600.0 * field.project.cdelt1);
  gfits_modify (header, "NBRIGHT",  "%d",  1, Nastro);

  fprintf (stderr, "bright: %f %f %d\n", DL*3600.0 * field.project.cdelt1, DM*3600.0 * field.project.cdelt1, Nastro);

  /* make a better selection for the mosaic photcode? */
  gfits_scan (&chip[0].header, "PHOTCODE", "%s", 1, line);
  gfits_modify (header, "PHOTCODE", "%s", 1, line);

  gfits_scan (&chip[0].header, "ZERO_PT", "%lf", 1, &tmp);
  gfits_modify (header, "ZERO_PT", "%lf", 1, tmp);

  gfits_scan (&chip[0].header, ExptimeKeyword,  "%lf", 1, &tmp);
  gfits_modify (header, ExptimeKeyword,  "%lf", 1, tmp);

  /* insert time data from chip[0] */
  /* try JD first */
  if (strcasecmp (JDKeyword, "NONE")) {
    uppercase (JDKeyword);
    gfits_scan (&chip[0].header, JDKeyword, "%lf", 1, &tmp);
    gfits_modify (header, JDKeyword, "%lf", 1, tmp);
    goto got_time;
  }

  /* try MJD next */
  if (strcasecmp (MJDKeyword, "NONE")) {
    uppercase (MJDKeyword);
    gfits_scan (&chip[0].header, MJDKeyword, "%lf", 1, &tmp);
    gfits_modify (header, MJDKeyword, "%lf", 1, tmp);
    goto got_time;
  }
    
  /* get UT and DATE */
  if (strcasecmp (UTKeyword, "NONE") && strcasecmp (DateKeyword, "NONE")) {
    uppercase (UTKeyword);
    gfits_scan (&chip[0].header, UTKeyword, "%s", 1, line);
    gfits_modify (header, UTKeyword, "%s", 1, line);
    uppercase (DateKeyword);
    gfits_scan (&chip[0].header, DateKeyword, "%s",  1, line);
    gfits_modify (header, DateKeyword, "%s", 1, line);
    goto got_time;
  }
  fprintf (stderr, "ERROR: missing time abstraction in config\n");
  exit (1);

got_time:

  PutCoords (coords, header);
  return (header);
}
