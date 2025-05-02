# include "astro.h"

int spec (int argc, char **argv) {

  int i, j, Xo, X1, y1, y2, Nx;
  int Nlong, Ngap, Nrow, N, Nring;
  float *buffer, *V;
  double sky, sky2, S, SX, F, R, Npts;
  Vector *xvec, *yvec;
  Buffer *buf;

  Nlong = 31;
  if ((N = get_argument (argc, argv, "-Nlong"))) {
    remove_argument (N, &argc, argv);
    Nlong  = 0.5*atof(argv[N]);
    Nlong = 2*Nlong + 1;  /* force an odd number */
    remove_argument (N, &argc, argv);
  }
  
  Ngap = 15;
  if ((N = get_argument (argc, argv, "-Ngap"))) {
    remove_argument (N, &argc, argv);
    Ngap  = 0.5*atof(argv[N]);
    Ngap = 2*Ngap + 1;  /* force an odd number */
    remove_argument (N, &argc, argv);
  }
  
  Nrow = 1;
  if ((N = get_argument (argc, argv, "-Nrow"))) {
    remove_argument (N, &argc, argv);
    Nrow  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  if (argc != 7) {
    gprint (GP_ERR, "USAGE: spec buffer x y1 y2 X Y [-Nlong N] [-Ngap N] [-Nrow N]\n");
    return (FALSE);
  }
  
  if ((Nrow < 1) || (Nlong < 2) || (Ngap < 1) || (Nlong - Ngap < 2)) {
    gprint (GP_ERR, "bad values for options\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  Nx = buf[0].matrix.Naxis[0];
  // int Ny = buf[0].matrix.Naxis[1];

  Xo = atof (argv[2]);
  y1 = atof (argv[3]);
  y2 = atof (argv[4]);

  if ((xvec = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  N = y2 - y1;
  ResetVector (xvec, OPIHI_FLT, N);
  ResetVector (yvec, OPIHI_FLT, N);
  
  ALLOCATE (buffer, float, Nlong);

  for (j = 0; j < y2 - y1; j++) {
    V = (float *) (buf[0].matrix.buffer) + Nx*(y1 + j) + Xo - (int)(0.5*Nlong);
    /* find sky on edge */
    for (i = 0, Nring = 0; i < 0.5*(Nlong - Ngap); i++, V++, Nring++) {
      buffer[i] = *V;
    }
    fsort (buffer, Nring);
    for (Npts = sky = 0, i = 0.25*Nring; i < 0.75*Nring; i++, Npts += 1.0) {
      sky += buffer[i];
    }
    sky = sky / Npts;
    /* find center column for this row */
    for (S = SX = i = 0, Nring = 0; i < Ngap; i++, V++, Nring++) {
      S += (*V - sky);
      SX += (*V - sky)*(i + Xo - 0.5*Ngap);
    }
    X1 = SX / S;
    gprint (GP_ERR, "%4d %4d %5.1f ", j+y1, X1, sky);
    /*    X1 = MAX (MIN (X1, Xo + 0.5+Ngap), Xo - 0.5+Ngap); */
    V = (float *) (buf[0].matrix.buffer) + Nx*(y1 + j) + X1 - (int)(0.5*Nlong);
    /* find sky on edges */
    for (i = 0, Nring = 0; i < 0.5*(Nlong - Ngap); i++, V++, Nring++) {
      buffer[Nring] = *V;
    }
    V = (float *) (buf[0].matrix.buffer) + Nx*(y1 + j) + X1 + (int)(0.5*Ngap);
    for (i = 0; i < 0.5*(Nlong - Ngap); i++, V++, Nring++) {
      buffer[Nring] = *V;
    }
    fsort (buffer, Nring);
    for (Npts = sky = sky2 = 0, i = 0.25*Nring; i < 0.75*Nring; i++, Npts += 1.0) {
      sky += buffer[i];
      sky2 += buffer[i]*buffer[i];
    }
    sky = sky / Npts;
    sky2 = (sky2 / Npts - sky*sky);
    /* find weighted flux */
    V = (float *) (buf[0].matrix.buffer) + Nx*(y1 + j) + X1 - (int)(0.5*Ngap);
    for (F = R = i = 0; i < Ngap; i++, V++) {
      F += (*V - sky) / sky2;
      R += 1.0 / sky2;
    }
    xvec[0].elements.Flt[j] = j + y1; 
    yvec[0].elements.Flt[j] = F / R; 
    gprint (GP_ERR, " %5.1f %7.1f  %6.2f\n", sky, sky2, (F/R));
  }    

  free (buffer);
  return (TRUE);
}
