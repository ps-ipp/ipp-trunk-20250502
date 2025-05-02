# include "data.h"

// distribute data from one vector to a list of vectors of the form name_nnn where nnn is
// defined by the index vector. save a list of the vector names for reference.

int VERBOSE = FALSE;

opihi_int *_generate_uniq_index (Vector *tvec, opihi_int *nuniq);
int _bisection_scan (opihi_int *values, int Nvalues, opihi_int threshold);

int distribute (int argc, char **argv) {

  int N;

  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  if ((argc != 6) && (argc != 7)) goto syntax;

  int valid = TRUE;
  valid &= !strcmp(argv[2], "from") || !strcmp(argv[2], "=");
  valid &= !strcmp(argv[4], "using") || !strcmp (argv[4], "index");
  if (argc == 7) {
    valid &= !strcmp(argv[4], "using") && !strcmp (argv[5], "index");
  }
  if (!valid) goto syntax;

  // select vector command-line arguments
  // ivec is the input, tvec is the index (the 'test')

  Vector *ivec = NULL, *tvec = NULL, *idxV = NULL;
  if ((ivec = SelectVector (argv[3],      OLDVECTOR, TRUE)) == NULL) goto error;
  if ((tvec = SelectVector (argv[argc-1], OLDVECTOR, TRUE)) == NULL) goto error;
  if (ivec->Nelements != tvec->Nelements) {
    gprint (GP_ERR, "index vector has different length from input vector\n");
    goto error;
  }
  if (tvec->type != OPIHI_INT) {
    gprint (GP_ERR, "index vector must be integer type\n");
    goto error;
  }
  int Nvalues = ivec->Nelements;
  int dNvalues = MAX (ivec->Nelements / 1000, 100);

  if ((idxV = SelectVector ("index", ANYVECTOR, TRUE)) == NULL) goto error;

  // we will generate a bunch of output vectors with names like outbase_(index)
  char *outbase = argv[1];

  // generate a unique list of index values
  opihi_int Nuniq = 0;
  opihi_int *inuniq = _generate_uniq_index (tvec, &Nuniq);
  if (Nuniq > 1000) {
    gprint (GP_ERR, "number of index values (%d) exceeds allowed max of 1000\n", (int) Nuniq);
    free (inuniq);
    goto error;
  }

  // generate an array of output vectors in the order of the index values
  ALLOCATE_PTR (oveclist, Vector *, Nuniq);
  memset (oveclist, 0, Nuniq*sizeof(Vector *)); // zero so we can free allocated elements below
  ALLOCATE_PTR (ovecNmax, int,      Nuniq); // list of currently-allocated elements for vectors
  for (int i = 0; i < Nuniq; i++) {
    char name[64];
    snprintf (name, 64, "%s_%03d", outbase, i);
    char varname[64];
    if ((oveclist[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) {
      goto cleanup;
    }
    snprintf (varname, 64, "distname:%d", i);
    set_str_variable (varname, name);

    ovecNmax[i] = dNvalues;
    ResetVector (oveclist[i], ivec->type, ovecNmax[i]); // allocate ovecNmax[] elements
    oveclist[i]->Nelements = 0; // set actual length to 0
  }    
  set_int_variable ("distname:n", Nuniq);

  // we now have a list of the possible index values. to assign a specific pixel,
  // we will need to traverse the list to find the entry that matches (use bisection)

  // ovec matches ivec in type

  // we have two cases: (ivec == flt or int)
  if (ivec->type == OPIHI_FLT) {
    opihi_flt *vi = ivec[0].elements.Flt;
    opihi_int *vt = tvec[0].elements.Int;
    
    for (int i = 0; i < Nvalues; i++, vi++, vt++) {
      int idx = _bisection_scan (inuniq, Nuniq, *vt);
      myAssert (inuniq[idx] >= 0, "oops");
      myAssert (inuniq[idx] == *vt, "oops");
      int Npts = oveclist[idx]->Nelements;
      oveclist[idx]->elements.Flt[Npts] = *vi;
      oveclist[idx]->Nelements ++;
      if (oveclist[idx]->Nelements >= ovecNmax[idx]) {
	ovecNmax[idx] += dNvalues;
	REALLOCATE (oveclist[idx]->elements.Flt, opihi_flt, ovecNmax[idx]);
      }
    }
  }

  ResetVector (idxV, OPIHI_INT, Nuniq);
  free (idxV->elements.Int);
  idxV->elements.Int = inuniq;

  // free up unused memory
  for (int i = 0; i < Nuniq; i++) {
    REALLOCATE (oveclist[i]->elements.Flt, opihi_flt, oveclist[i]->Nelements);
  }

  return (TRUE);

error:
  return (FALSE);

// need to free the array of output vectors already allocated
cleanup:
  for (int i = 0; i < Nuniq; i++) {
    DeleteVector (oveclist[i]);
  }
  free (oveclist);
  free (ovecNmax);
  return (FALSE);

syntax:
  gprint (GP_ERR, "SYNTAX: distribute vec = vec [using index] (vec)\n");
  gprint (GP_ERR, "SYNTAX: distribute vec = vec [using] (vec)\n");
  gprint (GP_ERR, "SYNTAX: distribute vec = vec [index] (vec)\n");
  gprint (GP_ERR, "SYNTAX: distribute vec from vec [index] (vec)\n");
  return (FALSE);
}

// generate an array of unique values from the vector
opihi_int *_generate_uniq_index (Vector *tvec, opihi_int *nuniq) {

  int Nvalues = tvec->Nelements;

  ALLOCATE_PTR (inuniq, opihi_int, Nvalues);
  ALLOCATE_PTR (indata, opihi_int, Nvalues);

  // copy the input data to a temporary array to avoid damaging it with sort
  memcpy (indata, tvec->elements.Int, Nvalues*sizeof(opihi_int));
  llsort (indata, Nvalues);

  opihi_int Nuniq = 0;
  opihi_int *vtgt = inuniq;
  opihi_int *vsrc = indata;

  int onePercent = Nvalues / 100;

  struct sigaction *old_sigaction = SetInterrupt();
  for (int i = 0; (i < Nvalues) && !interrupt; Nuniq++) {
    vtgt[Nuniq] = *vsrc;
    opihi_int lastValue = *vsrc;
    while ((i < Nvalues) && (*vsrc == lastValue)) {
      i++;
      vsrc ++;
      if (VERBOSE && (i % onePercent == 0)) gprint (GP_ERR, ".");
    }
  }
  ClearInterrupt (old_sigaction);
  if (VERBOSE) gprint (GP_ERR, "\n");
  free (indata);

  *nuniq = Nuniq;
  return inuniq;
}

// in this context, we are guaranteed to have a value == threshold
// return the index of the value == threshold 
int _bisection_scan (opihi_int *values, int Nvalues, opihi_int threshold) {

  int Nlo = 0; 
  int Nhi = Nvalues - 1;

  if (Nvalues < 1) return (-1);
  if (values[Nlo] > threshold) return (-1);

  if (Nvalues < 2) return (0);
  if (values[Nhi] < threshold) return (Nhi);

  int N;
  while (Nhi - Nlo > 4) {
    N = 0.5*(Nlo + Nhi);
    if (values[N] < threshold) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nvalues - 1);
    }
  }
  // values[Nlo] < threshold 
  // values[Nhi] >= threshold 

  for (N = Nlo; N <= Nhi; N++) {
    if (values[N] == threshold) return N;
  }

  // we should never reach here
  return (-1);
}
