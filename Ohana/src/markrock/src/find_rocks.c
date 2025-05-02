# include "markrock.h"

Rocks *find_rocks (Catalog *catalog, CatStats *catstats, int *nrocks) {

  int i, j, k, n, m, N, M, first_j, NOBJECT, Nobject, Nrocks, NROCKS;
  double X, Y, RADIUS2;
  double *X1, *Y1;
  double dX, dY, dR;
  float dtime;
  int *N1, *N0;
  unsigned int Tref;
  int *T1, Ttmp;
  int Nstar, Nghost, Ng;
  unsigned int flags;
  unsigned short NotRock, IsProper;
  Coords *tcoords;
  double ax, ay, cx, cy, dt, dt1, dt2, dt3, dX1, dX2, speed, distance;
  Rocks *rocks;
  struct timeval now, then;  
  double MaxSeparation;

  gettimeofday (&then, (void *) NULL);

  Nstar = catalog[0].Naverage;

  NOBJECT = 1000;
  ALLOCATE (X1, double, NOBJECT);
  ALLOCATE (Y1, double, NOBJECT);
  ALLOCATE (T1, int,   NOBJECT);
  ALLOCATE (N1, int,   NOBJECT);
  Nobject = 0;
  N0 = catstats[0].N;

  NROCKS = 3000;
  ALLOCATE (rocks, Rocks, NROCKS);
  Nrocks = 0;

  Tref = 0;
  NotRock = (ID_ROCK ^ 0xffff);
  IsProper = ID_USNO | ID_PROPER;
  for (i = 0; i < Nstar; i++) {
    N = N0[i];
    if (RESET && ((catalog[0].average[N].code & ID_ROCK) == ID_ROCK)) {
      /* resets both relevant bits */
      catalog[0].average[N].code &= NotRock;
    }
    if (RESET && (catalog[0].average[N].code == ID_COSMIC)) {
      /* resets both relevant bits */
      catalog[0].average[N].code = 0;
    }
    /* accept a measurement if it is either unmarked or marked as a proper motion star */
    if ((catalog[0].average[N].Nm == 1) && (catalog[0].average[N].Nn > 0) && ((catalog[0].average[N].code == 0) || ((catalog[0].average[N].code & IsProper) == IsProper))) {
      if (Tref == 0) Tref = catalog[0].measure[catalog[0].average[N].offset].t;
      X1[Nobject] = catstats[0].X[i];
      Y1[Nobject] = catstats[0].Y[i];
      if (catalog[0].measure[catalog[0].average[N].offset].t > Tref) 
	T1[Nobject] = catalog[0].measure[catalog[0].average[N].offset].t - Tref;
      else {
	Ttmp = Tref - catalog[0].measure[catalog[0].average[N].offset].t;
	T1[Nobject] = -Ttmp;
      }      
      N1[Nobject] = N;
      Nobject ++;
      if (Nobject == NOBJECT - 1) {
	NOBJECT += 1000;
	REALLOCATE (X1, double, NOBJECT);
	REALLOCATE (Y1, double, NOBJECT);
	REALLOCATE (T1, int, NOBJECT);
	REALLOCATE (N1, int,   NOBJECT);
      }
    }
  }
  REALLOCATE (X1, double, Nobject);
  REALLOCATE (Y1, double, Nobject);
  REALLOCATE (T1, int, Nobject);
  REALLOCATE (N1, int,   Nobject);
  if (Nobject > 1) sort_set (X1, Y1, T1, N1, Nobject);

  fprintf (stderr, "Nobj: %d\n", Nobject);

  MaxSeparation = MAX_SPEED * MAX_DELAY;
  fprintf (stderr, "%f %f %f %f %f\n", MaxSeparation, MAX_RADIUS, RADIUS, MAX_SPEED, MAX_DELAY);

  /** find 3 rocks in a line **/
  for (i = 0; i < Nobject - 2; i++) {
    fprintf (stderr, ".");
    for (j = i + 1; j < Nobject - 1; j++) {
      if (X1[j] - X1[i] > MaxSeparation) goto NEXTi;
      if (X1[i] - X1[j] > MaxSeparation) goto NEXTj;
      if (T1[i] == T1[j]) continue;
      dt = (int)(T1[j] - T1[i]);
      if (fabs(dt) > MAX_DELAY) continue;
      ax = (X1[j] - X1[i])/dt;
      cx = (X1[i]*T1[j] - X1[j]*T1[i])/dt;
      ay = (Y1[j] - Y1[i])/dt;
      cy = (Y1[i]*T1[j] - Y1[j]*T1[i])/dt;
      speed = hypot (ax, ay);
      if (speed > MAX_SPEED) continue;
      for (k = j + 1; k < Nobject; k++) {
	if (X1[k] - X1[j] > MaxSeparation) goto NEXTj;
	if (X1[j] - X1[k] > MaxSeparation) goto NEXTk;
	if (T1[i] == T1[k]) continue;
	if (T1[j] == T1[k]) continue;
	dt1 = fabs(dt);
	dt2 = fabs ((int)(T1[i] - T1[k]));
	dt3 = fabs ((int)(T1[j] - T1[k]));
	dt1 = MIN(MIN(dt1,dt2),dt3);
	dt3 = MAX(MAX(dt1,dt2),dt3);
	if (fabs(dt3) > MAX_DELAY) continue;
	/* errors are too large by this point */
	if (RADIUS * dt3 / dt1 > MAX_RADIUS) continue; 
	dX1 = T1[k]*ax - X1[k] + cx;
	dX2 = T1[k]*ay - Y1[k] + cy;
	distance = hypot (dX1, dX2);
	/*
        fprintf (stderr, "%d %d %d %f %f %d %f %f\n", i, j, k, ax, cx, T1[k], X1[k], dX1);
	if (k > 100) exit (0);
	*/
	if (distance < RADIUS){
	  catalog[0].average[N1[i]].code |= ID_ROCK;
	  catalog[0].average[N1[j]].code |= ID_ROCK;
	  catalog[0].average[N1[k]].code |= ID_ROCK;

	  rocks[Nrocks].X[0]     = X1[i];
	  rocks[Nrocks].Y[0]     = Y1[i];
	  rocks[Nrocks].ra[0]    = catalog[0].average[N1[i]].R;
	  rocks[Nrocks].dec[0]   = catalog[0].average[N1[i]].D;

	  m = catalog[0].average[N1[i]].offset;
	  rocks[Nrocks].mag[0]   = PhotRel (&catalog[0].measure[m], &photcodes, ZERO_POINT);
	  rocks[Nrocks].t[0]     = catalog[0].measure[m].t;

	  rocks[Nrocks].X[1]     = X1[j];
	  rocks[Nrocks].Y[1]     = Y1[j];
	  rocks[Nrocks].ra[1]    = catalog[0].average[N1[j]].R;
	  rocks[Nrocks].dec[1]   = catalog[0].average[N1[j]].D;

	  m = catalog[0].average[N1[j]].offset;
	  rocks[Nrocks].mag[1]   = PhotRel (&catalog[0].measure[m], &photcodes, ZERO_POINT);
	  rocks[Nrocks].t[1]     = catalog[0].measure[m].t;

	  rocks[Nrocks].X[2]     = X1[k];
	  rocks[Nrocks].Y[2]     = Y1[k];
	  rocks[Nrocks].ra[2]    = catalog[0].average[N1[k]].R;
	  rocks[Nrocks].dec[2]   = catalog[0].average[N1[k]].D;

	  m = catalog[0].average[N1[k]].offset;
	  rocks[Nrocks].mag[2]   = PhotRel (&catalog[0].measure[m], &photcodes, ZERO_POINT);
	  rocks[Nrocks].t[2]     = catalog[0].measure[m].t;

	  fprintf (stderr, "*");
	  Nrocks ++;
	  if (Nrocks >= NROCKS) {
	    NROCKS += 3000;
	    REALLOCATE (rocks, Rocks, NROCKS);
	  }
	}
      NEXTk:
      }
    NEXTj:
    }
    gettimeofday (&now, (void *) NULL);
    dtime = (now.tv_sec - then.tv_sec) + 1e-6*(now.tv_usec - then.tv_usec);
    if (dtime > 200) {
      fprintf (stderr, "taking too long to find all rocks, giving up...\n");
      goto escape_loop;
    }
  NEXTi:
  }

escape_loop:
  for (i = 0; i < Nobject; i++) {
    if ((catalog[0].average[N1[i]].code == 0) && (catalog[0].average[N1[i]].Nn >= 2)) {
      catalog[0].average[N1[i]].code = ID_COSMIC;
    }
  }

  fprintf (stderr, "found %d rocks\n", Nrocks);

  free (T1);
  free (X1);
  free (Y1);
  free (N1);

  REALLOCATE (rocks, Rocks, MAX (1, Nrocks));
  *nrocks = Nrocks;
  return (rocks);

}
