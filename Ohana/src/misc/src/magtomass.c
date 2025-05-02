# include <ohana.h>
# define NEWWAY 1

void main (argc, argv)
int argc;
char **argv;
{

  Header mass_h, age_h;
  Matrix mass_i, age_i;
  double UV0, V0, DV, DUV, ra, dec;
  double U, V, dUV, dU, dV, mass, age, d;
  double M, dM, Ms, M2, m, ldM, lMo, lAo, ldA, dlogM;
  double v, uv, Av, eta, deta, Ngood;
  double ***Masses, *maxmass, *minmass;
  float *Mbuffer, *Abuffer;
  int x, y, i, j, I, J, Ntry, dump, NMASS, N, NAGE, k, try;
  int NX, NY;
  char line[1024];
  int col1, col2;
  
  dump = FALSE;
  if (N = get_argument (argc, argv, "-dump")) {
    remove_argument (N, &argc, argv);
    dump = TRUE;
  }

  col1 = 8;
  if (N = get_argument (argc, argv, "-col1")) {
    remove_argument (N, &argc, argv);
    col1 = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  col2 = 12;
  if (N = get_argument (argc, argv, "-col2")) {
    remove_argument (N, &argc, argv);
    col2 = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  fprintf (stderr, "using mags in columns %d & %d\n", col1, col2);

  ohana_gaussdev_init ();

  lAo = 0.1;
  ldA =  0.3;
  NAGE = 10;
  lMo = 0.0;
  ldM =  0.05;
  if (NEWWAY)
    NMASS = 87;
  else 
    NMASS = 50;
  if (N = get_argument (argc, argv, "-mass")) {
    if (dump) {
      fprintf (stderr, "-mass and -dump incompatible\n");
      exit (0);
    }
    remove_argument (N, &argc, argv);
    lMo = atof(argv[N]);
    remove_argument (N, &argc, argv);
    ldM = atof(argv[N]);
    remove_argument (N, &argc, argv);
    NMASS = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) {
    fprintf (stderr, "USAGE: magtomass massfile agefile d Av Ntry\n");
    exit (0);
  }
  d = atof (argv[3]);
  Av = atof (argv[4]);
  Ntry = atof (argv[5]);

  gfits_read_header (argv[1], &mass_h);
  gfits_read_matrix (argv[1], &mass_i);
  gfits_read_header (argv[2], &age_h);
  gfits_read_matrix (argv[2], &age_i); 

  gfits_scan (&mass_h, "RA_O",  "%lf", 1, &UV0);
  gfits_scan (&mass_h, "RA_X",  "%lf", 1, &DUV);
  gfits_scan (&mass_h, "DEC_O", "%lf", 1, &V0);
  gfits_scan (&mass_h, "DEC_Y", "%lf", 1, &DV);
  Mbuffer = (float *)mass_i.buffer;
  Abuffer = (float *)age_i.buffer;
  NX = mass_h.Naxis[0];
  NY = mass_h.Naxis[1];
  
  ALLOCATE (Masses, double **, NAGE);
  for (i = 0; i < NAGE; i++) {
    ALLOCATE (Masses[i], double *, NMASS);
    for (j = 0; j < NMASS; j++) {
      ALLOCATE (Masses[i][j], double, 3);
      Masses[i][j][0] = 0.0;
	Masses[i][j][1] = 0.0;
    }
  }

  fprintf (stderr, "beginning main loop\n");
  while (scan_line (stdin, line) != EOF) {
    if (!stripwhite (line)) continue;
    if (line[0] == '#') continue;
    /* appropriate columns hardwired */
    dparse (&ra, 1, line);
    dparse (&dec, 2, line);
    dparse (&V, col1, line);
    dparse (&dV, (col1+1), line);
    dparse (&U, col2, line);
    dparse (&dU, (col2+1), line);
    dUV = sqrt (dU*dU + dV*dV);
    x = (U-V - UV0 - 0.7*Av) / DUV;
    y = (V - d - V0 - Av) / DV;
    if ((x > 0) && (x < mass_h.Naxis[0]) && (y > 0) && (y < mass_h.Naxis[1])) {
      mass = Mbuffer[x + NX*y];
      /*      mass = gfits_get_matrix_value (&mass_i, x, y); */
      if (mass > 0.0) {
	age = Abuffer[x + NX*y];
	fprintf (stdout, "%f %f %f %f %f %f\n", ra, dec, V, U-V, mass, age);
      }
    }
  }
}



/*

	**	age = gfits_get_matrix_value (&age_i, x, y);  **
	I = MAX (MIN ((log10(age) - lAo) / ldA, NAGE - 1), 0); 
	if (NEWWAY) {
	  J = MAX (MIN ((4.5 - sqrt(10.0/mass)) / ldM, NMASS - 1), 0);
	} else {
	  J = MAX (MIN ((log10(mass) - lMo) / ldM, NMASS - 1), 0); 
	}
	Ngood = 1.0;
	for (k = 0; k < Ntry; k++) {
	  v = ohana_gaussdev_rnd (V, dV);
	  uv = ohana_gaussdev_rnd ((U-V), dUV);
	  x = (uv - UV0 - 0.7*Av) / DUV;
	  y = (v - d - V0 - Av) / DV;
	  if ((x > 0) && (x < mass_h.Naxis[0]) && (y > 0) && (y < mass_h.Naxis[1])) {
	    mass = Mbuffer[x + NX*y];
	    **	    mass = gfits_get_matrix_value (&mass_i, x, y); **
	    if (mass > 0.0) {
	      age = Abuffer[x + NX*y];
	      ** age = gfits_get_matrix_value (&age_i, x, y);  **
	      i = MAX (MIN ((log10(age) - lAo) / ldA, NAGE - 1), 0);
	      if (NEWWAY) {
		j = MAX (MIN ((4.5 - sqrt(10.0/mass)) / ldM, NMASS - 1), 0);
	      } else {
		j = MAX (MIN ((log10(mass) - lMo) / ldM, NMASS - 1), 0); 
	      }
	      if ((i == I) && (j == J)) {
		Ngood += 1.0;
	      }
	    }
	  }
	}
	Masses[I][J][0] += (Ntry / Ngood);
	Masses[I][J][1] += 1.0;
      }
    }
  }

  fprintf (stderr, "finding scatter per bin\n");
  fprintf (stdout, "# measured IMF \n");
  fprintf (stdout, "# distance modulus = %5.2, extinction (A_V) = %5.2f\n", d, Av);
  fprintf (stdout, "# reference mass and age filess: %s, %s\n", argv[1], argv[2]);
  fprintf (stdout, "# eta = N(logM, logM+dlogM) / dlogM \\sim ln(10.0) * M dN/dM)\n");
  fprintf (stdout, "# i  Mass   N(mass)  eta   d_eta\n");
  for (i = 0; i < NAGE; i++) {
    for (j = 0; j < NMASS; j++) {
      if (NEWWAY) {
	mass = 10.0 / SQ(4.5 - ldM*(j+0.5));
	dlogM = 2.0 * (log10(4.5 - ldM*j) - log10(4.5 - ldM*(j+1)));
      } else {
	mass = pow (10.0, (lMo + ldM*(j+0.5)));
	dlogM = ldM;
      }
      eta  = Masses[i][j][0] / dlogM;
      deta = (Masses[i][j][1] > 0) ? eta / sqrt(Masses[i][j][1]) : 0.0;
      age  = pow (10.0, (ldA*i + lAo));
      M = (Masses[i][j][1] > 0) ? Masses[i][j][0]/Masses[i][j][1] : 0;  ** average correction factor **
      dM = (Masses[i][j][1] > 0) ? Masses[i][j][0]/sqrt(Masses[i][j][1]) : 0;  ** average correction factor **
      fprintf (stdout, "%4d %4d %5.1f %5.1f %7.1f %7.1f %6.2f %4.0f\n", i, j, mass, age, eta, deta, M, Masses[i][j][1]);
    }
  }
}

*/

