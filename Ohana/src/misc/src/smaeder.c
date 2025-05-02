# include <ohana.h>

typedef struct {
  double age;
  double UV;
  double V;
} Model;

typedef struct {
  double mass;
  int   Nmodel;
  Model *model;
} Track;

int metal;
double UV0, UV1, dUV;
double V0, V1, dV;
int dump, sdump;
int col1, col2;
char massfile[1024], agefile[1024];

void main (argc, argv)
int argc;
char **argv;
{

  int Ntrack;
  Track *track, *newtrack;

  args (argc, argv);

  fprintf (stderr, "reading in tracks...\n");
  readmaeder (argv[1], &track, &Ntrack);

  fprintf (stderr, "smoothing tracks...\n");
  smoothtracks (&newtrack, track, Ntrack); 

  fprintf (stderr, "interpolating tracks...\n");
  interpolate_tracks (newtrack, Ntrack);

}

args (argc, argv) 
int argc;
char **argv;
{

  int N;

  metal = 4; /* solar metal */
  if (N = get_argument (argc, argv, "-metal")) {
    remove_argument (N, &argc, argv);
    metal = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  else {
    fprintf (stderr, "using solar metallicity\n");
  }
  
  dump = -1;
  if (N = get_argument (argc, argv, "-dump")) {
    remove_argument (N, &argc, argv);
    dump = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  sdump = -1;
  if (N = get_argument (argc, argv, "-sdump")) {
    remove_argument (N, &argc, argv);
    sdump = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  col1 = 3;
  if (N = get_argument (argc, argv, "-col1")) {
    remove_argument (N, &argc, argv);
    col1 = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  col2 = 5;
  if (N = get_argument (argc, argv, "-col2")) {
    remove_argument (N, &argc, argv);
    col2 = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  fprintf (stderr, "using mags in columns %d & %d\n", col1, col2);
  
  UV0 = -2.0;
  UV1 =  2.0;
  dUV =  0.01; 
  if (N = get_argument (argc, argv, "-color")) {
    remove_argument (N, &argc, argv);
    UV0 = atof(argv[N]);
    remove_argument (N, &argc, argv);
    UV1 = atof(argv[N]);
    remove_argument (N, &argc, argv);
    dUV = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  V0 =   5.0;
  V1 = -15.0;
  dV =  -0.02; 
  if (N = get_argument (argc, argv, "-mag")) {
    remove_argument (N, &argc, argv);
    V0 = atof(argv[N]);
    remove_argument (N, &argc, argv);
    V1 = atof(argv[N]);
    remove_argument (N, &argc, argv);
    dV = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  strcpy (massfile, "mass.fits");
  if (N = get_argument (argc, argv, "-mass")) {
    remove_argument (N, &argc, argv);
    strcpy (massfile, argv[N]);
    remove_argument (N, &argc, argv);
  }

  strcpy (agefile, "age.fits");
  if (N = get_argument (argc, argv, "-age")) {
    remove_argument (N, &argc, argv);
    strcpy (agefile, argv[N]);
    remove_argument (N, &argc, argv);
  }

  
  if (argc < 2) {
    fprintf (stderr, "USAGE: smaeder (filename) [options]\n");
    fprintf (stderr, "  options:\n");
    fprintf (stderr, "  [-metal N] (N = 0,1,2,3,4)\n");
    fprintf (stderr, "  [-color min max delta] \n");
    fprintf (stderr, "  [-mag min max delta] \n");
    fprintf (stderr, "  [-mass file] \n");
    fprintf (stderr, "  [-age file]\n");
    exit (0);
  }
}

/* read in the tracks for the requested metalicity */
/* fairly dependent on the format of the files that Andy produces */
readmaeder (maederfile, track, Ntrack)
char  *maederfile;
Track **track;
int   *Ntrack;
{

  int i, j, m;
  FILE *f;
  char line[1024];
  double U, V, ntrack, nmodel;

  f = fopen (maederfile, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "couldn't find maeder tracks (%s)\n", maederfile);
    exit (0);
  }

  for (m = 0; m < 5; m ++) {
    scan_line (f, line);
    dparse (&ntrack, 1, line);
    *Ntrack = ntrack;
    if (m == metal) {
      ALLOCATE (track[0], Track, *Ntrack);
    }
    for (i = 0; i < ntrack; i++) {
      scan_line (f, line);
      dparse (&nmodel, 2, line);
      if (m == metal) {
	track[0][i].Nmodel = nmodel;
	dparse (&track[0][i].mass, 1, line);
	ALLOCATE (track[0][i].model, Model, track[0][i].Nmodel);
      }
      for (j = 0; j < nmodel; j++) {
	scan_line (f, line);
	if (m == metal) {
	  dparse (&track[0][i].model[j].age, 1, line);
	  track[0][i].model[j].age /= 1000000.0;   /* convert to Myrs */
	  dparse (&U, col1, line);
	  dparse (&V, col2, line);
	  track[0][i].model[j].V = V;
	  track[0][i].model[j].UV = U-V;
	}
      }
    }
  }
  fclose (f);

  if (dump > -1) {
    if (dump >= *Ntrack) {
      fprintf (stderr, "track %d not found\n", dump);
      exit (0);
    } 
    for (i = 0; i < track[0][dump].Nmodel; i++) {
      fprintf (stderr, "%d %d %f %e %f %f\n", 
	       i, track[0][dump].Nmodel, track[0][dump].mass, track[0][dump].model[i].age,
	       track[0][dump].model[i].V, track[0][dump].model[i].UV);
    }
    exit (0);
  }

}


/* convert the tracks to smooth, monotonically increasing color tracks */
smoothtracks (newtrack, track, Ntrack)
Track **newtrack, *track;
int Ntrack;
{

  int i, j, J1, J2, npts, done;
  double minC, maxC, dadc, dmdc, age1, mag1, dcolor;
  double resolution, c1, c2;
  
  fprintf (stderr, "smoothing..\n");
  resolution = dUV;
  ALLOCATE (newtrack[0], Track, Ntrack);
  for (i = 0; i < Ntrack ; i++) {
    /* find min & max colors */
    minC = track[i].model[0].UV;
    maxC = -1000;
    for (j = 0; j < track[i].Nmodel; j++) {
      maxC = MAX (track[i].model[j].UV, maxC);
    }
    npts = (int) ((maxC - minC) / resolution) + 1;
    ALLOCATE (newtrack[0][i].model, Model, npts);
    newtrack[0][i].mass = track[i].mass;
    newtrack[0][i].Nmodel = npts;
    for (j = 0; j < npts; j++) {
      newtrack[0][i].model[j].UV = minC + resolution*j;
    }
    if (npts == 1) {
      newtrack[0][i].model[0].age = track[i].model[0].age;
      newtrack[0][i].model[0].V = track[i].model[0].V;
      continue;
    }
    J1 = 0;
    for (J2 = J1+1; (J2 < track[i].Nmodel) 
	   && (track[i].model[J2].UV < track[i].model[J1].UV); J2++);
    if (J2 == track[i].Nmodel) {
      fprintf (stderr, "logical error 1\n");
      exit (0);
    }
    dadc = (track[i].model[J2].age   - track[i].model[J1].age) /
      (track[i].model[J2].UV - track[i].model[J1].UV);
    dmdc = (track[i].model[J2].V   - track[i].model[J1].V) /
      (track[i].model[J2].UV - track[i].model[J1].UV);
    age1 = track[i].model[J1].age;
    mag1 = track[i].model[J1].V;
    for (j = 0; j < npts; j++) {
      if (track[i].model[J2].UV < newtrack[0][i].model[j].UV) {
	J1 = J2;
	for (J2 = J1+1; (J2 < track[i].Nmodel) && ((track[i].model[J2].UV < track[i].model[J1].UV) || (track[i].model[J2].UV < newtrack[0][i].model[j].UV)); J2++) {
	}
	if (J2 == track[i].Nmodel) {
	  fprintf (stderr, "logical error 2\n");
	  exit (0);
	}
	dadc = (track[i].model[J2].age   - track[i].model[J1].age) /
	       (track[i].model[J2].UV - track[i].model[J1].UV);
	dmdc = (track[i].model[J2].V   - track[i].model[J1].V) /
	       (track[i].model[J2].UV - track[i].model[J1].UV);
	age1 = track[i].model[J1].age;
	mag1 = track[i].model[J1].V;
      }	
      dcolor = newtrack[0][i].model[j].UV - track[i].model[J1].UV;
      newtrack[0][i].model[j].age = dadc * dcolor + age1; 
      newtrack[0][i].model[j].V = dmdc * dcolor + mag1; 
    }
  }
  fprintf (stderr, "\n");

  if (sdump > -1) {
    if (sdump >= Ntrack) {
      fprintf (stderr, "track %d not found\n", sdump);
      exit (0);
    }
    for (i = 0; i < newtrack[0][sdump].Nmodel; i++) {
      fprintf (stderr, "%d %f %e %f %f\n", 
	       i, newtrack[0][sdump].mass, newtrack[0][sdump].model[i].age,
	       newtrack[0][sdump].model[i].V, newtrack[0][sdump].model[i].UV);
    }
    exit (0);
  }
  
}

interpolate_tracks (newtrack, Ntrack)
Track *newtrack;
int Ntrack;
{
 
  Header mass_h, age_h;
  Matrix mass_i, age_i;
  double Ms, As, Vs, dM, dA, DV, m, a, a1, a2, m1, m2, v, dummy, tmp;
  double d, minD, uv;
  int i, j, k, J1, Sx, Sy, Ey;
  int ii, I0, I1;

  mass_h.bitpix = -32;
  mass_h.Naxes = 2;
  mass_h.Naxis[0] = (UV1 - UV0) / dUV;
  mass_h.Naxis[1] = fabs((V1 - V0) / dV);
  mass_h.bzero = 0.0;
  mass_h.bscale = 1.0;
  mass_h.unsign = FALSE;
  mass_h.extend = FALSE;

  age_h.bitpix = -32;
  age_h.Naxes = 2;
  age_h.Naxis[0] = (UV1 - UV0) / dUV;
  age_h.Naxis[1] = fabs((V1 - V0) / dV);
  age_h.bzero = 0.0;
  age_h.bscale = 1.0;
  age_h.unsign = FALSE;
  age_h.extend = FALSE;

  gfits_init_header (&mass_h);
  gfits_create_header (&mass_h);
  gfits_create_matrix (&mass_h, &mass_i);

  gfits_init_header (&age_h);
  gfits_create_header (&age_h);
  gfits_create_matrix (&age_h, &age_i);

  fprintf (stderr, "created FITS buffers\n");

  for (i = 1; i < Ntrack; i++) {
    fprintf (stderr, "%d  %d\n", i, newtrack[i].Nmodel);
    J1 = 0;
    for (j = 0; j < newtrack[i].Nmodel; j++) {
      if ((newtrack[i].model[j].V < V0) &&
	  (newtrack[i].model[j].V > V1) &&
	  (newtrack[i].model[j].UV > UV0) &&
	  (newtrack[i].model[j].UV < UV1)) {
	while ((J1 < newtrack[i-1].Nmodel - 1) && ((int)((newtrack[i].model[j].UV  - UV0) / dUV) > (int)((newtrack[i-1].model[J1].UV  - UV0) / dUV))) {
	  J1++;
	}
	uv = newtrack[i].model[j].UV;
	Sx = (newtrack[i].model[j].UV  - UV0) / dUV;
	Sy = (newtrack[i].model[j].V   -  V0) / dV;
	if (newtrack[i].model[j].UV < newtrack[i-1].model[0].UV) {
	  tmp = (uv - newtrack[i-1].model[0].UV) / (newtrack[i].model[0].UV -  newtrack[i-1].model[0].UV);
	  Vs = newtrack[i-1].model[0].V +  tmp * (newtrack[i].model[0].V -  newtrack[i-1].model[0].V);
	}
	else {
	  Vs = newtrack[i-1].model[J1].V;
	}
	Ey = (Vs - V0) / dV;
	if ((j > 20) && (J1 > 20)) {
	  Vs = newtrack[i-1].model[J1].V;
	  Ms = newtrack[i-1].mass;
	  As = newtrack[i-1].model[J1].age;
	  Ey = (newtrack[i-1].model[J1].V - V0) / dV;
	  DV = newtrack[i].model[j].V - Vs;
	  dM = newtrack[i].mass - Ms;
	  dA = newtrack[i].model[j].age - As;
	  for (k = Sy; (k >= Ey) && (k >= 0); k--) {
	    v = k*dV + V0;
	    m = (v - Vs) * dM / DV + Ms;
	    a = (v - Vs) * dA / DV + As;
	    gfits_set_matrix_value (&mass_i, Sx, k, m);
	    gfits_set_matrix_value (&age_i, Sx, k, a);
	  }
	}
	else {
	  for (k = Sy; (k >= Ey) && (k >= 0); k--) {
	    v = k*dV + V0;
	    minD = 1000;
	    for (ii = 0; (ii < newtrack[i].Nmodel); ii++) {
	      I0 = MIN (ii, 20);
	      tmp = 1.0 / hypot(newtrack[i].model[ii].V - newtrack[i-1].model[I0].V, newtrack[i].model[ii].UV - newtrack[i-1].model[I0].UV);
	      d = -tmp * ((newtrack[i].model[ii].UV - newtrack[i-1].model[I0].UV)*(v - newtrack[i-1].model[I0].V) - 
			  (newtrack[i].model[ii].V - newtrack[i-1].model[I0].V)*(uv - newtrack[i-1].model[I0].UV));
	      if ((d > 0) && (d < minD)) {
		I1 = ii;
		minD = d;
	      }
	      if (d > minD) {
		break;
	      }
	    }
	    I0 = MIN (I1, 20);
	    tmp = ((newtrack[i].model[I1].UV - newtrack[i-1].model[I0].UV)*(uv - newtrack[i-1].model[I0].UV) + 
		   (newtrack[i].model[I1].V - newtrack[i-1].model[I0].V)*(v - newtrack[i-1].model[I0].V)) / 
	      hypot(newtrack[i].model[I1].V - newtrack[i-1].model[I0].V, newtrack[i].model[I1].UV - newtrack[i-1].model[I0].UV);
	    d = tmp / hypot (newtrack[i].model[I1].V - newtrack[i-1].model[I0].V, newtrack[i].model[I1].UV - newtrack[i-1].model[I0].UV);
	    dM = newtrack[i].mass - newtrack[i-1].mass;
	    dA = newtrack[i].model[I1].age - newtrack[i-1].model[I0].age;
	    m1 = d * dM + newtrack[i-1].mass;
	    a1 = d * dA + newtrack[i-1].model[I0].age;
	    /*	    fprintf (stderr, "%d  %f  %f  %f  %f  %f\n", I1, minD, d, tmp, dM, m1); */
	    
	    tmp = ((newtrack[i].model[I1+1].UV - newtrack[i-1].model[I0+1].UV)*(uv - newtrack[i-1].model[I0+1].UV) + 
		   (newtrack[i].model[I1+1].V - newtrack[i-1].model[I0+1].V)*(v - newtrack[i-1].model[I0+1].V)) / 
	      hypot(newtrack[i].model[I1+1].V - newtrack[i-1].model[I0+1].V, newtrack[i].model[I1+1].UV - newtrack[i-1].model[I0+1].UV);
	    d = tmp / hypot (newtrack[i].model[I1+1].V - newtrack[i-1].model[I0+1].V, newtrack[i].model[I1+1].UV - newtrack[i-1].model[I0+1].UV);
	    dM = newtrack[i].mass - newtrack[i-1].mass;
	    dA = newtrack[i].model[I1+1].age - newtrack[i-1].model[I0+1].age;
	    m2 = d * dM + newtrack[i-1].mass;
	    a2 = d * dA + newtrack[i-1].model[I0+1].age;

	    d = -((newtrack[i].model[I1].UV - newtrack[i-1].model[I0].UV)*(v - newtrack[i-1].model[I0].V) - 
		 (newtrack[i].model[I1].V - newtrack[i-1].model[I0].V)*(uv - newtrack[i-1].model[I0].UV))
	      / hypot(newtrack[i].model[I1].V - newtrack[i-1].model[I0].V, newtrack[i].model[I1].UV - newtrack[i-1].model[I0].UV);
	    tmp = ((newtrack[i].model[I1+1].UV - newtrack[i-1].model[I0+1].UV)*(v - newtrack[i-1].model[I0+1].V) - 
		 (newtrack[i].model[I1+1].V - newtrack[i-1].model[I0+1].V)*(uv - newtrack[i-1].model[I0+1].UV))
	      / hypot(newtrack[i].model[I1+1].V - newtrack[i-1].model[I0+1].V, newtrack[i].model[I1+1].UV - newtrack[i-1].model[I0+1].UV);

	    a = d * (a2 - a1) / (d + tmp) + a1;
	    m = d * (m2 - m1) / (d + tmp) + m1;
	    gfits_set_matrix_value (&mass_i, Sx, k, m);
	    gfits_set_matrix_value (&age_i, Sx, k, a);
	  }
	}
      }
    }
  }
  /*   
  for (i = 1; i < Ntrack; i++) {
    fprintf (stderr, "%d  %d\n", i, newtrack[i].Nmodel);
    for (j = 0; j < newtrack[i].Nmodel; j++) {
      if ((newtrack[i].model[j].V < V0) &&
	  (newtrack[i].model[j].V > V1) &&
	  (newtrack[i].model[j].UV > UV0) &&
	  (newtrack[i].model[j].UV < UV1)) {
	Sx = (newtrack[i].model[j].UV  - UV0) / dUV;
	Sy = (newtrack[i].model[j].V   -  V0) / dV;
	gfits_set_matrix_value (&age_i, Sx, Sy, 1000.0);
	gfits_set_matrix_value (&mass_i, Sx, Sy, 1000.0);
      }
    }
  }
  */

  gfits_modify (&mass_h, "RA_O", "%lf", 1, UV0);
  gfits_modify (&mass_h, "RA_X", "%lf", 1, dUV);
  gfits_modify (&mass_h, "RA_Y", "%lf", 1, 0.0);
  gfits_modify (&mass_h, "DEC_O", "%lf", 1, V0);
  gfits_modify (&mass_h, "DEC_Y", "%lf", 1, dV);
  gfits_modify (&mass_h, "DEC_X", "%lf", 1, 0.0);

  gfits_write_header (massfile, &mass_h);
  gfits_write_matrix (massfile, &mass_i);

  gfits_modify (&age_h, "RA_O",  "%lf", 1, UV0);
  gfits_modify (&age_h, "RA_X",  "%lf", 1, dUV);
  gfits_modify (&age_h, "RA_Y",  "%lf", 1, 0.0);
  gfits_modify (&age_h, "DEC_O", "%lf", 1, V0);
  gfits_modify (&age_h, "DEC_Y", "%lf", 1, dV);
  gfits_modify (&age_h, "DEC_X", "%lf", 1, 0.0);

  gfits_write_header (agefile, &age_h);
  gfits_write_matrix (agefile, &age_i);

}

/*
 	  minD = 1000;
	  for (ii = 0; ii < newtrack[i-1].Nmodel; ii++) {
	    d = hypot(newtrack[i-1].model[ii].V - v, newtrack[i-1].model[ii].UV - uv);
	    if (d < minD) {
	      I0 = ii;
	      minD = d;
	    }
	  }
	  minD = 1000;
	  for (ii = 0; ii < newtrack[i].Nmodel; ii++) {
	    d = hypot(newtrack[i].model[ii].V - v, newtrack[i].model[ii].UV - uv);
	    if (d < minD) {
	      I1 = ii;
	      minD = d;
	    }
	  }
 for (i = 1; i < Ntrack; i++) {
    fprintf (stderr, "%d  %d\n", i, newtrack[i].Nmodel);
    J1 = 0;
    for (j = 0; j < newtrack[i].Nmodel; j++) {
      if ((newtrack[i].model[j].V < V0) &&
	  (newtrack[i].model[j].V > V1) &&
	  (newtrack[i].model[j].UV > UV0) &&
	  (newtrack[i].model[j].UV < UV1)) {
	while ((J1 < newtrack[i-1].Nmodel - 1) && ((int)((newtrack[i].model[j].UV  - UV0) / dUV) > (int)((newtrack[i-1].model[J1].UV  - UV0) / dUV))) {
	  J1++;
	}
	Sx = (newtrack[i].model[j].UV  - UV0) / dUV;
	Sy = (newtrack[i].model[j].V   -  V0) / dV;
	if (newtrack[i].model[j].UV < newtrack[i-1].model[0].UV) {
	  tmp = (newtrack[i].model[j].UV - newtrack[i-1].model[0].UV) / (newtrack[i].model[0].UV -  newtrack[i-1].model[0].UV);
	  Vs = newtrack[i-1].model[0].V +  tmp * (newtrack[i].model[0].V -  newtrack[i-1].model[0].V);
	  As = newtrack[i-1].model[0].age + tmp * (newtrack[i].model[0].age -  newtrack[i-1].model[0].age);
	  Ms = newtrack[i-1].mass + tmp * (newtrack[i].mass - newtrack[i-1].mass);
	  Ey = (Vs  - V0) / dV;
	}
	else {
	  Vs = newtrack[i-1].model[J1].V;
	  Ms = newtrack[i-1].mass;
	  As = newtrack[i-1].model[J1].age;
	  Ey = (newtrack[i-1].model[J1].V - V0) / dV;
	}
	DV = newtrack[i].model[j].V - Vs;
	dM = newtrack[i].mass - Ms;
	dA = newtrack[i].model[j].age - As;
	for (k = Sy; (k >= Ey) && (k >= 0); k--) {
	  v = k*dV + V0;
	  m = (v - Vs) * dM / DV + Ms;
	  gfits_set_matrix_value (&mass_i, Sx, k, m);
	  a = (v - Vs) * dA / DV + As;
	  gfits_set_matrix_value (&age_i, Sx, k, a);
	}
      }
    }
  }
*/
