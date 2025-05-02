# include "imclean.h"

void find_trails (SMPData *stars, int Nstars) {

  int i;
  char *mark;
  double Angle, axis, m, b;
  
  ALLOCATE (mark, char, Nstars);
  bzero (mark, Nstars);
  
  for (i = 0; i < Nstars; i++) {
    /* already marked, ignore */
    if (mark[i]) continue;
    if (find_group (stars, mark, Nstars, i, &Angle)) {
      /* this point has an excess nearby concentration, find the line */
      axis = find_line (stars, mark, Nstars, i, &m, &b, Angle);
      find_better_line (stars, mark, Nstars, i, &m, &b, axis);
      /* mark_trail (stars, mark, Nstars, i, m, b, axis); */
    }
  }
}

