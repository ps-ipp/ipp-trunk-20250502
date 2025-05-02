# include "relastro.h"

/* The sequence of calibration is something like this:

 * apply the current image.coords to measure.X,Y to get measure.R,D
 * use the measure.R,D values to find the mean position average.R,D
 * fit the measure.X,Y positions to average.R,D to get image.coords

 At some point in this sequence, we want to compare our current average.R,D (for specific
 objects) to the ICRF values for those objects to get a correction set in R,D.  

 Apply that correction to get a new set of average.R,D values.  The impact of the
 adjustment will then be to modify the image.coords to compensate for the offset.

*/

int FrameCorrectionParallelMaster (RegionHostTable *regionHosts) {

  if (!USE_ICRF_CORRECT) return TRUE;

  int i;
  for (i = 0; i < NLOOP; i++) {
    if ((i > 1) || !USE_GALAXY_MODEL) {
      // if GALAXY_MODEL is selected, we want to delay the frame correction until we have 
      // applied the galaxy model a couple of times.

      ICRFobj *icrfobj = slurp_icrf_obj (regionHosts, i);
      FrameCorrectionSet *set = FrameCorrectionMeasure (icrfobj);
      
      char *filename = make_filename (CATDIR, "master", 0, "frame.corr.fits");
      FrameCorrectionSetSave (filename, set);
      free (filename);
      
      char *syncfile = make_filename (CATDIR, "master", 0, "frame.corr.sync");
      update_sync_file (syncfile, i);
      free (syncfile);
    }
  }

  int status = HarvestRegionHosts (regionHosts);
  return status;
}

int FrameCorrectionParallelSlave (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop) {

  if (!USE_ICRF_CORRECT) return TRUE;

  share_icrf_obj (catalog, Ncatalog, regionHosts, nloop); 

  char *syncfile = make_filename (CATDIR, "master", 0, "frame.corr.sync");
  check_sync_file (syncfile, nloop);
  free (syncfile);
    
  char *filename = make_filename (CATDIR, "master", 0, "frame.corr.fits");
  FrameCorrectionSet *set = FrameCorrectionSetLoad (filename);
  free (filename);

  // XXX NOTE : do not apply correction for now.  let's just measure it and see how it evolves
  // Now apply the correction to all of the average.R,D values
  // FrameCorrectionApply (catalog, Ncatalog, set->frame, set->coords);
  FrameCorrectionSetFree(set);

  return TRUE;
}

int FrameCorrectionSerial (Catalog *catalog, int Ncatalog) {

  if (!USE_ICRF_CORRECT) return TRUE;

  ICRFobj *icrfobj = get_ICRF_data (catalog, Ncatalog);
  if (!icrfobj->Nicrfobj) {
    fprintf (stderr, "no matched ICRF quasars for reference correction, skipping\n");
    return FALSE;
  }

  FrameCorrectionSet *set = FrameCorrectionMeasure (icrfobj);
  ICRFobjFree (icrfobj);

  // write out an image to represent the correction
  static int version = 0;
  char filename[1024];
  snprintf_nowarn (filename, 1024, "%s/frame.%03d.corr.fits", CATDIR, version);
  FrameCorrectionSetSave (filename, set);
  version ++;

  // Now apply the correction to all of the average.R,D values
  FrameCorrectionApply (catalog, Ncatalog, set->frame, set->coords);
  FrameCorrectionSetFree(set);

  return TRUE;
}

FrameCorrectionSet *FrameCorrectionMeasure (ICRFobj *icrfobj) {

  // we have only a few thousand ICRF reference points.  I need to have a lookup table to
  // go from ICRF object to the catalog entry.  

  // we can have 2 kinds of corrections:
  // 'frame' is a map in spherical coords of the corrections based on spherical harmonics
  // 'map' is a correction based on a local projection to a linear coordinate system.  I
  // am going to use this for the pole, but also for SAS for testing.
  // We need to be sure only one of the two is applied. For the pole, the boundary is a
  // fixed line of DEC; for the SAS, the boundary is the projection 'image'

  FrameCorrectionSet *set = FrameCorrectionSetInit();
  if (!USE_ICRF_LOCAL && !USE_ICRF_SHFIT) {
    fprintf (stderr, "ERROR: no ICRF correction mode selected\n");
    exit (0);
  }

  if (USE_ICRF_LOCAL) {
    // for the local frame correction, we are going to convert R,D into 
    // X,Y linear coordinates in an 'image' centered on the field center. 
    // we can then use an AstromOffsetMap structure to carry the correction values

    // what parameters define a local frame correction?
    // * Ro, Do -- map/projection center
    // * scale  -- degrees / pixel
    // * Nx, Ny -- size of map in pixels
    // * Rmin,Rmax,Dmin,Dmax

    // example parameters for SAS: 320 - 340, -10 - +10
    double Ro = 330.0;
    double Do =   0.0;
    double scale = 2.0; // degrees per patch
    int Nx = 10;
    int Ny = 10;
    if (USE_ICRF_POLE) {
       Ro =  0.0;
       Do = 90.0;
       scale = 5.0; // degrees per patch
       Nx = 4;
       Ny = 4;
    }

    
    Coords *coords = NULL;
    ALLOCATE (coords, Coords, 1);

    InitCoords (coords, "DEC--SIN");
    coords->cdelt1 = coords->cdelt2 = scale;
    coords->crval1 = Ro; // SAS center
    coords->crval2 = Do;
    coords->crpix1 = 0.5*Nx; // middle of projection is middle of map
    coords->crpix2 = 0.5*Ny; // middle of projection is middle of map

    // map is a component of coords
    coords->offsetMap = AstromOffsetMapInit (Nx, Ny);
    coords->offsetMap->dX = 1.0; // scale from projection (in arcsec) to correction patches
    coords->offsetMap->dY = 1.0;

    FrameCorrectionFitLocal (icrfobj, coords);
    set->coords = coords;
  } 

  if (USE_ICRF_SHFIT) {
    // Lmax in recipe
    int Lmax = 5;

    // dR and dD will carry the fit coefficients in RA and DEC
    SHterms *dRc = SHtermsInit (Lmax);
    SHterms *dDc = SHtermsInit (Lmax);

    INITTIME;

    FrameCorrectionFitSH (icrfobj, dRc, dDc);
    MARKTIME ("done with FitSH: %f sec\n", dtime);

    double pltscale = 0.5; // degrees per pixel
    FrameCorrectionType *frame = FrameCorrectionInit (pltscale);
  
    FrameCorrectionFromSH (frame, dRc, dDc);
    MARKTIME ("done with FromSH: %f sec\n", dtime);

    SHtermsFree (dRc);
    SHtermsFree (dDc);

    set->frame = frame;
  }

  return set;
}

// I need to generate a collection of values dR,dD(R,D), where dR = average.R - ICRF.R, etc.  
// I will then fit the set of dR,dD values to a set of spherical harmonics (up to Lmax).
// I will then generate correction images dR,dD(R,D)  
// I will then correct all values average.R,D by the interpolated values from the images
// average.R' = average.R - dR(R,d), average.D' = average.D - dD(R,d)

int FrameCorrectionFitSH (ICRFobj *icrfobj, SHterms *dRc, SHterms *dDc) {

  int i;

  myAssert (dRc->lmax == dDc->lmax, "dR and dD must match\n");

  fprintf (stderr, "start Fit %d SH l-modes for %d QSOs\n", dRc->lmax, icrfobj->Nicrfobj);

  int *mask = NULL;
  ALLOCATE (mask, int, icrfobj->Nicrfobj);
  memset (mask, 0, icrfobj->Nicrfobj*sizeof(int));

  double *dPosObs, *dPosSrt = NULL;
  ALLOCATE (dPosObs, double, icrfobj->Nicrfobj);
  ALLOCATE (dPosSrt, double, icrfobj->Nicrfobj);

  int Niter;
  for (Niter = 0; Niter < 3; Niter ++) {

    SHfitWithMask (icrfobj->Rave, icrfobj->Dave, icrfobj->dRoff, mask, icrfobj->Nicrfobj, dRc);
    SHfitWithMask (icrfobj->Rave, icrfobj->Dave, icrfobj->dDoff, mask, icrfobj->Nicrfobj, dDc);

    // the bit below is for testing

    // allocate an SHterms structure to hold the Ylm values 
    SHterms *Ylm = SHtermsInit (dRc->lmax);

    static int version = 0;
    char filename[1024];
    snprintf_nowarn (filename, 1024, "%s/sh.%02d.dat", CATDIR, version);
    FILE *f = fopen (filename, "w");
    myAssert (f, "oops");
    version ++;

    int Nkeep = 0;
    for (i = 0; i < icrfobj->Nicrfobj; i++) {
    
      SHtermsForRD (Ylm, icrfobj->Rave[i], icrfobj->Dave[i]);
      
      double dRfit = 0.0;
      double dDfit = 0.0;

      int j;
      for (j = 0; j < Ylm->Nterms; j++) {
	dRfit += dRc->Fr[j]*Ylm->Fr[j] + dRc->Fi[j]*Ylm->Fi[j];
	dDfit += dDc->Fr[j]*Ylm->Fr[j] + dDc->Fi[j]*Ylm->Fi[j];
	
      }
      fprintf (f, "%12.8f %12.8f %7.3f %7.3f : %7.3f %7.3f : %d\n", 
	       icrfobj->Rave[i], icrfobj->Dave[i], icrfobj->dRoff[i], icrfobj->dDoff[i], dRfit, dDfit, mask[i]);
      dPosObs[i] = hypot(icrfobj->dRoff[i] - dRfit, icrfobj->dDoff[i] - dDfit);
      if (mask[i]) continue;
      if (isnan(dPosObs[i])) continue;
      dPosSrt[Nkeep] = dPosObs[i];
      Nkeep ++;
    }
    fclose (f);
    
    dsort (dPosSrt, Nkeep);
    double dPmax = 1.5*dPosSrt[(int)(0.95*Nkeep)];

    Nkeep = 0;
    for (i = 0; i < icrfobj->Nicrfobj; i++) {
      if (dPosObs[i] > dPmax) mask[i] = 1;
      if (isnan(dPosObs[i])) mask[i] = 1;
      if (!mask[i]) Nkeep++;
    }
    fprintf (stderr, "keeping %d of %d points\n", Nkeep, icrfobj->Nicrfobj);
  }

  free (mask);
  free (dPosObs);
  free (dPosSrt);

  return TRUE;
}

int SHfitWithMask (double *R, double *D, double *value, int *mask, int Npts, SHterms *fit) {

  int i, j, k;

  // allocate an SHterms structure to hold the Ylm values 
  SHterms *Ylm = SHtermsInit (fit->lmax);

  // we only fit the linearly independent terms: Re(m >= 0), Im(m > 0)
  int NtermRE = 0;
  int NtermIM = 0;
  for (i = 0; i < Ylm->Nterms; i++) {
    if (Ylm->m[i] >= 0) NtermRE ++;
    if (Ylm->m[i] >  0) NtermIM ++;
  }
  int *Nre = NULL;
  int *Nim = NULL;
  ALLOCATE (Nre, int, NtermRE);
  ALLOCATE (Nim, int, NtermIM);

  NtermRE = NtermIM = 0;
  for (i = 0; i < Ylm->Nterms; i++) {
    if (Ylm->m[i] >= 0) {
      Nre[NtermRE] = i;
      NtermRE ++;
    }
    if (Ylm->m[i] >  0) {
      Nim[NtermIM] = i;
      NtermIM ++;
    }
  }

  double **Are, **bre, **Aim, **bim;
  ALLOCATE (Are, double *, NtermRE);
  ALLOCATE (bre, double *, NtermRE);
  ALLOCATE (Aim, double *, NtermIM);
  ALLOCATE (bim, double *, NtermIM);
  for (i = 0; i < NtermRE; i++) {
    ALLOCATE_ZERO (Are[i], double, NtermRE);
    ALLOCATE_ZERO (bre[i], double, 1);
  }
  for (i = 0; i < NtermIM; i++) {
    ALLOCATE_ZERO (Aim[i], double, NtermIM);
    ALLOCATE_ZERO (bim[i], double, 1);
  }
  
  // measure the dot product \sum(F_i * Ylm_i) and the cross terms (\sum(Y_lm * Y_jk))
  int Nfit = 0;
  for (i = 0; i < Npts; i++) {
    if (mask[i]) continue;
    Nfit ++;

    // set the values of Ylm(R[i],D[i])
    SHtermsForRD (Ylm, R[i], D[i]); 

    double Fv = value[i];

    for (j = 0; j < NtermRE; j++) {
      int jre = Nre[j];
      bre[j][0] += Fv * Ylm->Fr[jre];
    }
    for (j = 0; j < NtermIM; j++) {
      int jim = Nim[j];
      bim[j][0] += Fv * Ylm->Fi[jim];
    }

    for (j = 0; j < NtermRE; j++) {
      int jre = Nre[j];
      for (k = j; k < NtermRE; k++) {
	int kre = Nre[k];
	Are[j][k] += Ylm->Fr[jre] * Ylm->Fr[kre];
      }
    }

    for (j = 0; j < NtermIM; j++) {
      int jim = Nim[j];
      for (k = j; k < NtermIM; k++) {
	int kim = Nim[k];
	Aim[j][k] += Ylm->Fi[jim] * Ylm->Fi[kim];
      }
    }
  }

  for (j = 1; j < NtermRE; j++) {
    for (k = 0; k < j; k++) {
      Are[j][k] = Are[k][j];
    }	
  }
  for (j = 1; j < NtermIM; j++) {
    for (k = 0; k < j; k++) {
      Aim[j][k] = Aim[k][j];
    }	
  }

  if (!dgaussjordan (Are, bre, NtermRE, 1)) {
    gprint (GP_ERR, "failed to fit data : ill-conditioned matrix\n");
    return FALSE;
  }
  if (!dgaussjordan (Aim, bim, NtermIM, 1)) {
    gprint (GP_ERR, "failed to fit data : ill-conditioned matrix\n");
    return FALSE;
  }

  for (j = 0; j < fit->Nterms; j++) { 
    fit->Fr[j] = 0.0;
    fit->Fi[i] = 0.0;
  }
  for (j = 0; j < NtermRE; j++) {
    int jre = Nre[j];
    // fit->Fr[jre] = bre[j][0] * fit->Nterms / (float) Nfit; // XXX EAM : why is this factor needed?
    fit->Fr[jre] = bre[j][0];
  }
  for (j = 0; j < NtermIM; j++) {
    int jim = Nim[j];
    // fit->Fi[jim] = bim[j][0] * fit->Nterms / (float) Nfit; // XXX EAM : why is this factor needed?
    fit->Fi[jim] = bim[j][0];
  }

  SHtermsFree (Ylm);

  for (i = 0; i < NtermRE; i++) {
    free (Are[i]);
    free (bre[i]);
  }
  for (i = 0; i < NtermIM; i++) {
    free (Aim[i]);
    free (bim[i]);
  }
  free (Are);
  free (bre);
  free (Aim);
  free (bim);

  return TRUE;
}

int FrameCorrectionFitLocal (ICRFobj *icrfobj, Coords *coords) {

  int i;
  double Xave, Yave, Xmeas, Ymeas;
  float *X, *Y, *dX, *dY;
  float *dXf, *dYf, *dPos;
  double *Rave, *Dave, *Rmeas, *Dmeas;

  int Nicrf = icrfobj->Nicrfobj;

  AstromOffsetMap *map = coords->offsetMap;

  int Npts = 0;
  ALLOCATE (X,  float, Nicrf);
  ALLOCATE (Y,  float, Nicrf);
  ALLOCATE (dX, float, Nicrf);
  ALLOCATE (dY, float, Nicrf);
  ALLOCATE (dXf, float, Nicrf);
  ALLOCATE (dYf, float, Nicrf);

  ALLOCATE (dPos, float, Nicrf);

  // arrays to store the points actually in the local region
  ALLOCATE (Rave, double, Nicrf);
  ALLOCATE (Dave, double, Nicrf);
  ALLOCATE (Rmeas, double, Nicrf);
  ALLOCATE (Dmeas, double, Nicrf);

  int *mask = NULL;
  ALLOCATE (mask, int, Nicrf);
  memset (mask, 0, Nicrf*sizeof(int));

  // select the ICRF QSOS and save the necessary data
  for (i = 0; i < Nicrf; i++) {

    // this local correction is defined for (Rmin < R < Rmax, Dmin < D < Dmax)
    int status = RD_to_XY (&Xave, &Yave, icrfobj->Rave[i], icrfobj->Dave[i], coords);
    if (!status) continue;
    if (Xave < 0.0) continue;
    if (Xave > map->Nx) continue;
    if (Yave < 0.0) continue;
    if (Yave > map->Ny) continue;

    double Rm = icrfobj->Rave[i] - icrfobj->dRoff[i] / 3600.0 / cos(icrfobj->Rave[i]*RAD_DEG);
    double Dm = icrfobj->Dave[i] - icrfobj->dDoff[i] / 3600.0;
    RD_to_XY (&Xmeas, &Ymeas, Rm, Dm, coords);

    // record these in arcsec or degree?
    // correct for cos(D) or not?
    X[Npts] = Xave;
    Y[Npts] = Yave;
    dX[Npts] = Xave - Xmeas;
    dY[Npts] = Yave - Ymeas;
    Rave[Npts] = icrfobj->Rave[i];
    Dave[Npts] = icrfobj->Dave[i];
    Rmeas[Npts] = Rm;
    Dmeas[Npts] = Dm;
    Npts ++;
  }

  float *Xfit, *Yfit, *dXfit, *dYfit;
  ALLOCATE (Xfit, float, Npts);
  ALLOCATE (Yfit, float, Npts);
  ALLOCATE (dXfit, float, Npts);
  ALLOCATE (dYfit, float, Npts);

  int Niter;
  for (Niter = 0; Niter < 3; Niter ++) {
    int Nkeep = 0;
    for (i = 0; i < Npts; i++) {
      if (mask[i]) continue;
      
      Xfit[Nkeep] = X[i];
      Yfit[Nkeep] = Y[i];
      dXfit[Nkeep] = dX[i];
      dYfit[Nkeep] = dY[i];
      Nkeep ++;
    }

    AstromOffsetMapFit (map, Xfit, Yfit, dXfit, NULL, Nkeep, TRUE);
    AstromOffsetMapFit (map, Xfit, Yfit, dYfit, NULL, Nkeep, FALSE);

    for (i = 0; i < Npts; i++) {
      dXf[i] = AstromOffsetMapValue (map, X[i], Y[i], TRUE);
      dYf[i] = AstromOffsetMapValue (map, X[i], Y[i], FALSE);
    }

    // generate a histogram of the dPos = sqrt((dXf - dX)^2 + (dYf - dY)^2) values
    Nkeep = 0;
    for (i = 0; i < Npts; i++) {
      if (mask[i]) continue;
      float dP = hypot((dXf[i] - dX[i]), (dYf[i] - dY[i]));
      if (isnan(dP)) continue; // not all points are in the map
      dPos[Nkeep] = dP;
      Nkeep ++;
    }
    fsort (dPos, Nkeep);
    
    float dPmax = 1.5*dPos[(int)(0.95*Nkeep)];

    // mask points based on dPmax
    Nkeep = 0;
    for (i = 0; i < Npts; i++) {
      float dP = hypot((dXf[i] - dX[i]), (dYf[i] - dY[i]));
      if (dP > dPmax) mask[i] = 1;
      if (isnan(dP)) mask[i] = 1;
      if (!mask[i]) Nkeep++;
    }
    fprintf (stderr, "keeping %d of %d points\n", Nkeep, Npts);
  }
  free (Xfit);
  free (Yfit);
  free (dXfit);
  free (dYfit);

  // *** save the icrf points and fit residuals
  { 
    static int version = 0;
    char filename[1024];
    snprintf_nowarn (filename, 1024, "%s/map.%02d.dat", CATDIR, version);
    FILE *f = fopen (filename, "w");
    version ++;

    for (i = 0; i < Npts; i++) {
      fprintf (f, "%3d %12.8f %12.8f  %12.8f %12.8f  %f %f : %f %f : %f %f : %d\n", i, Rave[i], Dave[i], Rmeas[i], Dmeas[i], X[i], Y[i], dX[i], dY[i], dXf[i], dYf[i], mask[i]);
    }    
    fclose (f);
  }

  free (X);
  free (Y);
  free (dX);
  free (dY);
  free (dXf);
  free (dYf);
  free (dPos);
  free (Rave);
  free (Dave);
  free (Rmeas);
  free (Dmeas);
  free (mask);

  return TRUE;
}

int FrameCorrectionApply (Catalog *catalog, int Ncatalog, FrameCorrectionType *frame, Coords *coords) {

  double Xave, Yave;

  AstromOffsetMap *map = coords ? coords->offsetMap : NULL;

  int i, j;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {
      
      Average *average = &catalog[i].average[j];

      double R = average->R;
      double D = average->D;

      int inLocalRegion = FALSE;
      if (USE_ICRF_LOCAL) {
	myAssert (coords, "no local frame coords defined");

	// this local correction is defined for (Rmin < R < Rmax, Dmin < D < Dmax)
	inLocalRegion = RD_to_XY (&Xave, &Yave, R, D, coords);
	if (USE_ICRF_POLE) {
	  inLocalRegion = (D > 85.0);
	} else {
	  inLocalRegion = (inLocalRegion && (Xave > 0.0));
	  inLocalRegion = (inLocalRegion && (Xave < map->Nx));
	  inLocalRegion = (inLocalRegion && (Yave > 0.0));
	  inLocalRegion = (inLocalRegion && (Yave < map->Ny));
	}
      }

      if (inLocalRegion) {
	myAssert (map, "no local frame map defined");
	double dX = AstromOffsetMapValue (map, Xave, Yave, TRUE);
	double dY = AstromOffsetMapValue (map, Xave, Yave, FALSE);
	if (!isnan(dX) && !isnan(dY)) {
	  double Xmeas = Xave - dX;
	  double Ymeas = Yave - dY;
	  XY_to_RD (&average->R, &average->D, Xmeas, Ymeas, coords);
	}
	continue;
      }
      
      if (USE_ICRF_SHFIT) {
	myAssert (frame, "no frame correction defined");

	int iD = (D + 89.0) / frame->scale;
	if (iD < 0) continue;
	if (iD >= frame->Ndec) continue;

	int iR = R / frame->dR[iD];
	if (iR < 0) continue;
	if (iR >= frame->Nra[iD]) continue;

	double dR = frame->Roff[iD][iR];
	double dD = frame->Doff[iD][iR];

	// XXX tighten this up??
	// do not apply if the fabs(offset) is > 8 arcsec)
	if ((fabs(dR) > 8.0) || (fabs(dD) > 8.0)) {
	  fprintf (stderr, "skip: %10.6f %10.6f : %7.3f %7.3f\n", R, D, dR, dD);
	  continue;
	}

	average->R -= dR / 3600.0 / cos(average->R*RAD_DEG);
	average->D -= dD / 3600.0;
      }
    }
  }
  return TRUE;
}

