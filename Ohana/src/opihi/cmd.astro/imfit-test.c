  if (ShapeVariation) {
    /* find dChi/dSx and dChi/dSy given by increasing shape terms by 5% */
    float tp1, tp2, chix, chiy;
    chix = chiy = 0;
    if (fitfunc == sgaussTD) {
      tp1 = par[2];
      tp2 = par[7];
      par[2] = par[2]*1.05;
      par[7] = par[7]*1.05;
      chix = mrq2dchi (x, y, z, dz, Npts, par, Npar, fitfunc) - chisq;
      par[2] = tp1;
      par[7] = tp2;

      tp1 = par[3];
      tp2 = par[8];
      par[3] = par[3]*1.05;
      par[8] = par[8]*1.05;
      chiy = mrq2dchi (x, y, z, dz, Npts, par, Npar, fitfunc) - chisq;
      par[3] = tp1;
      par[8] = tp2;
    }
    if (fitfunc == pgaussTD) {
      tp1 = par[2];
      par[2] = par[2]*1.05;
      chix = mrq2dchi (x, y, z, dz, Npts, par, Npar, fitfunc) - chisq;
      par[2] = tp1;

      tp1 = par[3];
      par[3] = par[3]*1.05;
      chiy = mrq2dchi (x, y, z, dz, Npts, par, Npar, fitfunc) - chisq;
      par[3] = tp1;
    }
    if (fitfunc == sgauss_psfTD) {
      tp1 = par[0];
      tp2 = par[3];
      par[0] = par[0]*1.05;
      par[3] = par[3]*1.05;
      chix = mrq2dchi (x, y, z, dz, Npts, par, Npar, fitfunc) - chisq;
      par[0] = tp1;
      par[3] = tp2;

      tp1 = par[1];
      tp2 = par[4];
      par[1] = par[1]*1.05;
      par[4] = par[4]*1.05;
      chiy = mrq2dchi (x, y, z, dz, Npts, par, Npar, fitfunc) - chisq;
      par[1] = tp1;
      par[4] = tp2;
    }
    if (fitfunc == pgauss_psfTD) {
      tp1 = par[0];
      par[0] = par[0]*1.05;
      chix = mrq2dchi (x, y, z, dz, Npts, par, Npar, fitfunc) - chisq;
      par[0] = tp1;

      tp1 = par[1];
      par[1] = par[1]*1.05;
      chiy = mrq2dchi (x, y, z, dz, Npts, par, Npar, fitfunc) - chisq;
      par[1] = tp1;
    }
    set_variable ("dChiX", chix/chisq);
    set_variable ("dChiY", chiy/chisq);
  }

# if (0)
/* pars: x, y, sx, sy, sxy, I, sky */
float fgalaxyTD (float x, float y, float *par, int Npar, float *dpar) {

  float X, Y, Z, E, F, q, R, f, p2, p3;

  X = x - par[0];
  Y = y - par[1];
  
  p2 = X / par[2];
  p3 = Y / par[3];

  Z = sqrt (0.5*p2*X + X*Y*par[4] + 0.5*p3*Y);                 /* R */
  E = 1.0 / (1 + Z);   

  q = par[5] * E;
  R = q*E;
  F = 0.5 / Z;
  
  f = q + par[6];

  dpar[0] = F*R*(p2 + par[4]*Y);
  dpar[1] = F*R*(p3 + par[4]*X);
  dpar[2] = F*0.5*R*p2*p2;
  dpar[3] = F*0.5*R*p3*p3;
  dpar[4] = -R*X*Y*F;
    
  dpar[5] = E;
  dpar[6] = 1;
  return (f);
}

/* pars: x, y, sx, sy, sxy, I, sky */
float fbarTD (float x, float y, float *par, int Npar, float *dpar) {

  float X, Y, Z, E, F, q, R, f, p2, p3;

  X = x - par[0];
  Y = y - par[1];
  
  p2 = X / par[2];
  p3 = Y / par[3];

  Z = 0.5*p2*X + X*Y*par[4] + 0.5*p3*Y;                 /* R */
  E = 1.0 / (1 + Z*Z*Z);   

  q = par[5] * E;
  F = 3*Z*Z;
  R = q*E*F;
  
  f = q + par[6];

  dpar[0] = R*(p2 + par[4]*Y);
  dpar[1] = R*(p3 + par[4]*X);
  dpar[2] = 0.5*R*p2*p2;
  dpar[3] = 0.5*R*p3*p3;
  dpar[4] = -R*X*Y;
    
  dpar[5] = E;
  dpar[6] = 1;
  return (f);
}

/* convert from x,y to major,minor */
void fix_ellipsegauss_pars (float *par, int Npar) {

  float p2, p4, angle, t1, t2, tmp, area;

  /* par[0], par[1] = Xo, Yo - stay the same */

  p2 = 1/par[2];
  p4 = 1/par[3];

  angle = 0.5 * atan2 (-2*par[4], p4 - p2); 

  tmp = sqrt (SQ(p2 - p4) + 4*SQ(par[4]));
  t1 = (p2 + p4 + tmp) / 2;
  t2 = t1 - tmp;

  par[2] = 2.35482*sqrt(1/t2);
  par[3] = 2.35482*sqrt(1/t1);
  par[4] = DEG_RAD * angle;

  area = 2*M_PI/sqrt(t1*t2);

  par[5] *= area;

}
# endif

/***  options for later

  Subtract = FALSE;
  if ((N = get_argument (argc, argv, "-sub"))) {
    remove_argument (N, &argc, argv);
    Subtract  = TRUE;
  }

  DFact = 1;
  if ((N = get_argument (argc, argv, "-D"))) {
    remove_argument (N, &argc, argv);
    DFact  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  fitfunc = fgaussTD;
  if ((N = get_argument (argc, argv, "-gal"))) {
    remove_argument (N, &argc, argv);
    fitfunc = fgalaxyTD; 
  }
  if ((N = get_argument (argc, argv, "-bar"))) {
    remove_argument (N, &argc, argv);
    fitfunc = fbarTD; 
  }


  f1 = 1;
  if ((c = get_variable ("BETA1")) != (char *) NULL) f1 = atof (c);

  f2 = 1;
  if ((c = get_variable ("BETA2")) != (char *) NULL) f2 = atof (c);

  if (Subtract) {
    tmpsky = par[6];
    par[6] = 0;
    for (N = j = 0; j < ny; j++) {
      V = (float *)(buf[0].matrix.buffer) + (j+sy)*buf[0].matrix.Naxis[0] + sx; 
      for (i = 0; i < nx; i++, V++, N++) {
	dx = i + sx;
	dy = j + sy;
	*V -= fitfunc (dx, dy, par, Npar, (float *) NULL);
      }
    }
    par[6] = tmpsky;
  }

***/

# if (0)

/* these two tests were not very succcessful.  the first did not model the shape well because 
   it could not match the change in roundness with radius.  the second did not work because the 
   parameters were degenerate (amplitude and slope of second component) */

/* test: fixed, non-integer higher-order term -- x, y, sx, sy, sxy, I, sky, f1, f2 */
float qgaussTD (float x, float y, float *par, int Npar, float *dpar) {

  float X, Y, px, py;
  float z, r, q, f, k;

  X = x - par[0];
  Y = y - par[1];
  
  px = par[2]*X;
  py = par[3]*Y;

  z = 0.5*SQ(px) + 0.5*SQ(py) + par[4]*X*Y;
  k = pow(z,1.75*par[8]);
  r = 1.0 / (1 + z + par[7]*k); /* ~ exp (-Z) */
  q = par[5]*r*r*(1 + 1.75*par[7]*par[8]*pow(z,1.75*par[8]-1));
  /* note difference from gaussian: q = par[5]*r */
  f = par[5]*r + par[6];

  dpar[0] = q*(2*px*par[2] + par[4]*Y);
  dpar[1] = q*(2*py*par[3] + par[4]*X);
  dpar[2] = -2*q*px*X;
  dpar[3] = -2*q*py*Y;
  dpar[4] = -q*X*Y;
  dpar[5] = +r;
  dpar[6] = +1;
  dpar[7] = -10*par[5]*r*r*k;
  dpar[8] = -10*par[5]*r*r*par[7]*k*1.75*log(z);

  return (f);
}

/* test: two component model: inner pseudo gaussian with outer z^1.75 x, y, sx, sy, sxy, I, sky */
float rgaussTD (float x, float y, float *par, int Npar, float *dpar) {

  float X, Y, px1, py1, px2, py2;
  float z1, z2, r1, r2, q1, q2, f;

  X = x - par[0];
  Y = y - par[1];
  
  px1 = par[2]*X;
  py1 = par[3]*Y;
  px2 = par[8]*X;
  py2 = par[9]*Y;

  z1 = 0.5*SQ(px1) + 0.5*SQ(py1) + par[4]*X*Y;
  z2 = 0.5*SQ(px2) + 0.5*SQ(py2) + par[10]*X*Y;

  r1 = 1.0 / (1 + z1 + 0.5*SQ(z1)*(1 + z1/3)); /* ~ exp (-Z) */
  r2 = 1.0 / (1 + pow(z2,1.75));

  f = par[5]*r1 + par[6] + par[7]*r2;

  q1 = par[5]*SQ(r1)*(1 + z1 + 0.5*SQ(z1));
  q2 = par[7]*SQ(r2)*(1.75*pow(z2,0.75));

  dpar[	0] = q1*(2*px1*par[2] + par[4]*Y) + q2*(2*px2*par[8] + par[10]*Y);
  dpar[	1] = q1*(2*py1*par[3] + par[4]*X) + q2*(2*py2*par[9] + par[10]*X);
  dpar[	2] = -2*q1*px1*X;
  dpar[	3] = -2*q1*py1*Y;
  dpar[	4] = -q1*X*Y;
  dpar[	5] = +r1;
  dpar[	6] = +1;
  dpar[	7] = +r2*2;
  dpar[	8] = -2*q2*px2*X*2;
  dpar[	9] = -2*q2*py2*Y*2;
  dpar[10] = -q2*X*Y;

  return (f);
}

# endif

  /* forcing values to have a rational range
  ALLOCATE (parmin, float, Npar);
  ALLOCATE (parmax, float, Npar);
  bzero (parmin, Npar*sizeof(float));
  bzero (parmax, Npar*sizeof(float));
  parmin[0] = parmin[1] = 0;
  parmax[0] = buf[0].matrix.Naxis[0];
  parmax[1] = buf[0].matrix.Naxis[1];

  parmin[2] = parmin[3] = 0.01;
  parmax[2] = parmax[3] = 100.0;
  parmin[4] = -1000;
  parmax[4] = -1000;
  
  parmin[5] = 1;
  parmax[5] = 1e5;

  parmin[6] = 0.0;
  parmax[6] = 1e5;

  if (Npar == 9) {
    parmin[7] = parmin[8] = 0.01;
    parmax[7] = parmax[8] = 10.0;
  }
  if (Npar == 10) {
    parmin[7] = parmin[8] = 0.01;
    parmax[7] = parmax[8] = 10.0;
    parmin[9] = -1000;
    parmax[9] = -1000;
  }
  */

