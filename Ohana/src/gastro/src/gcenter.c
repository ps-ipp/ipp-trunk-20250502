# include "gastro.h"
# define NSIGMA 2.0

/* stars1.X,Y and stars2.X,Y are in image pixels 
   stars2 (ref catalog) is approx, based on guess for scale */

int gcenter (SStars *stars1in, SStars *stars2, int N1, int N2, Coords *coords, int NX, int NY, double *dR) {

  double mean, sigma, gx, gy, gx0, gy0, n, SearchRadius;
  int NPIX, minN, Nmin0;
  int i, j, k;
  double *N, *DX, *DY, *D2, *tX1, *tY1, *tX2, *tY2;
  double rot, Rot, Smin, Smin0, s, Fmin, Fmin0, f;
  double Xmin, Xmin0, Ymin, Ymin0;
  double RA, DEC, RAo, DECo;
  double dX, dY, refX, refY;
  double cs, sn;
  double dR1, dD1, dR2, dD2, d1, d2;
  double *sx1, *sy1, *sx2, *sy2;
  SStars *stars1;  
  char c;
  Graphdata graphdata;
  float *xvect, *yvect;
  int Nvect, NVECT;
  
  SearchRadius = NX * SEARCH_RADIUS;
  xvect = yvect = NULL;
  Nvect = 0;

  if (PLOTSTUFF) {
    NVECT = N1*N2;
    ALLOCATE (xvect, float, NVECT);
    ALLOCATE (yvect, float, NVECT);
    graphdata.xmin = 0;
    graphdata.xmax = 2000;
    graphdata.ymin = 0;
    graphdata.ymax =  4000;
    graphdata.style = 2;
    graphdata.ptype = 2;
    graphdata.ltype = 0;
    graphdata.etype = 0;
    graphdata.color = 0;
    graphdata.lweight = 0;
    graphdata.size = 0.5;
  }
 
  /*  NPIX = MAX (300, sqrt (20*(N1*N2) / MIN (N1, N2))); */
  /* NPIX = 10 * sqrt (N2); */
  NPIX = 300;
  mean = 2 * N1*N2 / (NPIX*NPIX);
  sigma = sqrt (mean);
  minN =  MAX (6, mean + NSIGMA*sigma);
  fprintf (stderr, "N1: %d, N2: %d, minN: %d\n", N1, N2, minN);

  ALLOCATE (N,    double, NPIX*NPIX);
  ALLOCATE (DX,   double, NPIX*NPIX);
  ALLOCATE (DY,   double, NPIX*NPIX);
  ALLOCATE (D2,   double, NPIX*NPIX);

  ALLOCATE (stars1, SStars, N1);
  for (i = 0; i < N1; i++) {
    stars1[i] = stars1in[i];
  }
  
  if (PLOTSTUFF) {
    for (i = 0; i < N1; i++) {
      xvect[Nvect] = stars1in[i].X;
      yvect[Nvect] = stars1in[i].Y;
      Nvect ++;
    }
  }
  if (PLOTSTUFF) {
    PlotReset (0);
    PrepPlotting (Nvect, &graphdata, 0);
    PlotVector (Nvect, xvect, 0, 0);
    PlotVector (Nvect, yvect, 1, 0);
    DonePlotting (&graphdata, 0);
    usleep (300000);
    fprintf (stderr, "plotting %d points\n", Nvect);
    fprintf (stderr, "type return to continue");
    if (fscanf (stdin, "%c", &c) != 1) {
      fprintf (stderr, "\n");
    }
    Nvect = 0;
  }

  Xmin = Ymin = 0;
  Fmin = Smin = 1000000.0;
  rot = Rot = ROT_ZERO-dROT*NROT;
  rotate (stars1, N1, Rot, (int)coords[0].crpix1, (int)coords[0].crpix2); 

  graphdata.xmin = -200;
  graphdata.xmax =  200;
  graphdata.ymin = -200;
  graphdata.ymax =  200;

  ALLOCATE (tX1, double, N1);
  ALLOCATE (tY1, double, N1);
  ALLOCATE (tX2, double, N2);
  ALLOCATE (tY2, double, N2);
  for (i = 0; i < N2; i++) {
    tX2[i] = stars2[i].X;
    tY2[i] = stars2[i].Y;
  }
  for (Rot = ROT_ZERO-dROT*NROT; Rot <= ROT_ZERO+dROT*NROT; Rot += dROT) {
    granges (stars1, stars2, N1, N2, NPIX, &gx, &gy, &gx0, &gy0);
    bzero (N,   NPIX*NPIX*sizeof(double));
    bzero (DX,  NPIX*NPIX*sizeof(double));
    bzero (DY,  NPIX*NPIX*sizeof(double));
    bzero (D2,  NPIX*NPIX*sizeof(double));
    for (i = 0; i < N1; i++) {
      tX1[i] = stars1[i].X;
      tY1[i] = stars1[i].Y;
    }
    sx1 = tX1; sy1 = tY1;
    for (i = 0; i < N1; i++, sx1++, sy1++) {
      sx2 = tX2; sy2 = tY2;
      for (j = 0; j < N2; j++, sx2++, sy2++) {
	dX = *sx1 - *sx2;
	dY = *sy1 - *sy2;
	if (hypot (dX, dY) > SearchRadius) continue;
	if (PLOTSTUFF) {
	  xvect[Nvect] = stars1[i].X - stars2[j].X;
	  yvect[Nvect] = stars1[i].Y - stars2[j].Y;
	  Nvect ++;
	}
	k = NPIX*(int)(gx*dX+gx0) + (int)(gy*dY+gy0);
	N[k]   += 1.0;
	DX[k]  += dX;
	DY[k]  += dY;
	D2[k]  += dX*dX + dY*dY;
      }
    }

    Fmin0 = 1000.0;
    Smin0 = Nmin0 = Xmin0 = Ymin0 = 0;
    for (k = 0; k < NPIX*NPIX; k++) {
      n = N[k]; /* 1*/ 
      if (n < minN)
	continue;
      s = D2[k] - (SQ(DX[k]) + SQ(DY[k])) / n; 
      f = s / (pow(n,4.0)); /* = 12 ops */
      
      if (f < Fmin0) { 
	Fmin0 = f;
	Smin0 = s;
	Nmin0 = n;
	Xmin0 = DX[k] / n;
	Ymin0 = DY[k] / n;
      }
    }
    if (VERBOSE) fprintf (stderr, "best offset: %7.1f %7.1f at %.1f deg  (%f %f %d)\n", Xmin0, Ymin0, Rot, Fmin0, sqrt(Smin0), Nmin0);
    if (Fmin0 < Fmin) {
      Fmin = Fmin0;
      Smin = Smin0;
      Xmin = Xmin0;
      Ymin = Ymin0;
      rot  = Rot;
    }
    if (PLOTSTUFF) {
      PlotReset (0);
      PrepPlotting (Nvect, &graphdata, 0);
      PlotVector (Nvect, xvect, 0, 0);
      PlotVector (Nvect, yvect, 1, 0);
      DonePlotting (&graphdata, 0);
      usleep (300000);
      fprintf (stderr, "plotting %d points\n", Nvect);
      fprintf (stderr, "type return to continue");
      if (fscanf (stdin, "%c", &c) != 1) {
	fprintf (stderr, "\n");
      }
      Nvect = 0;
    }
    
    rotate (stars1, N1, dROT, (int)coords[0].crpix1, (int)coords[0].crpix2);
  }
  rotate (stars1, N1, -ROT_ZERO-dROT*NROT, (int)coords[0].crpix1, (int)coords[0].crpix2);

  free (N);
  free (DX);
  free (DY);
  free (D2);

  /* dx = dy = 10 pix */
  refX = coords[0].crpix1 - Xmin;
  refY = coords[0].crpix2 - Ymin;
  XY_to_RD (&RA, &DEC, refX, refY, coords);
  XY_to_RD (&RAo, &DECo, (refX + 10), refY, coords);
  dR1 = (RAo - RA)*cos(DEC*RAD_DEG);
  dD1 = (DECo - DEC);
  XY_to_RD (&RAo, &DECo, refX, (refY + 10), coords);
  dR2 = (RAo - RA)*cos(DEC*RAD_DEG);
  dD2 = (DECo - DEC);
  d1 = coords[0].cdelt1;  d2 = coords[0].cdelt2;
  cs = cos(RAD_DEG*rot);  sn = sin(RAD_DEG*rot);

  coords[0].pc1_1 =  cs*dR1 / (10*d1) + sn*dR2 / (10*d1);    coords[0].pc1_2 = cs*dR2 / (10*d1) - sn*dR1 / (10*d1);
  coords[0].pc2_1 =  cs*dD1 / (10*d2) + sn*dD2 / (10*d2);    coords[0].pc2_2 = cs*dD2 / (10*d2) - sn*dD1 / (10*d2);
  coords[0].crval1 = RA;
  coords[0].crval2 = DEC;

  /* diameter of 1 pixel box */
  *dR = 1*sqrt(Smin);

  fprintf (stderr, "%f x %f, %f\n", 1/gx, 1/gy, *dR);
  if (VERBOSE) fprintf (stderr, "using: %7.1f %7.1f at %.1f deg\n", Xmin, Ymin, rot);

# if (0)
  if (VERBOSE) {
    fprintf (stderr, "%s\n", coords[0].ctype);
    fprintf (stderr, "%f %f\n", coords[0].crval1, coords[0].crval2);
    fprintf (stderr, "%f %f\n", coords[0].crpix1, coords[0].crpix2);
    fprintf (stderr, "%f %f\n", coords[0].pc1_1, coords[0].pc1_2);
    fprintf (stderr, "%f %f\n", coords[0].pc2_1, coords[0].pc2_2);
    fprintf (stderr, "%f %f\n", coords[0].cdelt1, coords[0].cdelt2);
  }
# endif

  DEFAULT_RADIUS = MAX ((5*NX / NPIX), DEFAULT_RADIUS);
  return (TRUE);
}

  /* we now have Xmin, Ymin, rot, get coords in unrotate stars1 frame of correct crref 
  RAo = coords[0].crval1;
  DECo = coords[0].crval2;

  cs = cos(RAD_DEG*rot);  sn = sin(RAD_DEG*rot);
  refX = coords[0].crpix1;
  refY = coords[0].crpix2;

  coords[0].crpix1 =  Xmin*cs + Ymin*sn + refX;
  coords[0].crpix2 = -Xmin*sn + Ymin*cs + refY;

  xx = coords[0].pc1_1; xy = coords[0].pc1_2; 
  yx = coords[0].pc2_1; yy = coords[0].pc2_2; 
  coords[0].pc1_1 =  cs*xx + sn*xy;    coords[0].pc1_2 = cs*xy - sn*xx;
  coords[0].pc2_1 =  cs*yx + sn*yy;    coords[0].pc2_2 = cs*yy - sn*yx;

  XY_to_RD (&RA, &DEC, refX, refY, coords);
  if (fabs(RAo - RA) > 90) {
    RA = (RA > 180.0) ? (RA - 180) : (RA + 180);
    DEC = (DEC > 0.0) ? (180.0 - DEC) : (-180.0 - DEC);
  }
  coords[0].crval1 = RA;
  coords[0].crval2 = DEC;
  coords[0].crpix1 = refX;
  coords[0].crpix2 = refY;

  Rot = RAo - RA;
  cs = cos(RAD_DEG*rot);  sn = sin(RAD_DEG*rot);
  xx = coords[0].pc1_1; xy = coords[0].pc1_2; 
  yx = coords[0].pc2_1; yy = coords[0].pc2_2; 
  coords[0].pc1_1 =  cs*xx + sn*xy;    coords[0].pc1_2 = cs*xy - sn*xx;
  coords[0].pc2_1 =  cs*yx + sn*yy;    coords[0].pc2_2 = cs*yy - sn*yx;

  

*/
