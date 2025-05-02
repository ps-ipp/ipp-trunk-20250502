# include <ohana.h>


void main (argc, argv)
int argc;
char **argv;
{

  Header mass_h, age_h, UV_h, V_h;
  Matrix mass_i, age_i, UV_i, V_i;
  int x, y, X, Y, X0, X1, Y0, Y1, NAGE, NMASS, N;
  double ldA, lAo, ldM, lMo, age, mass;
  double v, uv, UV0, DUV, V0, DV, slope_x, slope_y, tmp;
  
  lAo =   0.0;
  ldA =   0.01;
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

  lMo = -0.1;
  ldM =  0.05;
  NMASS = 50;
  if (N = get_argument (argc, argv, "-mass")) {
    remove_argument (N, &argc, argv);
    lMo = atof(argv[N]);
    remove_argument (N, &argc, argv);
    ldM = atof(argv[N]);
    remove_argument (N, &argc, argv);
    NMASS = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    fprintf (stderr, "USAGE %s massfile agefile colorfile magfile\n", argv[0]);
    fprintf (stderr, "  options:\n");
    fprintf (stderr, "  [-mass lMo ldM NMASS]\n");
    fprintf (stderr, "  [-age  lAo ldA NAGE]\n");
    exit (0);
  }

  gfits_read_header (argv[1], &mass_h);
  gfits_read_matrix (argv[1], &mass_i);
  gfits_read_header (argv[2], &age_h);
  gfits_read_matrix (argv[2], &age_i); 

  gfits_scan (&mass_h, "RA_O",  "%lf", 1, &UV0);
  gfits_scan (&mass_h, "RA_X",  "%lf", 1, &DUV);
  gfits_scan (&mass_h, "DEC_O", "%lf", 1, &V0);
  gfits_scan (&mass_h, "DEC_Y", "%lf", 1, &DV);

  V_h.bitpix = -32;
  V_h.Naxes = 2;
  V_h.Naxis[0] = NAGE;
  V_h.Naxis[1] = NMASS;
  V_h.bzero = 0.0;
  V_h.bscale = 1.0;
  V_h.unsign = FALSE;
  V_h.extend = FALSE;

  UV_h.bitpix = -32;
  UV_h.Naxes = 2;
  UV_h.Naxis[0] = NAGE;
  UV_h.Naxis[1] = NMASS;
  UV_h.bzero = 0.0;
  UV_h.bscale = 1.0;
  UV_h.unsign = FALSE;
  UV_h.extend = FALSE;

  gfits_init_header (&V_h);
  gfits_create_header (&V_h);
  gfits_create_matrix (&V_h, &V_i);

  gfits_init_header (&UV_h);
  gfits_create_header (&UV_h);
  gfits_create_matrix (&UV_h, &UV_i);
  fprintf (stderr, "created FITS buffers\n");

  gfits_modify (&V_h, "RA_O", "%lf", 1, lAo);
  gfits_modify (&V_h, "RA_X", "%lf", 1, ldA);
  gfits_modify (&V_h, "RA_Y", "%lf", 1, 0.0);
  gfits_modify (&V_h, "DEC_O", "%lf", 1, lMo);
  gfits_modify (&V_h, "DEC_Y", "%lf", 1, ldM);
  gfits_modify (&V_h, "DEC_X", "%lf", 1, 0.0);

  gfits_modify (&UV_h, "RA_O",  "%lf", 1, lAo);
  gfits_modify (&UV_h, "RA_X",  "%lf", 1, ldA);
  gfits_modify (&UV_h, "RA_Y",  "%lf", 1, 0.0);
  gfits_modify (&UV_h, "DEC_O", "%lf", 1, lMo);
  gfits_modify (&UV_h, "DEC_Y", "%lf", 1, ldM);
  gfits_modify (&UV_h, "DEC_X", "%lf", 1, 0.0);

  /* the value of 100 is hard wired in this and the others as a bad color */

  for (x = 0; x < NAGE; x++) {
    for (y = 0; y < NMASS; y++) {
      gfits_set_matrix_value (&UV_i, x, y, 100.0); 
      gfits_set_matrix_value (&V_i, x, y, 100.0); 
    }
  }

  for (x = 0; x < mass_h.Naxis[0]; x++) {
    fprintf (stderr, ".");
    for (y = 0; y < mass_h.Naxis[1]; y++) {
      mass = gfits_get_matrix_value (&mass_i, x, y);
      age = gfits_get_matrix_value (&age_i, x, y); 
      if (mass > 0) {
	X = (log10(age) - lAo) / ldA;
	Y = (log10(mass) - lMo) / ldM;
	if ((X >= 0) && (X < NAGE) &&
	    (Y >= 0) && (Y < NMASS)) {
	  uv = x*DUV + UV0;
	  v  = y*DV + V0;
	  gfits_set_matrix_value (&UV_i, X, Y, uv); 
	  gfits_set_matrix_value (&V_i, X, Y, v); 
	}
      }
    }
  }
  fprintf (stderr, "\n");

  /* fix the holes */
  for (x = 0; x < NAGE; x++) {
    for (y = 0; y < NMASS; y++) {
      uv = gfits_get_matrix_value (&UV_i, x, y);
      if (uv == 100.0) {
	/* find neighbors up and down */
	X1 = NAGE; Y1 = NMASS;
	X0 = Y0 = -1;
	for (X = x + 1; X < NAGE; X++) {
	  if ((gfits_get_matrix_value (&UV_i, X, y)) != 100.0) {
	    X1 = X;
	    break;
	  }
	}
	for (X = x - 1; X >= 0; X--) {
	  if ((gfits_get_matrix_value (&UV_i, X, y)) != 100.0) {
	    X0 = X;
	    break;
	  }
	}
	for (Y = y + 1; Y < NMASS; Y++) {
	  if ((gfits_get_matrix_value (&UV_i, x, Y)) != 100.0) {
	    Y1 = Y;
	    break;
	  }
	}
	for (Y = y - 1; Y >= 0; Y--) {
	  if ((gfits_get_matrix_value (&UV_i, x, Y)) != 100.0) {
	    Y0 = Y;
	    break;
	  }
	}
	slope_x = slope_y = 100000.0;
	if ((X1 < NAGE) && (X0 > -1)) {
	  slope_x = (gfits_get_matrix_value (&UV_i, X1, y) - gfits_get_matrix_value (&UV_i, X0, y)) / (X1 - X0);
	}
	if ((Y1 < NMASS) && (Y0 > -1)) {
	  slope_y = (gfits_get_matrix_value (&UV_i, x, Y1) - gfits_get_matrix_value (&UV_i, x, Y0)) / (Y1 - Y0);
	}
	if ((fabs(slope_x) < fabs(slope_y)) && (slope_x != 100000.0)) {
	  uv = (x - X0) * slope_x + gfits_get_matrix_value (&UV_i, X0, y);
	  tmp = (gfits_get_matrix_value (&V_i, X1, y) - gfits_get_matrix_value (&V_i, X0, y)) / (X1 - X0);
	  v  = (x - X0) * tmp + gfits_get_matrix_value (&V_i, X0, y);
	  gfits_set_matrix_value (&UV_i, x, y, uv); 
	  gfits_set_matrix_value (&V_i, x, y, v); 
	}	  
	if ((fabs(slope_x) >= fabs(slope_y)) && (slope_y != 100000.0)) {
	  uv = (y - Y0) * slope_y + gfits_get_matrix_value (&UV_i, x, Y0);
	  tmp = (gfits_get_matrix_value (&V_i, x, Y1) - gfits_get_matrix_value (&V_i, x, Y0)) / (Y1 - Y0);
	  v  = (y - Y0) * tmp + gfits_get_matrix_value (&V_i, x, Y0);
	  gfits_set_matrix_value (&UV_i, x, y, uv); 
	  gfits_set_matrix_value (&V_i, x, y, v); 
	}	  
      }
    }
  }
	


  gfits_write_header (argv[3], &V_h);
  gfits_write_matrix (argv[3], &V_i);
  gfits_write_header (argv[4], &UV_h);
  gfits_write_matrix (argv[4], &UV_i);

}

