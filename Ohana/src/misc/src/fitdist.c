# include <ohana.h>

# define MMIN 1.0
# define MMAX 120.0
double rnd_mass();
double MassTerm;

# define NPARMAX 30
int Nmass, N_Av, N_dist, N_alpha, N_age, NTRY;
double *Mass, Av[NPARMAX], dist[NPARMAX], alpha[NPARMAX], age[NPARMAX];

int COL1, COL2;
double *mag, *color, *noise;
double *obsdata;

double UV0, V0, DV, DUV;
Header fake_h, mass_h, age_h;
Matrix fake_m, mass_i, age_i;
int MS_NX, MS_NY;
float *Mbuffer, *Abuffer;

char datafile[256], fakefile[256], massfile[256], agefile[256];

double get_obs(), get_fake(), get_error(), get_fake_err(), get_chisq();

void main (argc, argv)
int argc;
char **argv;
{
  
  Header header;
  Matrix matrix;
  int i, j, m, n, M, A, Nstars, I, J, k, done, grid, tries;
  double mass, inflation, sumterm, errterm;
  double dm, a1, a2, chisq_0, chisq_1, chisq_min, Ndof, *delta;
  double temp_Factor, *Factor, *dFactor, *SFR, dFactor1, dFactor2;

  load_parameters (argc, argv);
  fprintf (stderr, "read in parameters\n");

  ohana_gaussdev_init ();

  read_datafiles ();
  fprintf (stderr, "read in data files (%d x %d)\n", Nmass, N_age);

  Nstars = load_realstars ();
  fprintf (stderr, "read in %d real stars\n", Nstars);

  ALLOCATE (SFR, double, N_age);
  ALLOCATE (Factor, double, N_age);
  ALLOCATE (dFactor, double, N_age);
  ALLOCATE (delta, double, N_age);

  fprintf (stdout, "# Av   dist   alpha  chisq Ndof");
  for (n = 0; n < N_age; n++) {
    fprintf (stdout, " %3.0f My ", age[n]);
  }
  fprintf (stdout, "\n");

  for (i = 0; i < N_Av; i++) {
    for (j = 0; j < N_dist; j++) {
      
      /* unique obs. matrix for each Av, dist value only */
      bzero (obsdata, sizeof(double) * N_age*Nmass*2);
      for (k = 0; k < Nstars; k++) {
	magtomass (Av[i], dist[j], mag[k], color[k], noise[k], &M, &A, &inflation);
	if (inflation > 0.0) add_obs (A, M, inflation);
      }
      
      /* now fit to fake matrices */
      for (m = 0; m < N_alpha; m++) {

	for (n = 0; n < N_age; n++) {
	  dm = get_fake (i, j, m, n, N_age, 0);
	  a1 = get_fake (i, j, m, n, N_age, 4);
	  a2 = get_fake (i, j, m, n, N_age, 5);
	  SFR[n] = dm / (a2 - a1);
	  delta[n] = 0.1;
	  Factor[n] = 0.0;
	}	
	
	chisq_1 = get_chisq (Factor, i, j, m, &Ndof);
	chisq_0 = 2*chisq_1;
	done = FALSE;
	/* 	while (!done) { */
	for (tries = 0; ((fabs (chisq_1 - chisq_0) / Ndof > 0.001) && (tries < 100)); tries++) {
	  chisq_0 = chisq_1;
	  for (n = 0; n < N_age; n++) {
	    Factor[n] += delta[n];
	    chisq_1 = get_chisq (Factor, i, j, m, &Ndof);
	    dFactor1 = MAX (0.0, MIN (0.5*delta[n], 5.0 * delta[n] * (1.0 - chisq_1/chisq_0)));
	    Factor[n] -= 2*delta[n];
	    chisq_1 = get_chisq (Factor, i, j, m, &Ndof);
	    dFactor2 = MIN (0.0, MAX (-0.5*delta[n], -5.0 * delta[n] * (1.0 - chisq_1/chisq_0)));
	    if (dFactor1 > fabs(dFactor2)) {
	      dFactor[n] = dFactor1;
	    }
	    else {
	      dFactor[n] = dFactor2;
	    }
	    Factor[n] += delta[n];
	    if ((delta[n] > 0.001) && (!(tries % 5) || (fabs(dFactor[n]) < 0.01 * delta[n]))) {
	      /* small grid search */
	      dFactor1 = temp_Factor = Factor[n];
	      chisq_min = chisq_0;
	      for (grid = 0, Factor[n] = MAX (0.0, Factor[n] - 2*delta[n]); grid < 5; grid ++, Factor[n] += delta[n]) {
		chisq_1 = get_chisq (Factor, i, j, m, &Ndof);
		if (chisq_1 < chisq_min) {
		  dFactor1 = Factor[n];
		  chisq_min = chisq_1;
		}
	      }
	      if (dFactor1 == temp_Factor) {
		delta[n] = delta[n] / 2.0;
	      }
	      Factor[n] = dFactor1;
	    }
	  }
	  done = TRUE; 
	  for (n = 0; n < N_age; n++) {
	    Factor[n] += dFactor[n];
	    Factor[n] = MAX (0.0, Factor[n]);  /* negative star formation is unphysical */
	    if ((Factor[n] > 0.0) && (fabs(dFactor[n]) == delta[n])) {
	      delta[n] = MIN (0.1, delta[n] * 1.5);
	    }
	  }
	  chisq_1 = get_chisq (Factor, i, j, m, &Ndof);
	  /*
	  fprintf (stderr, "%f %f ", chisq_0, chisq_1);
	  for (n = 0; n < N_age; n++) {
	    fprintf (stderr, "%f ", Factor[n]);
	  }
	  fprintf (stderr, "\n");
	  fprintf (stderr, "                       ");
	  for (n = 0; n < N_age; n++) {
	    fprintf (stderr, "%f ", delta[n]);
	  }
	  fprintf (stderr, "\n");
	  */
	}

	chisq_0 = chisq_1;
	for (n = 0; n < N_age; n++) {
	  temp_Factor = Factor[n];
	  for (; chisq_1 < chisq_0 + 1; Factor[n] += 0.1*delta[n])
	    chisq_1 = get_chisq (Factor, i, j, m, &Ndof);
	  dFactor[n] = Factor[n] - temp_Factor;
	  /* fprintf (stderr, "%d %f %f\n", n, Factor[n], dFactor[n]); */
	  Factor[n] = temp_Factor;
	  chisq_1 = chisq_0;
	  for (; chisq_1 < chisq_0 + 1; Factor[n] -= 0.1*delta[n])
	    chisq_1 = get_chisq (Factor, i, j, m, &Ndof);
	  dFactor[n] =  0.5 * (dFactor[n] + temp_Factor  - Factor[n]);
	  Factor[n] = temp_Factor;
	}
	if (tries == 100) {
	  fprintf (stdout, "*** failed to converge *** \n");
	}
	fprintf (stdout, "%5.2f %6.2f %4.1f   %6.2f  %.0f ", Av[i], dist[j], alpha[m], chisq_1/Ndof, Ndof);
	for (n = 0; n < N_age; n++) {
	  fprintf (stdout, "%7.0f ", SFR[n]*Factor[n]);
	}
	fprintf (stdout, "\n");
	fprintf (stdout, "                               ");
	for (n = 0; n < N_age; n++) {
	  fprintf (stdout, "%7.0f ", SFR[n]*dFactor[n]);
	}
	fprintf (stdout, "\n");
      }
    }
  }
}


/*****************************************************************************/

double get_chisq (Factor, AV, DIST, ALPHA, Ndof)
double *Factor;
int AV, DIST, ALPHA;
double *Ndof;
{

  double chisq, sumterm, errterm;
  int I, J, n;

  *Ndof = chisq = 0.0;
  for (I = 0; I < Nmass; I++) {
    for (J = 0; J < N_age; J++) {
      errterm = get_error (J, I);
      sumterm = get_obs (J, I);
      for (n = 0; n < N_age; n++) {
	sumterm -= Factor[n]*get_fake (AV, DIST, ALPHA, n, J, I);
	errterm += Factor[n]*get_fake_err (AV, DIST, ALPHA, n, J, I);
      }
      if (errterm > 0.0) {
	chisq += SQ (sumterm) / errterm;
	*Ndof += 1.0;
      }
    }
  }
  *Ndof -= N_age + N_Av + N_dist + N_alpha;

  return (chisq);
}

/*****************************************************************************/

load_parameters (argc, argv)
int argc;
char **argv;
{

  int i;
  int Dfile, Ffile, Afile, MSfile;
  FILE *f;
  char line[1024];
  double tmp;

  if (argc < 2) {
    fprintf (stderr, "USAGE: %s pfile\n", argv[0]);
    exit (0);
  }

  f = fopen (argv[1], "r");
  if (f == NULL) {
    fprintf (stderr, "parameter file %s not found\n", argv[1]);
    exit (0);
  }

  COL1 = COL1 = NTRY = 0.0;
  Dfile = Ffile = Afile = MSfile = FALSE;
  while (scan_line (f, line) != EOF) {
    if (!stripwhite (line)) continue;
    if (line[0] == '#') continue;
        
    if (!strncmp (line, "ntry ", strlen ("ntry "))) {
      dparse (&tmp, 2, line);
      NTRY = tmp;
    }
  
    if (!strncmp (line, "col1 ", strlen ("col1 "))) {
      dparse (&tmp, 2, line);
      COL1 = tmp;
    }
  
    if (!strncmp (line, "col2 ", strlen ("col2 "))) {
      dparse (&tmp, 2, line);
      COL2 = tmp;
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
    
    if (!strncmp (line, "datafile ", strlen ("datafile "))) {
      sscanf (line, "%*s %s", datafile);
      fprintf (stderr, "datafile: %s\n", datafile);
      Dfile = TRUE;
    }
    
    if (!strncmp (line, "fakefile ", strlen ("fakefile "))) {
      sscanf (line, "%*s %s", fakefile);
      fprintf (stderr, "fakefile: %s\n", fakefile);
      Ffile = TRUE;
    }
    
    if (COL1 && COL2 && NTRY && Dfile && Ffile && Afile && MSfile) {
      return;
    }
  }
  
  if (!(COL1 && COL2 && NTRY && Dfile && Ffile && Afile && MSfile)) {
    fprintf (stderr, "failed to get all parameter lines\n");
    fprintf (stderr, "NTRY: %d\n", NTRY);
    fprintf (stderr, "COL1: %d\n", COL1);
    fprintf (stderr, "COL2: %d\n", COL2);
    fprintf (stderr, "Dfile: %d\n", Dfile);
    fprintf (stderr, "Ffile: %d\n", Ffile);
    fprintf (stderr, "Afile: %d\n", Afile);
    fprintf (stderr, "MSfile: %d\n", MSfile);
    exit (0);
  }
  
}

/*****************************************************************************/

read_datafiles ()
{

  int i;
  char line[1024];

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

  gfits_read_header (fakefile, &fake_h);
  gfits_read_matrix (fakefile, &fake_m);
  Nmass = fake_h.Naxis[0];
  ALLOCATE (Mass, double, Nmass + 1);
  for (i = 0; i < Nmass + 1; i++) {
    sprintf (line, "PAR_1_%0d\0", i);
    gfits_scan (&fake_h, line, "%lf", 1, &Mass[i]);
  }
  
  N_age = fake_h.Naxis[1] - 1;
  for (i = 0; i < N_age + 1; i++) {
    sprintf (line, "PAR_2_%0d\0", i);
    gfits_scan (&fake_h, line, "%lf", 1, &age[i]);
  }

  N_alpha = fake_h.Naxis[3];
  for (i = 0; i < N_alpha; i++) {
    sprintf (line, "PAR_4_%0d\0", i);
    gfits_scan (&fake_h, line, "%lf", 1, &alpha[i]);
  }

  N_dist = fake_h.Naxis[4];
  for (i = 0; i < N_dist; i++) {
    sprintf (line, "PAR_5_%0d\0", i);
    gfits_scan (&fake_h, line, "%lf", 1, &dist[i]);
  }

  N_Av = fake_h.Naxis[5];
  for (i = 0; i < N_Av; i++) {
    sprintf (line, "PAR_6_%0d\0", i);
    gfits_scan (&fake_h, line, "%lf", 1, &Av[i]);
  }

  ALLOCATE (obsdata, double, Nmass*N_age*2);

}
 

/*****************************************************************************/

/*****************************************************************************/

magtomass (A_V, Dist, Mag, Color, Noise, M, A, inflation)
double A_V, Dist, Mag, Color, Noise;
int *M, *A;
double *inflation;
{

  int x, y, I, J, i, j, k;
  double mass, Age, v, uv, Ngood;

  *inflation = 0.0;
  x = (Color - UV0 - 0.7*A_V) / DUV;   /* again, depends on U-V and extinction */
  y = (Mag - Dist - V0 - A_V) / DV;
  if ((x > 0) && (x < MS_NX) && (y > 0) && (y < MS_NY)) {
    mass = Mbuffer[x + MS_NX*y];
    if (mass > 0.0) {
      Age = Abuffer[x + MS_NX*y];
      if ((Age < age[0]) || (Age > age[N_age])) {
	*inflation = 0.0;
	return;
      }
      for (I = 0; (I < N_age) && (Age > age[I+1]); I++);
      if ((mass < Mass[0]) || (mass > Mass[Nmass])) {
	*inflation = 0.0;
	return;
      }
      for (J = 0; (J < Nmass) && (mass > Mass[J+1]); J++);
      Ngood = 1.0;
      for (k = 0; k < NTRY; k++) {
	v = ohana_gaussdev_rnd (Mag, Noise);
	uv = ohana_gaussdev_rnd (Color, 1.4*Noise);
	x = (uv - UV0 - 0.7*A_V) / DUV;
	y = (v - V0 - Dist - A_V) / DV;
	if ((x > 0) && (x < MS_NX) && (y > 0) && (y < MS_NY)) {
	  mass = Mbuffer[x + MS_NX*y];
	  if (mass > 0.0) {
	    Age = Abuffer[x + MS_NX*y];
	    for (i = -1; (i < N_age) && (Age > age[i+1]); i++);
	    for (j = -1; (j < Nmass) && (mass > Mass[j+1]); j++);
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

add_obs (AGE, MASS, value)
int AGE, MASS;
double value;
{

  if (value == 0.0) {
    fprintf (stderr, "error: %d %d %f\n", AGE, MASS, value);
  }
  else {
    obsdata[MASS + Nmass*(AGE        )] += value;
    obsdata[MASS + Nmass*(AGE + N_age)] += 1.0;
  }
}


/*****************************************************************************/

double get_obs (AGE, MASS)
int AGE, MASS;
{

  double value;

  value = obsdata[MASS + Nmass*(AGE)];

  return (value);

}

/*****************************************************************************/

double get_error (AGE, MASS)
int AGE, MASS;
{

  double value;

  if (obsdata[MASS + Nmass*(AGE + N_age)] > 5) {
    value = SQ (obsdata[MASS + Nmass*(AGE)]) / obsdata[MASS + Nmass*(AGE + N_age)];
    return (value);
  }
  if ((obsdata[MASS + Nmass*(AGE + N_age)] < 1) && (obsdata[MASS + Nmass*(AGE)] > 0)) {
    fprintf (stderr, "error: %d %d %e %e\n", AGE, MASS, 
	     obsdata[MASS + Nmass*(AGE + 0)], obsdata[MASS + Nmass*(AGE + 1)]);
  }
  if (obsdata[MASS + Nmass*(AGE + N_age)] > 0) {
    value = SQ (3.0 * obsdata[MASS + Nmass*(AGE)] / obsdata[MASS + Nmass*(AGE + N_age)]);
    return (value);
  }
  value = 0.0;
  return (value);

}


/*****************************************************************************/

double get_fake (AV, DIST, ALPHA, AGE_IN, AGE_OUT, MASS)
int AV, DIST, ALPHA, AGE_IN, AGE_OUT, MASS;
{

  double value;
  float *buffer;

  buffer = (float *)fake_m.buffer;
  
  /* elements in the (N_age) row of each Mass*Age matrix contain special numbers for the matrix */
  value = buffer[MASS + Nmass*(AGE_OUT + (N_age + 1)*(AGE_IN + N_age*(ALPHA + N_alpha*(DIST + N_dist*(AV + N_Av*(0))))))];
  /* buffer[MASS + Nmass*(AGE_OUT + (N_age + 1)*(AGE_IN + N_age*(ALPHA + N_alpha*(DIST + N_dist*(AV + N_Av*(1))))))] += 1.0; */

  return (value);

}

/*****************************************************************************/

double get_fake_err (AV, DIST, ALPHA, AGE_IN, AGE_OUT, MASS)
int AV, DIST, ALPHA, AGE_IN, AGE_OUT, MASS;
{

  double value;
  float *buffer;
  int pix1, pix2;

  buffer = (float *)fake_m.buffer;
  
  /* elements in the (N_age) row of each Mass*Age matrix contain special numbers for the matrix */
  pix1 = MASS + Nmass*(AGE_OUT + (N_age + 1)*(AGE_IN + N_age*(ALPHA + N_alpha*(DIST + N_dist*(AV + N_Av*(0))))));
  pix2 = MASS + Nmass*(AGE_OUT + (N_age + 1)*(AGE_IN + N_age*(ALPHA + N_alpha*(DIST + N_dist*(AV + N_Av*(1))))));

  if (buffer[pix2] > 5) {
    value = SQ (buffer[pix1]) / buffer[pix2];
    return (value);
  }

  if (buffer[pix2] > 0) {
    value = SQ (3.0 * buffer[pix1] / buffer[pix2]);
    return (value);
  }

  value = 0.0;
  return (value);

}

/***************************************************************************/

int load_realstars () 

{

  char line[1024];
  FILE *f;
  double U;
  int i, Nstars;

  f = fopen (datafile, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "couldn't find data file %s\n", datafile);
    exit (0);
  }
  
  Nstars = 1000;
  ALLOCATE (mag, double, Nstars);
  ALLOCATE (color, double, Nstars);
  ALLOCATE (noise, double, Nstars);
  
  /* need to fix allocations */
  for (i = 0; scan_line (f, line) != EOF; i++) {
    if (!stripwhite (line)) continue;
    if (line[0] == '#') continue;
    dparse (&mag[i], COL1, line);
    dparse (&noise[i], (COL1+1), line);
    dparse (&U, COL2, line);
    color[i] = U - mag[i];
    if (i == Nstars - 1) {
      Nstars += 1000;
      REALLOCATE (mag, double, Nstars);
      REALLOCATE (color, double, Nstars);
      REALLOCATE (noise, double, Nstars);
    }
  }

  return (i);
  
}
