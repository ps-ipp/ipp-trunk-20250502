# include "astro.h"

int precess (int argc, char **argv) {

  int i, Julian, Besselian;
  double T, in_epoch, out_epoch;
  double A, D, RA, DEC, zeta, z, theta;
  double SA, CA, SD, CD;
  Vector *xvec, *yvec;

  Besselian = Julian = 0;
  Besselian = get_argument (argc, argv, "B");
  Julian    = get_argument (argc, argv, "J");

  in_epoch = out_epoch = 2000.0;
  if (argc != 5) {
    gprint (GP_ERR, "USAGE:  precess (from) (to) RA DEC \n");
    gprint (GP_ERR, "   you may use B for B1950.0 or J for J2000.0\n");
    return (FALSE);
  }

  if (!Julian && !Besselian) { /* assume Julian! */
    in_epoch  = get_epoch (argv[1], 'J');
    out_epoch = get_epoch (argv[2], 'J');
  }

  if ((Julian == 1) && !Besselian) {
    in_epoch  = 2000.0;
    out_epoch = get_epoch(argv[2], 'J');
  }

  if ((Julian == 2) && !Besselian) {
    in_epoch  = get_epoch(argv[1], 'J');
    out_epoch = 2000.0;
  }

  if ((Besselian == 1) && !Julian) {
    in_epoch  = BtoJ(1950.0); 
    out_epoch = get_epoch(argv[2], 'B'); 
  }

  if ((Besselian == 2) && !Julian) {
    in_epoch  = get_epoch(argv[1], 'B'); 
    out_epoch = BtoJ(1950.0); 
  }
  
  if (Julian && Besselian) {
    if (Julian > Besselian) {
      in_epoch  = BtoJ(1950.0); 
      out_epoch = 2000.0;
    }
    else {
      in_epoch  = 2000.0;
      out_epoch = BtoJ(1950.0); 
    }
  }

  gprint (GP_ERR, "converting from J%f to J%f\n", in_epoch, out_epoch);

  T = (out_epoch - in_epoch) / 100.0;
  
  zeta  = RAD_DEG*(0.6406161*T + 0.0000839*T*T + 0.0000050*T*T*T);
  theta = RAD_DEG*(0.5567530*T - 0.0001185*T*T - 0.0000116*T*T*T);
  z     =          0.6406161*T + 0.0003041*T*T + 0.0000051*T*T*T;

  if (ISNUM(argv[3][0]) && ISNUM(argv[4][0])) {
    A = atof (argv[3]);
    D = atof (argv[4]);
    SD =  cos(RAD_DEG*A + zeta)*sin(theta)*cos(RAD_DEG*D) + cos(theta)*sin(RAD_DEG*D);
    CD = sqrt (1 - SD*SD);
    SA =  sin(RAD_DEG*A + zeta)*cos(RAD_DEG*D)/CD;
    CA = (cos(RAD_DEG*A + zeta)*cos(theta)*cos(RAD_DEG*D) - sin(theta)*sin(RAD_DEG*D))/CD;

    DEC = DEG_RAD*asin(SD);
    RA  = DEG_RAD*atan2(SA, CA) + z;

    if (RA < 0)
      RA += 360;
    gprint (GP_LOG, "%f %f -> %f %f\n", A, D, RA, DEC);
    return (TRUE);
  }    

  /* find vectors */
  if ((xvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[3], argv[4]);
    return (FALSE);
  }
  
  // cast xvec or yvec to FLT if it is type INT
  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);

  opihi_flt *Av = xvec[0].elements.Flt;
  opihi_flt *Dv = yvec[0].elements.Flt;

  for (i = 0; i < xvec[0].Nelements; i++, Av++, Dv++) {
    A = *Av;
    D = *Dv;
    SD =  cos(RAD_DEG*A + zeta)*sin(theta)*cos(RAD_DEG*D) + cos(theta)*sin(RAD_DEG*D);
    CD = sqrt (1 - SD*SD);
    SA =  sin(RAD_DEG*A + zeta)*cos(RAD_DEG*D)/CD;
    CA = (cos(RAD_DEG*A + zeta)*cos(theta)*cos(RAD_DEG*D) - sin(theta)*sin(RAD_DEG*D))/CD;

    DEC = DEG_RAD*asin(SD);
    RA  = DEG_RAD*atan2(SA, CA) + z;

    if (RA < 0) RA += 360;
    
    *Av = RA;
    *Dv = DEC; 
  }

  return (TRUE);

}  
