# include <ohana.h>
# define MMIN 1.0
# define MMAX 120.0
extern double rnd_mass();
double rnd_mass();
double term;

double AgeS, AgeE, alpha, d, Av, dMo, dVo, dVref;
char colorfile[256], magfile[256];

void main (argc, argv)
int argc;
char **argv;
{

  Header UV_h, V_h;
  Matrix UV_i, V_i;
  double Mtot, M, mass;
  double age, noise, v, uv, V, UV;
  double lAo, ldA, lMo, ldM, stage;
  int X, Y;
  long A, B;
  
  load_parameters (argc, argv);
  Mtot = atof (argv[2]);
  term = (pow((MMAX/MMIN), -alpha) - 1.0);
  
  gfits_read_header (colorfile, &UV_h);
  gfits_read_matrix (colorfile, &UV_i);
  gfits_read_header (magfile, &V_h);
  gfits_read_matrix (magfile, &V_i); 

  gfits_scan (&UV_h, "RA_O",  "%lf", 1, &lAo);
  gfits_scan (&UV_h, "RA_X",  "%lf", 1, &ldA);
  gfits_scan (&UV_h, "DEC_O", "%lf", 1, &lMo);
  gfits_scan (&UV_h, "DEC_Y", "%lf", 1, &ldM);
  
  ohana_gaussdev_init ();

  fprintf (stderr, "beginning main loop\n");
  
  stage = 1;
  for (M = 0; M < Mtot; ) {
    age = (AgeE - AgeS)*drand48() + AgeS;
    X = (log10(age) - lAo) / ldA;
    if (M > stage * Mtot / 10.0) {
      fprintf (stderr, "M: %f\n", M);
      stage += 1.0;
    }
    mass = rnd_mass (alpha);
    Y = (log10(mass) - lMo) / ldM;
    if ((X >= 0) && (X < UV_h.Naxis[0]) &&
	(Y >= 0) && (Y < UV_h.Naxis[1])) {
      uv = gfits_get_matrix_value (&UV_i, X, Y); 
      if (uv != 100.0) {
	uv += 0.7*Av;
	v = gfits_get_matrix_value (&V_i, X, Y) + d + Av; 
	noise = dVo*sqrt(1.0 + pow (10.0, (0.4*(v - dVref)))); 
	V = ohana_gaussdev_rnd (v, noise);
	UV = ohana_gaussdev_rnd (uv, 1.4*noise);
	if (noise < dMo) {
	  fprintf (stdout, "%f %f   %f %f   %f  %f\n", V, noise, V+UV, noise, mass, age);
	}
      }
    }
    M += mass;
  }
}

/* rnd_mass only returns stars with masses in the range MMIN to MMAX */
double
rnd_mass (alpha)
double alpha;
{
  
  double dP, x;
  
  x = (MMAX + 1);
  while (x > MMAX) {
    dP = drand48();
    x = MMIN * pow ((dP*term + 1.0), 1.0 / (-alpha));
  }
  return (x);

}


double 
rnd_integrate (function, range, mean, sigma) 
double (*function) ();
double range, mean, sigma;
{

  double val, x, dx, dx1, dx2, dx3, df;

  range += 0.0001;
  val = 0;
  dx = sigma / 10.0;
  dx1 = dx / 3.0;
  dx2 = 2.0*dx/3.0;
  dx3 = dx;

  for (x = mean - 7*sigma; (val < range) && (x < mean + 7*sigma); x += dx)  {
    df = (3.0*function(x    , mean, sigma) + 
	  9.0*function(x+dx1, mean, sigma) +
	  9.0*function(x+dx2, mean, sigma) + 
	  3.0*function(x+dx3, mean, sigma)) * (dx1/8.0);
    val += df;
  }
  return (x + dx / 2.0);
}

/*****************************************************************************/

load_parameters (argc, argv)
int argc;
char **argv;
{

  int test_dVo, test_dVref, test_dMo;
  int test_Age, test_Av, test_alpha, test_d;
  int Mfile, Cfile;
  FILE *f;
  char line[1024];

  if (argc < 3) {
    fprintf (stderr, "USAGE: %s pfile Mtot\n", argv[0]);
    exit (0);
  }

  f = fopen (argv[1], "r");
  if (f == NULL) {
    fprintf (stderr, "parameter file %s not found\n", argv[1]);
    exit (0);
  }

  test_Age = test_Av = test_alpha = test_d = FALSE;
  test_dVo = test_dVref = test_dMo = FALSE;
  Mfile = Cfile = FALSE;
  while (scan_line (f, line) != EOF) {
    if (!stripwhite (line)) continue;
    if (line[0] == '#') continue;
    
    fprintf (stderr, "%s\n", line);
    if (!strncmp (line, "Av ", strlen ("Av "))) {
      dparse (&Av, 2, line);
      test_Av = TRUE;
    }

    if (!strncmp (line, "dist ", strlen ("dist "))) {
      dparse (&d, 2, line);
      test_d = TRUE;
    }

    if (!strncmp (line, "alpha ", strlen ("alpha "))) {
      dparse (&alpha, 2, line);
      test_alpha = TRUE;
    }

    if (!strncmp (line, "age ", strlen ("age "))) {
      test_Age  = dparse (&AgeS, 2, line);
      test_Age &= dparse (&AgeE, 3, line);
    }

    if (!strncmp (line, "dVo ", strlen ("dVo "))) {
      dparse (&dVo, 2, line);
      test_dVo = TRUE;
    }
    
    if (!strncmp (line, "dVref ", strlen ("dVref "))) {
      dparse (&dVref, 2, line);
      test_dVref = TRUE;
    }
    
    if (!strncmp (line, "dMo ", strlen ("dMo "))) {
      dparse (&dMo, 2, line);
      test_dMo = TRUE;
    }
    
    if (!strncmp (line, "magfile ", strlen ("magfile "))) {
      sscanf (line, "%*s %s", magfile);
      fprintf (stderr, "magfile: %s\n", magfile);
      Mfile = TRUE;
    }
    
    if (!strncmp (line, "colorfile ", strlen ("colorfile "))) {
      sscanf (line, "%*s %s", colorfile);
      fprintf (stderr, "colorfile: %s\n", colorfile);
      Cfile = TRUE;
    }
    
    if (test_Age && test_Av && test_d && test_alpha && 
	test_dVo && test_dVref && test_dMo &&
	Mfile && Cfile) {
      return;
    }
  }
  
  if (!(test_Age && test_Av && test_d && test_alpha && 
	test_dVo && test_dVref && test_dMo && 
	Mfile && Cfile)) {
    fprintf (stderr, "failed to get all parameter lines\n");
    fprintf (stderr, "test_Av: %d\n", test_Av);
    fprintf (stderr, "test_d: %d\n", test_d);
    fprintf (stderr, "test_alpha: %d\n", test_alpha);
    fprintf (stderr, "test_age: %d\n", test_Age);
    fprintf (stderr, "dVo: %f (%d)\n", dVo, test_dVo);
    fprintf (stderr, "dVref: %f (%d)\n", dVref, test_dVref);
    fprintf (stderr, "dMo: %f (%d)\n", dMo, test_dMo);
    fprintf (stderr, "Mfile: %d\n", Mfile);
    fprintf (stderr, "Cfile: %d\n", Cfile);
    exit (0);
  }
  
}

/*****************************************************************************/
