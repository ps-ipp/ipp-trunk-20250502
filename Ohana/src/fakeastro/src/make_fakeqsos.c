# include "fakeastro.h"

FakeAstro_Stars *make_fakeqsos (int Nstars, int ICRF) {

  FakeAstro_Stars *stars;
  ALLOCATE (stars, FakeAstro_Stars, Nstars);

  // do I want to add in a galaxy obscuration (P(b) ?)

  float dR = UserPatch.Rmax - UserPatch.Rmin;
  float dPmin = sin(RAD_DEG*UserPatch.Dmin);
  float dPmax = sin(RAD_DEG*UserPatch.Dmax);
  float dP = dPmax - dPmin;

  CoordTransform *transback = InitTransform (COORD_CELESTIAL, COORD_GALACTIC);

  FILE *f = NULL;
  if (ICRF) {
    f = fopen ("fake.icrf.dat", "w");
    if (!f) {
      fprintf (stderr, "unable to open fake.icrf.dat\n");
    }
  } else {
    f = fopen ("fake.zero.dat", "w");
    if (!f) {
      fprintf (stderr, "unable to open fake.zero.dat\n");
    }
  } 

  int i;
  for (i = 0; i < Nstars; i++) {

    // ra, dec, distance, (absolute mag, colors) or (mass) or (MK type)

    if (i % 100000 == 0) fprintf (stderr, ".");
    double R,D;

    int inPatch = FALSE;
    while (!inPatch) {
      double phi = dP * drand48() + dPmin;
      D = DEG_RAD * asin(phi); // random in degrees
      R = drand48() * dR + UserPatch.Rmin;   // random in degrees
      if (R < UserPatch.Rmin) continue;
      if (R > UserPatch.Rmax) continue;
      if (D < UserPatch.Dmin) continue;
      if (D > UserPatch.Dmax) continue;
      break;
    }
    double L, B;
    ApplyTransform (&L, &B, R, D, transback);

    // Mr will be interpretted as m_r
    double Mr = ohana_gaussdev_rnd (18.0, 1.5);
    
    stars[i].R = R;
    stars[i].D = D;
    stars[i].flag  = FALSE;
    stars[i].found = FALSE;

    stars[i].starpar.R      = R;
    stars[i].starpar.D      = D;
    stars[i].starpar.galLon = L;
    stars[i].starpar.galLat = B;

    // how shall I distinguish ICRF and ZERO quasars
    stars[i].starpar.Ebv      = 0.0;
    stars[i].starpar.dEbv     = 0.0;
    stars[i].starpar.DistMag  = 0.0;
    stars[i].starpar.dDistMag = 0.0;
    stars[i].starpar.M_r      = Mr;
    stars[i].starpar.dM_r     = 0.0;

    // overload FeH to identify the ICRF vs ZERO QSOs
    if (ICRF) {
      stars[i].starpar.FeH      = -100.0;
    } else {
      stars[i].starpar.FeH      = +100.0;
    }
    stars[i].starpar.dFeH     = 0.0;

    stars[i].starpar.uRA  = 0.0;
    stars[i].starpar.uDEC = 0.0;

/*
#                       hr mn seconds   deg mn seconds      mas    mas                   Jy     Jy      Jy     Jy      Jy     Jy     Jy     Jy      Jy     Jy
C  2357+080 J0000+0816  00 00 07.031141 +08 16 45.05175    0.46   0.85   0.758     41  -1.00  -1.00   -1.00  -1.00    0.020 <0.014  -1.00  -1.00   -1.00  -1.00   X    rfc_2014c
01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
0         1         2         3         4         5         6         7         8         9         0         1         2         3         4         5         6         
*/

    if (f) {
      char Rline[32], Dline[32];
      {
	int flag = SIGN(R);
	double hours = fabs(R / 15.0);  /* convert from degrees to hours */
	int h = hours;
	int m = 60.000001*(hours - h);
	float s = 3600*(hours - h - m / 60.0);
	if (flag > 0) {
	  snprintf (Rline, 32, " %02d %02d %09.6f  ", h, m, s);
	} else {
	  snprintf (Rline, 32, "-%02d %02d %09.6f  ", h, m, s);
	}	
      }
      {
	int flag = SIGN(D);
	double dec = fabs(D);
	int d = dec;
	int m = 60.000001*(dec - d);
	float s = 3600*(dec - d - m / 60.0);
	if (flag > 0) {
	  snprintf (Dline, 32, " %02d %02d %09.6f  ", d, m, s);
	} else {
	  snprintf (Dline, 32, "-%02d %02d %09.6f  ", d, m, s);
	}	
      }

      fprintf (f, "C  2357+080 J0000+0816 %16s %15s    0.46   0.85   0.758     41  -1.00  -1.00   -1.00  -1.00    0.020 <0.014  -1.00  -1.00   -1.00  -1.00   X    rfc_2014c\n", Rline, Dline);
    }
  }
  fprintf (stderr, "\n");
  return stars;
}
