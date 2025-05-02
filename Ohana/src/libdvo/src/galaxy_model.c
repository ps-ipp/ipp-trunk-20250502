# include <dvo.h>

// velocity / distance / proper-motion scale:
static double iFkap = 0.001 / 4.74047; // (km/sec/kpc) / (arcsec/ year)

double A_oort = 0.0; // km/sec/kpc
double B_oort = 0.0; // km/sec/kpc
double U_sol  = 0.0; // km/sec
double V_sol  = 0.0; // km/sec
double W_sol  = 0.0; // km/sec

int InitGalaxyModel (char *version) {

  // galaxy model parameters from Siegfreid Roeser:
  if (!strcmp(version, "ROESER")) {
    A_oort = +14.50; // km/sec/kpc
    B_oort = -13.00; // km/sec/kpc
    U_sol  =   9.44; // km/sec
    V_sol  =  11.90; // km/sec
    W_sol  =   7.20; // km/sec
    return TRUE;
  }
  if (!strcmp(version, "FEAST-HIPPARCOS")) {
    // http://arxiv.org/pdf/astro-ph/9706293v1.pdf
    A_oort = +14.82; // km/sec/kpc
    B_oort = -12.37; // km/sec/kpc
    U_sol  =   9.32; // km/sec
    V_sol  =  11.18; // km/sec
    W_sol  =   7.61; // km/sec
    return TRUE;
  }
  if (!strcmp(version, "TEST-CONSTANT")) {
    // use for testing
    A_oort = +47.40; // km/sec/kpc
    B_oort = -47.40; // km/sec/kpc
    U_sol  =   0.00; // km/sec
    V_sol  =   0.00; // km/sec
    W_sol  =   0.00; // km/sec
    return TRUE;
  }
  if (!strcmp(version, "TEST-ZERO")) {
    // use for testing
    A_oort =   0.00; // km/sec/kpc
    B_oort =   0.00; // km/sec/kpc
    U_sol  =   0.00; // km/sec
    V_sol  =   0.00; // km/sec
    W_sol  =   0.00; // km/sec
    return TRUE;
  }
  return FALSE;
}

int GalaxyMotionModel (double *uL_gal, double *uB_gal, double L, double B) {
  double Lrad = L * RAD_DEG;
  double Brad = B * RAD_DEG;
  *uL_gal =     (A_oort * cos(2.0*Lrad) + B_oort) * cos(Brad)     * iFkap;
  *uB_gal = -0.5*A_oort * sin(2.0*Lrad)           * sin(Brad*2.0) * iFkap;
  return TRUE;
}

int GalaxyMotionModel_radians (double *uL_gal, double *uB_gal, double Lrad, double Brad) {
  *uL_gal =     (A_oort * cos(2.0*Lrad) + B_oort) * cos(Brad)     * iFkap;
  *uB_gal = -0.5*A_oort * sin(2.0*Lrad) *           sin(Brad*2.0) * iFkap;
  return TRUE;
}

int SolarMotionModel (double *uL_sol, double *uB_sol, double L, double B, double distance) {
  double Lrad = L * RAD_DEG;
  double Brad = B * RAD_DEG;
  *uL_sol =  (U_sol * sin(Lrad) - V_sol * cos(Lrad))                              * iFkap / distance;
  *uB_sol = ((U_sol * cos(Lrad) + V_sol * sin(Lrad))*sin(Brad) - W_sol*cos(Brad)) * iFkap / distance;
  return TRUE;
}

int SolarMotionModel_radians (double *uL_sol, double *uB_sol, double Lrad, double Brad, double distance) {
  *uL_sol =  (U_sol * sin(Lrad) - V_sol * cos(Lrad))                              * iFkap / distance;
  *uB_sol = ((U_sol * cos(Lrad) + V_sol * sin(Lrad))*sin(Brad) - W_sol*cos(Brad)) * iFkap / distance;
  return TRUE;
}

// Use with the forward transformation: CELESTIAL->GALACTIC for uR,uD -> uL,uB
int TransformProperMotionForewards (double *uL, double *uB, double uR, double uD, double R, double D, CoordTransform *transform) {

  double Rrad = R*RAD_DEG;
  double Drad = D*RAD_DEG;

  // C1, C2 are from http://arxiv.org/pdf/1306.2945v2.pdf
  double C1 = 
    cos(Drad)*transform->cos_phi +
    sin(Drad)*cos(Rrad)*transform->sin_phi_sin_Xo - 
    sin(Drad)*sin(Rrad)*transform->sin_phi_cos_Xo;

  double C2 =
    - cos(Rrad)*transform->sin_phi_cos_Xo
    - sin(Rrad)*transform->sin_phi_sin_Xo;

  double cosBinv = 1.0 / sqrt(C1*C1 + C2*C2);

  // XXX add errors : I need to be able to choose the stars based on the error distribution
  *uL = cosBinv * (C1 * uR + C2 * uD);
  *uB = cosBinv * (C1 * uD - C2 * uR);

  return TRUE;
}

// Use with the forward transformation: CELESTIAL->GALACTIC for uL,uB -> uR,uD
int TransformProperMotionBackwards (double *uR, double *uD, double uL, double uB, double R, double D, CoordTransform *transform) {

  double Rrad = R*RAD_DEG;
  double Drad = D*RAD_DEG;

  // C1, C2 are from http://arxiv.org/pdf/1306.2945v2.pdf
  double C1 = 
    cos(Drad)*transform->cos_phi +
    sin(Drad)*cos(Rrad)*transform->sin_phi_sin_Xo - 
    sin(Drad)*sin(Rrad)*transform->sin_phi_cos_Xo;

  double C2 =
    - cos(Rrad)*transform->sin_phi_cos_Xo
    - sin(Rrad)*transform->sin_phi_sin_Xo;

  double cosBinv = 1.0 / sqrt(C1*C1 + C2*C2);

  // XXX add errors : I need to be able to choose the stars based on the error distribution
  *uR = cosBinv * (C1 * uL - C2 * uB);
  *uD = cosBinv * (C1 * uB + C2 * uL);

  return TRUE;
}

int TransformProperMotion_radians (double *uR, double *uD, double uL, double uB, double Rrad, double Drad, CoordTransform *transform) {

  // C1, C2 are from http://arxiv.org/pdf/1306.2945v2.pdf
  double C1 = 
    cos(Drad)*transform->cos_phi +
    sin(Drad)*cos(Rrad)*transform->sin_phi_sin_Xo - 
    sin(Drad)*sin(Rrad)*transform->sin_phi_cos_Xo;

  double C2 =
    - cos(Rrad)*transform->sin_phi_cos_Xo
    - sin(Rrad)*transform->sin_phi_sin_Xo;

  double cosBinv = 1.0 / sqrt(C1*C1 + C2*C2);

  // XXX add errors : I need to be able to choose the stars based on the error distribution
  *uR = cosBinv * (C1 * uL - C2 * uB);
  *uD = cosBinv * (C1 * uB + C2 * uL);

  return TRUE;
}

// I am using the galactic rotation and solar motion model to predict the reflex proper
// motion of stars in the database. 

/*****

From Siegfreid Roeser:

Dear Gene,

 please find attached our approach for galactic rotation and
 solar motion that we assumed in our approach last fall.
(Hope I understood correctly what you requested at the
telecon). If not, please come back.

      fkap = 4.74047d-0   {km/s/kpc}/{mas/y}

      gl,gb  is galactic longitude resp. latitude


      PARAMETER (distance= 1.d0,Aoort=14.5d0 ,Boort=-13.d0  )
                      kpc           km/s/kpc   km/s/kpc
      PARAMETER (Usol = 9.44d0, Vsol =11.90d0, Wsol = 7.20d0)
                                   km/s
C
C      Gal. Rotation
C
      proper motions in longitude/latitude

      emulgal = (AOORT*dcos(2.d0*gl)+BOORT)*dcos(gb)/fkap
      emubgal = -0.5d0*AOORT*dsin(2.d0*gl)*dsin(2.d0*gb)/fkap
c
c     Solar Motion
cc
      emulsol = Usol*dsin(gl) - Vsol*dcos(gl)
      emulsol = emulsol/fkap/distance
      emubsol = Usol*dsin(gb)*dcos(gl)+Vsol*dsin(gb)*dsin(gl)
      emubsol = emubsol -Wsol*dcos(gb)
      emubsol = emubsol/fkap/distance


******/
