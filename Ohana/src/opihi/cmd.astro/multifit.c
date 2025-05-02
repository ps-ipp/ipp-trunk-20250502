# include "astro.h"

int multifit (int argc, char **argv) {
  
  char *p, name[64];
  double **a, **b, v;
  int i, j, I, J, n, valid;
  Vector **Nc, **Nmb, **NMb, **Nwb, **Nmh, **Nml, **Nwo;
  int *nterm;
  int Ndim, Norder, Nx, Ny;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: multifit (Norder)\n");
    return (FALSE);
  }

  Ndim = 0;
  Norder = atoi (argv[1]);
  ALLOCATE (nterm, int, Norder);
  for (i = 0; i < Norder; i++) {
    sprintf (name, "nterm:%d", i);
    p = get_variable (name);
    nterm[i] = atoi (p); 
    Ndim += nterm[i];
    free (p);
  }

  ALLOCATE (a, double *, Ndim);
  ALLOCATE (b, double *, Ndim);
  for (i = 0; i < Ndim; i++) {
    ALLOCATE (a[i], double, Ndim);
    ALLOCATE (b[i], double, 1);
    bzero (a[i], Ndim*sizeof(double));
    bzero (b[i], sizeof(double));
  }
 
  ALLOCATE (Nc,  Vector *, Norder);
  ALLOCATE (NMb, Vector *, Norder);
  ALLOCATE (Nmb, Vector *, Norder);
  ALLOCATE (Nwb, Vector *, Norder);
  ALLOCATE (Nmh, Vector *, Norder - 1);
  ALLOCATE (Nml, Vector *, Norder - 1);
  ALLOCATE (Nwo, Vector *, Norder - 1);
  
  for (i = 0; i < Norder; i++) {
    sprintf (name, "c%d", i);
    if ((Nc[i]  = SelectVector (name, ANYVECTOR, TRUE)) == NULL) goto escape;
    ResetVector (Nc[i], OPIHI_FLT, 1); 
    sprintf (name, "Mb%d", i);
    if ((NMb[i] = SelectVector (name, OLDVECTOR, TRUE)) == NULL) goto escape;
    REQUIRE_VECTOR_FLT (NMb[i], FALSE); 
    sprintf (name, "mb%d", i);
    if ((Nmb[i] = SelectVector (name, OLDVECTOR, TRUE)) == NULL) goto escape;
    REQUIRE_VECTOR_FLT (Nmb[i], FALSE); 
    sprintf (name, "wb%d", i);
    if ((Nwb[i] = SelectVector (name, OLDVECTOR, TRUE)) == NULL) goto escape;
    REQUIRE_VECTOR_FLT (Nwb[i], FALSE); 
  }
  for (i = 0; i < Norder - 1; i++) {
    sprintf (name, "ml%d", i);
    if ((Nml[i] = SelectVector (name, OLDVECTOR, TRUE)) == NULL) goto escape;
    REQUIRE_VECTOR_FLT (Nml[i], FALSE); 
    sprintf (name, "mh%d", i);
    if ((Nmh[i] = SelectVector (name, OLDVECTOR, TRUE)) == NULL) goto escape;
    REQUIRE_VECTOR_FLT (Nmh[i], FALSE); 
    sprintf (name, "wo%d", i);
    if ((Nwo[i] = SelectVector (name, OLDVECTOR, TRUE)) == NULL) goto escape;
    REQUIRE_VECTOR_FLT (Nwo[i], FALSE); 
  }

  Ny = 0;
  for (i = 0; i < Norder; i++) {
    for (j = 0; j < nterm[i]; j++, Ny++) {
      Nx = 0;
      valid = FALSE;
      for (I = 0; I < Norder; I++) {
	if (I == i - 1) { 
	  for (J = 0; J < nterm[I]; J++) {
	    v = 0;
	    for (n = 0; n < Nwo[i-1][0].Nelements; n++) {
	      v += -pow (Nwo[i-1][0].elements.Flt[n], (double)(j+J));
	    }
	    a[Ny][Nx] = v;
	    Nx ++;
	  }
	  valid = TRUE;
	}
	if (I == i + 1) { 
	  for (J = 0; J < nterm[I]; J++) {
	    v = 0;
	    for (n = 0; n < Nwo[i][0].Nelements; n++) {
	      v += -pow (Nwo[i][0].elements.Flt[n], (double)(j+J));
	    }
	    a[Ny][Nx] = v;
	    Nx ++;
	  }
	  valid = TRUE;
	}
	if (I == i) { 
	  for (J = 0; J < nterm[I]; J++) {
	    v = 0;
	    for (n = 0; n < Nwb[i][0].Nelements; n++) {
	      v += pow (Nwb[i][0].elements.Flt[n], (double)(j+J));
	    }
	    if (i > 0) {
	      for (n = 0; n < Nwo[i-1][0].Nelements; n++) {
		v += pow (Nwo[i-1][0].elements.Flt[n], (double)(j+J));
	      }
	    }
	    if (i < Norder - 1) {
	      for (n = 0; n < Nwo[i][0].Nelements; n++) {
		v += pow (Nwo[i][0].elements.Flt[n], (double)(j+J));
	      }
	    }
	    a[Ny][Nx] = v;
	    Nx ++;
	  }
	  valid = TRUE;
	}
	if (!valid) {
	  Nx += nterm[I];
	}
      }
    }
  }

  Ny = 0;
  for (i = 0; i < Norder; i++) {
    for (j = 0; j < nterm[i]; j++, Ny++) {
      v = 0;
      for (n = 0; n < Nwb[i][0].Nelements; n++) {
	v += NMb[i][0].elements.Flt[n]*pow (Nwb[i][0].elements.Flt[n], (double)j);
	v -= Nmb[i][0].elements.Flt[n]*pow (Nwb[i][0].elements.Flt[n], (double)j);
      }
      if (i > 0) {
	for (n = 0; n < Nwo[i-1][0].Nelements; n++) {
	  v += Nmh[i-1][0].elements.Flt[n] * pow (Nwo[i-1][0].elements.Flt[n], (double)j);
	  v -= Nml[i-1][0].elements.Flt[n] * pow (Nwo[i-1][0].elements.Flt[n], (double)j);
	}
      }
      if (i < Norder - 1) {
	for (n = 0; n < Nwo[i][0].Nelements; n++) {
	  v += Nml[i][0].elements.Flt[n] * pow (Nwo[i][0].elements.Flt[n], (double)j);
	  v -= Nmh[i][0].elements.Flt[n] * pow (Nwo[i][0].elements.Flt[n], (double)j);
	}
      }
      b[Ny][0] = v;
    }
  }
  dgaussjordan (a, b, Ndim, 1);

  Ny = 0;
  for (i = 0; i < Norder; i++) {
    Nc[i][0].Nelements = nterm[i];
    REALLOCATE (Nc[i][0].elements.Flt, opihi_flt, nterm[i]);
    for (j = 0; j < nterm[i]; j++, Ny++) {
      Nc[i][0].elements.Flt[j] = b[Ny][0];
    }
  }

  for (i = 0; i < Ndim; i++) {
    free (a[i]);
    free (b[i]);
  }
  free (a);
  free (b);
  free (nterm);
  free (Nc);
  free (NMb);
  free (Nmb);
  free (Nwb);
  free (Nmh);
  free (Nml);
  free (Nwo);
  
  return (TRUE);
  
 escape: 
  gprint (GP_ERR, "syntax error\n");
  return (FALSE);
  
}




