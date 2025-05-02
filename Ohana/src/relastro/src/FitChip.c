# include "relastro.h"
# include "relastroVisual.h"

int FitChip (StarData *raw, StarData *ref, int Nmatch, Image *image) {

  int i, NstatFull, Nstat, Niter, Nkeep;
  float dLsig, dMsig, dRsig;
  float dLsigFull, dMsigFull, dRsigFull;
  float dL, dM, dR, dRmax;

  CoordFit *fit = NULL;

  dRmax = 0.0;

  for (Niter = 0; Niter < IMFIT_CLIP_NITER; Niter ++) {

    // measure the scatter for the unmarked (good) measurements with dM < limit
    // SIGMA_LIM here is redundant with ImageOps.c:915
    GetScatterRawRef(&dLsig, &dMsig, &dRsig, &Nstat, raw, ref, Nmatch, SIGMA_LIM);
    dRmax = IMFIT_CLIP_NSIGMA*dRsig;

    // (a) skip unfittable points
    // (b) mark good and bad points for fit (in and outliers)

    // measure the scatter distribution (use only the bright end detections)
    Nkeep = 0;
    for (i = 0; i < Nmatch; i++) {
      if (raw[i].mask) continue;
      if (isnan(raw[i].dMag)) { 
	raw[i].mask |= MARK_NAN_MAG_ERROR; 
	continue; 
      }
      if (raw[i].dMag > SIGMA_LIM) { 
	raw[i].mask |= MARK_BIG_MAG_ERROR; 
	continue; 
      } // is this redundant with ImageOps.c:915?

      dL = raw[i].L - ref[i].L;
      dM = raw[i].M - ref[i].M;
      dR = hypot (dL, dM);
      if (dR > dRmax) {
	raw[i].mask |= MARK_BIG_OFFSET;
	continue;
      }
      Nkeep ++;
    }

    // I'm rejecting some points from the fit above; I count the remainders and actually
    // use that count to set order_use below

    // 20140925 : the same logic below is used to constrain the dimensions of the image map

    // figures to assess the fitting process:
    // x vs dx, x vs dy, y vs dx, y vs dy : 
    // residual vector field 
    // dx vs mag, dy vs mag
    if (relastroGetVisual()) {
      fprintf (stderr, "fitting chip %s, %d matched stars\n", image->name, Nmatch);
    }
    relastroVisualPlotChipFit(raw, ref, dRmax, Nmatch);

    // for polynomial fits, I used (5,30,60) = (5,6,7) stars per term

    // set the maximum order for the polynomial (based on number of stars kept above)
    int order_use = 0;
    if (Nkeep >   5) order_use = 1; //  5 stars per cell
    if (Nkeep >  24) order_use = 2; //  6 stars per cell
    if (Nkeep >  63) order_use = 3; //  7 stars per cell
    if (Nkeep > 128) order_use = 4; //  8 stars per cell
    if (Nkeep > 225) order_use = 5; //  9 stars per cell
    if (Nkeep > 360) order_use = 6; // 10 stars per cell
    if (order_use < 1) {
      if (VERBOSE2) fprintf (stderr, "insufficient measurements (%d) for linear fit\n", Nkeep);
      image[0].flags |= ID_IMAGE_ASTROM_FEW;
      return FALSE;
    }
    if (CHIPMAP) {
      order_use = MIN(MIN(order_use, CHIPMAP), 6); // can only go up to 6th order map (can be user limited)
    } else {
      order_use = MIN(MIN(order_use, CHIPORDER), 3); // can only go up to 3rd order for polynomials
    }

    if (VERBOSE2) {
      fprintf (stderr, "using %d for %s\n", order_use, image[0].name);
    }

    // when fitting the map, first fit a linear model (below? change Npolyterms to -1)
    image[0].coords.Npolyterms = CHIPMAP ? 1 : order_use;

    if (fit) fit_free (fit);
    fit = fit_init (image[0].coords.Npolyterms);

    // generate the fit matches
    for (i = 0; i < Nmatch; i++) {
      if (raw[i].mask) continue;
      fit_add (fit, raw[i].X, raw[i].Y, ref[i].L, ref[i].M, ref[i].dPos);
    }

    // measure the fit, update the coords & object coordinates
    if (!fit_eval (fit)) {
      fprintf (stderr, "failed to fit new model\n");
      image[0].flags |= ID_IMAGE_ASTROM_FAIL;
      if (fit) fit_free (fit);
      return FALSE;
    }

    if (!fit_apply_coords (fit, &image[0].coords, FALSE)) {
      fprintf (stderr, "failed to fit new model\n");
      image[0].flags |= ID_IMAGE_ASTROM_FAIL;
      if (fit) fit_free (fit);
      return FALSE;
    }

    // apply fit to get the fitted X,Y coordinates.  we need these to fit the residual map below
    for (i = 0; i < Nmatch; i++) {
      // we have not yet fitted the map, so Npolyterms needs to be 1 here:
      LM_to_XY (&ref[i].X, &ref[i].Y, ref[i].L, ref[i].M, &image[0].coords);
    }

    if (CHIPMAP) {
      if (image[0].coords.offsetMap == NULL) {
	// allocate a new table and assign to this image
	// need to lock threads here because 'table' is a static with 
	// modifiable elements
	lockUpdateChips ();
	AstromOffsetTable *table = get_astrom_table ();
	AstromOffsetTableNewMap(table, order_use, order_use, image);
	unlockUpdateChips ();
      }
      // XXX Need to check / update the order of the map here.  if we already have a map, it might have been
      // allocated with too small arrays
      AstromOffsetMapSetOrder (image[0].coords.offsetMap, order_use, order_use, image);
      image[0].coords.offsetMap->keep = TRUE; // if we are trying or re-trying this map, we should keep it unless it fails
      fit_map (image[0].coords.offsetMap, raw, ref, Nmatch);
      image[0].coords.Npolyterms = -1;
    }

    if (!strcmp(image[0].name, "o5745g0403o.356777.cm.982487.smf[XY17.hdr]")) {
      fprintf (stderr, "test image\n");
    }

    for (i = 0; i < Nmatch; i++) {
      // if we have fitted the map above, Npolyterms needs to be -1 here:
      XY_to_LM (&raw[i].L, &raw[i].M, raw[i].X, raw[i].Y, &image[0].coords);
    }
  }

  // count mask classes
  int nMask1 = 0;
  int nMask2 = 0;
  int nMask3 = 0;
  int nMask4 = 0;
  int nMask5 = 0;
  for (i = 0; i < Nmatch; i++) {
    if (!raw[i].mask) continue;
    if (raw[i].mask & MARK_TOO_FEW_MEAS ) nMask1 ++;
    if (raw[i].mask & MARK_NAN_POS_ERROR) nMask2 ++;
    if (raw[i].mask & MARK_NAN_MAG_ERROR) nMask3 ++;
    if (raw[i].mask & MARK_BIG_MAG_ERROR) nMask4 ++;
    if (raw[i].mask & MARK_BIG_OFFSET   ) nMask5 ++;
  }

  int Ncolor;
  float colorMedian;
  float *colorList = NULL;
  ALLOCATE (colorList, float, Nmatch);

  // calculate the median blue color
  Ncolor = 0;
  for (i = 0; i < Nmatch; i++) {
    if (!raw[i].mask) continue;
    if (isnan(ref[i].ColorBlue)) continue;
    colorList[Ncolor] = ref[i].ColorBlue;
    Ncolor ++;
  }
  fsort (colorList, Ncolor);
  colorMedian = (Ncolor > 0) ? colorList[(int)(0.5*Ncolor)] : NAN;
  image[0].refColorBlue = colorMedian;

  // calculate the median red color
  Ncolor = 0;
  for (i = 0; i < Nmatch; i++) {
    if (!raw[i].mask) continue;
    if (isnan(ref[i].ColorRed)) continue;
    colorList[Ncolor] = ref[i].ColorRed;
    Ncolor ++;
  }
  fsort (colorList, Ncolor);
  colorMedian = (Ncolor > 0) ? colorList[(int)(0.5*Ncolor)] : NAN;
  image[0].refColorRed = colorMedian;

  free (colorList);

  GetScatterRawRef(&dLsigFull, &dMsigFull, &dRsigFull, &NstatFull, raw, ref, Nmatch, SIGMA_LIM);
  GetScatterRawRef(&dLsig,     &dMsig,     &dRsig,     &Nstat,     raw, ref, Nmatch, IMFIT_SYS_SIGMA_LIM);

  int Nm = 0;
  int Ns = 0;
  for (i = 0; i < Nmatch; i++) {
    if (raw[i].mask) continue;
    Nm += raw[i].Nmeas;
    Ns++;
  }
  image[0].nLinkAstrom = (Nm / Ns);

  if (VERBOSE2) fprintf (stderr, "fit sigma: %f (%f, %f) : full: %f (%f, %f), scatter limit: %f (%d full, %d bright, %d fit, %d all) (%d %d %d %d %d)\n", dRsig, dLsig, dMsig, dRsigFull, dLsigFull, dMsigFull, dRmax, NstatFull, Nstat, fit[0].Npts, Nmatch, nMask1, nMask2, nMask3, nMask4, nMask5);

  // need to convert dLsig, dMsig back to pixel scale (or up to arcsec)

  float plateScale;
  if (image[0].coords.mosaic) {
    // NOTE: for the full pixel to sky plate scale, use this:
    // float plateScaleX = 3600.0*image[0].coords.mosaic->cdelt1*image[0].coords.cdelt1;
    // float plateScaleY = 3600.0*image[0].coords.mosaic->cdelt2*image[0].coords.cdelt2;

    // since we are compare L,M values, just need to compensate for focal plate to sky:
    float plateScaleX = 3600.0*fabs(image[0].coords.mosaic->cdelt1);
    float plateScaleY = 3600.0*fabs(image[0].coords.mosaic->cdelt2);
    plateScale = 0.5*(plateScaleX + plateScaleY);
  } else {
    // since we are compare L,M values, just need to compensate for arcsec vs degrees:
    plateScale = 3600.0;
  }

  image[0].dXpixSys = plateScale*dLsig;
  image[0].dYpixSys = plateScale*dMsig;
  image[0].nFitAstrom = fit[0].Npts;

  if (VERBOSE2) fprintf (stderr, "%s | %6.3f %6.3f | %4d %4d | %6.3f %6.3f\n", image[0].name, image[0].refColorRed, image[0].refColorBlue, Ncolor, image[0].nFitAstrom, image[0].dXpixSys, image[0].dYpixSys);

  if (fit) fit_free (fit);

  return TRUE;
}

// measure the scatter distribution (limit selected objects by dMag)
int GetScatterRawRef(float *dLsig, float *dMsig, float *dRsig, int *nKeep, StarData *raw, StarData *ref, int Nstars, float SigmaLimit) {

  int i, Ns;
  float dL, dM;

  ALLOCATE_PTR (dRvec, double, Nstars);
  ALLOCATE_PTR (dLvec, double, Nstars);
  ALLOCATE_PTR (dMvec, double, Nstars);

  Ns = 0;
  for (i = 0; i < Nstars; i++) {
    if (raw[i].mask) continue;
    if (isnan(raw[i].dMag)) continue;
    if ((SigmaLimit > 0.0) && (raw[i].dMag > SigmaLimit)) continue;
    
    dL = raw[i].L - ref[i].L;
    dM = raw[i].M - ref[i].M;

    dLvec[Ns] = dL;
    dMvec[Ns] = dM;
    dRvec[Ns] = hypot (dL, dM);
    Ns++;
  }

  dsort (dLvec, Ns);
  dsort (dMvec, Ns);

  double Slo, Shi;
  Slo = VectorFractionInterpolate (dLvec, 0.158655, Ns);
  Shi = VectorFractionInterpolate (dLvec, 0.841345, Ns);
  *dLsig = (Shi - Slo) / 2.0;
  free (dLvec);

  Slo = VectorFractionInterpolate (dMvec, 0.158655, Ns);
  Shi = VectorFractionInterpolate (dMvec, 0.841345, Ns);
  *dMsig = (Shi - Slo) / 2.0;
  free (dMvec);

  *nKeep = Ns;

  if (Ns < 5) {
    *dRsig = NAN;
    free  (dRvec);
    return (FALSE);
  }

  // for a 2D Gaussian, 40% of the points are within R = 1 sigma
  dsort (dRvec, Ns);
  *dRsig = dRvec[(int)(0.40*Ns)];
  
  free  (dRvec);
  return (TRUE);
}

/* in the mosaic case, we have four coord systems of interest:
   R,D : the sky
   P,Q : the tangent plane
   L,M : the focal plane
   X,Y : the chip

   R,D -> P,Q (projection)
   P,Q -> L,M (polynomial transformation : DIS)
   L,M -> X,Y (polynomial transformation : WRP)

   Some details about this function:

   - the initial value of the clipping radius is set based on the distribution of the dR
   values for the bright sources.

   - NITER clipping passes are performed

   - the per-star astrometric errors are included in the fit.  The photcode table controls
   whether only the reported positional errors are used, or if the magnitudes errors are
   scaled to determine the error, and if there is a systematic floor for all sources.

   - input detections are pre-filtered on the basis of the photFlags, etc, in bcatalog

   - outlying detections are clipped in the iterative passes, but the clipped detections are
   not recorded

*/

/* example using fit_apply() :

   f = fopen ("test3.dat", "w");

   // apply new coords to raw (X,Y -> L,M)
   for (i = 0; i < Nmatch; i++) {
   fprintf (f, "%f %f  %f %f  ", raw[i].X, raw[i].Y, raw[i].L, raw[i].M);
   fit_apply (newfit, &L1, &M1, raw[i].X - coords[0].crpix1, raw[i].Y - coords[0].crpix2);
   fprintf (f, "%f %f\n", L1, M1);
   }
   fclose (f);
*/

int fit_map (AstromOffsetMap *map, StarData *raw, StarData *ref, int Npts) {

  // we are actually fitting the residual after the linear fit has been taken off
  
  // fit the linear terms as above
  // calculate dX (raw.X - ref.X) and dY 

  int i, N;

  float *x, *y, *dX, *dY, *dP;
  ALLOCATE (x,  float, Npts);
  ALLOCATE (y,  float, Npts);
  ALLOCATE (dX, float, Npts);
  ALLOCATE (dY, float, Npts);
  ALLOCATE (dP, float, Npts);

  N = 0;
  for (i = 0; i < Npts; i++) {
    if (raw[i].mask) continue;
    x[N] = raw[i].X;
    y[N] = raw[i].Y;
    dX[N] = ref[i].X - raw[i].X;
    dY[N] = ref[i].Y - raw[i].Y;
    dP[N] = ref[i].dPos;
    N++;
  }

  // in coordsops.c:XY_to_LM, the map is defined to carry dX,dY so that:
  // (L,M) = f(X',Y') : (X',Y') = (X,Y) + (dX,dY)

  AstromOffsetMapFit (map, x, y, dX, dP, N, TRUE);
  AstromOffsetMapFit (map, x, y, dY, dP, N, FALSE);

  AstromOffsetMapRepair (map, TRUE);
  AstromOffsetMapRepair (map, FALSE);

  free (x);
  free (y);
  free (dX);
  free (dY);
  free (dP);

  return TRUE;
}

int dump_stardata_pts (StarData *raw, int Npts, char *filename) {

  FILE *f = fopen (filename, "w");
  if (!f) { 
    fprintf (stderr, "failed to open file %s\n", filename);
    return FALSE;
  }

  int i;
  for (i = 0; i < Npts; i++) {
    fprintf (f, "%4d %10.6f %10.6f  %10.6f %10.6f  %9.2f %9.2f  %9.2f %9.2f  %9.4f %3d\n", 
	     i, raw[i].R, raw[i].D, raw[i].P, raw[i].Q, raw[i].L, raw[i].M, raw[i].X, raw[i].Y, raw[i].dPos, raw[i].mask);
  }
  fclose (f);

  return TRUE;
}
