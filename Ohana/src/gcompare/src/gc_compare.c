# include "gcompare.h"
# define D_NMATCH 500;

double gcdist (double r1, double d1, double r2, double d2) {
  double num,den;
  r1 *= M_PI / 180;
  d1 *= M_PI / 180;
  r2 *= M_PI / 180;
  d2 *= M_PI / 180;

  num = sqrt(pow((cos(d2) * sin(r2 - r1)),2) +
	     pow((cos(d1) * sin(d2) -
		  sin(d1) * cos(d2) * cos(r2 - r1)),2));
  den = (sin(d1) * sin(d2) + cos(d1) * cos(d2) * cos(r2 - r1));
  return(atan2(num,den) * (180 / M_PI));
}
  

match_type *gc_compare (data_type data1, data_type data2, int *Nmatches, double radius, double DX, double DY, double noauto) {

  int i, j, Nmatch, NMATCH;
  double dR;
  match_type *match;
  fprintf (stderr, "using gc_compare\n");
  fprintf (stderr, "%f  %f\n", DX, DY);
  Nmatch = 0;
  NMATCH = D_NMATCH;
  ALLOCATE (match, match_type, NMATCH);

  for (i = 0; i < data1.Nvalues ;i++) {
    if (!(i % 100))
      fprintf (stderr, ".");

    for (j = 0; j < data2.Nvalues ; j++) {
      dR = gcdist(data1.values[i].X,data1.values[i].Y,
		  data2.values[j].X,data2.values[j].Y);
/*       fprintf(stderr,"%g %g %g %g => %g\n", */
/* 	      data1.values[i].X,data1.values[i].Y,data2.values[j].X,data2.values[j].Y, */
/* 	      dR); */
      if ((dR < radius) && ((noauto == 0) || (dR > noauto))) {
	match [Nmatch].line1 = data1.values[i].line;
	match [Nmatch].line2 = data2.values[j].line;
	match [Nmatch].dX = data1.values[i].X - data2.values[j].X - DX;
	match [Nmatch].dY = data1.values[i].Y - data2.values[j].Y - DY;
	data1.values[i].match = TRUE;
	data2.values[j].match = TRUE;
	Nmatch ++;
	if (Nmatch == NMATCH - 1) {
	  NMATCH += D_NMATCH;
	  REALLOCATE (match, match_type, NMATCH);
	}
      }
    }
  }
  *Nmatches = Nmatch;
  return (match);
}

