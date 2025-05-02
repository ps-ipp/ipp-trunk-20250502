# include "dvoImageOverlaps.h"
# ifndef FLT_MAX
# define FLT_MAX 1e32
# endif

/* given image, find catalog images which overlap it */
off_t *MatchImage (Image *dbImages, off_t NdbImages, Image *image, off_t *Nmatch) {
  
  off_t i, NMATCH, nmatch, *match;
  int j, N, addtolist, status;
  Coords tcoords;
  double r, d;
  double Xi[4], Yi[4], Xo[4], Yo[4];  /* image and original corners */
  double Xmin, Xmax, Ymin, Ymax;
  double xmin, xmax, ymin, ymax;

  *Nmatch = 0;

  if (!WITH_PHU && !strcmp (&image[0].coords.ctype[4], "-DIS")) return NULL;
  if ( SOLO_PHU &&  strcmp (&image[0].coords.ctype[4], "-DIS")) return NULL;

  // exclude chips / images with errors larger than OVERLAP_MAX_CERROR:
  // 2021.11.03 : MAX_CERROR is now compared to the header value CERSTD
  if (isfinite(MAX_CERROR)) {
    if (image[0].cerror * 0.02 > MAX_CERROR) return NULL;
  }

  /* project onto rectilinear grid with 1 arcsec pixels */
  /* we keep the original crpix1,2 and crref1,2 */
  /* for mosaic astrometry, the grid should be w.r.t. the tangent-plane, not chip coords */

  InitCoords (&tcoords, "DEC--TAN");
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;

  tcoords.crval1 = image[0].coords.crval1;
  tcoords.crval2 = image[0].coords.crval2;

  if (!strcmp (&image[0].coords.ctype[4], "-WRP")) {
    myAssert (image[0].coords.mosaic, "should already have tested this");
    tcoords.crval1 = image[0].coords.mosaic[0].crval1;
    tcoords.crval2 = image[0].coords.mosaic[0].crval2;
  }

  /* define original corners */
  SetImageCorners (Xo, Yo, &image[0]);
  
  Ymin = Xmin = +FLT_MAX;
  Ymax = Xmax = -FLT_MAX;
  for (j = 0; j < 4; j++) {
    /* XY-to_RD is two-level if ctype == WRP */
    XY_to_RD (&r, &d, Xo[j], Yo[j], &image[0].coords);
    RD_to_XY (&Xo[j], &Yo[j], r, d, &tcoords);
    Xmin = MIN (Xmin, Xo[j]);
    Xmax = MAX (Xmax, Xo[j]);
    Ymin = MIN (Ymin, Yo[j]);
    Ymax = MAX (Ymax, Yo[j]);
  }

  /* match represents the subset of overlapping images */
  nmatch = 0;
  NMATCH = 20;
  ALLOCATE (match, off_t, NMATCH);

  /* setup links for mosaic WRP and DIS entries */
  BuildChipMatch (dbImages, NdbImages);

  /* run through image table and search for overlaps
     also define the vtable entries for the images we keep  */

  for (i = 0; i < NdbImages; i++) {

    if (!WITH_PHU && !strcmp (&dbImages[i].coords.ctype[4], "-DIS")) continue;
    if ( SOLO_PHU &&  strcmp (&dbImages[i].coords.ctype[4], "-DIS")) continue;

    /* define image corners */
    SetImageCorners (Xi, Yi, &dbImages[i]);
    // Xi[4] = Xi[0]; Yi[4] = Yi[0];

    /* transform to tcoords, skip corners off image */
    /*** XXX this will fail for very large images which extend beyond 180deg */
    ymin = xmin = +FLT_MAX;
    ymax = xmax = -FLT_MAX;
    for (j = N = 0; j < 4; j++) {
      status = XY_to_RD (&r, &d, Xi[j], Yi[j], &dbImages[i].coords);
      if (!status) continue;
      status = RD_to_XY (&Xi[N], &Yi[N], r, d, &tcoords);
      if (!status) continue;
      xmin = MIN (xmin, Xi[N]);
      xmax = MAX (xmax, Xi[N]);
      ymin = MIN (ymin, Yi[N]);
      ymax = MAX (ymax, Yi[N]);
      N++;
    }

    /* check if one corner of dbImages[i] is inside image[0] */
    for (j = 0; j < N; j++) {
      addtolist = TRUE;
      addtolist &= (Xi[j] >= Xmin);
      addtolist &= (Xi[j] <= Xmax);
      addtolist &= (Yi[j] >= Ymin);
      addtolist &= (Yi[j] <= Ymax);
      if (addtolist) goto addtolist;
    }

    /* or else, check if one corner of image[0] is inside dbImages[i] */
    for (j = 0; j < 4; j++) {
      addtolist = TRUE;
      addtolist &= (Xo[j] >= xmin);
      addtolist &= (Xo[j] <= xmax);
      addtolist &= (Yo[j] >= ymin);
      addtolist &= (Yo[j] <= ymax);
      if (addtolist) goto addtolist;
    }

    // no match, skip this dbImage
    continue;

  addtolist:
    match[nmatch] = i;
    nmatch ++;
    if (nmatch == NMATCH) {
      NMATCH += 20;
      REALLOCATE (match, off_t, NMATCH);
    }
  }
  
  if (VERBOSE) fprintf (stderr, "found "OFF_T_FMT" overlapping images\n",  nmatch);

  *Nmatch = nmatch;
  return (match);
}
  
void SetImageCorners (double *X, double *Y, Image *image) {

  if (!strcmp(&image[0].coords.ctype[4], "-DIS")) {
    X[0] = -0.5*image[0].NX; Y[0] = -0.5*image[0].NY;
    X[1] = +0.5*image[0].NX; Y[1] = -0.5*image[0].NY;
    X[2] = +0.5*image[0].NX; Y[2] = +0.5*image[0].NY;
    X[3] = -0.5*image[0].NX; Y[3] = +0.5*image[0].NY;
  } else {
    X[0] = 0;           Y[0] = 0;
    X[1] = image[0].NX; Y[1] = 0;
    X[2] = image[0].NX; Y[2] = image[0].NY;
    X[3] = 0;           Y[3] = image[0].NY;
  }
}
