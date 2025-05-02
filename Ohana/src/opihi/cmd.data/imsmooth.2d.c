# include "data.h"

# define NRAD_MAX 44
static float radii2[NRAD_MAX] = {  0.0,  1.0,  2.0,  4.0,  5.0, 
				   8.0,  9.0, 10.0, 13.0, 16.0, 
				   17.0, 18.0, 20.0, 25.0, 26.0, 
				   29.0, 32.0, 34.0, 36.0, 37.0, 
				   40.0, 41.0, 45.0, 49.0, 50.0, 
				   52.0, 53.0, 58.0, 61.0, 64.0,
				   65.0, 68.0, 72.0, 73.0, 74.0, 
				   80.0, 81.0, 82.0, 85.0, 89.0,
				   90.0, 97.0, 98.0, 100.0
};

static int   radiiN[NRAD_MAX] = {    1,  4, 4,  4,  8,
				     4,  4, 8,  8,  4,
				     8,  4, 8, 12,  8, 
				     8,  4, 8,  4,  8,
				     8,  8, 8,  4, 12,
				     8,  8, 8,  8,  4, 
				     16, 8, 4,  8,  8, 
				      8, 4, 8, 16,  8, 
				      8, 8, 4, 12
};

/*
 0,  0 :  0 : 0  :   0
 0,  1 :  1 : 1  :   1
 1,  1 :  2 : 1  :   2
 0,  2 :  3 : 2  :   4
 1,  2 :  4 : 2  :   5
 2,  2 :  5 : 2  :   8
 0,  3 :  6 : 3  :   9
 1,  3 :  7 : 3  :  10
 2,  3 :  8 : 3  :  13
 0,  4 :  9 : 4  :  16
 1,  4 : 10 : 4  :  17
 3,  3 : 11 : 4  :  18
 2,  4 : 12 : 4  :  20
 0,  5 : 13 : 5  :  25
 3,  4 : 13 : 5  :  25
 1,  5 : 14 : 5  :  26
 2,  5 : 15 : 5  :  29
 4,  4 : 16 : 5  :  32
 3,  5 : 17 : 5  :  34
 0,  6 : 18 : 6  :  36
 1,  6 : 19 : 6  :  37
 2,  6 : 20 : 6  :  40
 4,  5 : 21 : 6  :  41
 3,  6 : 22 : 6  :  45
 0,  7 : 23 : 7  :  49
 1,  7 : 24 : 7  :  50 *
 5,  5 : 24 : 7  :  50 *
 4,  6 : 25 : 7  :  52
 2,  7 : 26 : 7  :  53
 3,  7 : 27 : 7  :  58
 5,  6 : 28 : 7  :  61
 0,  8 : 29 : 8  :  64
 1,  8 : 30 : 8  :  65 *
 4,  7 : 30 : 8  :  65 *
 2,  8 : 31 : 8  :  68
 6,  6 : 32 : 8  :  72
 3,  8 : 33 : 8  :  73
 5,  7 : 34 : 8  :  74
 4,  8 : 35 : 8  :  80
 0,  9 : 36 : 9  :  81
 1,  9 : 37 : 9  :  82
 2,  9 : 38 : 9  :  85 *
 6,  7 : 38 : 9  :  85 *
 5,  8 : 39 : 9  :  89
 3,  9 : 40 : 9  :  90
 4,  9 : 41 : 9  :  97
 7,  7 : 42 : 9  :  98
 0, 10 : 43 : 10 : 100 *
 6,  8 : 43 : 10 : 100 *
 */

# define ADD_AXIS(RAD,DD) s += radflux[RAD]*(vi[p - DD] + vi[p + DD] + vi[p - DD*Nx] + vi[p + DD*Nx]);
# define ADD_DIAG(RAD,DD) s += radflux[RAD]*(vi[p - DD - DD*Nx] + vi[p + DD - DD*Nx] + vi[p - DD + DD*Nx] + vi[p + DD + DD*Nx]);
# define ADD_RAND(RAD,DX,DY) s += radflux[RAD]*(vi[p - DX - DY*Nx] + vi[p + DX - DY*Nx] + vi[p - DX + DY*Nx] + vi[p + DX + DY*Nx] + \
						vi[p - DY - DX*Nx] + vi[p + DY - DX*Nx] + vi[p - DY + DX*Nx] + vi[p + DY + DX*Nx]);

int imsmooth_2d (int argc, char **argv) {
  
  int i, j;
  float *vi, *vo;
  Buffer *in;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: imsmooth (input) sigma Nsigma\n");
    return (FALSE);
  }
  
  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  float  sigma = atof(argv[2]);
  float Nsigma = atof(argv[3]);

  int Ns = (int)(Nsigma * sigma);
  Ns = MAX (3, MIN (Ns, 10));
  int Ns2 = Ns * Ns;

  // we are going to use a hard-wired radial profile
  float *radflux = NULL;
  ALLOCATE (radflux, float, NRAD_MAX);
  float sum = 0.0;
  for (i = 0; i < NRAD_MAX; i++) {
    float z = radii2[i] / SQ(sigma);
    radflux[i] = 1.0 / (1.0 + z + pow(z,1.666));
    if (radii2[i] > Ns2) continue;
    sum += radiiN[i] * radflux[i];
  }
  for (i = 0; i < NRAD_MAX; i++) {
    radflux[i] = radflux[i] / sum;
  }

  int Nx = in[0].matrix.Naxis[0];
  int Ny = in[0].matrix.Naxis[1];
  ALLOCATE (vo, float, Nx*Ny);
  memset (vo, 0, Nx*Ny*sizeof(float));

  /* smooth in X direction */
  vi = (float *) in[0].matrix.buffer;
  for (j = Ns; j < Ny - Ns; j++) {
    for (i = Ns; i < Nx - Ns; i++) {

      int p = i + j*Nx; // current pixel of interest
      float s = radflux[0]*(vi[p]);

      // r <= 1.0
      ADD_AXIS (1,  1);      // r^2 = 1

      // r <= 2.0
      ADD_DIAG (2,  1);      // r^2 = 2
      ADD_AXIS (3,  2);      // r^2 = 4

      // r <= 3.0
      ADD_RAND (4,  1,  2);  // r^2 = 5
      ADD_DIAG (5,  2);      // r^2 = 8
      ADD_AXIS (6,  3);      // r^2 = 9
      if (Ns <= 3) goto finish;

      // r <= 4.0
      ADD_RAND (7,  1,  3);  // r^2 = 10
      ADD_RAND (8,  2,  3);  // r^2 = 13
      ADD_AXIS (9,  4);      // r^2 = 16
      if (Ns <= 4) goto finish;

      // r <= 5.0
      ADD_RAND (10,  1,  4); // r^2 = 17
      ADD_DIAG (11,  3);     // r^2 = 18
      ADD_RAND (12,  2,  4); // r^2 = 20
      ADD_RAND (13,  3,  4); // r^2 = 25
      ADD_AXIS (13,  5);     // r^2 = 25
      if (Ns <= 5) goto finish;

      // r <= 6.0
      ADD_RAND (14,  1,  5); // r^2 = 26
      ADD_RAND (15,  2,  5); // r^2 = 29
      ADD_DIAG (16,  4);     // r^2 = 32
      ADD_RAND (17,  3,  5); // r^2 = 34
      ADD_AXIS (18,  6);     // r^2 = 36
      if (Ns <= 6) goto finish;

      // r <= 7.0
      ADD_RAND (19,  1,  6); // r^2 = 37
      ADD_RAND (20,  2,  6); // r^2 = 40
      ADD_RAND (21,  4,  5); // r^2 = 41
      ADD_RAND (22,  3,  6); // r^2 = 45
      ADD_AXIS (23,  7);     // r^2 = 49
      if (Ns <= 7) goto finish;

      // r <= 8.0
      ADD_RAND (24,  1,  7); // r^2 = 50
      ADD_DIAG (24,  5);     // r^2 = 50
      ADD_RAND (25,  4,  6); // r^2 =   52
      ADD_RAND (26,  2,  7); // r^2 =   53
      ADD_RAND (27,  3,  7); // r^2 =   58
      ADD_RAND (28,  5,  6); // r^2 =   61
      ADD_AXIS (29,  8);     // r^2 =   64
      if (Ns <= 8) goto finish;

      ADD_RAND (30,  1,  8); // r^2 =   65 *
      ADD_RAND (30,  4,  7); // r^2 =   65 *
      ADD_RAND (31,  2,  8); // r^2 =   68
      ADD_DIAG (32,  6);     // r^2 =   72
      ADD_RAND (33,  3,  8); // r^2 =   73
      ADD_RAND (34,  5,  7); // r^2 =   74
      ADD_RAND (35,  4,  8); // r^2 =   80
      ADD_AXIS (36,  9);     // r^2 =   81
      if (Ns <= 9) goto finish;

      ADD_RAND (37,  1,  9); // r^2 =   82
      ADD_RAND (38,  2,  9); // r^2 =   85 *
      ADD_RAND (38,  6,  7); // r^2 =   85 *
      ADD_RAND (39,  5,  8); // r^2 =   89
      ADD_RAND (40,  3,  9); // r^2 =   90
      ADD_RAND (41,  4,  9); // r^2 =   97
      ADD_DIAG (42,  7);     // r^2 =   98
      ADD_RAND (43,  6,  8); // r^2 =  100 *
      ADD_AXIS (43, 10);     // r^2 =  100 *
      if (Ns <= 10) goto finish;

    finish:
      vo[p] = s;
    }
  }

  free (radflux);
  free (in[0].matrix.buffer);
  in[0].matrix.buffer = (char *) vo;

  return (TRUE);
}

