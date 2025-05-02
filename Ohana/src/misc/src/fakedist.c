# include <ohana.h>

# define MMIN 1.0
# define MMAX 120.0
double rnd_mass();
double MassTerm;

# define NMASS 87
# define DMASS 0.05
# define NPARMAX 30
int N_Av, N_dist, N_alpha, N_age, NSTARS, NTRY;
double Av[NPARMAX], dist[NPARMAX], alpha[NPARMAX], age[NPARMAX];

double dVo, dVref, dMo;   /* determine the noise distribution */

double ldA, lAo, ldM, lMo;
double UV0, V0, DV, DUV;
Header header, UV_h, V_h, mass_h, age_h;
Matrix matrix, UV_i, V_i, mass_i, age_i;
int UV_NX, UV_NY;
float *UVbuffer, *Vbuffer;
int MS_NX, MS_NY;
float *Mbuffer, *Abuffer;

char fakefile[256], magfile[256], colorfile[256], massfile[256], agefile[256];

void main (argc, argv)
int argc;
char **argv;
{

  int i, j, m, n, M, A, Nstars;
  double mag, color, mass, noise, inflation;
  float *in;
  char line[1024];

  load_parameters (argc, argv);

  init_outmatrix (); 
 
  ohana_gaussdev_init ();

  read_datafiles ();

  gfits_modify (&header, "SFR", "%s", 1, "Star Formation Rate (Mo / Myr)");
  for (i = 0; i < N_Av; i++) {
    for (j = 0; j < N_dist; j++) {
      for (m = 0; m < N_alpha; m++) {
	
	MassTerm = (pow((MMAX/MMIN), -alpha[m]) - 1.0);
	for (n = 0; n < N_age; n++) {
	  fprintf (stderr, "age: %d %f -- %f\n", n, age[n], age[n+1]);
	  
	  mass = 0;
	  for (Nstars = 0; Nstars < NSTARS; Nstars++) {
	    fakestar (Av[i], dist[j], alpha[m], age[n], age[n+1], 
		      &mag, &color, &mass, &noise);
	    
	    magtomass (Av[i], dist[j], mag, color, noise, &M, &A, &inflation);

	    if (inflation > 0.0) add_to_outmatrix (i, j, m, n, A, M, inflation);
	    
	  }
	  /* store the parameters and total mass for this Mass*Age matrix in the (N_age) column */
	  /* mass is the total mass generated between 1 and 120 Mo for the NSTARS points in this matrix */
	  sprintf (line, "SFR_%0d\0", n);
	  gfits_modify (&header, line, "%lf", 1, mass / (age[n+1] - age[n]));
	  add_to_outmatrix (i, j, m, n, N_age, 0, mass);
	  add_to_outmatrix (i, j, m, n, N_age, 1, Av[i]);
	  add_to_outmatrix (i, j, m, n, N_age, 2, dist[j]);
	  add_to_outmatrix (i, j, m, n, N_age, 3, alpha[m]);
	  add_to_outmatrix (i, j, m, n, N_age, 4, age[n]);
	  add_to_outmatrix (i, j, m, n, N_age, 5, age[n+1]);
	  
	}
      }
    }
  }

  /* 
  in = (float *)matrix.buffer;
  for (i = 0; i < matrix.Naxis[0]; i++) {
    for (j = 0; j < matrix.Naxis[1]; j++) {
      mass = in[i + matrix.Naxis[0]*(j + matrix.Naxis[1]*2)];
      fprintf (stderr, "%f ", mass);
    }
    fprintf (stderr, "\n");
  }
  */

  gfits_write_header (fakefile, &header);
  gfits_write_matrix (fakefile, &matrix);
}

/*****************************************************************************/

load_parameters (argc, argv)
int argc;
char **argv;
{

  int i, stat, test_dVo, test_dVref, test_dMo;
  int Ffile, Mfile, Cfile, Afile, MSfile;
  FILE *f;
  char line[1024];
  double N_stars, N_try, Ntotal, time;

  if (argc < 2) {
    fprintf (stderr, "USAGE: %s pfile\n", argv[0]);
    exit (0);
  }

  f = fopen (argv[1], "r");
  if (f == NULL) {
    fprintf (stderr, "parameter file %s not found\n", argv[1]);
    exit (0);
  }

  N_Av = N_dist = N_alpha = N_age = N_stars = N_try = 0.0;
  test_dVo = test_dVref = test_dMo = FALSE;
  Ffile = Mfile = Cfile = Afile = MSfile = FALSE;
  while (scan_line (f, line) != EOF) {
    if (!stripwhite (line)) continue;
    if (line[0] == '#') continue;
    
    if (!strncmp (line, "Av ", strlen ("Av "))) {
      for (i = 0; (i < NPARMAX) && (stat = dparse (&Av[i], i+2, line)); i++);
      if (i == NPARMAX) {
	fprintf (stderr, "maximum of %d parameter values per parameter\n", NPARMAX);
	exit (0);
      }
      N_Av = i;
    }

    if (!strncmp (line, "dist ", strlen ("dist "))) {
      for (i = 0; (i < NPARMAX) && dparse (&dist[i], i+2, line); i++);
      if (i == NPARMAX) {
	fprintf (stderr, "maximum of %d parameter values per parameter\n", NPARMAX);
	exit (0);
      }
      N_dist = i;
    }

    if (!strncmp (line, "alpha ", strlen ("alpha "))) {
      for (i = 0; (i < NPARMAX) && dparse (&alpha[i], i+2, line); i++);
      if (i == NPARMAX) {
	fprintf (stderr, "maximum of %d parameter values per parameter\n", NPARMAX);
	exit (0);
      }
      N_alpha = i;
    }

    if (!strncmp (line, "age ", strlen ("age "))) {
      for (i = 0; (i < NPARMAX) && dparse (&age[i], i+2, line); i++);
      if (i == NPARMAX) {
	fprintf (stderr, "maximum of %d parameter values per parameter\n", NPARMAX);
	exit (0);
      }
      N_age = MAX (0, i - 1);
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
    
    if (!strncmp (line, "nstars ", strlen ("nstars "))) {
      dparse (&N_stars, 2, line);
      NSTARS = N_stars;
    }
    
    if (!strncmp (line, "ntry ", strlen ("ntry "))) {
      dparse (&N_try, 2, line);
      NTRY = N_try;
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
    
    if (!strncmp (line, "agefile ", strlen ("agefile "))) {
      sscanf (line, "%*s %s", agefile);
      fprintf (stderr, "agefile: %s\n", agefile);
      Afile = TRUE;
    }
    
    if (!strncmp (line, "massfile ", strlen ("massfile "))) {
      sscanf (line, "%*s %s", massfile);
      fprintf (stderr, "massfile: %s\n", massfile);
      MSfile = TRUE;
    }
    
    if (!strncmp (line, "fakefile ", strlen ("fakefile "))) {
      sscanf (line, "%*s %s", fakefile);
      fprintf (stderr, "fakefile: %s\n", fakefile);
      Ffile = TRUE;
    }
    
    if (N_Av && N_dist && N_alpha && N_age && N_stars && N_try && 
	test_dVo && test_dVref && test_dMo &&
	Ffile && Mfile && Cfile && Afile && MSfile) {
      Ntotal = N_Av * N_dist * N_alpha * N_age * N_stars * N_try;
      time = (Ntotal / 1250000.0);
      fprintf (stderr, "%.0f iterations\n", Ntotal);
      fprintf (stderr, "process should take about %.1f minutes\n", time);
      return;
    }
  }
  
  if (!(N_Av && N_dist && N_alpha && N_age && NSTARS && NTRY && 
	test_dVo && test_dVref && test_dMo && 
	Ffile && Mfile && Cfile && Afile && MSfile)) {
    fprintf (stderr, "failed to get all parameter lines\n");
    fprintf (stderr, "N_Av: %d\n", N_Av);
    fprintf (stderr, "N_dist: %d\n", N_dist);
    fprintf (stderr, "N_alpha: %d\n", N_alpha);
    fprintf (stderr, "N_age: %d\n", N_age);
    fprintf (stderr, "N_stars: %d\n", NSTARS);
    fprintf (stderr, "N_try: %d\n", NTRY);
    fprintf (stderr, "dVo: %f (%d)\n", dVo, test_dVo);
    fprintf (stderr, "dVref: %f (%d)\n", dVref, test_dVref);
    fprintf (stderr, "dMo: %f (%d)\n", dMo, test_dMo);
    fprintf (stderr, "Ffile: %d\n", Ffile);
    fprintf (stderr, "Mfile: %d\n", Mfile);
    fprintf (stderr, "Cfile: %d\n", Cfile);
    fprintf (stderr, "Afile: %d\n", Afile);
    fprintf (stderr, "MSfile: %d\n", MSfile);
    exit (0);
  }
  
}

/*****************************************************************************/


init_outmatrix () 
{

  int i;
  char line[32];
  double value;

  gfits_init_header (&header);
  header.bitpix = -32;
  header.Naxes = 7;
  header.Naxis[0] = NMASS;
  header.Naxis[1] = N_age + 1;
  header.Naxis[2] = N_age;
  header.Naxis[3] = N_alpha;
  header.Naxis[4] = N_dist;
  header.Naxis[5] = N_Av;
  header.Naxis[6] = 2;
  header.bzero = 0.0;
  header.bscale = 1.0;
  header.unsign = FALSE;
  header.extend = FALSE;

  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_modify (&header, "PAR_1", "%s", 1, "mass (Mo)");
  for (i = 0; i < NMASS + 1; i++) {
    sprintf (line, "PAR_1_%0d\0", i);
    value = 10.0 / SQ(4.5 - DMASS*i);
    gfits_modify (&header, line, "%lf", 1, value);
  }
  
  gfits_modify (&header, "PAR_2", "%s", 1, "age out (Myr)");
  for (i = 0; i < N_age + 1; i++) {
    sprintf (line, "PAR_2_%0d\0", i);
    gfits_modify (&header, line, "%lf", 1, age[i]);
  }

  gfits_modify (&header, "PAR_3", "%s", 1, "age in (Myr)");
  for (i = 0; i < N_age + 1; i++) {
    sprintf (line, "PAR_3_%0d\0", i);
    gfits_modify (&header, line, "%lf", 1, age[i]);
  }
  
  gfits_modify (&header, "PAR_4", "%s", 1, "alpha");
  for (i = 0; i < N_alpha; i++) {
    sprintf (line, "PAR_4_%0d\0", i);
    gfits_modify (&header, line, "%lf", 1, alpha[i]);
  }

  gfits_modify (&header, "PAR_5", "%s", 1, "dist (mag)");
  for (i = 0; i < N_dist; i++) {
    sprintf (line, "PAR_5_%0d\0", i);
    gfits_modify (&header, line, "%lf", 1, dist[i]);
  }

  gfits_modify (&header, "PAR_6", "%s", 1, "A_V (mag)");
  for (i = 0; i < N_Av; i++) {
    sprintf (line, "PAR_6_%0d\0", i);
    gfits_modify (&header, line, "%lf", 1, Av[i]);
  }

}


/*****************************************************************************/

fakestar (A_V, Dist, Alpha, AgeS, AgeE, mag, color, Mass, Noise)
double A_V, Dist, Alpha, AgeS, AgeE;
double *Mass, *mag, *color, *Noise;
{

  int X, Y, i;
  double mass, Age, v, uv, noise;

  for (i = 0; i < 500; i++) {
    mass = rnd_mass (Alpha);
    Age = (AgeE - AgeS)*drand48() + AgeS;
    *Mass += mass;
    
    /* find star in color, mag images */
    X = (log10(Age) - lAo) / ldA;
    Y = (log10(mass) - lMo) / ldM;
    if ((X >= 0) && (X < UV_NX) && (Y >= 0) && (Y < UV_NY)) {
      uv = UVbuffer[X + UV_NX*Y];
      if (uv != 100.0) {
	uv += 0.7*A_V;   /***** this depends on the extinction law and colors U-V ***/
	v = Vbuffer[X + UV_NX*Y] + Dist + A_V; 
	noise = dVo*sqrt(1.0 + pow (10.0, (0.4*(v - dVref)))); 
	if (noise < dMo) {
	  *Noise = noise;
	  *mag = ohana_gaussdev_rnd (v, noise);
	  *color = ohana_gaussdev_rnd (uv, 1.4*noise);
	  return;
	}
      }
    }
  }
  fprintf (stderr, "problem with generating stars:  none land in safe zone\n");
  exit (0);
}



/* rnd_mass only returns stars with masses in the range MMIN to MMAX */
double
rnd_mass (Alpha)
double Alpha;
{
  
  double dP, x;
  
  x = (MMAX + 1);
  while (x > MMAX) {
    dP = drand48();
    x = MMIN * pow ((dP*MassTerm + 1.0), 1.0 / (-Alpha));
  }
  return (x);

}

/*****************************************************************************/

read_datafiles ()
{

  gfits_read_header (colorfile, &UV_h);
  gfits_read_matrix (colorfile, &UV_i);
  gfits_read_header (magfile, &V_h);
  gfits_read_matrix (magfile, &V_i); 
  UV_NX = UV_h.Naxis[0];
  UV_NY = UV_h.Naxis[1];
  UVbuffer = (float *)UV_i.buffer;
  Vbuffer = (float *)V_i.buffer;

  gfits_scan (&UV_h, "RA_O",  "%lf", 1, &lAo);
  gfits_scan (&UV_h, "RA_X",  "%lf", 1, &ldA);
  gfits_scan (&UV_h, "DEC_O", "%lf", 1, &lMo);
  gfits_scan (&UV_h, "DEC_Y", "%lf", 1, &ldM);

  gfits_read_header (massfile, &mass_h);
  gfits_read_matrix (massfile, &mass_i);
  gfits_read_header (agefile, &age_h);
  gfits_read_matrix (agefile, &age_i); 
  MS_NX = mass_h.Naxis[0];
  MS_NY = mass_h.Naxis[1];
  Mbuffer = (float *)mass_i.buffer;
  Abuffer = (float *)age_i.buffer;

  gfits_scan (&mass_h, "RA_O",  "%lf", 1, &UV0);
  gfits_scan (&mass_h, "RA_X",  "%lf", 1, &DUV);
  gfits_scan (&mass_h, "DEC_O", "%lf", 1, &V0);
  gfits_scan (&mass_h, "DEC_Y", "%lf", 1, &DV);
} 

/*****************************************************************************/

/*****************************************************************************/

magtomass (A_V, Dist, mag, color, noise, M, A, inflation)
double A_V, Dist, mag, color, noise;
int *M, *A;
double *inflation;
{

  int x, y, I, J, i, j, k;
  double mass, Age, v, uv, Ngood;

  *inflation = 0.0;
  x = (color - UV0 - 0.7*A_V) / DUV;   /* again, depends on U-V and extinction */
  y = (mag - Dist - V0 - A_V) / DV;
  if ((x > 0) && (x < MS_NX) && (y > 0) && (y < MS_NY)) {
    mass = Mbuffer[x + MS_NX*y];
    if (mass > 0.0) {
      Age = Abuffer[x + MS_NX*y];
      /* age[i]: i = 0,N_age */
      if ((Age < age[0]) || (Age > age[N_age])) {
	*inflation = 0.0;
	return;
      }
      for (I = 0; (I < N_age) && (Age > age[I+1]); I++);
      J = MAX (MIN ((4.5 - sqrt(10.0/mass)) / DMASS, NMASS - 1), 0);
      Ngood = 1.0;
      for (k = 0; k < NTRY; k++) {
	v = ohana_gaussdev_rnd (mag, noise);
	uv = ohana_gaussdev_rnd (color, 1.4*noise);
	x = (uv - UV0 - 0.7*A_V) / DUV;
	y = (v - V0 - Dist - A_V) / DV;
	if ((x > 0) && (x < MS_NX) && (y > 0) && (y < MS_NY)) {
	  mass = Mbuffer[x + MS_NX*y];
	  if (mass > 0.0) {
	    Age = Abuffer[x + MS_NX*y];
	    for (i = -1; (i < N_age) && (Age > age[i+1]); i++);
	    j = MAX (MIN ((4.5 - sqrt(10.0/mass)) / DMASS, NMASS - 1), 0);
	    if ((i == I) && (j == J)) {
	      Ngood += 1.0;
	    }
	  }
	}
      }
      *inflation = (NTRY / Ngood);
      *M = J;
      *A = I;
    }
  }
  if (mass == 0.0) *inflation = 0.0;
}
 

/*****************************************************************************/

add_to_outmatrix (AV, DIST, ALPHA, AGE_IN, AGE_OUT, MASS, value)
int AV, DIST, ALPHA, AGE_IN, AGE_OUT, MASS;
double value;
{

  float *buffer;

  buffer = (float *)matrix.buffer;
  
  /* elements in the (N_age) row of each Mass*Age matrix contain special numbers for the matrix */
  buffer[MASS + NMASS*(AGE_OUT + (N_age + 1)*(AGE_IN + N_age*(ALPHA + N_alpha*(DIST + N_dist*(AV + N_Av*(0))))))] += value;
  buffer[MASS + NMASS*(AGE_OUT + (N_age + 1)*(AGE_IN + N_age*(ALPHA + N_alpha*(DIST + N_dist*(AV + N_Av*(1))))))] += 1.0;

}

