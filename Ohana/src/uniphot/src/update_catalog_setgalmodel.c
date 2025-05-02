# include "setgalmodel.h"

int update_catalog_setgalmodel (Catalog *catalog) {

  off_t i, m;

  Average *average = catalog[0].average;

  if ((catalog[0].Nstarpar_disk > 0) && (catalog[0].Nstarpar == 0)) {
    fprintf (stderr, "ERROR: failed to load starpar data\n");
    exit (2);
  }

  CoordTransform *transform = InitTransform (COORD_CELESTIAL, COORD_GALACTIC);

  // the typical motions do not vary so much as a function of position, we can use the
  // median uR,uD values for all objects without stellar parameters

  int Nsave = 0;
  ALLOCATE_PTR (uRsave, double, catalog[0].Nstarpar);
  ALLOCATE_PTR (uDsave, double, catalog[0].Nstarpar);

  // first set the proper motion based on Galactic rotation and solar motion
  for (i = 0; i < catalog[0].Naverage; i++) {

    average[i].uRgal = NAN;
    average[i].uDgal = NAN;

    if (average[i].Nstarpar == 0) continue;

    m = average[i].starparOffset;
    StarPar *starpar = &catalog[0].starpar[m];

    // fake or real QSOs are marked with FeH = +/- 100.0
    if (fabs(starpar->FeH) > 99.0) { 
      starpar->uRA  = 0.0;
      starpar->uDEC = 0.0;
      average[i].uRgal = 0.0;
      average[i].uDgal = 0.0;
      continue;
    }

    // NOTE: DistMag is standard (10pc reference).  SolarMotionModel wants distance in kiloparsec:
    double distance = pow(10.0, 0.2*(starpar->DistMag + 5.0)) / 1000.0;

    double Lrad = starpar->galLon * RAD_DEG;
    double Brad = starpar->galLat * RAD_DEG;

    double uL_gal, uB_gal;
    GalaxyMotionModel_radians(&uL_gal, &uB_gal, Lrad, Brad);

    double uL_sol, uB_sol;
    SolarMotionModel_radians(&uL_sol, &uB_sol, Lrad, Brad, distance);

    double uL = uL_gal + uL_sol;
    double uB = uB_gal + uB_sol;

    // XXX: amplify motion to make tests easier:
    uL *= TEST_SCALE;
    uB *= TEST_SCALE;
    
    double uR, uD;
    TransformProperMotionBackwards (&uR, &uD, uL, uB, average[i].R, average[i].D, transform);

    starpar->uRA  = uR;
    starpar->uDEC = uD;

    average[i].uRgal = uR;
    average[i].uDgal = uD;

    if (Nsave < catalog[0].Nstarpar) {
      uRsave[Nsave] = uR;
      uDsave[Nsave] = uD;
      Nsave ++;
    }
  }

  // below we attempt to set the uRgal, uDgal values for objects
  // without starpar data.  but if we do not have enough information, 
  // give up and return
  if (Nsave < 3) return TRUE;

  dsort (uRsave, Nsave);
  dsort (uDsave, Nsave);

  int Nmid = 0.5*Nsave;
  double uRmid = uRsave[Nmid];
  double uDmid = uDsave[Nmid];

  // set the value for unassigned stars based on the median uR,uD values for this patch
  for (i = 0; i < catalog[0].Naverage; i++) {
    if (isfinite(average[i].uRgal)) continue;

    average[i].uRgal = uRmid;
    average[i].uDgal = uDmid;
  }  

  return (TRUE);
}

