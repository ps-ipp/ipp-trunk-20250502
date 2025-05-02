# include "gastro.h"

/* stars1.X,Y and stars2.X,Y are both in image pixels. 
   The conversion terms (X_O, X_X, etc) make linear 
   corrections in pixel coordinates */

int gfit (SStars *stars1, SStars *stars2, int N1, int N2, Coords *coords, int NX, int NY, double *Radius, double *DR, int *Nmatch, int mode) {
  
  int i, j, iteration, Niter, last, halt, first_j, extras;
  off_t *tmpN1, *tmpN2;
  double X_O, X_X, X_Y, dX;
  double Y_O, Y_X, Y_Y, dY;
  double x, y, x2, y2, xy, N, R, wt;
  double X, Y, Xx, Yy, Xy, Yx;
  double Dx, Dy, DD, d2X, d2Y;
  double RA, DEC, radius, radius2, fratio;
  double *tmpX1, *tmpX2, *tmpY1, *tmpY2;
  double tX1, tX2, tY1, tY2;
  double dS;

  char c;
  Graphdata graphdata;
  float *xvect, *yvect;
  float *xvect2, *yvect2;
  int Nvect, NVECT;
  int Nvect2, NVECT2;
  
  Nvect = NVECT = Nvect2 = NVECT2 = 0;
  xvect = yvect = xvect2 = yvect2 = NULL;

  if (PLOTSTUFF) {
    NVECT2 = MAX(N1, N2);
    NVECT = MAX(N1, N2);
    ALLOCATE (xvect, float, NVECT);
    ALLOCATE (yvect, float, NVECT);
    ALLOCATE (xvect2, float, NVECT2);
    ALLOCATE (yvect2, float, NVECT2);
  }
    

  /* allocate space for star coords */
  ALLOCATE (tmpX1, double, N1);
  ALLOCATE (tmpY1, double, N1);
  ALLOCATE (tmpN1, off_t, N1);
  
  ALLOCATE (tmpX2, double, N2);
  ALLOCATE (tmpY2, double, N2);
  ALLOCATE (tmpN2, off_t, N2);

  dX = dY = N = 0;

  /* assign and sort list */
  for (i = 0; i < N1; i++) {
    tmpX1[i] = stars1[i].X;
    tmpY1[i] = stars1[i].Y;
    tmpN1[i] = i;
  }
  if (N1 > 1) sort_coords_index (tmpX1, tmpY1, tmpN1, N1);

   
  /* choose iteration ranges */
  fratio = 1.41421;
  extras = halt = last = FALSE;
  radius = *Radius;
  Niter = 2 + log (radius/MINIMUM_RADIUS) / log (fratio);
  
  /* initial values for fit coeffs */
  X_X = 1; X_Y = 0; X_O = 0;
  Y_X = 0; Y_Y = 1; Y_O = 0;
  
  for (iteration = 0; iteration < Niter; iteration ++) {

    if (iteration >= Niter - 1) { /* next loop is the last one */
      radius *= fratio;
      last = TRUE;
    }

    /* setup and define */
    radius2 = radius*radius;
    dX = dY = d2X = d2Y = x = y = x2 = y2 = xy = X = Y = Xx = Xy = Yx = Yy = N = R = 0;
    for (i = 0; i < N2; i++) {
      tmpX2[i] = (X_O) + (X_X)*stars2[i].X + (X_Y)*stars2[i].Y;
      tmpY2[i] = (Y_O) + (Y_X)*stars2[i].X + (Y_Y)*stars2[i].Y;
      tmpN2[i] = i;
    }
    
    if (PLOTSTUFF) {
      for (i = 0; i < N1; i++) {
	xvect2[i] = tmpX1[i];
	yvect2[i] = tmpY1[i];
      }
      Nvect2 = N1;

      graphdata.xmin = 0;
      graphdata.xmax = 2000;
      graphdata.ymin = 4000;
      graphdata.ymax =  0;
      graphdata.style = 2;
      graphdata.ptype = 3;
      graphdata.ltype = 0;
      graphdata.etype = 0;
      graphdata.color = 0;
      graphdata.lweight = 0;
      graphdata.size = 1.5;
    
      PlotReset (1);
      PrepPlotting (Nvect2, &graphdata, 1);
      PlotVector (Nvect2, xvect2, 0, 1);
      PlotVector (Nvect2, yvect2, 1, 1);
      DonePlotting (&graphdata, 1);
      Nvect2 = 0;

      for (i = 0; i < N2; i++) {
	xvect2[i] = tmpX2[i];
	yvect2[i] = tmpY2[i];
      }
      Nvect2 = N2;

      graphdata.ptype = 1;

      PrepPlotting (Nvect2, &graphdata, 1);
      PlotVector (Nvect2, xvect2, 0, 1);
      PlotVector (Nvect2, yvect2, 1, 1);
      DonePlotting (&graphdata, 1);
      Nvect2 = 0;
    }

    if (N2 > 1) sort_coords_index (tmpX2, tmpY2, tmpN2, N2);
    
    /* find matched stars */
    for (i = j = 0; (i < N1) && (j < N2); ) {  
      tX1 = tmpX1[i];
      tX2 = tmpX2[j];
      Dx = tX1 - tX2;
      if (Dx <= -2.0*radius) {
	i++;
	continue;
      }
      if (Dx >= 2.0*radius) {
	j++;
	continue;
      }

      /**** possible improvement: find only the closest match 
	    for stars that have more than one (save DD and i for each
            cat star j */
      /* in the right range */
      first_j = j;
      for (; (Dx > -2.0*radius) && (j < N2); j++) {
	tY1 = tmpY1[i];
	tX2 = tmpX2[j];
	tY2 = tmpY2[j];
	Dx = tX1 - tX2;
	Dy = tY1 - tY2;
	DD = Dx*Dx + Dy*Dy;
	/* stars matched */
	if (DD < radius2) {
	  if (PLOTSTUFF) {
	    xvect[Nvect] = Dx;
	    yvect[Nvect] = tmpY1[i];
	    Nvect ++;
	    if (Nvect == NVECT) {
	      NVECT += 100;
	      REALLOCATE (xvect, float, NVECT);
	      REALLOCATE (yvect, float, NVECT);
	    }
	    xvect2[Nvect2] = tmpX1[i];
	    yvect2[Nvect2] = tmpY1[i];
	    Nvect2 ++;
	    if (Nvect2 == NVECT2 - 1) {
	      NVECT2 += 100;
	      REALLOCATE (xvect2, float, NVECT2);
	    REALLOCATE (yvect2, float, NVECT2);
	    }
	  }
	  /* wt = sqrt(sqrt(DD)); */
	  wt = DD + 1;
	  dX += Dx;
	  dY += Dy;
	  d2X += Dx*Dx;
	  d2Y += Dy*Dy;
	  x  += tX2/wt;
	  y  += tY2/wt;
	  x2 += tX2*tX2/wt;
	  y2 += tY2*tY2/wt;
	  xy += tX2*tY2/wt;
	  X  += tX1/wt;
	  Y  += tY1/wt;
	  Xx += tX1*tX2/wt;
	  Xy += tX1*tY2/wt;
	  Yx += tY1*tX2/wt;
	  Yy += tY1*tY2/wt;
	  N  += 1.0;
	  R  += 1.0/wt;
	  if (last && MATCHDUMP && mode) {
	    XY_to_RD (&RA, &DEC, stars2[tmpN2[j]].X, stars2[tmpN2[j]].Y, coords);
	    fprintf (stdout, "%f %f %f %f %f\n", RA, DEC, stars2[tmpN2[j]].X, stars2[tmpN2[j]].Y, stars2[tmpN2[j]].mag);
	  } 
	}
      }
      j = first_j;
      i++;
    }
    
    if (PLOTSTUFF) {
      
      graphdata.xmin = 0;
      graphdata.xmax = 2000;
      graphdata.ymin = 4000;
      graphdata.ymax =  0;
      graphdata.style = 2;
      graphdata.ptype = 2;
      graphdata.ltype = 0;
      graphdata.etype = 0;
      graphdata.color = 0;
      graphdata.lweight = 0;
      graphdata.size = 1.5;

      PrepPlotting (Nvect2, &graphdata, 1);
      PlotVector (Nvect2, xvect2, 0, 1);
      PlotVector (Nvect2, yvect2, 1, 1);
      DonePlotting (&graphdata, 1);

      graphdata.xmin = -150;
      graphdata.xmax = 150;
      graphdata.ymin = 0;
      graphdata.ymax =  4000;
      graphdata.style = 2;
      graphdata.ptype = 2;
      graphdata.ltype = 0;
      graphdata.etype = 0;
      graphdata.color = 0;
      graphdata.lweight = 0;
      graphdata.size = 1.5;
    
      PlotReset (0);
      PrepPlotting (Nvect, &graphdata, 0);
      PlotVector (Nvect, xvect, 0, 0);
      PlotVector (Nvect, yvect, 1, 0);
      DonePlotting (&graphdata, 0);
      usleep (300000);
      fprintf (stderr, "plotting %d points\n", Nvect);
      fprintf (stderr, "type return to continue");
      if (fscanf (stdin, "%c", &c) != 1) fprintf (stderr, "\n");
      Nvect = 0;
    }

    /* 
    if (extras == 1) {
      extras = 2;
      halt = last = FALSE;
      iteration -= 2;
      radius *= fratio*fratio;
    }
    */

    if (MATCHDUMP && mode && last) exit (0);
    
    if (NOMATCHDUMP && mode && last) {
      for (i = 0; i < N2; i++) {
	XY_to_RD (&RA, &DEC, stars2[i].X, stars2[i].Y, coords);
	fprintf (stdout, "%f %f %f %f %f\n", RA, DEC, stars2[i].X, stars2[i].Y, stars2[i].mag);
      }
      exit (0);
    }
    
    /* calculate the fit parameters */
    if (!last) { 
      double XX1, XY1, YX1, YY1, XO1, YO1;
      double XX0, XY0, YX0, YY0, XO0, YO0;
      double **matrix, **vector;
      int NR, NC;
      
      if (VERBOSE) fprintf (stderr, "radius: %f, No. of matched stars: %d\n", radius, (int) N);
      
      if (N < 3) {
	fprintf (stderr, "ERROR: too few stars\n");
	X_O = X_X = X_Y = Y_O = Y_X = Y_Y = 0;
	return (FALSE);
      }
      
      NR = 3; NC = 2;
      ALLOCATE (matrix, double *, NR);
      ALLOCATE (vector, double *, NR);
      for (i = 0; i < NR; i++) {
	ALLOCATE (matrix[i], double, NR);
	ALLOCATE (vector[i], double, NC);
	bzero (vector[i], NC*sizeof(double));
	bzero (matrix[i], NR*sizeof(double));
      }

      matrix[0][0] = R;
      matrix[0][1] = matrix[1][0] = x;
      matrix[0][2] = matrix[2][0] = y;
      matrix[1][2] = matrix[2][1] = xy;
      matrix[1][1] = x2;
      matrix[2][2] = y2;

      vector[0][0] = X;
      vector[1][0] = Xx;
      vector[2][0] = Xy;

      vector[0][1] = Y;
      vector[1][1] = Yx;
      vector[2][1] = Yy;

      dgaussjordan (matrix, vector, NR, NC); 
      
      /* 
      Sx2 = x2 - x*x/N;
      Sy2 = y2 - y*y/N;
      Sxy = xy - x*y/N;
      SXx = Xx - X*x/N;
      SXy = Xy - X*y/N;
      SYx = Yx - Y*x/N;
      SYy = Yy - Y*y/N;
      */

      XX0 = X_X; XY0 = X_Y; XO0 = X_O;
      YX0 = Y_X; YY0 = Y_Y; YO0 = Y_O;
      
      /* fit parameters relative to rotated frame */
      /* 
      XX1 = (SXx*Sy2 - SXy*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
      XY1 = (SXy*Sx2 - SXx*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
      XO1 = X/N - (XX1)*x/N - (XY1)*y/N;
      
      YX1 = (SYx*Sy2 - SYy*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
      YY1 = (SYy*Sx2 - SYx*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
      YO1 = Y/N - (YX1)*x/N - (YY1)*y/N;
      */

      XO1 = vector[0][0];
      XX1 = vector[1][0];
      XY1 = vector[2][0];
			
      YO1 = vector[0][1];
      YX1 = vector[1][1];
      YY1 = vector[2][1];

      fprintf (stderr, "%f %f %f\n", vector[0][0], vector[1][0], vector[2][0]);
      fprintf (stderr, "%f %f %f\n", vector[0][1], vector[1][1], vector[2][1]);

      /* fit parameters relative to original frame */
      X_X = XX1*XX0 + XY1*YX0;
      X_Y = XX1*XY0 + XY1*YY0;
      X_O = XX1*XO0 + XY1*YO0 + XO1;
      
      Y_X = YX1*XX0 + YY1*YX0;
      Y_Y = YX1*XY0 + YY1*YY0;
      Y_O = YX1*XO0 + YY1*YO0 + YO1;
      
    }
    dX = sqrt(d2X/N - dX*dX/(N*N));  /* scatter in pixels in the X direction */
    dY = sqrt(d2Y/N - dY*dY/(N*N));  /* scatter in pixels in the Y direction */
    dS = hypot (dX, dY) / sqrt(N);
    if (VERBOSE) {
      fprintf (stderr, "scatter in pixels: %5.2f x %5.2f -- %5.2f %d %d %d %d\n", dX, dY, dS, halt, last, extras, iteration);
    }
# if (0)
    if (!halt && !extras) {
      if (iteration > 0) {
	dSS = (Sprev - dS) / Sprev;
      } else {
	dSS = 1.0;
      }
      if (dSS < 0.05) { /* no fractional improvement, stop at last value */
	radius = Rprev * fratio;
	halt = TRUE;
	/* recalculate fit parameters on first, get scatter on second, exit on third */
      }
      Rprev = radius;
      Sprev = dS;
    }
# endif
    radius /= fratio;
  }
  radius *= fratio;
  
  /* convert X_X, etc to coords */ 
  { 
    double X0, Y0, S1, S2, p11, p21, p12, p22;
    double delt, A, B, C, D, dRot;
    
    *Nmatch = N;
    
    delt = 1.0 / (X_X*Y_Y - X_Y*Y_X);
    X = (coords[0].crpix1 - X_O);
    Y = (coords[0].crpix2 - Y_O);
    X0 = delt * (X*Y_Y - Y*X_Y);
    Y0 = delt * (Y*X_X - X*Y_X);
    XY_to_RD (&RA, &DEC, X0, Y0, coords);
    
    S1 = coords[0].cdelt1;
    S2 = coords[0].cdelt2;
    p11 = coords[0].pc1_1;    p12 = coords[0].pc1_2;
    p21 = coords[0].pc2_1;    p22 = coords[0].pc2_2;
    
    A =  S1*p11*Y_Y - S2*p12*Y_X;   B = S2*p12*X_X - S1*p11*X_Y;
    C =  S1*p21*Y_Y - S2*p22*Y_X;   D = S2*p22*X_X - S2*p21*X_Y;
    
    coords[0].cdelt1 = sqrt (A*A + C*C);
    coords[0].cdelt2 = sqrt (B*B + D*D);
    
    coords[0].pc1_1 = A / coords[0].cdelt1; 
    coords[0].pc1_2 = B / coords[0].cdelt2; 
    coords[0].pc2_1 = C / coords[0].cdelt1; 
    coords[0].pc2_2 = D / coords[0].cdelt2; 
    
    coords[0].cdelt1 = coords[0].cdelt1 * delt;
    coords[0].cdelt2 = coords[0].cdelt2 * delt;
    if ((fabs(coords[0].cdelt1) > 2.0*ASEC_PIX/3600.0) || 
	(fabs(coords[0].cdelt1) < 0.5*ASEC_PIX/3600.0) ||
	(fabs(coords[0].cdelt2) > 2.0*ASEC_PIX/3600.0) || 
	(fabs(coords[0].cdelt2) < 0.5*ASEC_PIX/3600.0)) {
      fprintf (stderr, "ERROR: absurd solution\n");
      return (FALSE);
    }
    
    dRot = coords[0].crval1 - RA ;
    coords[0].crval1 = RA;
    coords[0].crval2 = DEC;
    
    A = cos (dRot*RAD_DEG);
    B = sin (dRot*RAD_DEG);

    p11 = coords[0].pc1_1;    p12 = coords[0].pc1_2;
    p21 = coords[0].pc2_1;    p22 = coords[0].pc2_2;
    
    coords[0].pc1_1 = p11*A - p12*B;
    coords[0].pc1_2 = p11*B + p12*A;
    coords[0].pc2_1 = p21*A - p22*B;
    coords[0].pc2_2 = p21*B + p22*A;

  }

  coords[0].crval1 = ohana_normalize_angle (coords[0].crval1);

  *DR = sqrt (SQ(dX*coords[0].cdelt1*3600.0) + SQ(dY*coords[0].cdelt1*3600.0));
  *Radius = radius;

  if ((mode == 1) && VERBOSE) {
    fprintf (stderr, "linear astrometric solution:\n");
    fprintf (stderr, "mode: %s\n", coords[0].ctype);
    fprintf (stderr, "ref value: %f %f\n", coords[0].crval1, coords[0].crval2);
    fprintf (stderr, "ref pixel: %f %f\n", coords[0].crpix1, coords[0].crpix2);
    fprintf (stderr, "ra  terms: %f %f\n", coords[0].pc1_1, coords[0].pc1_2);
    fprintf (stderr, "dec terms: %f %f\n", coords[0].pc2_1, coords[0].pc2_2);
    fprintf (stderr, "plt scale: %f %f\n", coords[0].cdelt1, coords[0].cdelt2);
    fprintf (stderr, "accuracy:  %f %f\n", *DR, *DR / sqrt ((double)(*Nmatch)));
  }

  return (TRUE);

  

}
