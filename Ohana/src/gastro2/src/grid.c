# include "gastro2.h"

/* this needs to be user-configured */
static int NX, NY, Nbin;
static double C0x, C1x, C0y, C1y;
static double *N, *DX, *DY, *D2;

void grid (CmpCatalog *Target, RefCatalog *Subset, Answer *answer) {

  int i, j, n, Imin, Nmin, Nval;
  double XMIN, XMAX, YMIN, YMAX;
  double dX, dY;
  double s, f, Fmin, Smin, *ntmp;

  RefCatalog Ref;
  StarData *st, *sr;

  /* Ref is temporary in this function.  Free Ref.stars before exiting */
  rotate (Subset, &Ref, answer[0].angle);

  st = Target[0].stars;
  sr = Ref.stars;
  
  /* NFIELD represents the search box; it is also the extra padding used 
     to select the reference stars */
  XMIN = -0.5*NFIELD*Target[0].header.Naxis[0];
  XMAX = +0.5*NFIELD*Target[0].header.Naxis[0];
  YMIN = -0.5*NFIELD*Target[0].header.Naxis[1];
  YMAX = +0.5*NFIELD*Target[0].header.Naxis[1];

  dX = dY = 0;
  /* make two passes, with grids offset by 0.5 box for the second */
  for (n = 0; n < 2; n++) {
    gridinit (XMIN, XMAX, YMIN, YMAX, Ref.N, Target[0].N);
    
    if (PLOTSTUFF) plot_init_gridplot ();
    /* fill in grid points */
    for (i = 0; i < Target[0].N; i++) {
      for (j = 0; j < Ref.N; j++) {
	
	dX = st[i].X - sr[j].X;
	if (dX < XMIN) continue;
	if (dX > XMAX) continue;
	
	dY = st[i].Y - sr[j].Y;
	if (dY < YMIN) continue;
	if (dY > YMAX) continue;
	
	gridbin (dX, dY);
	if (PLOTSTUFF) plot_addpt_gridplot (dX, dY);
      }
    }

    /* use sorted N list to define Nmin cut */
    ALLOCATE (ntmp, double, Nbin);
    bcopy (N, ntmp, Nbin*sizeof(double));
    dsort (ntmp, Nbin);
    for (i = 0; (ntmp[i] == 0) && (i < Nbin); i++);
    Nval = MIN (Nbin - 1, (int)(0.75*(Nbin - i)) + i);
    Nmin = ntmp[Nval];
    free (ntmp);
    
    /* select 'best' grid point - is this statistic good enough? */
    Fmin = 1e10;
    Imin = -1;
    for (i = 0; i < Nbin; i++) {
      
      if (N[i] < Nmin) continue;
      
      /* s is the variance, f is variance overweighted by number */
      s = fabs ((D2[i]/N[i]) - SQ(DX[i]/N[i]) - SQ(DY[i]/N[i]));
      f = s / SQ(SQ(N[i]));
      
      if (f < Fmin) {
	Smin = s;
	Fmin = f;
	Imin = i;
      }
    }
    if (Imin == -1) { 
      fprintf (stderr, "ERROR: odd min value\n");
      exit (1);
    }
    
    if ((n == 0) || (Fmin < answer[0].Chi)) {
      Smin = fabs ((D2[Imin]/N[Imin]) - SQ(DX[Imin]/N[Imin]) - SQ(DY[Imin]/N[Imin]));
      answer[0].Xoff = DX[Imin] / N[Imin];
      answer[0].Yoff = DY[Imin] / N[Imin];
      answer[0].dR   = sqrt (Smin);
      answer[0].Chi  = Fmin;
      answer[0].N    = N[Imin];
    }
    
    fprintf (stderr, "angle: %6.1f, (%6.1f,%6.1f) - %6.2f : %10.8f for %d pairs\n", 
	     answer[0].angle, answer[0].Xoff, answer[0].Yoff, answer[0].dR, answer[0].Chi, answer[0].N);

    if (PLOTSTUFF) plot_gridpts (N, Nbin);
    if (PLOTSTUFF) plot_done_gridplot ();

    XMIN -= 0.5*NGRID_PIX;
    XMAX -= 0.5*NGRID_PIX;
    YMIN -= 0.5*NGRID_PIX;
    YMAX -= 0.5*NGRID_PIX;
    gridfree ();
  }

  free (Ref.stars);

}

void gridinit (double XMIN, double XMAX, double YMIN, double YMAX, int Nr, int Nt) {

  NX = (XMAX - XMIN) / NGRID_PIX;
  NY = (YMAX - YMIN) / NGRID_PIX;

  C1x =          NX / (XMAX - XMIN); 
  C0x = - XMIN * NX / (XMAX - XMIN);

  C1y =          NY / (YMAX - YMIN); 
  C0y = - YMIN * NY / (YMAX - YMIN);

  Nbin = NX*NY;

  ALLOCATE (N, double, Nbin);
  ALLOCATE (DX, double, Nbin);
  ALLOCATE (DY, double, Nbin);
  ALLOCATE (D2, double, Nbin);

  bzero (N,  Nbin*sizeof(double));
  bzero (DX, Nbin*sizeof(double));
  bzero (DY, Nbin*sizeof(double));
  bzero (D2, Nbin*sizeof(double));

}

int gridbin (double dX, double dY) {

  int bin, xbin, ybin;

  xbin = (int) (C0x + dX * C1x);
  ybin = (int) (C0y + dY * C1y);

  bin =  xbin + NX * ybin;

  if (bin < 0)     return (0);
  if (bin >= Nbin) return (0);

  N[bin]   += 1.0;
  DX[bin]  += dX;
  DY[bin]  += dY;
  D2[bin]  += dX*dX + dY*dY;
  
  return (bin);

}
  
void gridfree () {

  free (N);
  free (DX);
  free (DY);
  free (D2);

}

# if (0) 
  /* use sorted N list to define Nmin cut */
  ALLOCATE (ntmp, double, Nbin);
  bcopy (N, ntmp, Nbin*sizeof(double));
  dsort (ntmp, Nbin);
  for (i = 0; (ntmp[i] == 0) && (i < Nbin); i++);
  Nval = MIN (Nbin - 1, (int)(0.75*(Nbin - i)) + i);
  Nmin = ntmp[Nval];
  free (ntmp);

  /* select 'best' grid point - is this statistic good enough? */
  Fmin = 1e10;
  Imin = -1;
  for (i = 0; i < Nbin; i++) {

    if (N[i] < Nmin) continue;

    /* s is the variance, f is variance overweighted by number */
    s = fabs ((D2[i]/N[i]) - SQ(DX[i]/N[i]) - SQ(DY[i]/N[i]));
    f = s / SQ(SQ(N[i]));

    if (f < Fmin) {
      Smin = s;
      Fmin = f;
      Imin = i;
    }
  }
  if (Imin == -1) { 
    fprintf (stderr, "ERROR: odd min value\n");
    exit (1);
  }
# endif

# if (0)
    /* find N sigma */
    Ns = Ns2 = 0;
    for (i = 0; i < Nbin; i++) {
      Ns += N[i];
      Ns2 += N[i]*N[i];
    }
    Ns = Ns / Nbin;
    Ns2 = sqrt (Ns2 / Nbin - Ns*Ns);
    fprintf (stderr, "N sigma: %f\n", Ns2);
    
    Imin = 0;
    Fmin = N[0] / Ns2;
    for (i = 0; i < Nbin; i++) {
      f = N[i] / Ns2;
      if (f > Fmin) {
	Fmin = f;
	Imin = i;
      }
    }
# endif
