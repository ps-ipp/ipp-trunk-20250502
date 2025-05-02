# include "fakeastro.h"

int dump_fit_stars (Stars *stars, double *L, double *M, int Nstars, char *filename) {

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "failed to open %s\n", filename);
    return FALSE;
  }

  int i;
  for (i = 0; i < Nstars; i++) {
    fprintf (f, "%d  %f %f : %f %f\n", i, stars[i].measure.Xccd, stars[i].measure.Yccd, L[i], M[i]);
  }
  fclose (f);
  return TRUE;
}

int fit_fake_stars (Stars *stars, int Nstars, Image *image) {

  // if we are doing imagemap or high-order polynomial fits, this is the order we should
  // use
  int order_use = 0;
  if (Nstars >   5) order_use = 1; //  5 stars per cell
  if (Nstars >  24) order_use = 2; //  6 stars per cell
  if (Nstars >  63) order_use = 3; //  7 stars per cell
  // if (Nstars > 128) order_use = 4; //  8 stars per cell
  // if (Nstars > 225) order_use = 5; //  9 stars per cell
  // if (Nstars > 360) order_use = 6; // 10 stars per cell
  if (order_use < 1) {
    fprintf (stderr, "insufficient measurements (%d) for linear fit\n", Nstars);
    image[0].flags |= ID_IMAGE_ASTROM_FEW;
    return FALSE;
  }
  order_use = 1; // hard-wire linear solutions (nothing higher is needed for now)

  // I have Nstars with "reference" positions stars[].Rref,Dref and 
  // "observed" chip coordinates stars[].measure.Xccd,Yccd.

  // fit Xccd,Yccd to Rref,Dref

  int CHIPMAP = (image[0].coords.Npolyterms == -1);

  // XX1 image[0].coords.Npolyterms = CHIPMAP ? 1 : order_use;
  CoordFit *fit = CHIPMAP ? fit_init (1) : fit_init (order_use);

  double *L, *M;
  ALLOCATE (L, double, Nstars);
  ALLOCATE (M, double, Nstars);

  // static int Ntest = 0;
  // char name[64];
  // snprintf (name, 64, "test.%03d.dat", Ntest); Ntest ++;
  // FILE *f = fopen (name, "w");

  // generate the fit matches
  int i;
  for (i = 0; i < Nstars; i++) {
    RD_to_XY (&L[i], &M[i], stars[i].Rref, stars[i].Dref, image[0].coords.mosaic);
    fit_add (fit, stars[i].measure.Xccd, stars[i].measure.Yccd, L[i], M[i], 0.05);

    // double Lo, Mo, Xo, Yo;
    // RD_to_XY (&Lo, &Mo, stars[i].average.R, stars[i].average.D, image[0].coords.mosaic);
    // RD_to_XY (&Xo, &Yo, stars[i].average.R, stars[i].average.D, &image[0].coords);
    // fprintf (f, "%d : %f %f : %f %f : %f %f : %f %f : %f %f : %f %f\n", i, stars[i].Rref, stars[i].Dref, stars[i].average.R, stars[i].average.D, stars[i].measure.Xccd, stars[i].measure.Yccd, L[i], M[i], Lo, Mo, Xo, Yo);
  }
  // fclose (f);

  // measure the fit, update the coords & object coordinates
  // XXX do something more drastic on failure?
  if (!fit_eval (fit)) {
    fprintf (stderr, "failed to fit new model\n");
    image[0].flags |= ID_IMAGE_ASTROM_FAIL;
    if (fit) fit_free (fit);
    free (L);
    free (M);
    return FALSE;
  }

  image[0].coords.Npolyterms = CHIPMAP ? 1 : order_use;
  if (!fit_apply_coords (fit, &image[0].coords, FALSE)) {
    fprintf (stderr, "failed to fit new model\n");
    image[0].flags |= ID_IMAGE_ASTROM_FAIL;
    if (fit) fit_free (fit);
    free (L);
    free (M);
    return FALSE;
  }

  // measure chip corners and find dX,dY
  {
    double Lo, Mo;
    XY_to_LM (&Lo, &Mo, 0.0, 0.0, &image[0].coords);
    
    double Ls, Ms;
    XY_to_LM (&Ls, &Ms, 4850.0, 0.0, &image[0].coords);

    double Le, Me;
    XY_to_LM (&Le, &Me, 0.0, 4850.0, &image[0].coords);

    if (FALSE) fprintf (stderr, "%s : (dL,dM)_1 : %f : (dL,dM)_2 : %f\n", image->name, hypot(Le-Lo, Me-Mo), hypot(Ls-Lo, Ms-Mo));
  }

  if (CHIPMAP) {

    // apply fit to get the fitted X,Y coordinates.  we need these to fit the residual map below
    float *dX, *dY, *Xref, *Yref;
    ALLOCATE (dX,   float, Nstars);
    ALLOCATE (dY,   float, Nstars);
    ALLOCATE (Xref, float, Nstars);
    ALLOCATE (Yref, float, Nstars);

    for (i = 0; i < Nstars; i++) {
      // we have not yet fitted the map, so Npolyterms needs to be 1 here:
      double tmpX, tmpY;
      LM_to_XY (&tmpX, &tmpY, L[i], M[i], &image[0].coords);

      // fit the linear terms as above
      // calculate dX (raw.X - ref.X) and dY 
      Xref[i] = tmpX;
      Yref[i] = tmpY;
      dX[i] = Xref[i] - stars[i].measure.Xccd;
      dY[i] = Yref[i] - stars[i].measure.Yccd;
    }

    // in coordsops.c:XY_to_LM, the map is defined to carry dX,dY so that:
    // (L,M) = f(X',Y') : (X',Y') = (X,Y) + (dX,dY)

    if (image[0].coords.offsetMap == NULL) {
      // allocate a new table and assign to this image
      // need to allocate table?
      AstromOffsetTable *table = get_astrom_table ();
      AstromOffsetTableNewMap(table, order_use, order_use, image);
    }
    AstromOffsetMapFit (image[0].coords.offsetMap, Xref, Yref, dX, NULL, Nstars, TRUE);
    AstromOffsetMapFit (image[0].coords.offsetMap, Xref, Yref, dY, NULL, Nstars, FALSE);

    image[0].coords.Npolyterms = -1;

    free (dX);
    free (dY);
    free (Xref);
    free (Yref);
  }

  for (i = 0; i < Nstars; i++) {
    // if we have fitted the map above, Npolyterms needs to be -1 here:
    XY_to_RD (&stars[i].average.R, &stars[i].average.D, stars[i].measure.Xccd, stars[i].measure.Yccd, &image[0].coords);
    stars[i].average.R = ohana_normalize_angle (stars[i].average.R);
  }

  free (L);
  free (M);

  return TRUE;
}
