# include "gastro2.h"

void gproject (CmpCatalog *Target, RefCatalog *Ref, RefCatalog *Subset) {

  int i, N;
  double X, Y, P, Q, M, Moff;
  double XMIN, XMAX, YMIN, YMAX, MMIN, MMAX;
  Coords TPtoSky, FPtoTP, *coords;
  StarData *in, *out;

  Subset[0] = Ref[0];

  ALLOCATE (Subset[0].stars, StarData, MAX (1, Ref[0].N));

  in     = Ref[0].stars;
  out    = Subset[0].stars;

  coords = &Target[0].coords;

  /* create Tangent Plane to Sky transformation from input coords */
  InitCoords (&TPtoSky, coords[0].ctype);
  TPtoSky.crval1 = coords[0].crval1;
  TPtoSky.crval2 = coords[0].crval2;

  /* create Focal Plane to Tangent Plane transformation from input coords */
  InitCoords (&FPtoTP, "FP---PLY");
  FPtoTP.cdelt1 = coords[0].cdelt1;
  FPtoTP.cdelt2 = coords[0].cdelt2;
  FPtoTP.crpix1 = coords[0].crpix1;
  FPtoTP.crpix2 = coords[0].crpix2;
  FPtoTP.pc1_1  = coords[0].pc1_1;
  FPtoTP.pc1_2  = coords[0].pc1_2;
  FPtoTP.pc2_1  = coords[0].pc2_1;
  FPtoTP.pc2_2  = coords[0].pc2_2;

  FPtoTP.Npolyterms = coords[0].Npolyterms;
  for (i = 0; i < 7; i++) {
    FPtoTP.polyterms[i][0] = coords[0].polyterms[i][0];
    FPtoTP.polyterms[i][1] = coords[0].polyterms[i][1];
  }

  Moff = Ref[0].Moff;

  XMIN = -0.5*NFIELD*Target[0].header.Naxis[0];
  XMAX =  0.5*NFIELD*Target[0].header.Naxis[0] + Target[0].header.Naxis[0];
  YMIN = -0.5*NFIELD*Target[0].header.Naxis[1];
  YMAX =  0.5*NFIELD*Target[0].header.Naxis[1] + Target[0].header.Naxis[1];
 
  /* need to allow some leeway? use a fixed +/- 0.5 mag for now */
  MMIN = MMAX = 0;
  if (MAGLIMS && !MAGMANUAL) {
      MMAX = Target[0].lum.Mmax + 1.0;
      MMIN = Target[0].lum.Mmin - 1.0;

      if (MMAX < Ref[0].lum.Mmin + Moff) 
	  fprintf (stderr, "warning: reference catalog probably too faint:  %5.3f < %5.3f\n", Target[0].lum.Mmax, Ref[0].lum.Mmin + Moff);

      if (MMIN > Ref[0].lum.Mmax + Moff) 
	  fprintf (stderr, "warning: reference catalog probably too bright: %5.3f > %5.3f\n", Target[0].lum.Mmin, Ref[0].lum.Mmax + Moff);
  }
  if (MAGMANUAL) {
      MMIN = MAGLIM_MIN;
      MMAX = MAGLIM_MAX;
      Moff = 0;
  }

  if (VERBOSE) fprintf (stderr, "limited reference stars to mag range %f - %f\n", MMIN - Moff, MMAX - Moff);

  for (N = i = 0; i < Ref[0].N; i++) {
    RD_to_XY (&X, &Y, in[i].R, in[i].D, coords);
    M = in[i].M + Moff;

    if (X < XMIN) continue;
    if (X > XMAX) continue;
    if (Y < YMIN) continue;
    if (Y > YMAX) continue;

    if (MAGLIMS) {
      if (M < MMIN) continue;
      if (M > MMAX) continue;
    }

    /* get tangent-plane coordinates as well */
    RD_to_XY (&P, &Q, in[i].R, in[i].D, &TPtoSky);

    out[N] = in[i];
    out[N].X = X;
    out[N].Y = Y;
    out[N].P = P;
    out[N].Q = Q;
    out[N].M = M;
    N++;
  }

  if (N < 3) {
    fprintf (stderr, "ERROR: too few reference stars accepted\n");
    exit (1);
  }
    
  Subset[0].N = N;
  sort_stars_mag (Subset[0].stars, N);
  if (GASTRO_MAX_NSTARS && (GASTRO_MAX_NSTARS < Subset[0].N)) {
    Subset[0].N = GASTRO_MAX_NSTARS;
    REALLOCATE (Subset[0].stars, StarData, Subset[0].N);
  }
  if (VERBOSE) fprintf (stderr, "using %d stars from ref catalog\n", Subset[0].N);

  REALLOCATE (Subset[0].stars, StarData, MAX (1, Subset[0].N));

  /* get tangent-plane coords for target as well */
  for (i = 0; i < Target[0].N; i++) {
    XY_to_RD (&P, &Q, Target[0].stars[i].X, Target[0].stars[i].Y, &FPtoTP);
    Target[0].stars[i].P = P;
    Target[0].stars[i].Q = Q;
  }

}

/* in this function, we convert the Ra & Dec coords to the rough X, Y coords
   we also convert the magnitudes to the approximate system with Ref[0].Moff 
   also, reduce the domain to those within X, Y, M limits 
*/

