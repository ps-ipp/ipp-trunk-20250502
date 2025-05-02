# include "dimm.h"

# define dCOS(A)   ((double) cos ((double)RAD_DEG*A))
# define dSIN(A)   ((double) sin ((double)RAD_DEG*A))

double atan2 (double y, double x);

int altaz (int argc, char **argv) {
  
  double alt, az, lat, rot;
  double ha, dec;
  double sind, sinh, cosh;
  char *latstr;

  if (argc != 6) goto usage;

  if (!strcmp (argv[1], "-h")) goto radec;
  if (!strcmp (argv[1], "-c")) goto altaz;

 radec:
  /* ha/dec -> alt/az */
  ha  = atof (argv[2]);
  dec = atof (argv[3]);

  latstr = get_variable ("LATITUDE");
  if (latstr == (char *) NULL) {
    gprint (GP_ERR, "please define $LATITUDE\n");
    return (FALSE);
  }
  lat = atof (latstr);
  free (latstr);
 
  sind = dSIN (dec) * dSIN (lat) + dCOS (dec) * dCOS (ha) * dCOS (lat);
  alt  = DEG_RAD * asin (sind);

  sinh = - dCOS (dec) * dSIN (ha);
  cosh =   dSIN (dec) * dCOS (lat) - dCOS (dec) * dCOS (ha) * dSIN (lat);

  az = DEG_RAD * atan2 (sinh, cosh);
  set_variable (argv[4], alt);
  set_variable (argv[5], az);

  sinh = -dCOS(az) * dSIN(alt) * dSIN(ha) * dSIN(lat) + dSIN(az) * dSIN(alt) * dCOS(ha) - dSIN(ha) * dCOS(alt) * dCOS(lat);
  cosh = -dSIN(az) * dSIN(ha) * dSIN(lat) - dCOS(az) * dCOS(ha);
  rot = -DEG_RAD * atan2 (sinh, cosh);
  set_variable ("ROT", rot);

  return (TRUE);
  
 altaz:
  /* alt/az -> ha/dec */
  alt = atof (argv[4]);
  az  = atof (argv[5]);

  latstr = get_variable ("LATITUDE");
  if (latstr == (char *) NULL) {
    gprint (GP_ERR, "please define $LATITUDE\n");
    return (FALSE);
  }
  lat = atof (latstr);
  free (latstr);

  sind = dSIN (alt) * dSIN (lat) + dCOS (alt) * dCOS (az) * dCOS (lat);
  dec  = DEG_RAD * asin (sind);

  sinh = -dCOS (alt) * dSIN (az);
  cosh =  dSIN (alt) * dCOS (lat) - dCOS (alt) * dCOS (az) * dSIN (lat);

  ha = DEG_RAD * atan2 (sinh, cosh);
  set_variable (argv[2], ha);
  set_variable (argv[3], dec);

  sinh = -dCOS(az) * dSIN(alt) * dSIN(ha) * dSIN(lat) + dSIN(az) * dSIN(alt) * dCOS(ha) - dSIN(ha) * dCOS(alt) * dCOS(lat);
  cosh = -dSIN(az) * dSIN(ha) * dSIN(lat) - dCOS(az) * dCOS(ha);
  rot = -DEG_RAD * atan2 (sinh, cosh);
  set_variable ("ROT", rot);

  return (TRUE);
  
 usage:
  gprint (GP_ERR, "USAGE: altaz -h (ha) (dec) (alt) (az)\n");
  gprint (GP_ERR, "USAGE: altaz -c (ha) (dec) (alt) (az)\n");
  gprint (GP_ERR, "       -h alt/az to ha/dec, -c ha/dec to alt/az\n");
  gprint (GP_ERR, "       returned values in variables provided\n");
  return (FALSE);

}

