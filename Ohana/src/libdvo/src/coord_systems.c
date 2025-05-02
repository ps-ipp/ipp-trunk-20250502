# include "dvo.h"
    
CoordTransform *AllocTransform (double phi, double Xo, double xo) {

  CoordTransform *transform;
  ALLOCATE (transform, CoordTransform, 1);
  transform->isIdentity = FALSE;
  
  transform->phi = phi*RAD_DEG;
  transform->Xo  = Xo*RAD_DEG;
  transform->xo  = xo;

  // pre-calculated constants:
  transform->sin_phi_cos_Xo = sin(transform->phi)*cos(transform->Xo);
  transform->sin_phi_sin_Xo = sin(transform->phi)*sin(transform->Xo);
  transform->cos_phi        = cos(transform->phi);
  
  transform->cos_phi_cos_Xo = cos(transform->phi)*cos(transform->Xo);
  transform->cos_phi_sin_Xo = cos(transform->phi)*sin(transform->Xo);
  transform->sin_phi        = sin(transform->phi);
  transform->cos_Xo 	    = cos(transform->Xo);
  transform->sin_Xo 	    = sin(transform->Xo);

  return transform;
}

CoordTransform *InitTransform (CoordTransformSystem input, CoordTransformSystem output) {

  CoordTransform *transform;
  struct timeval now;
  struct tm *local;
  double T;

  ALLOCATE (transform, CoordTransform, 1);
  transform->isIdentity = FALSE;
  
  switch (input) {
    case COORD_CELESTIAL:
      switch (output) {
	case COORD_CELESTIAL:
	  transform->phi = 0.0;
	  transform->Xo  = 0.0;
	  transform->xo  = 0.0;
	  transform->isIdentity = TRUE;
	  break;
	case COORD_GALACTIC:

	  // J2000 FK5 transformation from Liu et al 2011 (A&A 526, A16)
	  transform->phi = -62.8717488056*RAD_DEG;
	  transform->Xo  = 282.8594812080*RAD_DEG;
	  transform->xo  =  32.9319185700;

	  // these are derived from their eqn (10):
	  // \alpha^p_J =  12h 51m 26.27549s
	  // \delta^p_J = +27d 07' 41.7043"
	  // \theta_J   = 122.93191857 deg
	  // phi = -90 - \alpha^p_J
	  // Xo  = +90 - \delta^p_J
	  // xo  = \theta_J - 90

	  // Liu et al also propose an alternative ICRS definition using:
	  // \alpha^p_J =  12h 51m 26.27469s
	  // \delta^p_J = +27d 07' 41.7087"
	  // \theta_J   = 122.93192526 deg

	  // these differ from the above at the several milliarcsec level

	  // These values were used in the past, but transform to/from B1950.0
	  // transform->phi = -62.60*RAD_DEG;
	  // transform->Xo  = 282.25*RAD_DEG;
	  // transform->xo  =  33.00;

	  break;
	case COORD_GALACTIC_REID_2004:

	  // J2000 FK5 transformation from Reid et al 2004, ApJ 616, 872
	  transform->phi = (27.128336111 - 90.0)*RAD_DEG; // +27d 07' 42.01" (J2000) 
	  transform->Xo  = (192.859508333 + 90.0)*RAD_DEG; // 12h 51m 26.282s (J2000)
	  transform->xo  =  32.932;

	  // comments from LSD code (lsd/builtins/misc.py, Mario Juric)
	  // # Appendix of Reid et al. (http://adsabs.harvard.edu/cgi-bin/bib_query?2004ApJ...616..872R)
	  // # This convention is also used by LAMBDA/WMAP (http://lambda.gsfc.nasa.gov/toolbox/tb_coordconv.cfm)

	  // _angp = np.radians(192.859508333) #  12h 51m 26.282s (J2000)
	  // _dngp = np.radians(27.128336111)  # +27d 07' 42.01" (J2000) 
	  // _l0   = np.radians(32.932)

	  break;
	case COORD_ECLIPTIC:
	  gettimeofday (&now, NULL);
	  local = localtime (&now.tv_sec);
	  T = local[0].tm_year / 100.0;
	  transform->phi = -1.0*RAD_DEG*(23.452294 - 0.013013*T - 0.000001639*T*T + 0.000000503*T*T*T);
	  transform->Xo  = 0.0;
	  transform->xo  = 0.0;
	  break;
	default:
	  abort();
      }
      break;
    case COORD_ECLIPTIC:
      switch (output) {
	case COORD_CELESTIAL:
	  gettimeofday (&now, (struct timezone *) NULL);
	  local = localtime (&now.tv_sec);
	  T = local[0].tm_year / 100.0;
	  transform->phi = RAD_DEG*(23.452294 - 0.013013*T - 0.000001639*T*T + 0.000000503*T*T*T);
	  transform->Xo = 0.0;
	  transform->xo = 0.0;
	  break;
	case COORD_GALACTIC:
	  return NULL;
	  break;
	case COORD_ECLIPTIC:
	  transform->phi = 0.0;
	  transform->Xo  = 0.0;
	  transform->xo  = 0.0;
	  transform->isIdentity = TRUE;
	  break;
	default:
	  abort();
      }
      break;
    case COORD_GALACTIC:
      switch (output) {
	case COORD_CELESTIAL:
	  // J2000 transformation from Liu et al 2011 (A&A 526, A16)
	  transform->phi =  62.8717488056*RAD_DEG;
	  transform->Xo  =  32.9319185700*RAD_DEG;
	  transform->xo  = 282.8594812080;

	  // These values were used in the past, but transform to/from B1950.0
	  // transform->phi =  62.60*RAD_DEG;
	  // transform->Xo  =  33.00*RAD_DEG;
	  // transform->xo  = 282.25;
	  break;
	case COORD_GALACTIC:
	  transform->phi = 0.0;
	  transform->Xo  = 0.0;
	  transform->xo  = 0.0;
	  transform->isIdentity = TRUE;
	  break;
	case COORD_ECLIPTIC:
	  return NULL;
	default:
	  abort();
      }
      break;
    case COORD_GALACTIC_REID_2004:
      switch (output) {
	case COORD_CELESTIAL:
	  // J2000 FK5 transformation from Reid et al 2004, ApJ 616, 872
	  transform->phi = (90.0 - 27.128336111)*RAD_DEG; // +27d 07' 42.01" (J2000) 
	  transform->Xo  =  32.932*RAD_DEG;
	  transform->xo  = (192.859508333 + 90.0); // 12h 51m 26.282s (J2000)

	  // These values were used in the past, but transform to/from B1950.0
	  // transform->phi =  62.60*RAD_DEG;
	  // transform->Xo  =  33.00*RAD_DEG;
	  // transform->xo  = 282.25;
	  break;
	case COORD_GALACTIC:
	  transform->phi = 0.0;
	  transform->Xo  = 0.0;
	  transform->xo  = 0.0;
	  transform->isIdentity = TRUE;
	  break;
	case COORD_ECLIPTIC:
	  return NULL;
	default:
	  abort();
      }
      break;
    default:
      abort();
  }
 
  // pre-calculated constants:
  transform->sin_phi_cos_Xo = sin(transform->phi)*cos(transform->Xo);
  transform->sin_phi_sin_Xo = sin(transform->phi)*sin(transform->Xo);
  transform->cos_phi        = cos(transform->phi);
  
  transform->cos_phi_cos_Xo = cos(transform->phi)*cos(transform->Xo);
  transform->cos_phi_sin_Xo = cos(transform->phi)*sin(transform->Xo);
  transform->sin_phi        = sin(transform->phi);
  transform->cos_Xo 	    = cos(transform->Xo);
  transform->sin_Xo 	    = sin(transform->Xo);

  return transform;
}

// input and output coordinates are in degrees
int ApplyTransform (double *x, double *y, double X, double Y, CoordTransform *transform) {

  double sin_x, sin_y, cos_x, cos_y;


  if (transform == NULL) return (FALSE);

  if (transform->isIdentity) {
    *x = X;
    *y = Y;
    return (TRUE);
  }

  X *= RAD_DEG;
  Y *= RAD_DEG;

  double cosY = cos(Y);
  double sinY = sin(Y);
  
  double cosX = cos(X);
  double sinX = sin(X);

  // recast with constants extracted:
  sin_y = cosY*sinX*transform->sin_phi_cos_Xo - cosY*cosX*transform->sin_phi_sin_Xo + sinY*transform->cos_phi;
  cos_y = sqrt (1 - sin_y*sin_y);

  sin_x = cosY*sinX*transform->cos_phi_cos_Xo - cosY*cosX*transform->cos_phi_sin_Xo - sinY*transform->sin_phi;
  cos_x = cosY*cosX*transform->cos_Xo + cosY*sinX*transform->sin_Xo;
      
  // atan2 returns -pi : +pi
  *x = DEG_RAD * atan2 (sin_x, cos_x) + transform->xo;
  if ((*x) <   0.0) (*x) += 360;
  if ((*x) > 360.0) (*x) -= 360;

  // should be in range -pi/2 : +pi/2
  *y = DEG_RAD * atan2 (sin_y, cos_y);

  return (TRUE);
}

// sin_y = cos(Y)*sin(X - Xo)*sin(phi) + sin(Y)*cos(phi);

// sin_x = (cos(Y)*sin(X - Xo)*cos(phi) - sin(Y)*sin(phi)) /  cos_y;
// cos_x = cos(Y)*cos(X - Xo) / cos_y;

// multiplying both sides by cos_y:
// sin_x = (cos(Y)*sin(X - Xo)*cos(phi) - sin(Y)*sin(phi));
// cos_x = cos(Y)*cos(X - Xo);
      
// sin(a - b) = sin(a)*cos(b) - sin(b)*cos(a);
// cos(a - b) = sin(a)*sin(b) + cos(a)*cos(b);
