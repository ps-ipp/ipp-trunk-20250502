# include "markrock.h"

count_neighbors (rocks, Nrocks, catalog, catstats)
Rocks *rocks;
int Nrocks;
Catalog catalog[];
CatStats catstats[];
{

  float *x, *y;
  double X, Y, dx, dy, dr;
  int i, j, k;

  x = catstats[0].X;
  y = catstats[0].Y;

  for (i = 0; i < Nrocks; i++) {
    for (k = 0; k < 3; k++) {
      X = rocks[i].X[k];
      Y = rocks[i].Y[k];
      rocks[i].N[k] = 0;
      for (j = 0; j < catalog[0].Naverage; j++) {
	if ((dx = X - x[j]) > ROCK_NEIGHBOR_RADIUS) continue;
	if (dx < -ROCK_NEIGHBOR_RADIUS) break;
	dy = Y - y[j];
	dr = hypot (dx, dy);
	if (dr > ROCK_NEIGHBOR_RADIUS) continue;
	rocks[i].N[k] ++;
      }
    }
  }
}
