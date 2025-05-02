# include "gophot.h"

sortkey (int *index, float *key, int N) {

# define SWAPFUNC(A,B){ \
  int   itmp; itmp = index[A]; index[A] = index[B]; index[B] = itmp; \
  float ftmp; ftmp = key[A]; key[A] = key[B]; key[B] = ftmp; \
}
# define COMPARE(A,B)(key[A] < key[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

paravg () {

  float sum[NPMAX][2];
  int i, j, n;
  FILE *f;
  char c;
  float *ave4, *ave5, *ave6, *key;
  int Ngood, NGOOD, *index, *idx;
  int Nave;
  float a4, a5, a6;

  for (j = 0; j < NPAR; j++) {
    sum[j][0] = 0;
    sum[j][1] = 0;
  }
  
  if (nstot < 1) return (0);

  NGOOD = 100;
  Ngood = 0;
  ALLOCATE (index, int, NGOOD);

  /* find good stars, store index */
  for (i = 0; i < nstot; i++) {
    if (starpar[i][1] < 100) continue;
    if ((imtype[i] == 1) || (imtype[i] == 101)) {
      index[Ngood] = i;
      Ngood ++;
      if (Ngood == NGOOD) {
	NGOOD += 100;
	REALLOCATE (index, int, NGOOD);
      }
    }
  }
  for (i = 0; i < Ngood; i++) {
    fprintf (stderr, "S: %d %d %d  %f %f   %f %f\n", i, index[i], imtype[index[i]], starpar[index[i]][1], shaderr[index[i]][1], starpar[index[i]][4], shadow[index[i]][4]);
  }

  /* find good stars, store index */
  if (Ngood < 10) { /* accept type 2 as well... */
    for (i = 0; i < nstot; i++) {
      if ((imtype[i] == 2) || (imtype[i] == 102)) {
	index[Ngood] = i;
	Ngood ++;
	if (Ngood == NGOOD) {
	  NGOOD += 100;
	  REALLOCATE (index, int, NGOOD);
	}
      }
    }
  }

  if (Ngood > 10) { /* don't change if not enough 'stars' */
    
    ALLOCATE (ave4, float, Ngood);
    ALLOCATE (ave5, float, Ngood);
    ALLOCATE (ave6, float, Ngood);
    ALLOCATE (key,  float, Ngood);
    ALLOCATE (idx,  int, Ngood);
    
    for (i = 0; i < Ngood; i++) {
      idx[i] = i;
      key[i] = shadow[index[i]][4];
      ave4[i] = shadow[index[i]][4];
      ave5[i] = shadow[index[i]][5];
      ave6[i] = shadow[index[i]][6];
    }
    sortkey (idx, key, Ngood);

    a4 = a5 = a6 = Nave = 0;
    for (i = 0.4*Ngood; i < 0.6*Ngood; i++) {
      a4 += ave4[idx[i]];
      a5 += ave5[idx[i]];
      a6 += ave6[idx[i]];
      Nave ++;
    }
    ava[4] = a4/Nave;
    ava[5] = a5/Nave;
    ava[6] = a6/Nave;

    free (key);
    free (idx);
    free (ave4);
    free (ave5);
    free (ave6);
  }
  free (index);
  

  mprint (1, "%d stars used to find parameter averages\n", Ngood);
  mprint (1, "average values so far of shape parameters for stars: \n");
  for (j = 4; j < NPAR; j++) mprint (1, "%f ", ava[j]);
  mprint (1, "\n");

}
/* this function uses C 0,N-1 for a[], fa[] */
