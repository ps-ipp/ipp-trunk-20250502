# include <stdio.h>
# include <gfitsio.h>
# include <math.h>
# include <malloc.h>
# include <string.h>
# include <sys/time.h>
# include <stdarg.h>
# include <stdlib.h>

# define TRUE (1)
# define FALSE (0)
# define DEG_RAD (57.29578)
# define RAD_DEG (0.017453293)

char PLTDECSN[5];

double NX, NY, PLTRAH, PLTRAM, PLTRAS, PLTDECD, PLTDECM, PLTDECS, Do, Ro;
double CNPIX1, CNPIX2, XPIXELSZ, YPIXELSZ, PPO1, PPO2, PPO3, PPO4, PPO5, PPO6;
double AMDX1, AMDX2, AMDX3, AMDX4, AMDX5, AMDX6, AMDX7, AMDX8, AMDX9, AMDX10, AMDX11, AMDX12, AMDX13;
double AMDY1, AMDY2, AMDY3, AMDY4, AMDY5, AMDY6, AMDY7, AMDY8, AMDY9, AMDY10, AMDY11, AMDY12, AMDY13;

convert (ra, dec, X, Y) 
     double *ra, *dec;
     double X, Y;
{

  double x, y, Azeta, Aeta, eta, zeta;

  x = (PPO3 - XPIXELSZ*(X+CNPIX1)) / 1000.0;
  y = (YPIXELSZ*(Y+CNPIX2) - PPO6) / 1000.0;
  Azeta = AMDX3 + x*(AMDX1 + x*(AMDX4 + AMDX7 + x*AMDX8)) + y*(AMDX2 + y*(AMDX6 + AMDX7 + y*AMDX11)) + x*y*(AMDX9*x + AMDX10*y + AMDX5) + x*(x*x+y*y)*(AMDX12 + AMDX13*(x*x+y*y));
  Aeta  = AMDY3 + y*(AMDY1 + y*(AMDY4 + AMDY7 + y*AMDY8)) + x*(AMDY2 + x*(AMDY6 + AMDY7 + x*AMDY11)) + x*y*(AMDY9*y + AMDY10*x + AMDY5) + y*(x*x+y*y)*(AMDY12 + AMDY13*(x*x+y*y));
  
  eta = Aeta*RAD_DEG/3600.0;
  zeta = Azeta*RAD_DEG/3600.0;
  
  *ra  = DEG_RAD*atan((zeta/cos(RAD_DEG*Do)) / (1.0 - eta*tan(RAD_DEG*Do))) + Ro;
  *dec = DEG_RAD*atan(((eta + tan(RAD_DEG*Do)) * (cos (RAD_DEG*(*ra - Ro)))) / (1 - eta * tan(RAD_DEG*Do)));
  
}


main (int argc, char **argv) 
{

  int i, status;
  Header header;
  double RA, DEC, ra, dec, dRA, dDEC;
  double X, Y, dX, dY;

  if (argc != 2) {
    fprintf (stderr, "USAGE: posscontour filename < contour.reg\n");
    exit (0);
  }

  status = gfits_read_header (argv[1], &header);
  if (!status) {
    fprintf (stderr, "error opening file %s\n", argv[1]);
    exit (0);
  }

  gfits_scan (&header, "NAXIS1", "%lf", 1, &NX);
  gfits_scan (&header, "NAXIS2", "%lf", 1, &NY);
  gfits_scan (&header, "PLTRAH", "%lf", 1, &PLTRAH);
  gfits_scan (&header, "PLTRAM", "%lf", 1, &PLTRAM);
  gfits_scan (&header, "PLTRAS", "%lf", 1, &PLTRAS);
  gfits_scan (&header, "PLTDECD", "%lf", 1, &PLTDECD);
  gfits_scan (&header, "PLTDECM", "%lf", 1, &PLTDECM);
  gfits_scan (&header, "PLTDECS", "%lf", 1, &PLTDECS);
  gfits_scan (&header, "PLTDECSN", "%s", 1, &PLTDECSN);
  gfits_scan (&header, "CNPIX1", "%lf", 1, &CNPIX1);
  gfits_scan (&header, "CNPIX2", "%lf", 1, &CNPIX2);
  gfits_scan (&header, "XPIXELSZ", "%lf", 1, &XPIXELSZ);
  gfits_scan (&header, "YPIXELSZ", "%lf", 1, &YPIXELSZ);
  gfits_scan (&header, "PPO1", "%lf", 1, &PPO1);
  gfits_scan (&header, "PPO2", "%lf", 1, &PPO2);
  gfits_scan (&header, "PPO3", "%lf", 1, &PPO3);
  gfits_scan (&header, "PPO4", "%lf", 1, &PPO4);
  gfits_scan (&header, "PPO5", "%lf", 1, &PPO5);
  gfits_scan (&header, "PPO6", "%lf", 1, &PPO6);
  gfits_scan (&header, "AMDX1", "%lf", 1, &AMDX1);
  gfits_scan (&header, "AMDX2", "%lf", 1, &AMDX2);
  gfits_scan (&header, "AMDX3", "%lf", 1, &AMDX3);
  gfits_scan (&header, "AMDX4", "%lf", 1, &AMDX4);
  gfits_scan (&header, "AMDX5", "%lf", 1, &AMDX5);
  gfits_scan (&header, "AMDX6", "%lf", 1, &AMDX6);
  gfits_scan (&header, "AMDX7", "%lf", 1, &AMDX7);
  gfits_scan (&header, "AMDX8", "%lf", 1, &AMDX8);
  gfits_scan (&header, "AMDX9", "%lf", 1, &AMDX9);
  gfits_scan (&header, "AMDX10", "%lf", 1, &AMDX10);
  gfits_scan (&header, "AMDX11", "%lf", 1, &AMDX11);
  gfits_scan (&header, "AMDX12", "%lf", 1, &AMDX12);
  gfits_scan (&header, "AMDX13", "%lf", 1, &AMDX13);
  gfits_scan (&header, "AMDY1", "%lf", 1, &AMDY1);
  gfits_scan (&header, "AMDY2", "%lf", 1, &AMDY2);
  gfits_scan (&header, "AMDY3", "%lf", 1, &AMDY3);
  gfits_scan (&header, "AMDY4", "%lf", 1, &AMDY4);
  gfits_scan (&header, "AMDY5", "%lf", 1, &AMDY5);
  gfits_scan (&header, "AMDY6", "%lf", 1, &AMDY6);
  gfits_scan (&header, "AMDY7", "%lf", 1, &AMDY7);
  gfits_scan (&header, "AMDY8", "%lf", 1, &AMDY8);
  gfits_scan (&header, "AMDY9", "%lf", 1, &AMDY9);
  gfits_scan (&header, "AMDY10", "%lf", 1, &AMDY10);
  gfits_scan (&header, "AMDY11", "%lf", 1, &AMDY11);
  gfits_scan (&header, "AMDY12", "%lf", 1, &AMDY12);
  gfits_scan (&header, "AMDY13", "%lf", 1, &AMDY13);
  Ro = 15.0*(PLTRAH + PLTRAM/60.0 + PLTRAS/3600.0);
  Do = (PLTDECD + PLTDECM/60.0 + PLTDECS/3600.0);
  if (PLTDECSN[0] == '-') {
    Do *= -1;
  }

  while (fscanf (stdin, "%*s %lf %lf %lf %lf", &X, &Y, &dX, &dY) != EOF) {
    convert (&RA, &DEC, X, Y);
    convert (&ra, &dec, (X+dX), (Y+dY));
    dRA = ra - RA;
    dDEC = dec - DEC;
    fprintf (stdout, "LINE %f %f %f %f\n", RA, DEC, dRA, dDEC);
  }
  
  gfits_free_header (&header);
  
}


/* USAGE: posscontour filename < contour.reg > contour2.reg */
