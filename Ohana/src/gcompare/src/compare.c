# include "gcompare.h"
# define D_NMATCH 500;

match_type *compare (data_type data1, data_type data2, int *Nmatches, double radius, double DX, double DY, double noauto) {

  int i, j, first_j, Nmatch, NMATCH;
  double dX, dY, dR;
  match_type *match;

  fprintf (stderr, "%f  %f\n", DX, DY);
  Nmatch = 0;
  NMATCH = D_NMATCH;
  ALLOCATE (match, match_type, NMATCH);

  for (i = j = 0;(i < data1.Nvalues) && (j < data2.Nvalues);) {
    
    dX = data1.values[i].X - data2.values[j].X - DX;

    if (!(i % 100))
      fprintf (stderr, ".");
    
    if (dX <= -radius)
      i++;
    if (dX >= radius)
      j++;

    if (fabs (dX) < radius) {
      first_j = j;
      for (j = first_j; (fabs (dX) < radius) && (j < data2.Nvalues); j++) {
	dX = data1.values[i].X - data2.values[j].X - DX;
	dY = data1.values[i].Y - data2.values[j].Y - DY;
	dR = hypot (dX, dY);
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
      j = first_j;
      i++;
    }

  }
  *Nmatches = Nmatch;
  return (match);
}

