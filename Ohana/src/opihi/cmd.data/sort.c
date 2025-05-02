# include "data.h"

// XXX add an option to NOT sort, but return an index instead?
int sort_vectors (int argc, char **argv) {
  
  int i, j, Nvec, Nval;
  opihi_flt *ftemp;
  opihi_int *itemp;
  int *index, *I;
  Vector **vec;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: sort (vector) [vectors ...] \n");
    gprint (GP_ERR, "  first vector is sort key for others\n");
    return (FALSE);
  }

  Nvec = (argc - 1);
  ALLOCATE (vec, Vector *, Nvec);

  Nval = 0;
  /* find vectors and check sizes */
  for (i = 0; i < Nvec; i++) {
    if ((vec[i] = SelectVector (argv[i + 1], OLDVECTOR, FALSE)) == NULL) {
      gprint (GP_ERR, "USAGE: sort vector vector ...\n");
      free (vec);
      return (FALSE);    
    }
    if (i == 0) {
      Nval = vec[i][0].Nelements;
    } else {
      if (Nval != vec[i][0].Nelements) {
	free (vec);
	gprint (GP_ERR, "vectors must all be same length\n");
	return (FALSE);
      }
    }
  }
  
  /* create index */
  ALLOCATE (index, int, Nval);
  for (i = 0; i < Nval; i++) index[i] = i;

  /* sort key & index */
  if (vec[0][0].type == OPIHI_FLT) {
    sort_opihi_flt_index (vec[0][0].elements.Flt, index, Nval);
  } else {
    sort_int_index (vec[0][0].elements.Int, index, Nval);
  }

  ALLOCATE (ftemp, opihi_flt, Nval);
  ALLOCATE (itemp, opihi_int, Nval);

  for (i = 1; i < Nvec; i++) {
    if (vec[i][0].type == OPIHI_FLT) {
      opihi_flt *T = ftemp;
      opihi_flt *V = vec[i][0].elements.Flt;
      I = index;
      for (j = 0; j < Nval; j++, T++, I++) {
	*T = V[*I];
      }
      /* swap .elements.Flt (== V) and ftemp */ 
      vec[i][0].elements.Flt = ftemp;
      ftemp = V;
    } else {
      opihi_int *T = itemp;
      opihi_int *V = vec[i][0].elements.Int;
      I = index;
      for (j = 0; j < Nval; j++, T++, I++) {
	*T = V[*I];
      }
      /* swap .elements.Flt (== V) and itemp */ 
      vec[i][0].elements.Int = itemp;
      itemp = V;
    }
  }
  free (itemp);
  free (ftemp);
  free (vec);
  free (index);

  return (TRUE);

}
