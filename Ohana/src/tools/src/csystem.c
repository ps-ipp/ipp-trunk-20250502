# include <ohana.h>

/* csystem: convert between celestial, galactic, ecliptic, and
   hoziron systems */

int main (int argc, char **argv) {

  /* USAGE: csystem [C/G/E/H] [C/G/E/H] [epoch] */
  double x, y, X, Y, Xo, xo, phi, T;
  double sin_x, sin_y, cos_x, cos_y;
  
  phi = xo = Xo = 0.0;

  if (((argc == 3) && ((argv[1][0] == 'E') || (argv[2][0] == 'E'))) || 
      ((argc == 4) && ((argv[1][0] != 'E') && (argv[2][0] != 'E'))) ||
      ((argc != 3) && (argc != 4))) {
    fprintf (stderr, "USAGE: csystems [C/G/E/H] [C/G/E/H] [epoch]\n");
    exit (2);
  }

  switch (argv[1][0]) {
  case 'C':
    switch (argv[2][0]) {
    case 'C': 
      phi = Xo = xo = 0.0;
      break;
    case 'G':
      phi = -62.6*RAD_DEG;
      Xo = 282.25;
      xo = 33;
      break;
    case 'E':
      T = (atof (argv[3]) - 1900) / 100.0;
      phi = -1*(23.452294 - 0.013013*T - 0.000001639*T*T + 0.000000503*T*T*T);
      phi *= RAD_DEG;
      Xo = xo = 0.0;
      break;
    }
    break;
  case 'E':
    switch (argv[2][0]) {
    case 'C': 
      T = (atof (argv[3]) - 1900) / 100.0;
      phi = 23.452294 - 0.013013*T - 0.000001639*T*T + 0.000000503*T*T*T;
      phi *= RAD_DEG;
      Xo = xo = 0.0;
      break;
    case 'G':
      fprintf (stderr, "error: not working!\n");
      exit (1);
      phi = -62.6*RAD_DEG;
      Xo = 282.25;
      xo = 33;
      break;
    case 'E':
      phi = Xo = xo = 0.0;
      break;
    }
    break;
  case 'G':
    switch (argv[2][0]) {
    case 'C': 
      phi = 62.6*RAD_DEG;
      Xo = 33;
      xo = 282.25;
      break;
    case 'G':
      phi = Xo = xo = 0.0;
      break;
    case 'E':
      fprintf (stderr, "error: not working!\n");
      exit (1);
      phi = -1*(23.452294 - 0.013013*T - 0.000001639*T*T + 0.000000503*T*T*T);
      Xo = xo = 0.0;
      break;
    }
  }
 
  Xo *= RAD_DEG;
  for (; fscanf (stdin, "%lf %lf", &X, &Y) == 2;) {
    
    X *= RAD_DEG;
    Y *= RAD_DEG;

    sin_y = cos(Y)*sin(X - Xo)*sin(phi) + sin(Y)*cos(phi);
    cos_y = sqrt (1 - sin_y*sin_y);
    sin_x = (cos(Y)*sin(X - Xo)*cos(phi) - sin(Y)*sin(phi)) /  cos_y;
    cos_x = cos(Y)*cos(X - Xo) / cos_y;
/*    fprintf (stderr, "%f %f   %f %f   %f %f\n", X, Y, sin_x, cos_x, sin_y, cos_y); */
    
    x = (DEG_RAD * atan2 (sin_x, cos_x) + xo + 360);
    
    while (x >= 360.0)
      x -= 360;
    y = DEG_RAD * atan2 (sin_y, cos_y);
    
    fprintf (stdout, "%10.6f %10.6f\n", x, y);
  }
  exit (0);
}
