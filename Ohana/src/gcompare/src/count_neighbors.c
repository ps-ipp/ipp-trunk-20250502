# include "gcompare.h"
# define D_NMATCH 500;

int count_neighbors (data, radius, DX, DY)
data_type  data;
double     radius;
double     DX;
double     DY;
{

  int i, j, first_j, Nfriends;
  double dX, dY, dR;

  fprintf (stderr, "%f  %f\n", DX, DY);

  for (i = j = 0; (i < data.Nvalues) && (j < data.Nvalues);) {
    
    dX = data.values[i].X - data.values[j].X - DX;

    if (!(i % 100))
      fprintf (stderr, ".");
    
    Nfriends = 0;

    if (dX <= -radius) {
      fprintf (stdout, "%f %f %d\n", data.values[i].X, data.values[i].Y, Nfriends);
      i++;
    }
    if (dX >= radius)
      j++;

    if (fabs (dX) < radius) {
      first_j = j;
      for (j = first_j; (fabs (dX) < radius) && (j < data.Nvalues); j++) {
	if (i == j) continue;
	dX = data.values[i].X - data.values[j].X - DX;
	dY = data.values[i].Y - data.values[j].Y - DY;
	dR = hypot (dX, dY);
	if (dR < radius) {
	  Nfriends ++;
	}
      }
      j = first_j;
      fprintf (stdout, "%f %f %d\n", data.values[i].X, data.values[i].Y, Nfriends);
      i++;
    }

  }
  return (TRUE);
}

