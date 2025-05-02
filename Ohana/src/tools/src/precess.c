# include <ohana.h>

double BtoJ (double in_epoch);
double get_epoch (char *in_epoch, char mode);

int main (int argc, char **argv) {

  double T, in_epoch, out_epoch;
  double A, D, RA, DEC, zeta, z, theta;
  double SA, CA, SD, CD;
  int Julian, Besselian;

  in_epoch = 2000.0;
  out_epoch = 2000.0;

  Besselian = Julian = 0;
  Besselian = get_argument (argc, argv, "B");
  Julian    = get_argument (argc, argv, "J");

  if (argc != 3) {
    fprintf (stderr, "USAGE:  precess in_epoch out_epoch\n");
    fprintf (stderr, "   you may use B for B1950.0 or J for J2000.0\n");
    fprintf (stderr, "   Enter values in degrees\n");
    exit (2);
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

  fprintf (stderr, "converting from J%f to J%f\n", in_epoch, out_epoch);

  T = (out_epoch - in_epoch) / 100.0;
  
  zeta  = RAD_DEG*(0.6406161*T + 0.0000839*T*T + 0.0000050*T*T*T);
  theta = RAD_DEG*(0.5567530*T - 0.0001185*T*T - 0.0000116*T*T*T);
  z     =          0.6406161*T + 0.0003041*T*T + 0.0000051*T*T*T;

  while (fscanf (stdin, "%lf %lf", &A, &D) != EOF) {
    SD =  cos(RAD_DEG*A + zeta)*sin(theta)*cos(RAD_DEG*D) + cos(theta)*sin(RAD_DEG*D);
    CD = sqrt (1 - SD*SD);
    SA =  sin(RAD_DEG*A + zeta)*cos(RAD_DEG*D)/CD;
    CA = (cos(RAD_DEG*A + zeta)*cos(theta)*cos(RAD_DEG*D) - sin(theta)*sin(RAD_DEG*D))/CD;

    DEC = DEG_RAD*asin(SD);
    RA  = DEG_RAD*atan2(SA, CA) + z;

    if (RA < 0)
      RA += 360;
    fprintf (stdout, "%f %f\n", RA, DEC);
  }
  exit (0);
}  
    

double get_epoch (char *in_epoch, char mode) {

  int done;
  double epoch;

  epoch = 2000.0;
  done = FALSE;
  if (in_epoch[0] == 'B') {
    epoch = BtoJ(atof(&in_epoch[1]));
    done = TRUE;
  }

  if (in_epoch[0] == 'J') {
    epoch = atof(&in_epoch[1]);
    done = TRUE;
  }

  if (!done && (mode == 'B')) {
    epoch = BtoJ(atof(in_epoch));
    done = TRUE;
  }
    
  if (!done && (mode == 'J')) {
    epoch = atof(in_epoch);
    done = TRUE;
  }

  if (!done) {
    fprintf (stderr, "error finding epoch %s\n", in_epoch);
    exit (1);
  }
  
  return (epoch);
}

double BtoJ (double in_epoch) {

  double JD, out_epoch;

  JD = (in_epoch - 1900.0)*365.242198781 + 2415020.31352;
  out_epoch = 2000.0 + (JD - 2451545.0)/365.25;

  return (out_epoch);
}

/*
  time =  (in_epoch - out_epoch) / 100.0;

  fprintf (stderr, "precessing from %f to %f\n", in_epoch, out_epoch);
  sinE = sin (epsilon*deg_to_rad);
  cosE = cos (epsilon*deg_to_rad);
  M = 1.281232*time + 0.0003879*(time*time);
  N = 0.5567*time - 0.0001185*(time*time);

  while (fscanf (stdin, "%lf %lf", &RA, &Dec) != EOF) {
    RA = RA - M - N*sin(RA*deg_to_rad)*tan(Dec*deg_to_rad);
    Dec = Dec - N*cos (RA*deg_to_rad);

    Dec = Dec + time*thetaD*sinE*cos(RA*deg_to_rad);
    RA = RA + time*thetaD*(cosE + sinE*sin(RA*deg_to_rad)*tan(Dec*deg_to_rad));

    fprintf (stdout, "%f %f\n", RA, Dec);
  }

*/


