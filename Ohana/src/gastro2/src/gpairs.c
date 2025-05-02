# include "gastro2.h"

static int NPAIR, Npair;
static int *idx1, *idx2;

void pair_init () {

  Npair = 0;
  NPAIR = 100;
  ALLOCATE (idx1, int, NPAIR);
  ALLOCATE (idx2, int, NPAIR);

}

void pair_add (int i1, int i2) {

  idx1[Npair] = i1;
  idx2[Npair] = i2;

  Npair ++;

  if (Npair == NPAIR) {
    NPAIR += 100;
    REALLOCATE (idx1, int, NPAIR);
    REALLOCATE (idx2, int, NPAIR);
  }

}

int pair_lists (int **index1, int **index2) {

  *index1 = idx1;
  *index2 = idx2;
  return (Npair);
}
