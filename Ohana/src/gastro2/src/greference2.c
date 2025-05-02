# include "gastro2.h"

void greference (CmpCatalog *Target, RefCatalog *Ref) {

  CatStats catstats;

  if (VERBOSE) fprintf (stderr, "loading astrometric reference data from %s\n", REFCAT); 

  define_region (&catstats, Target);

  Ref[0].N = 0;
  /* get stars from the USNO A catalog for the given region */
  if (!strcasecmp (REFCAT, "USNO")) {
    getusno (&catstats, Ref);
    /* calculate Ref[0].Moff from Target & Ref dMdN, Mo */
  }

  /* get stars from the USNO B catalog for the given region */
  if (!strcasecmp (REFCAT, "USNOB")) {
    getusnob (&catstats, Ref, 2000.0);
    /* calculate Ref[0].Moff from Target & Ref dMdN, Mo */
  }

  /* get stars from the HST GSC catalog for the given region */
  if (!strcasecmp (REFCAT, "GSC")) {
    getgsc (&catstats, Ref);
  }
  
  /* get stars from 2MASS for the given region -- add PHOTCODE check? */
  if (!strcasecmp (REFCAT, "2MASS")) {
    strcpy (CATDIR, TWO_MASS_DIR);
    getptolemy (&catstats, Ref);
  }
  
  /* get stars from the DVO CATDIR for the given region */
  if (!strcasecmp (REFCAT, "PTOLEMY")) {
    strcpy (CATDIR, ASTROM_CATDIR);
    getptolemy (&catstats, Ref);
  }
  
  if (Ref[0].N == 0) {
    fprintf (stderr, "no ref objs: %s\n", REFCAT);
    exit (1);
  }

  {
    double Mref, Mtar, logRho;

    /* what is the offset between the two lines at the average magnitude? */
    Mref = 0.5*(Ref[0].lum.Mmin + Ref[0].lum.Mmax);
    logRho = Mref * Ref[0].lum.dNdM + Ref[0].lum.Mo - log10(Ref[0].Area);
    Mtar = (logRho + log10(Target[0].Area) - Target[0].lum.Mo) / Target[0].lum.dNdM;

    Ref[0].lum.Mz = Mref;
    Target[0].lum.Mz = Mtar;
    Ref[0].Moff = Target[0].lum.Mz - Ref[0].lum.Mz;
    fprintf (stderr, "mag offset: %f (Areas: %f vs %f; log(Rho): %f @ %f mags)\n", Ref[0].Moff, Target[0].Area, Ref[0].Area, logRho, Mref);
  }

  if (PLOTSTUFF) plot_lumfunc (Target, Ref);

}

/* return RA, DEC bounds of the reigon of interest */  
void define_region (CatStats *catstats, CmpCatalog *Target) {
   
  int NX, NY, status;
  double x, y, X, Y, R, D, dX, dY, Xo, Yo;

  NX = Target[0].header.Naxis[0];
  NY = Target[0].header.Naxis[1];

  dX = NX + NFIELD*NX;
  dY = NY + NFIELD*NY;

  Xo = -0.5*NFIELD*NX;
  Yo = -0.5*NFIELD*NY;

  if (ASCA) {
      XY_to_RD (&R, &D, 0.5*NX, 0.5*NY, &Target[0].coords);
      catstats[0].RA[0]  = R - 90.0;
      catstats[0].RA[1]  = R + 90.0;
      catstats[0].DEC[0] = MAX (-90.0, D - 90.0);
      catstats[0].DEC[1] = MIN (+90.0, D + 90.0);
      if (VERBOSE) fprintf (stderr, "asca region: %f - %f, %f - %f\n", 
			    catstats[0].RA[0], catstats[0].RA[1], catstats[0].DEC[0], catstats[0].DEC[1]);
      return;
  }

  catstats[0].RA[0] =  360.0;
  catstats[0].RA[1] =    0.0;
  catstats[0].DEC[0] = +90.0;
  catstats[0].DEC[1] = -90.0;

  for (x = 0; x <= 1.0; x += 0.5) {
    for (y = 0; y <= 1.0; y += 0.5) {

      X = x*dX + Xo;
      Y = y*dY + Yo;
      status = XY_to_RD (&R, &D, X, Y, &Target[0].coords);
      if (!status) continue;
      if (isinf(R) || isnan(R)) continue;
      if (isinf(D) || isnan(D)) continue;

      catstats[0].RA[0]  = MIN (catstats[0].RA[0], R);
      catstats[0].RA[1]  = MAX (catstats[0].RA[1], R);
      catstats[0].DEC[0] = MIN (catstats[0].DEC[0], D);
      catstats[0].DEC[1] = MAX (catstats[0].DEC[1], D);
    }
  }

  /* is a pole in the image?  if so, include it... */
  status = RD_to_XY (&X, &Y, 0.0, 90.0, &Target[0].coords);
  if (status) {
      if (fabs(X - NX*0.5) > (NFIELD + 0.5)*NX) goto not_north;
      if (fabs(Y - NY*0.5) > (NFIELD + 0.5)*NY) goto not_north;
      catstats[0].DEC[1] = 90.0;
  }
not_north:
  
  status = RD_to_XY (&X, &Y, 0.0, -90.0, &Target[0].coords);
  if (status) {
      if (fabs(X - NX*0.5) > (NFIELD + 0.5)*NX) goto not_south;
      if (fabs(Y - NY*0.5) > (NFIELD + 0.5)*NY) goto not_south;
      catstats[0].DEC[0] = -90.0;
  }
not_south:

  if (VERBOSE) fprintf (stderr, "full region: %f - %f, %f - %f\n", 
	   catstats[0].RA[0], catstats[0].RA[1], catstats[0].DEC[0], catstats[0].DEC[1]);
}
