# include "astro.h"

# define dCOS(A)   ((double) cos ((double)RAD_DEG*A))
# define dSIN(A)   ((double) sin ((double)RAD_DEG*A))

int HD_to_AZ(double *alt, double *az, double *rot, double ha, double dec, double lat);
int AZ_to_HD(double *ha, double *dec, double *rot, double alt, double az, double lat);

double atan2 (double y, double x);

int altaz (int argc, char **argv) {
  
  int i;
  double alt, az, lat, rot;
  double ha, dec;
  char *latstr;
  Vector *hvec, *dvec, *avec, *zvec, *rvec;
  opihi_flt *Hv, *Dv, *Av, *Zv, *Rv;

  if (argc != 7) goto usage;
  if (strcasecmp (argv[4], "to")) goto usage;

  latstr = get_variable ("LATITUDE");
  if (latstr == (char *) NULL) {
    gprint (GP_ERR, "please define $LATITUDE\n");
    return FALSE;
  }
  lat = atof (latstr);
  gprint (GP_ERR, "using latitude of %f\n", lat);
  free (latstr);

  if (!strcmp (argv[1], "-az")) goto radec;
  if (!strcmp (argv[1], "-hd")) goto altaz;

 radec:
  if (ISNUM(argv[2][0]) && ISNUM(argv[3][0])) {
    /* ha/dec -> alt/az */
    ha  = atof (argv[2]);
    dec = atof (argv[3]);
  
    HD_to_AZ (&alt, &az, &rot, ha, dec, lat);

    set_variable (argv[5], alt);
    set_variable (argv[6], az);
    set_variable ("ROT", rot);
    return TRUE;
  } 

  /* find vectors */
  if ((hvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return FALSE;
  if ((dvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return FALSE;
  if ((avec = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return FALSE;
  if ((zvec = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) return FALSE;
  if ((rvec = SelectVector ("ROT",   ANYVECTOR, TRUE)) == NULL) return FALSE;

  if (hvec[0].Nelements != dvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[2], argv[3]);
    return FALSE;
  }
  
  // cast xvec or yvec to FLT if it is type INT
  CastVector (hvec, OPIHI_FLT);
  CastVector (dvec, OPIHI_FLT);
  ResetVector (avec, OPIHI_FLT, hvec[0].Nelements);
  ResetVector (zvec, OPIHI_FLT, hvec[0].Nelements);
  ResetVector (rvec, OPIHI_FLT, hvec[0].Nelements);

  Hv = hvec[0].elements.Flt;
  Dv = dvec[0].elements.Flt;
  Av = avec[0].elements.Flt;
  Zv = zvec[0].elements.Flt;
  Rv = rvec[0].elements.Flt;

  for (i = 0; i < hvec[0].Nelements; i++, Hv++, Dv++, Av++, Zv++, Rv++) {
    HD_to_AZ (Av, Zv, Rv, *Hv, *Dv, lat);
  }
  return TRUE;
  
 altaz:
  if (ISNUM(argv[2][0]) && ISNUM(argv[3][0])) {
    /* alt/az -> ha/dec */
    alt = atof (argv[2]);
    az  = atof (argv[3]);

    AZ_to_HD (&ha, &dec, &rot, alt, az, lat);

    set_variable (argv[5], ha);
    set_variable (argv[6], dec);
    set_variable ("ROT", rot);
    return TRUE;
  }
  
  /* find vectors */
  if ((avec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return FALSE;
  if ((zvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return FALSE;
  if ((hvec = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return FALSE;
  if ((dvec = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) return FALSE;
  if ((rvec = SelectVector ("ROT",   ANYVECTOR, TRUE)) == NULL) return FALSE;

  if (avec[0].Nelements != zvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[2], argv[3]);
    return FALSE;
  }
  
  // cast xvec or yvec to FLT if it is type INT
  CastVector (avec, OPIHI_FLT);
  CastVector (zvec, OPIHI_FLT);
  ResetVector (hvec, OPIHI_FLT, avec[0].Nelements);
  ResetVector (dvec, OPIHI_FLT, avec[0].Nelements);
  ResetVector (rvec, OPIHI_FLT, avec[0].Nelements);

  Hv = hvec[0].elements.Flt;
  Dv = dvec[0].elements.Flt;
  Av = avec[0].elements.Flt;
  Zv = zvec[0].elements.Flt;
  Rv = rvec[0].elements.Flt;

  for (i = 0; i < hvec[0].Nelements; i++, Hv++, Dv++, Av++, Zv++, Rv++) {
    AZ_to_HD (Hv, Dv, Rv, *Av, *Zv, lat);
  }
  return TRUE;
  
 usage:
  gprint (GP_ERR, "USAGE: altaz -az (ha) (dec) to (alt) (az)\n");
  gprint (GP_ERR, "USAGE: altaz -hd (alt) (az) to (ha) (dec)\n");
  gprint (GP_ERR, "       -hd alt/az to ha/dec, -az ha/dec to alt/az\n");
  gprint (GP_ERR, "       returned values in variables or vectors provided\n");
  return FALSE;
}

int HD_to_AZ(double *alt, double *az, double *rot, double ha, double dec, double lat) {

  double sind, sinh, cosh;

  sind = dSIN (dec) * dSIN (lat) + dCOS (dec) * dCOS (ha) * dCOS (lat);
  *alt  = DEG_RAD * asin (sind);

  sinh = - dCOS (dec) * dSIN (ha);
  cosh =   dSIN (dec) * dCOS (lat) - dCOS (dec) * dCOS (ha) * dSIN (lat);

  *az = DEG_RAD * atan2 (sinh, cosh);
  
  sinh = -dCOS(*az) * dSIN(*alt) * dSIN(ha) * dSIN(lat) + dSIN(*az) * dSIN(*alt) * dCOS(ha) - dSIN(ha) * dCOS(*alt) * dCOS(lat);
  cosh = -dSIN(*az) * dSIN(ha) * dSIN(lat) - dCOS(*az) * dCOS(ha);
  *rot = -DEG_RAD * atan2 (sinh, cosh);
  
  return TRUE;
}

int AZ_to_HD(double *ha, double *dec, double *rot, double alt, double az, double lat) {

  double sind, sinh, cosh;

  sind = dSIN (alt) * dSIN (lat) + dCOS (alt) * dCOS (az) * dCOS (lat);
  *dec  = DEG_RAD * asin (sind);

  sinh = -dCOS (alt) * dSIN (az);
  cosh =  dSIN (alt) * dCOS (lat) - dCOS (alt) * dCOS (az) * dSIN (lat);

  *ha = DEG_RAD * atan2 (sinh, cosh);

  sinh = -dCOS(az) * dSIN(alt) * dSIN(*ha) * dSIN(lat) + dSIN(az) * dSIN(alt) * dCOS(*ha) - dSIN(*ha) * dCOS(alt) * dCOS(lat);
  cosh = -dSIN(az) * dSIN(*ha) * dSIN(lat) - dCOS(az) * dCOS(*ha);
  *rot = -DEG_RAD * atan2 (sinh, cosh);

  return TRUE;
}
