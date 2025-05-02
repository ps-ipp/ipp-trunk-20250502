# include <ohana.h>
# define MMIN 1.0
# define MMAX 120.0

void main (argc, argv)
int argc;
char **argv;
{

  Header mass_h, age_h;
  Matrix mass_i, age_i;
  double UV0, V0, DV, DUV;
  double Uo, Vo;
  double U, V, dUV, dU, dV, mass, age, d, a, da;
  double M, dM, Ms, M2, m, dm, ldM, lMo, ldA, lAo;
  double u, v, uv, Av, f, top, bot, alpha;
  double **Masses, *maxmass, *minmass;
  double **Ages;
  int x, y, i, j, Ntry, NAGE, N;
  char line[1024];
  long A, B;
  
  ohana_gaussdev_init();

  lAo =   0.0;
  ldA =   1.0;
  NAGE =  320;
  if (N = get_argument (argc, argv, "-age")) {
    remove_argument (N, &argc, argv);
    lAo = atof(argv[N]);
    remove_argument (N, &argc, argv);
    ldA = atof(argv[N]);
    remove_argument (N, &argc, argv);
    NAGE = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 9) {
    fprintf (stderr, "USAGE: magtoage massfile agefile d Av Ntry Uo Vo alpha ldM\n");
    exit (0);
  }
  d = atof (argv[3]);
  Av = atof (argv[4]);
  Ntry = atof (argv[5]);
  Uo = atof (argv[6]);
  Vo = atof (argv[7]);
  alpha = atof (argv[8]);
  /*
  ldM   = atof (argv[9]);
  lMo = 0.0;
  */
  
  gfits_read_header (argv[1], &mass_h);
  gfits_read_matrix (argv[1], &mass_i);
  gfits_read_header (argv[2], &age_h);
  gfits_read_matrix (argv[2], &age_i); 

  gfits_scan (&mass_h, "RA_O",  "%lf", 1, &UV0);
  gfits_scan (&mass_h, "RA_X",  "%lf", 1, &DUV);
  gfits_scan (&mass_h, "DEC_O", "%lf", 1, &V0);
  gfits_scan (&mass_h, "DEC_Y", "%lf", 1, &DV);

  ALLOCATE (Ages, double *, (Ntry+2));
  ALLOCATE (maxmass, double, NAGE);
  ALLOCATE (minmass, double, NAGE);
  for (i = 0; i < Ntry + 2; i++) {
    ALLOCATE (Ages[i], double, NAGE);
    bzero (Ages[i], sizeof(double) * NAGE);
  }

  A = time(NULL);
  for (B = 0; A == time(NULL); B++);
  srand48(B);

  /* find max and min masses for each age bin */
  for (i = 0; i < NAGE; i++) {
    maxmass[i] = 0.0;
    minmass[i] = 1000.0;
  }
  for (x = 0; x < age_h.Naxis[0]; x++) {
    for (y = 0; y < age_h.Naxis[1]; y++) {
      mass = gfits_get_matrix_value (&mass_i, x, y);
      if (mass == 0.0) continue;
      age  = gfits_get_matrix_value (&age_i, x, y);
      uv = x*DUV + UV0 + 0.7*Av;
      v  = y*DV + V0 + Av + d;
      u  = v + uv;
      if ((u <= Uo) && (v <= Vo)) {
	i = (age - lAo) / ldA;
	if ((i > 0) && (i < NAGE)) {
	  maxmass[i] = MAX(mass, maxmass[i]);
	  minmass[i] = MIN(mass, minmass[i]);
	}
      }
    }
  }

  fprintf (stderr, "beginning main loop\n");
  while (scan_line (stdin, line) != EOF) {
    if (!stripwhite (line)) continue;
    if (line[0] == '#') continue;
    dparse (&V, 8, line);
    dparse (&dV, 9, line);
    dparse (&U, 12, line);
    dparse (&dU, 13, line);
    dUV = sqrt (dU*dU + dV*dV);
    for (i = 0; i < Ntry + 1; i++) {
      if (i == 0) {
	fprintf (stderr, ".");
	v = V;
	uv = U - V;
      }
      else {
	v = ohana_gaussdev_rnd (V, dV);
	uv = ohana_gaussdev_rnd ((U-V), dUV);
      }
      x = (uv - UV0 - 0.7*Av) / DUV;
      y = (v - d - V0 - Av) / DV;
      if ((x > 0) && (x < mass_h.Naxis[0]) &&
	  (y > 0) && (y < mass_h.Naxis[1])) {
	mass = gfits_get_matrix_value (&mass_i, x, y);
	if (mass > 0.0) {
	  age = gfits_get_matrix_value (&age_i, x, y); 
	  j = (age - lAo) / ldA;
	  if ((j >= 0) && (j < NAGE))
	    Ages[i][j] += pow(mass, alpha);
	}
      }
    }
  }

  top = pow (MMAX, 1.0 - alpha) - pow (MMIN, 1.0 - alpha);
  fprintf (stderr, "finding scatter per bin\n");
  for (j = 0; j < NAGE; j++) {
    Ms = M2 = 0.0;
    for (i = 1; i < Ntry + 1; i++) {
      Ms += Ages[i][j];
      M2 += (Ages[i][j]*Ages[i][j]);
    }
    M = dM = 0.0;
    if (Ms > 0) {
      M  = Ms / (1.0*Ntry);
      dM = sqrt (M2 / (1.0*Ntry) - M*M);
    }
    Ages[1][j] = M;
    Ages[2][j] = dM;
    /*
    a = pow (10.0, j*ldA + lAo);
    da = pow (10.0, (j+1)*ldA + lAo) - a;
    */
    a = j*ldA + lAo;
    da = ldA;
    f = 0.0;
    if (maxmass[j] > minmass[j]) {
      bot = pow (maxmass[j], 1.0 - alpha) - pow (minmass[j], 1.0 - alpha);
      if (bot != 0.0) 
	f = top * top / ((1.0 - alpha) * bot * 2.30158509);
    }
    fprintf (stdout, "%d %f %f %f %f %f %f %f\n", j, a, da, Ages[0][j], Ages[1][j], Ages[2][j], f, Ages[1][j]*f/da);
  }
}

