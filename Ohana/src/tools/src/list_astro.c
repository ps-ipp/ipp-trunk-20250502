# include <ohana.h>

int main (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  float *ra1, *ra2, *dec1, *dec2;
  float x, y, x2, y2, xy, R, D, Rx, Ry, Dx, Dy, N;
  float x3, x2y, x4, Rx2;
  float DEC, RA, DX, DY, RX, RY, d2RA, d2DEC, dRA, dDEC;
  float Sx2, Sy2, Sxy, Srx, Sry, Sdx, Sdy;
  int i, Npairs, NPAIR;

  NPAIR = 100;
  ALLOCATE (ra1, float, NPAIR);
  ALLOCATE (ra2, float, NPAIR);
  ALLOCATE (dec1, float, NPAIR);
  ALLOCATE (dec2, float, NPAIR);

/*
  ALLOCATE (a, double *, 3);
  ALLOCATE (a[0], double, 3);
  ALLOCATE (a[1], double, 3);
  ALLOCATE (a[2], double, 3);
  ALLOCATE (a[3], double, 4);
  ALLOCATE (b, double *, 3);
  ALLOCATE (b[0], double, 1);
  ALLOCATE (b[1], double, 1);
  ALLOCATE (b[2], double, 1);
  ALLOCATE (b[3], double, 1);
*/

  if (argc > 1) {
    fprintf (stderr, "USAGE: list_astro x\nTakes list from the stdin in the");
    fprintf (stderr, " format:\n  X Y RA DEC\nReturns:\n  RA, RX, RY, dRA\n  DEC, DX, DY, dDEC\n");
    exit(0);
  }
    
  for (i = 0; fscanf (stdin, "%f %f %f %f", &ra1[i], &dec1[i], &ra2[i], &dec2[i]) != EOF; i++) {
    if (i == NPAIR - 1) {
      NPAIR += 50;
      REALLOCATE (ra1, float, NPAIR);
      REALLOCATE (ra2, float, NPAIR);
      REALLOCATE (dec1, float, NPAIR);
      REALLOCATE (dec2, float, NPAIR);
    }
  }

  
  
  N = x = y = x2 = y2 = xy = R = D = Rx = Ry = Dx = Dy = 0;
  Npairs = i;
  for (i = 0; i < Npairs; i++) {
    x  += ra1[i];
    y  += dec1[i];
    x2 += ra1[i]*ra1[i];
    y2 += dec1[i]*dec1[i];
    xy += ra1[i]*dec1[i];
    x3 += ra1[i]*ra1[i]*ra1[i];
    x2y += ra1[i]*ra1[i]*dec1[i];
    x4 += ra1[i]*ra1[i]*ra1[i]*ra1[i];
    R  += ra2[i];
    D  += dec2[i];
    Rx += ra2[i]*ra1[i];
    Ry += ra2[i]*dec1[i];
    Rx2 += ra2[i]*ra1[i]*ra1[i];
    Dx += dec2[i]*ra1[i];
    Dy += dec2[i]*dec1[i];
    N  += 1.0;
  }

/*
  a[0][0] = N;
  a[0][1] = a[1][0] = x;
  a[0][2] = a[2][0] = y;
  a[0][3] = a[3][0] = x2;
  a[1][1] = x2;
  a[1][2] = a[2][1] = xy;
  a[1][3] = a[3][1] = x3;
  a[2][2] = y2;
  a[2][3] = a[3][2] = x2y;
  a[3][3] = x4;
  b[0][0] = R;
  b[1][0] = Rx;
  b[2][0] = Ry;
  b[1][0] = Rx2;
*/

  Sx2 = x2 - x*x/N;
  Sy2 = y2 - y*y/N;
  Sxy = xy - x*y/N;
  Srx = Rx - R*x/N;
  Sry = Ry - R*y/N;
  Sdx = Dx - D*x/N;
  Sdy = Dy - D*y/N;
  
  RX = (Srx*Sy2 - Sry*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
  RY = (Sry*Sx2 - Srx*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
  RA = R/N - RX*x/N - RY*y/N;

  DX  = (Sdx*Sy2 - Sdy*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
  DY  = (Sdy*Sx2 - Sdx*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
  DEC = D/N - DX*x/N - DY*y/N;

  d2RA = d2DEC = N = 0;
  N = x = y = x2 = y2 = xy = R = D = Rx = Ry = Dx = Dy = 0;
  for (i = 0; i < Npairs; i++) {
    d2RA += pow((ra2[i] - RA - RX*ra1[i] - RY*dec1[i]), 2.0);
    d2DEC += pow((dec2[i] - DEC - DX*ra1[i] - DY*dec1[i]), 2.0);
    N += 1.0;
  }

  dRA  = 3600.0*sqrt(d2RA / (N - 3.0));
  dDEC = 3600.0*sqrt(d2DEC / (N - 3.0));

  fprintf (stdout, " %12.9f %12.9e %12.9e %12.9f\n", RA, RX, RY, dRA);
  fprintf (stdout, " %12.9f %12.9e %12.9e %12.9f\n", DEC, DX, DY, dDEC);
  exit (0);
}
