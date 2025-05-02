# include "markrock.h"

wrocks (Rocks *rocks, int Nrocks) {
  
  int i, j;
  FILE *f;
  double X, Y, t, dSx, dSy, dS, speed;
  double Sx, Sy, Sxt, Syt, St, St2, Sn;
  double Mx, Bx, My, By, D;
  unsigned int Tref;

  f = fopen (RockCat, "a+");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't create/open rock catalog file: %s\n", RockCat);
    exit (0);
  }
  /* position to begining of file to write header */
  fseeko (f, 0, SEEK_END);

  /* get statistics on rocks */

  for (i = 0; i< Nrocks; i++) {
    /* fit a line to the three points, the ask for the scatter about the solution */
    Tref = rocks[i].t[0];
    Sx = Sy = Sxt = Syt = St = St2 = Sn = 0;
    for (j = 0; j < 3; j++) {
      X = rocks[i].X[j];
      Y = rocks[i].Y[j];
      if (rocks[i].t[j] > Tref) 
	t = rocks[i].t[j] - Tref;
      else
	t = -1*((double)(Tref - rocks[i].t[j]));
      Sx  += X;
      Sy  += Y;
      Sxt += X*t;
      Syt += Y*t;
      St  += t;
      St2 += t*t;
      Sn  += 1;
    }
    D = St2*Sn - St*St;
    My = (Syt*Sn - Sy*St) / D;
    By = (Sy*St2 - Syt*St) / D;
    Mx = (Sxt*Sn - Sx*St) / D;
    Bx = (Sx*St2 - Sxt*St) / D;
    
    dS = 0;
    for (j = 0; j < 3; j++) {
      X = rocks[i].X[j];
      Y = rocks[i].Y[j];
      if (rocks[i].t[j] > Tref) 
	t = rocks[i].t[j] - Tref;
      else 
	t = -1*((double)(Tref - rocks[i].t[j]));
      dSx = (Mx*t + Bx - X);
      dSy = (My*t + By - Y);
      dS += dSx*dSx + dSy*dSy;
    }
    dS = sqrt (dS/3);
    speed = hypot (Mx, My);
    fprintf (f, "%5.2f %9.4e ", dS, speed);
    for (j = 0; j < 3; j++) {
      fprintf (f, "%10d %10.6f %10.6f %6.3f ", rocks[i].t[j], rocks[i].ra[j], rocks[i].dec[j], rocks[i].mag[j]);
    }
    /* fprintf (f, "%3d %3d %3d\n", rocks[i].N[0], rocks[i].N[1], rocks[i].N[2]); */
    fprintf (f, "\n");
  }
  fclose (f);
  

}
