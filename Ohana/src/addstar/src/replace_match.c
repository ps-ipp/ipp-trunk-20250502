# include "addstar.h"

int replace_match (Average *average, Measure *measure, Measure *newmeas, off_t *found) {

  int i, j, m;

  // index to first measure for this object
  m = average[0].measureOffset;  

  /* search for entry and replace values M, dM, R, D */
  for (i = 0; i < average[0].Nmeasure; i++) {
    j = i + m;
    if (measure[j].photcode != newmeas->photcode) continue;
    measure[j].R  = newmeas->R;
    measure[j].D  = newmeas->D;
    measure[j].M  = newmeas->M;
    measure[j].dM = newmeas->dM;
    *found = average[0].measureOffset + i;
    return (TRUE);
  }
  return (FALSE);
}
