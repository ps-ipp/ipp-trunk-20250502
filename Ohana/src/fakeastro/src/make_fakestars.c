# include "fakeastro.h"

FakeAstro_Stars *make_fakestars (int Nstars) {

  FakeAstro_Stars *stars;
  ALLOCATE (stars, FakeAstro_Stars, Nstars);

  // libdvo uses Liu et al 2011 (A&A 526, A16) for default galactic coords,
  // but Greg Green / LSD use the older Reid et al 2004 (ApJ 616, 872) definition
  CoordTransform *transform = InitTransform (COORD_GALACTIC, COORD_CELESTIAL);
  CoordTransform *transback = InitTransform (COORD_CELESTIAL, COORD_GALACTIC);

  float dR = UserPatch.Rmax - UserPatch.Rmin;
  float dPmin = sin(RAD_DEG*UserPatch.Dmin);
  float dPmax = sin(RAD_DEG*UserPatch.Dmax);
  float dP = dPmax - dPmin;

  int i;
  for (i = 0; i < Nstars; i++) {

    // each star has the following value drawn from an appropriate random distribution:

    // ra, dec, distance, (absolute mag, colors) or (mass) or (MK type)

    /* very simple galaxy model:

     * rho(x,y,z) = exp(-0.5*(z/Zpc)^2) (r < 2000)

     */

    if (i % 100000 == 0) fprintf (stderr, ".");
    double z,r,L,B,R,D,Lrad,Brad;

    if (UNIFORM_RADEC) {
      // we can generate a distribution which is uniform on the sky, in which case
      // we can limit to the selected patch analyically

      // note: r & z are generated in parsec
      r = pow(drand48(), 0.33333) * FAKEASTRO_RGAL;
      z = 0.0;
      double phi = dP * drand48() + dPmin;
      D = DEG_RAD * asin(phi); // random in degrees
      R = drand48() * dR + UserPatch.Rmin;   // random in degrees
      R = ohana_normalize_angle (R);

      ApplyTransform (&L, &B, R, D, transback);
      Lrad = L*RAD_DEG;
      Brad = B*RAD_DEG;
    } else {
      int inPatch = FALSE;
      while (!inPatch) {
	// note: r & z are generated in parsec
	z = ohana_gaussdev_rnd (0.0, FAKEASTRO_ZGAL);
	r = sqrt(drand48()) * FAKEASTRO_RGAL;
	Lrad = drand48() * 2 * M_PI;
	Brad = atan2(z,r);
      
	L = Lrad*DEG_RAD;
	B = Brad*DEG_RAD;
      
	ApplyTransform (&R, &D, L, B, transform);
	if (R < UserPatch.Rmin) continue;
	if (R > UserPatch.Rmax) continue;
	if (D < UserPatch.Dmin) continue;
	if (D > UserPatch.Dmax) continue;
	break;
      }
    }

    // double x = r*cos(L);
    // double y = r*sin(L);

    // distance here is in parsec
    double distance = sqrt (SQ(r) + SQ(z));

    double uL_gal, uB_gal;
    GalaxyMotionModel_radians(&uL_gal, &uB_gal, Lrad, Brad);

    double uL_sol, uB_sol;
    SolarMotionModel_radians(&uL_sol, &uB_sol, Lrad, Brad, distance / 1000.0);
    // note: SolarMotionModel wants distance in kpc

    double uL = uL_gal + uL_sol;
    double uB = uB_gal + uB_sol;
    
    // XXX: amplify motion to make tests easier:
    uL *= TEST_SCALE;
    uB *= TEST_SCALE;
    
    double uR, uD;
    TransformProperMotionBackwards (&uR, &uD, uL, uB, R, D, transback);

    // crude Mr distribution from Bochanski et al 2010
    // http://iopscience.iop.org/1538-3881/139/6/2679/pdf/aj_139_6_2679.pdf
    
    // two gaussian distributions:
    // 75% in narrow gauss, with Mr = 11.25, sigma = 1.0
    // 25% is wide gauss, with Mr = 10.0, sigma = 3.0
    // first choose which gauss:
    int bigPeak = (drand48() > 0.60);
    double Mr;
    if (bigPeak) {
      Mr = ohana_gaussdev_rnd (11.25, 1.0);
    } else {
      Mr = ohana_gaussdev_rnd (10.00, 2.5);
    }
    
    stars[i].R = R;
    stars[i].D = D;
    stars[i].flag  = FALSE;
    stars[i].found = FALSE;

    stars[i].starpar.R      = R;
    stars[i].starpar.D 	    = D;
    stars[i].starpar.galLon = L;
    stars[i].starpar.galLat = B;

    stars[i].starpar.Ebv      = 0.0;
    stars[i].starpar.dEbv     = 0.0;
    stars[i].starpar.DistMag  = 5.0*log10(distance) - 5.0;
    stars[i].starpar.dDistMag = 0.0;
    stars[i].starpar.M_r      = Mr;
    stars[i].starpar.dM_r     = 0.0;
    stars[i].starpar.FeH      = 0.0;
    stars[i].starpar.dFeH     = 0.0;

    stars[i].starpar.uRA  = uR;
    stars[i].starpar.uDEC = uD;
  }
  fprintf (stderr, "\n");
  return stars;
}

int sortStars (FakeAstro_Stars *stars, int Nstars) {

# define SWAPFUNC(A,B){ FakeAstro_Stars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].R < stars[B].R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}

