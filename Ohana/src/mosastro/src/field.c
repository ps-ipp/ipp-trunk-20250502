# include "mosastro.h"

/* determine an initial guess to field parameters from data */ 
int init_field () {

  int i;

  if (FIELD != (char *) NULL) {
    load_field (FIELD);
    return (TRUE);
  }

  /* bore site center guess */
  InitCoords (&field.project, "DEC--TAN");

  # if (PSASTRO_MODE)
  // XXX : temporarily use a fixed ref point to compare with psastro
  field.project.crval1 = chip[0].coords.crval1;
  field.project.crval2 = chip[0].coords.crval2;
  # else
  field.project.crval1 = 0.5*(field.Rmin + field.Rmax);
  field.project.crval2 = 0.5*(field.Dmin + field.Dmax);
  # endif

  /* measure average plate scale - would be better using parabolic min... */
  field.project.cdelt1 = 0;
  field.project.cdelt2 = 0;
  for (i = 0; i < Nchip; i++) {
    field.project.cdelt1 += chip[i].coords.cdelt1;
    field.project.cdelt2 += chip[i].coords.cdelt2;
  }
  field.project.cdelt1 /= Nchip;
  field.project.cdelt2 /= Nchip;

  # if (PSASTRO_MODE)
  // XXX : temporarily use first chip as ref scale to compare with psastro
  field.project.cdelt1 = chip[0].coords.cdelt1;
  field.project.cdelt2 = chip[0].coords.cdelt2;
  # endif
  /* force starting guess to have equal x & y plate scales? */

  fprintf (stderr, "field: %f,%f  %f,%f\n", 
	   field.project.crval1, field.project.crval2, 
	   3600*field.project.cdelt1, 3600*field.project.cdelt2);

  /* bore site center guess */
  InitCoords (&field.distort, "DEC--PLY");
  return (TRUE);
}

void field_stats () {
  
  int i, j;
  double Rmin, Rmax, Dmin, Dmax;

  /* find range for single image */
  Rmin = Dmin = 360.0;
  Rmax = Dmax = -90.0;

  for (i = 0; i < Nchip; i++) {
    for (j = 0; j < chip[i].Nstars; j++) {
      Rmin = MIN (Rmin, chip[i].stars[j].R);
      Dmin = MIN (Dmin, chip[i].stars[j].D);
      Rmax = MAX (Rmax, chip[i].stars[j].R);
      Dmax = MAX (Dmax, chip[i].stars[j].D);
    }
  }
  field.Rmin = Rmin;
  field.Rmax = Rmax;
  field.Dmin = Dmin;
  field.Dmax = Dmax;

}

void field_combine () {

  int i;
  double cd1, cd2, pc11, pc12, pc21, pc22;

  /* combine boresite & distortion parameters: ctype DIS */
  strcpy (field.project.ctype, "DEC--DIS");

  cd1  = field.project.cdelt1;
  cd2  = field.project.cdelt2;
  pc11 = field.project.pc1_1;
  pc12 = field.project.pc1_2;
  pc21 = field.project.pc2_1;
  pc22 = field.project.pc2_2;

  field.project.Npolyterms = field.distort.Npolyterms;

  for (i = 0; i < 7; i++) {
    field.project.polyterms[i][0] = (pc11*cd1*field.distort.polyterms[i][0] + pc12*cd2*field.distort.polyterms[i][1]);
    field.project.polyterms[i][1] = (pc21*cd1*field.distort.polyterms[i][0] + pc22*cd2*field.distort.polyterms[i][1]);
  }
  for (i = 0; i < 2; i++) {
    field.project.polyterms[0][i] =  field.project.polyterms[0][i] / (cd1*cd1);
    field.project.polyterms[1][i] =  field.project.polyterms[1][i] / (cd1*cd2);
    field.project.polyterms[2][i] =  field.project.polyterms[2][i] / (cd2*cd2);

    field.project.polyterms[3][i] =  field.project.polyterms[3][i] / (cd1*cd1*cd1);
    field.project.polyterms[4][i] =  field.project.polyterms[4][i] / (cd1*cd1*cd2);
    field.project.polyterms[5][i] =  field.project.polyterms[5][i] / (cd1*cd2*cd2);
    field.project.polyterms[6][i] =  field.project.polyterms[6][i] / (cd2*cd2*cd2);
  }
}

int load_field (char *filename) {

  Coords coords;
  Header header;

  /* load header */
  if (!gfits_read_header (filename, &header)) {
    fprintf (stderr, "ERROR: can't read header for %s\n", filename);
    exit (1);
  }
  /* get astrometry information */
  if (!GetCoords (&coords, &header)) {
    fprintf (stderr, "ERROR: no astrometric solution in field %s\n", filename);
    exit (1);
  }
 
  /* separate field into boresite + distortion terms */

  /* bore site center guess */
  InitCoords (&field.project, "DEC--TAN");
  field.project.crval1 = 0.5*(field.Rmin + field.Rmax);
  field.project.crval2 = 0.5*(field.Dmin + field.Dmax);
  
  /* measure average plate scale - would be better using parabolic min... */
  field.project.cdelt1 = coords.cdelt1;
  field.project.cdelt2 = coords.cdelt2;

  /** allow guess at field rotation?? **/
  field.project.pc1_1  = coords.pc1_1;
  field.project.pc2_2  = coords.pc2_2;
  field.project.pc1_2  = coords.pc1_2;
  field.project.pc2_1  = coords.pc2_1;
  field.project.Npolyterms = 1;

  /* bore site center guess */
  InitCoords (&field.distort, "DEC--PLY");

  return (TRUE);
}

