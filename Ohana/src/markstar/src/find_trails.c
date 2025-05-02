# include "markstar.h"

find_trails (catalog, catstats)
Catalog catalog[];
CatStats catstats[];
{

  int i, j, N, Nave, axis, marked;
  double density, spacing, Area, Angle, m, b, RaCenter, DecCenter;
  double MinRA, MaxRA, MinDEC, MaxDEC;
  float *X1, *Y1;
  char *mark;
  int *N1;
  Coords tcoords;
  
  Nave = catalog[0].Naverage;
  N1 = catstats[0].N;

  ALLOCATE (mark, char, Nave);
  bzero (mark, Nave);

  for (i = 0; i < Nave; i++) {
    /* already marked, ignore */
    if ((mark[i]) || 
	(catalog[0].average[N1[i]].code == ID_BLEED) || 
	(catalog[0].average[N1[i]].code == ID_GHOST))
      continue;
    /* a good star, ignore */
    if ((catalog[0].average[N1[i]].Nm > 3) && 
	(catalog[0].average[N1[i]].Nm > 4*catalog[0].average[N1[i]].Nn)) {
      continue;
    }
    if (find_group (catstats, mark, Nave, i, &Angle)) {
      /* this point has an excess nearby concentration, find the line */
      axis = find_line (catstats, mark, Nave, i, &m, &b, Angle);
      find_better_line (catstats, mark, Nave, i, &m, &b, axis);
      mark_trail (catstats, mark, Nave, i, m, b, axis, catalog);
    }
  }
}

