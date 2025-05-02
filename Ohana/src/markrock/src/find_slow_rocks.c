# include "markrock.h"

Rocks *find_slow_rocks (catalog, catstats, nrocks)
Catalog catalog[];
CatStats catstats[];
int *nrocks;
{

  int i, j, k, m, N, Nm, NOBJECT, Nobject, Nrocks, NROCKS;
  double RADIUS2;
  double *R1, *D1, *X1, *Y1, *M1;
  double R, D, X, Y, M;
  double dX, dY, dR;
  int *N1, *N0;
  unsigned int *T1, T;
  int Nstar, Nghost, Ng, Nmax;
  unsigned int flags;
  Coords *tcoords;
  double ax, ay, cx, cy, dt, dX1, dX2, speed, distance;
  Rocks *rocks;
  unsigned short IsProper;

  Nstar = catalog[0].Naverage;

  NOBJECT = 1000;
  ALLOCATE (N1, int,   NOBJECT);
  Nobject = 0;
  N0 = catstats[0].N;

  NROCKS = 3000;
  ALLOCATE (rocks, Rocks, NROCKS);
  Nrocks = 0;

  Nmax = 0;
  IsProper = ID_USNO | ID_PROPER;
  for (i = 0; i < Nstar; i++) {
    N = N0[i];
    /* accept a measurement if it is either unmarked or marked as a proper motion star and may have multiple measurements */
    if ((catalog[0].average[N].code == 0) || (catalog[0].average[N].code & IsProper == IsProper)) {
      Nmax = MAX (catalog[0].average[N].Nm, Nmax);
      N1[Nobject] = N;
      Nobject ++;
      if (Nobject == NOBJECT - 1) {
	NOBJECT += 1000;
	REALLOCATE (N1, int,   NOBJECT);
      }
    }
  }
  REALLOCATE (N1, int,   Nobject);
  fprintf (stderr, "Nobj: %d\n", Nobject);

  ALLOCATE (R1, double, Nmax);
  ALLOCATE (D1, double, Nmax);
  ALLOCATE (X1, double, Nmax);
  ALLOCATE (Y1, double, Nmax);
  ALLOCATE (T1, unsigned int, Nmax);
  ALLOCATE (M1, double, Nmax);
  /** find multiple measured objects, find additional measurements along that line */

  for (i = 0; i < Nobject; i++) {
    fprintf (stderr, ",");
    if (catalog[0].average[N1[i]].Nm > 1) {
      N = N1[i];
      Nm = catalog[0].average[N].Nm;
      m = catalog[0].average[N].offset;
      for (j = 0; j < 2; j++) {
	R1[j] = catalog[0].average[N].R - catalog[0].measure[m+j].dR/360000.0;
	D1[j] = catalog[0].average[N].D - catalog[0].measure[m+j].dD/360000.0;
	RD_to_XY (&X1[j], &Y1[j], R1[j], D1[j], &catstats[0].coords);
	T1[j] = catalog[0].measure[m+j].t;
	M1[j] = catalog[0].measure[m+j].M - catalog[0].measure[m+j].McalPSF;
      }
      dt = T1[1] - T1[0];
      ax = (X1[1] - X1[0])/dt;
      cx = (X1[0]*T1[1] - X1[1]*T1[0])/dt;
      ay = (Y1[1] - Y1[0])/dt;
      cy = (Y1[0]*T1[1] - Y1[1]*T1[0])/dt;
      speed = hypot (ax, ay);
      for (k = 0; k < Nobject; k++) {
	N = N1[k];
	Nm = catalog[0].average[N].Nm;
	m = catalog[0].average[N].offset;
	for (j = 0; j < Nm; j++) {
	  R = catalog[0].average[N].R - catalog[0].measure[m+j].dR/360000.0;
	  D = catalog[0].average[N].D - catalog[0].measure[m+j].dD/360000.0;
	  RD_to_XY (&X, &Y, R, D, &catstats[0].coords);
	  T = catalog[0].measure[m+j].t;
	  M = catalog[0].measure[m+j].M - catalog[0].measure[m+j].McalPSF;
	  if (T1[0] == T) continue;
	  if (T1[1] == T) continue;
	  dX1 = T*ax - X + cx;
	  dX2 = T*ay - Y + cy;
	  distance = hypot (dX1, dX2);
	  if (distance < RADIUS){
	    catalog[0].average[N1[i]].code |= ID_ROCK;
	    catalog[0].average[N1[k]].code |= ID_ROCK;
	    rocks[Nrocks].ra    = R1[0];
	    rocks[Nrocks].dec   = D1[0];
	    rocks[Nrocks].mag   = M1[0];
	    rocks[Nrocks].t     = T1[0];
	    rocks[Nrocks+1].ra  = R1[1];
	    rocks[Nrocks+1].dec = D1[1];
	    rocks[Nrocks+1].mag = T1[1];
	    rocks[Nrocks+1].t   = M1[1];
	    rocks[Nrocks+2].ra  = R;
	    rocks[Nrocks+2].dec = D;
	    rocks[Nrocks+2].mag = M;
	    rocks[Nrocks+2].t   = T;
	    fprintf (stderr, "%f %f %f %d   %f\n", rocks[Nrocks].ra, rocks[Nrocks].dec, rocks[Nrocks].mag, rocks[Nrocks].t, speed);
	    Nrocks += 3;
	    if (Nrocks > NROCKS - 3) {
	      NROCKS += 3000;
	      REALLOCATE (rocks, Rocks, NROCKS);
	    }
	  }
	}
      }
    }
  }

  fprintf (stderr, "found %d rocks\n", Nrocks);

  free (R1);
  free (D1);
  free (M1);
  free (T1);
  free (X1);
  free (Y1);
  free (N1);

  REALLOCATE (rocks, Rocks, Nrocks);
  *nrocks = Nrocks;
  return (rocks);

}
