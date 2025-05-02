# include <dvo.h>
# define UKIRT_ONLY 1

/* note that Coords.ctype carries the DEC (ctype2) value */

// default coordinates have unit scale and no rotation
void InitCoords (Coords *coords, char *projection) {
  coords->crval1 = 0.0;
  coords->crval2 = 0.0;
  coords->crpix1 = 0.0;
  coords->crpix2 = 0.0;

  coords->cdelt1 = 1.0;
  coords->cdelt2 = 1.0;

  coords->pc1_1 = 1.0; // EW is flipped relative to sky parity (dRA/dX > 0)
  coords->pc2_2 = 1.0;
  coords->pc2_1 = 0.0;
  coords->pc1_2 = 0.0;

  coords->Npolyterms = 1;
  coords->mosaic = NULL;
  coords->offsetMap = NULL;

  memset (coords->polyterms, 0, 7*2*sizeof(float));

  if (projection) {
    strcpy (coords->ctype, projection);
  } else {
    strcpy (coords->ctype, "NONE");
  }
} 

// default coordinates have unit scale and no rotation
void CopyCoords (Coords *tgt, Coords *src) {
  tgt->crval1     = src->crval1;
  tgt->crval2     = src->crval2;
  tgt->crpix1     = src->crpix1;
  tgt->crpix2     = src->crpix2;
  tgt->cdelt1     = src->cdelt1;
  tgt->cdelt2     = src->cdelt2;
  tgt->pc1_1      = src->pc1_1;
  tgt->pc2_2      = src->pc2_2;
  tgt->pc2_1      = src->pc2_1;
  tgt->pc1_2      = src->pc1_2;
  tgt->Npolyterms = src->Npolyterms;
  tgt->mosaic     = src->mosaic;      // pointer to another structure
  tgt->offsetMap  = src->offsetMap;   // pointer to another structure

  memcpy (tgt->polyterms, src->polyterms, 7*2*sizeof(float));
  strcpy (tgt->ctype,     src->ctype);
} 

int XY_to_LM (double *L, double *M, double x, double y, Coords *coords) {

  OhanaProjection proj;
  OhanaProjectionMode mode;
  double X, Y, X2, XY, Y2, X3, Y3;

  proj = GetProjection (coords[0].ctype);
  mode = GetProjectionMode (proj);

  // if we have an image map, it should be applied to the pixel coordinates 
  // before anything else is done.  It is easier to invert the map if it is 
  // of the form (dX,dY) = f(X,Y), then if (dL,dM) = f(X,Y).
  if (coords[0].Npolyterms == -1) {
    if (coords[0].offsetMap) {
      // myAssert (coords[0].offsetMap, "offsetMap is requested by not defined");
      float dX = AstromOffsetMapValue (coords->offsetMap, x, y, TRUE);
      float dY = AstromOffsetMapValue (coords->offsetMap, x, y, FALSE);
      if (isnan(dX)) dX = 0.0;
      if (isnan(dY)) dY = 0.0;
      x += dX;
      y += dY;
    }
  }

  /** convert pixel coordinates to cartesian system **/
  X = coords[0].cdelt1*(x - coords[0].crpix1);
  Y = coords[0].cdelt2*(y - coords[0].crpix2);

  *L = (X*coords[0].pc1_1 + Y*coords[0].pc1_2);
  *M = (X*coords[0].pc2_1 + Y*coords[0].pc2_2);

  /** extra polynomial terms **/
  // for ZPN, these are used to modify the radial distance and not the X,Y coords
  if ((coords[0].Npolyterms > 1) && (proj != PROJ_ZPN)) {
    X2 = X*X;
    Y2 = Y*Y;
    XY = X*Y;
    *L += X2*coords[0].polyterms[0][0] + XY*coords[0].polyterms[1][0] + Y2*coords[0].polyterms[2][0];
    *M += X2*coords[0].polyterms[0][1] + XY*coords[0].polyterms[1][1] + Y2*coords[0].polyterms[2][1];

    if (coords[0].Npolyterms > 2) {
      X3 = X2*X;
      Y3 = Y2*Y;
      *L += X3*coords[0].polyterms[3][0] + X2*Y*coords[0].polyterms[4][0] + X*Y2*coords[0].polyterms[5][0] + Y3*coords[0].polyterms[6][0];
      *M += X3*coords[0].polyterms[3][1] + X2*Y*coords[0].polyterms[4][1] + X*Y2*coords[0].polyterms[5][1] + Y3*coords[0].polyterms[6][1];
    }
  }

  if (mode == PROJ_MODE_CARTESIAN) {
    *L += coords[0].crval1;
    *M += coords[0].crval2;
  }

  return (TRUE);
}

int LM_to_RD (double *ra, double *dec, double L, double M, Coords *coords) {

  OhanaProjection proj;
  OhanaProjectionMode mode;
  double R, T, Z, Z2, sphi, cphi, stht, ctht;
  double alpha, delta, salp, calp, sdel, sdp, cdp;

  proj = GetProjection (coords[0].ctype);
  mode = GetProjectionMode (proj);
  if (proj == PROJ_NONE) return (FALSE);
  if (mode == PROJ_MODE_NONE) return (FALSE);

  stht = ctht = 1;

  /** Locally Cartesian Projections **/
  if (mode == PROJ_MODE_CARTESIAN) {
    /* mosaic astrometry : WRP is chip astrometry; apply mosaic (DIS) term */
    if (proj == PROJ_WRP) {
      if (!coords->mosaic) {
	// myAbort ("missing mosaic element");
	*ra  = L;
	*dec = M;
	return (FALSE);
      }
      XY_to_RD (ra, dec, L, M, coords->mosaic);
      return (TRUE);
    } 
    *ra  = L;
    *dec = M;
    return (TRUE);
  }
  
  /** Zenithal Projections **/
  if (mode == PROJ_MODE_ZENITHAL) {
    R = hypot (L,M);
    if ((L == 0) && (M == 0)) {
      sphi = 0;
      cphi = 1;
    } else {
      sphi =  L / R;
      cphi = -M / R;
    }

    switch (proj) {
      case PROJ_TAN:
      case PROJ_TNX:
      case PROJ_DIS:
	// R = cot (theta) = cos(theta) / sin(theta)
	if (R == 0) {
	  stht = 1.0;
	  ctht = 0.0;
	} else {
	  // T = DEG_RAD / R; // T in 1/radians
	  // stht =   T / sqrt ( 1.0 + T*T);
	  // ctht = 1.0 / sqrt ( 1.0 + T*T);

	  T = RAD_DEG * R;
	  stht = 1.0 / sqrt ( 1.0 + T*T);
	  ctht =   T / sqrt ( 1.0 + T*T);
	}
	break;
      case PROJ_SIN:
	// R = (180/pi) cos (theta)
	ctht = RAD_DEG * R;
	stht = sqrt (1 - ctht*ctht);
	break;
      case PROJ_STG:
	// R = (180/pi) [2 cos (theta)  / 1 + sin (theta)]
	stht = (4 - RAD_DEG*R) / (4 + RAD_DEG*R);
	ctht = sqrt (1 - stht*stht);
	break;
      case PROJ_ARC:
	// R = 90 - theta (degrees)
	ctht = sin (RAD_DEG * R);
	stht = cos (RAD_DEG * R);
	break;

      case PROJ_ZEA:
      case PROJ_ZPL:
	if (R > 2*DEG_RAD) {
	  *ra = L;
	  *dec = M;
	  return (FALSE);
	}
	stht = 1 - 0.5*SQ(R*RAD_DEG);
	ctht = sqrt (1 - stht*stht);
	break;

      case PROJ_ZPN:

	// the forward projection is:
	// theta = atan2(stht, ctht)
	// gamma = (pi/2 - theta) : theta in radians
	// Ro = sum (P_i gamma^i)
	// R  = (180/pi) Ro

	// given R, we need to find theta:
	// Ro = R * (pi / 180) = sum (P_i gamma^i)
	// solve sum (P_i gamma^i) - Ro = 0 using Newton-Raphson

	// use Ro to get a guess for gamma and iterate

	{
	  double Ro = RAD_DEG * R;
	  
	  // find the roots of f(gamma) - Ro = 0
	  // starting guess for gamma is (Ro - P0) / P1
	  double gamma = (Ro - coords[0].polyterms[0][0]) / coords[0].polyterms[1][0];
	  
	  int iter;
	  for (iter = 0; iter < 5; iter++) {
	    
	    double Rc = 0.0; // this will hold the ander 
	    double dR = 0.0;
	    for (int i = coords[0].Npolyterms - 1; i > 1; i--) {
	      double Pi = (i < 7) ? coords[0].polyterms[i][0] : coords[0].polyterms[i-7][1];
	      Rc = (Rc + Pi)*gamma;
	      dR = (dR + i*Pi)*gamma;
	    }
	    double P0 = coords[0].polyterms[0][0];
	    double P1 = coords[0].polyterms[1][0];
	    Rc = (Rc + P1)*gamma + P0;
	    dR = (dR + P1);

	    double gamma_new = gamma - (Rc - Ro) / dR;
	    gamma = gamma_new;
	  }
	  
	  double theta = 90.0 - gamma * DEG_RAD ;
	  ctht = cos (RAD_DEG * theta);
	  stht = sin (RAD_DEG * theta);
	  break;
	}

      default:
	return (FALSE);
    }
    sdp  = sin(RAD_DEG*coords[0].crval2);
    cdp  = cos(RAD_DEG*coords[0].crval2);
    
    sdel = stht*sdp - ctht*cphi*cdp;
    salp = ctht*sphi;
    calp = stht*cdp + ctht*cphi*sdp;
    alpha = atan2 (salp, calp);
    delta = asin (sdel);
    
    *ra  = DEG_RAD*alpha + coords[0].crval1;
    *dec = DEG_RAD*delta;

    // *ra = ohana_normalize_angle (*ra);

    return (TRUE);
  }
  
  /**** Other Conventional Projections ****/
  if (mode == PROJ_MODE_PSEUDOCYL) {
    switch (proj) {
      case PROJ_AIT:
	Z2 = (1.0 - SQ(RAD_DEG*0.25*L) - SQ(RAD_DEG*0.5*M));
	if (Z2 < 0) return (FALSE);
	Z = sqrt (Z2);
	alpha = 2.0 * DEG_RAD * atan2 (RAD_DEG*0.5*Z*L, 2.0*Z2 - 1.0);
	delta = DEG_RAD * asin (RAD_DEG*M*Z);
	break;
      case PROJ_GLS:
	/* L,M in degrees, alpha,delta in degrees */
	alpha = L / cos (RAD_DEG * M);
	delta = M;
	break;
      case PROJ_PAR:
	/* L,M in degrees, alpha,delta in degrees */
	alpha = L / (1.0 - SQ(2.0*M/180));
	delta = 3 * DEG_RAD * asin (M/180.0);
	break;
      case PROJ_MOL:
	Z = sqrt(2.0 - SQ(RAD_DEG*M));
	alpha = M_PI*L/(2*Z);
	double T1 = asin(RAD_DEG*M*M_SQRT1_2)/90.0 + M*Z/180.0;
	delta = DEG_RAD*asin(T1);
      default:
	return (FALSE);
    }
    if (fabs(alpha) >= 180.0) return (FALSE);
    *ra  = alpha + coords[0].crval1;
    *dec = delta + coords[0].crval2;

    // *ra = ohana_normalize_angle (*ra);

    return (TRUE);
  }
  return (FALSE);
}

int RD_to_LM (double *L, double *M, double ra, double dec, Coords *coords) {

  int i;
  double phi, theta;
  double sphi, cphi, stht, ctht;
  double salp, calp, sdel, cdel, sdp, cdp;
  double P, A, Rc;
  OhanaProjection proj;
  OhanaProjectionMode mode;

  *L = *M = 0;

  proj = GetProjection (coords[0].ctype);
  mode = GetProjectionMode (proj);
  if (proj == PROJ_NONE) return (FALSE);
  if (mode == PROJ_MODE_NONE) return (FALSE);

  /**** Locally Cartesian Projections ****/
  if (mode == PROJ_MODE_CARTESIAN) {
    if (proj == PROJ_WRP) {
      if (!coords->mosaic) {
	myAbort ("missing mosaic element");
	return (FALSE);
      }
      RD_to_XY (L, M, ra, dec, coords->mosaic);
      return (TRUE);
    }
    *L = ra;
    *M = dec;
    return (TRUE);
  }
  
  /**** Zenithal Projections ****/
  if (mode == PROJ_MODE_ZENITHAL) {
    sdp  = sin(RAD_DEG*coords[0].crval2);
    cdp  = cos(RAD_DEG*coords[0].crval2);
    salp = sin(RAD_DEG*(ra - coords[0].crval1));
    calp = cos(RAD_DEG*(ra - coords[0].crval1));
    sdel = sin(RAD_DEG*dec);
    cdel = cos(RAD_DEG*dec);

    stht = sdel*sdp + cdel*cdp*calp;    /* sin(theta) */
    sphi = cdel*salp;                   /* = cos(theta)*sin(phi) */
    cphi = cdel*sdp*calp - sdel*cdp;    /* = cos(theta)*cos(phi) */

    // L = +R sin(phi)
    // M = -R cos(phi)

    switch (proj) {
      case PROJ_TAN:
      case PROJ_TNX:
      case PROJ_DIS:
	// R = cot (theta) = cos(theta) / sin(theta)
	Rc = hypot(sphi, cphi);
	*L = (stht == 0) ? 180.0 * sphi / Rc :  +DEG_RAD * sphi / stht;
	*M = (stht == 0) ? 180.0 * cphi / Rc :  -DEG_RAD * cphi / stht;
	return (stht > 0);
      case PROJ_STG:
	// R = 2 cos(theta) / [1 + sin(theta)]
	Rc = DEG_RAD * 2 / (1 + stht);
	*L = +Rc * sphi;
	*M = -Rc * cphi;
	return (stht > 0);
      case PROJ_SIN:
	// R = cos(theta)
	*L = +DEG_RAD * sphi;
	*M = -DEG_RAD * cphi;
	return (stht > 0);

      case PROJ_ARC:
	// R = 90 - theta
	ctht = hypot(sphi, cphi);
	theta = atan2 (stht, ctht);
	Rc = 90 - DEG_RAD * theta;
	*L = (ctht == 0.0) ? 0.0 : +Rc * sphi / ctht ;
	*M = (ctht == 0.0) ? 0.0 : -Rc * cphi / ctht ;
	return (TRUE);

      case PROJ_ZPN:
	// the forward projection is:
	// theta = atan2(stht, ctht)
	// gamma = (pi/2 - theta) : theta in radians
	// Ro = sum (P_i gamma^i)
	// R  = (180/pi) Ro

	// Ro = (pi/180)(90 - theta)
	// R = (180/pi)sum (P_i R^i)

	// is ZPN defined for Npolyterms = 0 or 1?

	ctht = hypot(sphi, cphi);
	theta = atan2 (stht, ctht);

	double Ro;
	double gamma = M_PI_2 - theta;

	// i = 0 .. Npolyterms - 1 (1 <= Npolyterms <= 21)
	Ro = 0.0;
	for (i = coords[0].Npolyterms - 1; i > 0; i --) {
	  double Pi = (i < 7) ? coords[0].polyterms[i][0] : coords[0].polyterms[i-7][1];
	  Ro = (Ro + Pi)*gamma;
	}
	Ro += coords[0].polyterms[0][0];

	Rc = DEG_RAD * Ro;

	*L = (ctht == 0.0) ? 0.0 : +Rc * sphi / ctht ;
	*M = (ctht == 0.0) ? 0.0 : -Rc * cphi / ctht ;
	return (TRUE);

      case PROJ_ZEA:
      case PROJ_ZPL:
	// R = 90 - theta
	Rc = DEG_RAD * M_SQRT2 / sqrt (1 + stht);
	*L =  Rc * sphi;
	*M = -Rc * cphi;
	return (stht > 0);
      default:
	return (FALSE);
    }
    return (FALSE);
  }

  /**** Other Standard Projections ****/
  if (mode == PROJ_MODE_PSEUDOCYL) {
    switch (proj) {
      case PROJ_AIT:
	phi = RAD_DEG*(ra - coords[0].crval1);
	theta = RAD_DEG*(dec - coords[0].crval2);
	P = 1.0 + cos (theta) * cos (0.5*phi);
	if (P == 0.0) {
	  *L =  0.0;
	  *M =  0.0;
	  return (TRUE);
	} 
	A =  DEG_RAD * sqrt (2.0 / P);
	*L =  2.0 * A * cos (theta) * sin (0.5*phi);
	*M =  A * sin (theta);
	return (TRUE);
      case PROJ_GLS:
	phi = ra - coords[0].crval1;
	theta = dec - coords[0].crval2;
	*L = phi * cos(RAD_DEG * theta);
	*M = theta;
	return (TRUE);
      case PROJ_PAR:
	phi = ra - coords[0].crval1;
	theta = dec - coords[0].crval2;
	*L = phi * (2.0*cos(2*RAD_DEG*theta/3.0) - 1);
	*M = 180.0 * sin (RAD_DEG*theta/3.0);
	return (TRUE);
      case PROJ_MOL: 
	phi = ra - coords[0].crval1;
	theta = dec - coords[0].crval2;
	// given theta, solve for gamma:
	double So = sin(theta*RAD_DEG);
	double Go = theta;
	if (theta > +80) { Go = theta - 10.0;}
	if (theta < -80) { Go = theta + 10.0;}
	for (int iter = 0; iter < 5; iter ++) {
	  double Fo = (Go / 90.0) + sin(Go*M_PI/90.0) / M_PI - So;
	  double dFdG = (1 / 90.0) + cos(Go*M_PI/90.0) / 90.0;
	  Go -= Fo / dFdG;
	}
	*L = 2*phi * cos(RAD_DEG*Go) * M_SQRT2 / M_PI;
	*M = 180.0 * sin(RAD_DEG*Go) * M_SQRT2 / M_PI;
	return (TRUE);
      default:
	return (FALSE);
    }
    return (FALSE);
  }
  return (FALSE);
}

int LM_to_XY (double *x, double *y, double L, double M, Coords *coords) {

  int i;
  double Ro, Xo, Yo;
  double dX, dY, Lo, Mo, dL, dM;
  double dLdX, dLdY, dMdX, dMdY, Do;
  OhanaProjection proj;
  OhanaProjectionMode mode;

  proj = GetProjection (coords[0].ctype);
  mode = GetProjectionMode (proj);

  *x = 0;
  *y = 0;

  if (mode == PROJ_MODE_CARTESIAN) {
    L -= coords[0].crval1;
    M -= coords[0].crval2;
  }

  /* start with linear solution for X,Y */
  Ro = (coords[0].pc1_1*coords[0].pc2_2 - coords[0].pc1_2*coords[0].pc2_1);
  Xo = (coords[0].pc2_2*L - coords[0].pc1_2*M) / Ro;
  Yo = (coords[0].pc1_1*M - coords[0].pc2_1*L) / Ro;

  /** extra polynomial terms **/
  // polyterm values are only valid for some projections.  for PROJ_ZPN, they are applied to R, not X,Y
  if ((coords[0].Npolyterms > 1) && (proj != PROJ_ZPN)) {
    for (i = 0; i < 10; i++) {
      // find derivatives at Xo, Yo
      dLdX = coords[0].pc1_1 + 2.0*Xo*coords[0].polyterms[0][0] + Yo*coords[0].polyterms[1][0];
      dLdY = coords[0].pc1_2 + 2.0*Yo*coords[0].polyterms[2][0] + Xo*coords[0].polyterms[1][0];
      dMdX = coords[0].pc2_1 + 2.0*Xo*coords[0].polyterms[0][1] + Yo*coords[0].polyterms[1][1];
      dMdY = coords[0].pc2_2 + 2.0*Yo*coords[0].polyterms[2][1] + Yo*coords[0].polyterms[1][1];

      if (coords[0].Npolyterms > 2) {
	dLdX += 3.0*Xo*Xo*coords[0].polyterms[3][0] + 2*Xo*Yo*coords[0].polyterms[4][0] + Yo*Yo*coords[0].polyterms[5][0];
	dLdY += 3.0*Yo*Yo*coords[0].polyterms[6][0] + 2*Xo*Yo*coords[0].polyterms[5][0] + Xo*Xo*coords[0].polyterms[4][0];
	dMdX += 3.0*Xo*Xo*coords[0].polyterms[3][1] + 2*Xo*Yo*coords[0].polyterms[4][1] + Yo*Yo*coords[0].polyterms[5][1];
	dMdY += 3.0*Yo*Yo*coords[0].polyterms[6][1] + 2*Xo*Yo*coords[0].polyterms[5][1] + Xo*Xo*coords[0].polyterms[4][1];
      }

      // find Lo,Mo for Xo,Yo:
      Lo = (Xo*coords[0].pc1_1 + Yo*coords[0].pc1_2);
      Mo = (Xo*coords[0].pc2_1 + Yo*coords[0].pc2_2);
      if (coords[0].Npolyterms > 1) {
	Lo += Xo*Xo*coords[0].polyterms[0][0] + Xo*Yo*coords[0].polyterms[1][0] + Yo*Yo*coords[0].polyterms[2][0];
	Mo += Xo*Xo*coords[0].polyterms[0][1] + Xo*Yo*coords[0].polyterms[1][1] + Yo*Yo*coords[0].polyterms[2][1];
      }
      if (coords[0].Npolyterms > 2) {
	Lo += Xo*Xo*Xo*coords[0].polyterms[3][0] + Xo*Xo*Yo*coords[0].polyterms[4][0] + Xo*Yo*Yo*coords[0].polyterms[5][0] + Yo*Yo*Yo*coords[0].polyterms[6][0];
	Mo += Xo*Xo*Xo*coords[0].polyterms[3][1] + Xo*Xo*Yo*coords[0].polyterms[4][1] + Xo*Yo*Yo*coords[0].polyterms[5][1] + Yo*Yo*Yo*coords[0].polyterms[6][1];
      }

      dL = (L - Lo);
      dM = (M - Mo);

      Do = 1.0 / (dLdX * dMdY - dLdY * dMdX);
      dX = (dL*dMdY - dM*dLdY) * Do;
      dY = (dM*dLdX - dL*dMdX) * Do;

      // fprintf (stderr, "Xo,Yo + dX,dY : %f, %f : %f, %f\n", Xo, Yo, dX, dY);

      Xo += dX;
      Yo += dY;
    }
  }
  /* check for correct size (iterate?) */

  *x = Xo / coords[0].cdelt1 + coords[0].crpix1;
  *y = Yo / coords[0].cdelt2 + coords[0].crpix2;

  // if we have an image map, it should be applied to the pixel coordinates 
  // after anything else is done.  It is easier to invert the map if it is 
  // of the form (dX,dY) = f(X,Y), then if (dL,dM) = f(X,Y).
  if (coords[0].Npolyterms == -1) {
    // myAssert (coords[0].offsetMap, "offsetMap is requested by not defined");

    if (coords[0].offsetMap) {

      double xraw = *x;
      double yraw = *y;

      double dXo = 0.0;
      double dYo = 0.0;

      int i;
      for (i = 0; i < 4; i++) {
	double dX = AstromOffsetMapValue (coords->offsetMap, xraw, yraw, TRUE);
	double dY = AstromOffsetMapValue (coords->offsetMap, xraw, yraw, FALSE);

	if (isnan(dX)) dX = 0.0;
	if (isnan(dY)) dY = 0.0;

	dX -= dXo;
	dY -= dYo;

	xraw -= dX;
	yraw -= dY;

	dXo += dX;
	dYo += dY;
      }

      *x = xraw;
      *y = yraw;

      // I need to iterate since the position at which I have first made the correction is
      // not the true position.  but if dX,dY is small and the gradient of dX,dY is also
      // small, then even a single pass gets us close.

      // XXX test this and use a while (hypot(dX,dY) > XXX) condition 
    }
  }
  return (TRUE);
}

int XY_to_RD (double *ra, double *dec, double x, double y, Coords *coords) {

  double L, M;
  int status;

  status = XY_to_LM (&L, &M, x, y, coords);
  if (!status) return FALSE;

  status = LM_to_RD (ra, dec, L, M, coords);
  return (status);
}

int RD_to_XY (double *x, double *y, double ra, double dec, Coords *coords) {

  double L, M;
  int status;

  status = RD_to_LM (&L, &M, ra, dec, coords);

  if (finite(L) && finite(M)) {
      LM_to_XY (x, y, L, M, coords);
  } else {
      *x = L;
      *y = M;
  }
  return (status);
}

int fRD_to_XY (float *x, float *y, double ra, double dec, Coords *coords) {

  int status;
  double tmpx, tmpy;

  status = RD_to_XY (&tmpx, &tmpy, ra, dec, coords);
  *x = tmpx;
  *y = tmpy;
  
  return (status);

}

int fXY_to_RD (float *ra, float *dec, double x, double y, Coords *coords) {

  int status;
  double tmpr, tmpd;

  status = XY_to_RD (&tmpr, &tmpd, x, y, coords);
  *ra = tmpr;
  *dec = tmpd;
  
  return (status);

}

enum {COORD_TYPE_NONE, COORD_TYPE_PC, COORD_TYPE_ROT, COORD_TYPE_CD, COORD_TYPE_LIN};

int GetRadialZPN (Coords *coords, Header *header);

int GetCoords (Coords *coords, Header *header) {
  
  int status, status1, status2, itmp, Polynomial, Polyterm;
  double Lambda, rotate, rotate1, rotate2, scale;
  double equinox;
  char *ctype;
  int mode;
  
  rotate = 0.0;

  InitCoords (coords, NULL);
  
  mode = COORD_TYPE_NONE;
  {    
    int haveCTYPE, haveCDELT, haveCROTA, haveCROTA1, haveCROTA2, haveCDij, havePCij, haveRAo;
    float tmp;
    char stmp[80];
    
    // there are a few different representations for scale and rotation.  choose an appropriate
    // set: (CDELTi + CROTAi), (CDELTi + PCij), (CDij), 

    haveCTYPE  = gfits_scan (header, "CTYPE2",   "%s", 1, stmp);
    haveCDELT  = gfits_scan (header, "CDELT1",   "%f", 1, &tmp);
    haveCROTA1 = gfits_scan (header, "CROTA1",   "%f", 1, &tmp);
    haveCROTA2 = gfits_scan (header, "CROTA2",   "%f", 1, &tmp);
    haveCDij   = gfits_scan (header, "CD1_1",    "%f", 1, &tmp);
    havePCij   = gfits_scan (header, "PC001001", "%f", 1, &tmp);
    haveRAo    = gfits_scan (header, "RA_O",     "%f", 1, &tmp);
    
    haveCROTA = haveCROTA1 || haveCROTA2;

    if (haveCTYPE && havePCij  && haveCDELT) { mode = COORD_TYPE_PC;   goto gotit; }
    if (haveCTYPE && haveCROTA && haveCDELT) { mode = COORD_TYPE_ROT;  goto gotit; }
    if (haveCTYPE && haveCDij)               { mode = COORD_TYPE_CD;   goto gotit; }
    if (haveRAo)                             { mode = COORD_TYPE_LIN;  goto gotit; }
    // fprintf (stderr, "no valid WCS keywords\n");
    return (FALSE);
  }
  
gotit:

  status = TRUE; 
  switch (mode) {
    case COORD_TYPE_PC:
      status &= gfits_scan (header, "CTYPE2", 	"%s",  1, coords[0].ctype);
      status &= gfits_scan (header, "CRVAL1", 	"%lf", 1, &coords[0].crval1);
      status &= gfits_scan (header, "CRPIX1", 	"%f",  1, &coords[0].crpix1);
      status &= gfits_scan (header, "CRVAL2", 	"%lf", 1, &coords[0].crval2);  
      status &= gfits_scan (header, "CRPIX2", 	"%f",  1, &coords[0].crpix2);

      status &= gfits_scan (header, "CDELT1", 	"%f",  1, &coords[0].cdelt1);
      status &= gfits_scan (header, "CDELT2", 	"%f",  1, &coords[0].cdelt2);
      status &= gfits_scan (header, "PC001001", "%f",  1, &coords[0].pc1_1);
      status &= gfits_scan (header, "PC001002", "%f",  1, &coords[0].pc1_2);
      status &= gfits_scan (header, "PC002001", "%f",  1, &coords[0].pc2_1);
      status &= gfits_scan (header, "PC002002", "%f",  1, &coords[0].pc2_2);

      ctype = &coords[0].ctype[4];

      // read the ZPN coeffients PV2_i (i = 0 < 14)
      // ZPN is inconsistent with the other Polynomial types
      if (!strcmp (ctype, "-ZPN")) { 
	GetRadialZPN (coords, header);
	break;
      }

      /* set NPLYTERM based on header.  if NPLYTERM is missing, it should have a 
	 value of 0, unless the projection type is one of PLY, DIS, WRP, in which
	 case it should be set to 3 */
      Polynomial = !strcmp (ctype, "-PLY") || !strcmp (ctype, "-DIS") || !strcmp (ctype, "-WRP");
      Polyterm = gfits_scan (header, "NPLYTERM", "%d", 1, &itmp);

      coords[0].Npolyterms = 0;
      if (Polynomial && !Polyterm) coords[0].Npolyterms = 1;
      if (Polyterm) coords[0].Npolyterms = itmp;

      switch (coords[0].Npolyterms) {
	case 3:
	  status &= gfits_scan (header, "PCA1X3Y0", "%f", 1, &coords[0].polyterms[3][0]);
	  status &= gfits_scan (header, "PCA1X2Y1", "%f", 1, &coords[0].polyterms[4][0]);
	  status &= gfits_scan (header, "PCA1X1Y2", "%f", 1, &coords[0].polyterms[5][0]);
	  status &= gfits_scan (header, "PCA1X0Y3", "%f", 1, &coords[0].polyterms[6][0]);
	  status &= gfits_scan (header, "PCA2X3Y0", "%f", 1, &coords[0].polyterms[3][1]);
	  status &= gfits_scan (header, "PCA2X2Y1", "%f", 1, &coords[0].polyterms[4][1]);
	  status &= gfits_scan (header, "PCA2X1Y2", "%f", 1, &coords[0].polyterms[5][1]);
	  status &= gfits_scan (header, "PCA2X0Y3", "%f", 1, &coords[0].polyterms[6][1]);
	case 2:
	  status &= gfits_scan (header, "PCA1X2Y0", "%f", 1, &coords[0].polyterms[0][0]);
	  status &= gfits_scan (header, "PCA1X1Y1", "%f", 1, &coords[0].polyterms[1][0]);
	  status &= gfits_scan (header, "PCA1X0Y2", "%f", 1, &coords[0].polyterms[2][0]);
	  status &= gfits_scan (header, "PCA2X2Y0", "%f", 1, &coords[0].polyterms[0][1]);
	  status &= gfits_scan (header, "PCA2X1Y1", "%f", 1, &coords[0].polyterms[1][1]);
	  status &= gfits_scan (header, "PCA2X0Y2", "%f", 1, &coords[0].polyterms[2][1]);
	case 0:
	case 1:
	  break;
      }
      break;

    case COORD_TYPE_ROT:
      status &= gfits_scan (header, "CTYPE2", "%s",  1, coords[0].ctype);
      status &= gfits_scan (header, "CRVAL1", "%lf", 1, &coords[0].crval1);
      status &= gfits_scan (header, "CRPIX1", "%f",  1, &coords[0].crpix1);
      status &= gfits_scan (header, "CRVAL2", "%lf", 1, &coords[0].crval2);  
      status &= gfits_scan (header, "CRPIX2", "%f",  1, &coords[0].crpix2);

      status &= gfits_scan (header, "CDELT1", "%f", 1, &coords[0].cdelt1);
      status &= gfits_scan (header, "CDELT2", "%f", 1, &coords[0].cdelt2);

      status1 = gfits_scan (header, "CROTA1", "%lf", 1, &rotate1);
      status2 = gfits_scan (header, "CROTA2", "%lf", 1, &rotate2);
      status &= status1 || status2;

      rotate = rotate2;
      if (status1 && !status2) rotate = rotate1;
      if (!status1 && !status2) rotate = 0.0;

      Lambda = coords[0].cdelt2 / coords[0].cdelt1;
      coords[0].pc1_1 =  cos(rotate*RAD_DEG);
      coords[0].pc1_2 = -sin(rotate*RAD_DEG) * Lambda;
      coords[0].pc2_1 =  sin(rotate*RAD_DEG) / Lambda;
      coords[0].pc2_2 =  cos(rotate*RAD_DEG);

      // read the ZPN coeffients PV2_i (i = 0 < 14)
      if (!strcmp (&coords[0].ctype[4], "-ZPN")) GetRadialZPN (coords, header);
      break;

    case COORD_TYPE_CD:
      status &= gfits_scan (header, "CTYPE2", "%s",  1, coords[0].ctype);
      status &= gfits_scan (header, "CRVAL1", "%lf", 1, &coords[0].crval1);
      status &= gfits_scan (header, "CRPIX1", "%f",  1, &coords[0].crpix1);
      status &= gfits_scan (header, "CRVAL2", "%lf", 1, &coords[0].crval2);  
      status &= gfits_scan (header, "CRPIX2", "%f",  1, &coords[0].crpix2);

      status &= gfits_scan (header, "CD1_1", "%f", 1, &coords[0].pc1_1);
      status &= gfits_scan (header, "CD1_2", "%f", 1, &coords[0].pc1_2);
      status &= gfits_scan (header, "CD2_1", "%f", 1, &coords[0].pc2_1);
      status &= gfits_scan (header, "CD2_2", "%f", 1, &coords[0].pc2_2);
      /* renormalize */
      scale = hypot (coords[0].pc1_1, coords[0].pc1_2);
      coords[0].cdelt1 = coords[0].cdelt2 = scale;
      coords[0].pc1_1 /= scale;
      coords[0].pc1_2 /= scale;
      coords[0].pc2_1 /= scale;
      coords[0].pc2_2 /= scale;

      // read the ZPN coeffients PV2_i (i = 0 < 14)
      if (!strcmp (&coords[0].ctype[4], "-ZPN")) GetRadialZPN (coords, header);
      break;

    case COORD_TYPE_LIN:
      /* some of my thesis data uses this simple linear model - convert on read? */
      status &= gfits_scan (header, "RA_O", "%lf", 1, &coords[0].crval1);
      status &= gfits_scan (header, "RA_X", "%f", 1, &coords[0].pc1_1);
      status &= gfits_scan (header, "RA_Y", "%f", 1, &coords[0].pc1_2);
      status &= gfits_scan (header, "DEC_O", "%lf", 1, &coords[0].crval2);  
      status &= gfits_scan (header, "DEC_X", "%f", 1, &coords[0].pc2_1);
      status &= gfits_scan (header, "DEC_Y", "%f", 1, &coords[0].pc2_2);
      coords[0].crpix1 = coords[0].crpix2 = 0.0;
      coords[0].cdelt1 = coords[0].cdelt2 = 1.0;
      strcpy (coords[0].ctype, "GENE");
      break;
  }

  if (status) {
    char equinoxString[80];
    int haveEquinox = gfits_scan (header, "EQUINOX", "%s", 1, equinoxString);
    if (haveEquinox) {
      // is the string a valid number (it is bad to interpret an error message as year 0.0)
      char *endptr;
      equinox = strtod (equinoxString, &endptr);
      if (endptr == equinoxString) haveEquinox = FALSE;
    }
    if (!haveEquinox) {
      haveEquinox = gfits_scan (header, "EPOCH", "%s", 1, equinoxString);
      if (haveEquinox) {
	// is the string a valid number (it is bad to interpret an error message as year 0.0)
	char *endptr;
	equinox = strtod (equinoxString, &endptr);
	if (endptr == equinoxString) haveEquinox = FALSE;
      }
    }
    if (!haveEquinox) {
      equinox = 2000.0;
    }
    if (fabs (equinox - 2000.0) > 0.1) {
      coords_precess (&coords[0].crval1, &coords[0].crval2, equinox, 2000.0);
    } 
  }
  
  if (!status) {
    // fprintf (stderr, "error getting all elements for coordinate mode %s\n", coords[0].ctype);
    coords[0].crval1 = coords[0].crpix1 = coords[0].cdelt1 = 0.0;
    coords[0].crval2 = coords[0].crpix2 = coords[0].cdelt2 = 0.0;
    coords[0].pc1_1 = coords[0].pc2_2 = 1.0;
    coords[0].pc2_1 = coords[0].pc1_2 = 0.0;
    strcpy (coords[0].ctype, "NONE");
  }
  return (status);
}

int GetRadialZPN (Coords *coords, Header *header) {

  // RA---ZPN can have up to 14 radial polynomial terms.  these are stored in
  // polyterms[0][0] - [6][0] for the first 7 and [0][1] - [6][1] for the rest
  // these terms are coeffients of a polynomial of the radial distances

  // read the ZPN coeffients PV2_i (i = 0 < 14)
  int found;
  int Nmax = 0;
  for (int i = 0; i < 14; i++) {
    char name[64];
    snprintf (name, 64, "PV2_%d", i);
    if (i < 7) {
      coords[0].polyterms[i][0] = 0.0;
      found = gfits_scan (header, name, "%f", 1, &coords[0].polyterms[i][0]);
    } else {
      coords[0].polyterms[i-7][1] = 0.0;
      found = gfits_scan (header, name, "%f", 1, &coords[0].polyterms[i-7][1]);
    }
    // PV2_1 is implicit if not present
    if ((i == 1) && !found) {
      coords[0].polyterms[1][0] = 1.0;
      continue;
    }
    // set Npolyterms based on the largest coefficient found
    if (found) {
      Nmax = i;
    }
  }
  coords[0].Npolyterms = Nmax + 1;
  return TRUE;
}

int PutCoords (Coords *coords, Header *header) {
  
  int OldAIPS;
  char csys[16], ctype[32];
  double rotate, Lambda;

  /* modifications to the ctype? */
  /* note that Coords.ctype carries the DEC (ctype2) value */
  OldAIPS = FALSE;
  gfits_modify (header, "CTYPE2",   "%s",  1, coords[0].ctype);
  if (!strcmp(coords[0].ctype, "MM")) {
    gfits_modify (header, "CTYPE1",   "%s",  1, "LL");
    OldAIPS = TRUE;
  } else {
    strcpy (csys, "NONE");
    if (!strncmp (coords[0].ctype, "DEC-", 4)) strcpy (csys, "RA--");
    if (!strncmp (coords[0].ctype, "GLAT", 4)) strcpy (csys, "GLON");
    if (!strncmp (coords[0].ctype, "ELAT", 4)) strcpy (csys, "ELON");
    if (!strncmp (coords[0].ctype, "HLAT", 4)) strcpy (csys, "HLON");
    if (!strncmp (coords[0].ctype, "SLAT", 4)) strcpy (csys, "SLON");
    if (!strcmp (csys, "NONE")) return (FALSE);
    snprintf (ctype, 32, "%s-%s", csys, &coords[0].ctype[5]);
    gfits_modify (header, "CTYPE1",   "%s",  1, ctype);
  }    

  gfits_modify (header, "CDELT1",   "%le", 1, coords[0].cdelt1); 
  gfits_modify (header, "CDELT2",   "%le", 1, coords[0].cdelt2);
  gfits_modify (header, "CRVAL1",   "%lf", 1, coords[0].crval1);
  gfits_modify (header, "CRVAL2",   "%lf", 1, coords[0].crval2);  
  gfits_modify (header, "CRPIX1",   "%lf", 1, coords[0].crpix1);
  gfits_modify (header, "CRPIX2",   "%lf", 1, coords[0].crpix2);

  if (OldAIPS) {
    Lambda = coords[0].cdelt2 / coords[0].cdelt1;
    rotate = DEG_RAD*atan2 (coords[0].pc2_1*Lambda, coords[0].pc1_1);
    gfits_modify (header, "CROTA1", "%f", 1, rotate);
    gfits_modify (header, "CROTA2", "%f", 1, rotate);
    return (TRUE);
  } 

  gfits_modify (header, "PC001001", "%le", 1, coords[0].pc1_1);
  gfits_modify (header, "PC001002", "%le", 1, coords[0].pc1_2);
  gfits_modify (header, "PC002001", "%le", 1, coords[0].pc2_1);
  gfits_modify (header, "PC002002", "%le", 1, coords[0].pc2_2);
  gfits_modify (header, "NPLYTERM", "%d",  1, coords[0].Npolyterms);

  /* RA Terms */
  if (coords[0].Npolyterms > 1) {
    gfits_modify (header, "PCA1X2Y0", "%le", 1, coords[0].polyterms[0][0]);   /* polyterms[0]); */
    gfits_modify (header, "PCA1X1Y1", "%le", 1, coords[0].polyterms[1][0]);   /* polyterms[1]); */
    gfits_modify (header, "PCA1X0Y2", "%le", 1, coords[0].polyterms[2][0]);   /* polyterms[2]); */
  }
  if (coords[0].Npolyterms > 2) {
    gfits_modify (header, "PCA1X3Y0", "%le", 1, coords[0].polyterms[3][0]);   /* polyterms[3]); */
    gfits_modify (header, "PCA1X2Y1", "%le", 1, coords[0].polyterms[4][0]);   /* polyterms[4]); */
    gfits_modify (header, "PCA1X1Y2", "%le", 1, coords[0].polyterms[5][0]);   /* polyterms[5]); */
    gfits_modify (header, "PCA1X0Y3", "%le", 1, coords[0].polyterms[6][0]);   /* polyterms[6]); */
  }

  /* Dec Terms */
  if (coords[0].Npolyterms > 1) {
    gfits_modify (header, "PCA2X2Y0", "%le", 1, coords[0].polyterms[0][1]);   /* polyterms[7]); */
    gfits_modify (header, "PCA2X1Y1", "%le", 1, coords[0].polyterms[1][1]);   /* polyterms[8]); */
    gfits_modify (header, "PCA2X0Y2", "%le", 1, coords[0].polyterms[2][1]);   /* polyterms[9]); */
  }
  if (coords[0].Npolyterms > 2) {
    gfits_modify (header, "PCA2X3Y0", "%le", 1, coords[0].polyterms[3][1]);   /* polyterms[10]); */
    gfits_modify (header, "PCA2X2Y1", "%le", 1, coords[0].polyterms[4][1]);   /* polyterms[11]); */
    gfits_modify (header, "PCA2X1Y2", "%le", 1, coords[0].polyterms[5][1]);   /* polyterms[12]); */
    gfits_modify (header, "PCA2X0Y3", "%le", 1, coords[0].polyterms[6][1]);   /* polyterms[13]); */
  }
  return (TRUE);
}

void coords_precess (double *ra, double *dec, double in_epoch, double out_epoch) {

  double T;
  double A, D, RA, DEC, zeta, z, theta;
  double SA, CA, SD, CD;
  
  T = (out_epoch - in_epoch) / 100.0;
  
  zeta  = RAD_DEG*(0.6406161*T + 0.0000839*T*T + 0.0000050*T*T*T);
  theta = RAD_DEG*(0.5567530*T - 0.0001185*T*T - 0.0000116*T*T*T);
  z     =          0.6406161*T + 0.0003041*T*T + 0.0000051*T*T*T;
  
  A = *ra;
  D = *dec;
  SD =  cos(RAD_DEG*A + zeta)*sin(theta)*cos(RAD_DEG*D) + cos(theta)*sin(RAD_DEG*D);
  CD = sqrt (1 - SD*SD);
  SA =  sin(RAD_DEG*A + zeta)*cos(RAD_DEG*D)/CD;
  CA = (cos(RAD_DEG*A + zeta)*cos(theta)*cos(RAD_DEG*D) - sin(theta)*sin(RAD_DEG*D))/CD;
  
  DEC = DEG_RAD*asin(SD);
  RA  = DEG_RAD*atan2(SA, CA) + z;
  
  if (RA < 0)
    RA += 360;
  
  *ra = RA;
  *dec = DEC; 
}

/* -PLY projection is an extrapolation of the -TAN projection. 
   In addition to the usual linear terms of CRPIXi, PC00i00j, there are 
   higher order polynomial terms (up to 3rd order):
   Axis 1 terms:
   PCA1X2Y0 = coords.polyterm[0][0] = x^2                                             
   PCA1X1Y1 = coords.polyterm[1][0] = xy                                          
   PCA1X0Y2 = coords.polyterm[2][0] = y^2                                             
   PCA1X3Y0 = coords.polyterm[3][0] = x^3                                             
   PCA1X2Y1 = coords.polyterm[4][0] = x^2 y                                             
   PCA1X1Y2 = coords.polyterm[5][0] = x y^2                                             
   PCA1X0Y3 = coords.polyterm[6][0] = y^2                                              
   Axis 2 terms:
   PCA2X2Y0 = coords.polyterm[0][1] = x^2                                               
   PCA2X1Y1 = coords.polyterm[1][1] = xy                                                
   PCA2X0Y2 = coords.polyterm[2][1] = y^2                                               
   PCA2X3Y0 = coords.polyterm[3][1] = x^3                                               
   PCA2X2Y1 = coords.polyterm[4][1] = x^2 y                                             
   PCA2X1Y2 = coords.polyterm[5][1] = x y^2                                             
   PCA2X0Y3 = coords.polyterm[6][1] = y^2                                               
*/

# if (0)

  /** convert pixel coordinates to cartesian system **/
  X = coords[0].cdelt1*(x - coords[0].crpix1);
  Y = coords[0].cdelt2*(y - coords[0].crpix2);
  if (Polynomi) {
    if (coords[0].Npolyterms > 2) {
      X += coords[0].cdelt1*(x*x*coords[0].polyterms[0][0] + x*y*coords[0].polyterms[1][0] + y*y*coords[0].polyterms[2][0]);
      Y += coords[0].cdelt2*(x*x*coords[0].polyterms[0][1] + x*y*coords[0].polyterms[1][1] + y*y*coords[0].polyterms[2][1]);
    }
    if (coords[0].Npolyterms > 2) {
      X += coords[0].cdelt1*(x*x*x*coords[0].polyterms[3][0] + x*x*y*coords[0].polyterms[4][0] + x*y*y*coords[0].polyterms[5][0] + y*y*y*coords[0].polyterms[6][0]);
      Y += coords[0].cdelt2*(x*x*x*coords[0].polyterms[3][1] + x*x*y*coords[0].polyterms[4][1] + x*y*y*coords[0].polyterms[5][1] + y*y*y*coords[0].polyterms[6][1]);
    }
  }

  L = (X*coords[0].pc1_1 + Y*coords[0].pc1_2);
  M = (X*coords[0].pc2_1 + Y*coords[0].pc2_2);
/** this code is the old method used for higher order terms.  they
    are essentially 6th order, with weird coupled terms.
    I don't think any real data used these terms, but they should 
    be re-calculated, I would think 
**/

# endif

/*

Projections and Transformations

The Coords structure is used to represent the standard FITS header representations of the WCS terms.
The FITS WCS encapsulates several concepts in one system: coordinate frame, projection, and
transformations.  The ctype variable defines both the frame (ie, RA/DEC, GLON/GLAT, etc) and the
projection (ie, AIT, SIN, TAN, etc).  The associated terms (PC00i00j) define transformations from
the projection system to another linear coordinate system.  I have extended the basic WCS
transformation terms to include higher-order polynomial terms.  The presence of higher-order terms
is indicated in the header by the NPLYTERM keyword, with a value greater than 1 (a value of 0 or 1
implies no higher-order coefficients).  The coefficients have keywords of the form PCAnXiYj where
'n' is the axis corresponding to the dependent variable, 'i' is the order of the X component and 'j'
is the order of the Y component.  A value of 2 indicates that the second-order coefficients are
defined (PCAnX2Y0, PCAnX1Y1, PCAnX0Y2); A value of 3 indicates that the third-order coefficients are
also defined (PCAnX3Y0, PCAnX2Y1, PCAnX1Y2, PCAnX0Y3).  Some headers in the past were generated
without the NPLYTERM keyword.  a value of PLY, DIS, or WRP for the projection without a
corresponding value for NPLYTERM implies that the value should be interpreted as '3'.

*/

  /* PLY is equiv to LIN with higher order terms
     ZPL is equiv to ZEA with higher order terms
     DIS is equiv to TAN with higher order terms
     WRP is equiv to PLY, with implied mosaic */

OhanaProjection GetProjection (char *ctype) {
  if (!strncmp(&ctype[4], "-ZEA", 4)) return PROJ_ZEA;
  if (!strncmp(&ctype[4], "-ZPL", 4)) return PROJ_ZPL;
  if (!strncmp(&ctype[4], "-ZPN", 4)) return PROJ_ZPN;
  if (!strncmp(&ctype[4], "-ARC", 4)) return PROJ_ARC;
  if (!strncmp(&ctype[4], "-STG", 4)) return PROJ_STG;
  if (!strncmp(&ctype[4], "-SIN", 4)) return PROJ_SIN;
  if (!strncmp(&ctype[0], "MM", 2))   return PROJ_SIN; // note ctype[0]
  if (!strncmp(&ctype[4], "-TAN", 4)) return PROJ_TAN;
  if (!strncmp(&ctype[4], "-TNX", 4)) return PROJ_TNX;
  if (!strncmp(&ctype[4], "-DIS", 4)) return PROJ_DIS;
  if (!strncmp(&ctype[4], "-LIN", 4)) return PROJ_LIN;
  if (!strncmp(&ctype[0], "GENE", 4)) return PROJ_LIN; // note ctype[0]
  if (!strncmp(&ctype[4], "-PLY", 4)) return PROJ_PLY;
  if (!strncmp(&ctype[4], "-WRP", 4)) return PROJ_WRP;
  if (!strncmp(&ctype[4], "-AIT", 4)) return PROJ_AIT;
  if (!strncmp(&ctype[4], "-GLS", 4)) return PROJ_GLS;
  if (!strncmp(&ctype[4], "-PAR", 4)) return PROJ_PAR;
  if (!strncmp(&ctype[4], "-MOL", 4)) return PROJ_MOL;
  return PROJ_NONE;
}
  
int SetProjection (char *ctype, OhanaProjection proj) {
  switch (proj) {
    case PROJ_ZEA: strcpy(&ctype[4], "-ZEA"); return TRUE;
    case PROJ_ZPL: strcpy(&ctype[4], "-ZPL"); return TRUE;
    case PROJ_ZPN: strcpy(&ctype[4], "-ZPN"); return TRUE;
    case PROJ_ARC: strcpy(&ctype[4], "-ARC"); return TRUE;
    case PROJ_STG: strcpy(&ctype[4], "-STG"); return TRUE;
    case PROJ_SIN: strcpy(&ctype[4], "-SIN"); return TRUE;
    case PROJ_TAN: strcpy(&ctype[4], "-TAN"); return TRUE;
    case PROJ_TNX: strcpy(&ctype[4], "-TNX"); return TRUE;
    case PROJ_DIS: strcpy(&ctype[4], "-DIS"); return TRUE;
    case PROJ_LIN: strcpy(&ctype[4], "-LIN"); return TRUE;
    case PROJ_PLY: strcpy(&ctype[4], "-PLY"); return TRUE;
    case PROJ_WRP: strcpy(&ctype[4], "-WRP"); return TRUE;
    case PROJ_AIT: strcpy(&ctype[4], "-AIT"); return TRUE;
    case PROJ_GLS: strcpy(&ctype[4], "-GLS"); return TRUE;
    case PROJ_PAR: strcpy(&ctype[4], "-PAR"); return TRUE;
    case PROJ_MOL: strcpy(&ctype[4], "-MOL"); return TRUE;
    case PROJ_NONE: return FALSE;
  }
  return FALSE;
}  

OhanaProjectionMode GetProjectionMode (OhanaProjection proj) {
  switch (proj) {
    case PROJ_ZEA:
    case PROJ_ZPL:
    case PROJ_ZPN:
    case PROJ_ARC:
    case PROJ_STG:
    case PROJ_SIN:
    case PROJ_TAN:
    case PROJ_TNX:
    case PROJ_DIS:
      return PROJ_MODE_ZENITHAL;
    case PROJ_LIN: 
    case PROJ_PLY: 
    case PROJ_WRP: 
      return PROJ_MODE_CARTESIAN;
    case PROJ_AIT:
    case PROJ_GLS:
    case PROJ_PAR:
    case PROJ_MOL:
      return PROJ_MODE_PSEUDOCYL;
    default: 
      return PROJ_MODE_NONE;
  }
  return PROJ_MODE_NONE;
}

