# include "opihi.h"

// NOTE: we refer to elements.Ptr if we do not care about the actual type

static Vector **vectors;
static int     Nvectors;
  
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// this function is NOT thread protected : it is only used in startup and/or shutdown
void InitVectors () {
  Nvectors = 0;
  ALLOCATE (vectors, Vector *, 1);
}

// this function is NOT thread protected : it is only used in startup and/or shutdown
void FreeVector (Vector *vec) {

  if (!vec) return;
  if (vec->elements.Int) {
    free (vec->elements.Int);
  }
  free (vec);
}

// this function is NOT thread protected : it is only used in startup and/or shutdown
void FreeVectorArray (Vector **vec, int Nvec) {

  int i;

  if (!vec) return;
  for (i = 0; i < Nvec; i++) {
    FreeVector (vec[i]);
  }
  free (vec);
}

// this function is NOT thread protected : it is only used in startup and/or shutdown
void FreeVectors () {
  FreeVectorArray (vectors, Nvectors);
}

Vector *InitVector () {
  Vector *vec;

  ALLOCATE (vec, Vector, 1);

  vec[0].type = OPIHI_FLT;    // is a float unless specified otherwise
  ALLOCATE (vec[0].elements.Flt, opihi_flt, 1);

  bzero (vec[0].name, OPIHI_NAME_SIZE);
  vec[0].Nelements = 0;
  return (vec);
}

int IsVector (char *name) {
 
  int i;

  if (name == NULL) return (FALSE);

  for (i = 0; (i < Nvectors) && (strcmp(vectors[i][0].name, name)); i++);
  if (i == Nvectors) return (FALSE);
  return (TRUE);
}

int IsVectorNameValid (char *name) {

  char *p = name;

  while (*p) {
    int valid = OHANA_WHITESPACE(*p);
    valid = valid || ISVEC(*p);
    if (!valid) return FALSE;
    p++;
  }
  return TRUE;
}

int IsVectorPtr (Vector *vec) {
 
  int i;

  if (vec == NULL) return (FALSE);

  for (i = 0; (i < Nvectors) && (vectors[i] != vec); i++);
  if (i == Nvectors) return (FALSE);
  return (TRUE);
}

// XXX add TYPE to arguments?
Vector *SelectVector (char *name, int mode, int verbose) {

  int i;

  if (name == NULL) goto error;
  if (ISNUM(name[0])) goto error;
  if (IsBuffer(name)) goto error;

  for (i = 0; (i < Nvectors) && (strcmp(vectors[i][0].name, name)); i++);
  /* is a new vector */
  if (i == Nvectors) { 
    if (mode == OLDVECTOR) goto error;
    // validate vector name syntax
    if (!IsVectorNameValid(name)) { gprint (GP_ERR, "invalid vector name %s\n", name); return NULL; }
    pthread_mutex_lock (&mutex);
    Nvectors ++;
    REALLOCATE (vectors, Vector *, Nvectors);
    vectors[i] = InitVector ();
    strcpy (vectors[i][0].name, name);
    pthread_mutex_unlock (&mutex);
    return (vectors[i]);
  } 
  /* is an old vector */
  if (mode == NEWVECTOR) goto error;
  return (vectors[i]);

 error:
  if (verbose) gprint (GP_ERR, "invalid vector %s\n", name);
  return (NULL);
}
  
// Assign the given Vector to the internal array of vectors by name
int AssignVector (Vector *vec, char *name, int mode, int verbose) {

  int i;

  if (name == NULL) goto error;
  if (ISNUM(name[0])) goto error;
  if (IsBuffer(name)) goto error;

  for (i = 0; (i < Nvectors) && (strcmp(vectors[i][0].name, name)); i++);
  /* is a new vector */
  if (i == Nvectors) { 
    if (mode == OLDVECTOR) goto error;
    pthread_mutex_lock (&mutex);
    Nvectors ++;
    REALLOCATE (vectors, Vector *, Nvectors);
    vectors[i] = vec;
    pthread_mutex_unlock (&mutex);
    return TRUE;
  } 
  /* is an old vector */
  if (mode == NEWVECTOR) goto error;
  if (vectors[i]) {
    // XXX warning: this will be a leak if vector is type OPIHI_STR
    if (vectors[i][0].elements.Ptr) free (vectors[i][0].elements.Ptr);
    free (vectors[i]);
  }
  vectors[i] = vec;
  return TRUE;

 error:
  if (verbose) gprint (GP_ERR, "invalid vector %s\n", name);
  return FALSE;
}
  
/* delete by pointer */
int DeleteVector (Vector *vec) {

  int i, j;

  if (vec == NULL) return (FALSE);

  for (i = 0; (i < Nvectors) && (vec != vectors[i]); i++);
  if (i == Nvectors) return (FALSE);

  // XXX warning: this will be a leak if vector is type OPIHI_STR
  if (vectors[i][0].elements.Ptr) free (vectors[i][0].elements.Ptr);
  free (vectors[i]);

  pthread_mutex_lock (&mutex);
  for (j = i; j < Nvectors - 1; j++) vectors[j] = vectors[j + 1];
  Nvectors --;
  REALLOCATE (vectors, Vector *, MAX (Nvectors, 1));
  pthread_mutex_unlock (&mutex);
  return (TRUE);
}
  
/* delete by name */
int DeleteNamedVector (char *name) {

  int i, j;

  if (name == NULL) return (FALSE);
  for (i = 0; (i < Nvectors) && (strcmp(vectors[i][0].name, name)); i++);
  if (i == Nvectors) return (FALSE);

  // XXX warning: this will be a leak if vector is type OPIHI_STR
  if (vectors[i][0].elements.Ptr) free (vectors[i][0].elements.Ptr);
  free (vectors[i]);

  pthread_mutex_lock (&mutex);
  for (j = i; j < Nvectors - 1; j++) vectors[j] = vectors[j + 1];
  Nvectors --;
  REALLOCATE (vectors, Vector *, MAX (Nvectors, 1));
  pthread_mutex_unlock (&mutex);
  return (TRUE);
}

/* copy data from in to out - new memory space */
int CopyNamedVector (char *out, char *in) {
  Vector *In, *Out;
  if ((In  = SelectVector (in,  OLDVECTOR, FALSE)) == NULL) return (FALSE);
  if ((Out = SelectVector (out, ANYVECTOR, FALSE)) == NULL) return (FALSE);
  CopyVector (Out, In);
  return (TRUE);
}

int CopyVector (Vector *out, Vector *in) {
  // XXX warning: this will be a leak if vector is type OPIHI_STR
  if (out[0].elements.Ptr) free (out[0].elements.Ptr);
  out[0].Nelements = in[0].Nelements;
  if (in[0].elements.Ptr) {
    if (in[0].type == OPIHI_FLT) {
      ALLOCATE (out[0].elements.Flt, opihi_flt, MAX(1,out[0].Nelements));
      memcpy (out[0].elements.Flt, in[0].elements.Flt, out[0].Nelements*sizeof(opihi_flt));
      out[0].type = OPIHI_FLT;
    } else {
      ALLOCATE (out[0].elements.Int, opihi_int, MAX(1,out[0].Nelements));
      memcpy (out[0].elements.Int, in[0].elements.Int, out[0].Nelements*sizeof(opihi_int));
      out[0].type = OPIHI_INT;
    }
  }
  return (TRUE);
}

int MatchVector(Vector *out, Vector *in, char type) {
  // XXX warning: this will be a leak if vector is type OPIHI_STR
  if (out[0].elements.Ptr) free (out[0].elements.Ptr);
  out[0].Nelements = in[0].Nelements;
  if (type == OPIHI_FLT) {
    ALLOCATE (out[0].elements.Flt, opihi_flt, MAX(1,out[0].Nelements));
    out[0].type = OPIHI_FLT;
  } else {
    ALLOCATE (out[0].elements.Int, opihi_int, MAX(1,out[0].Nelements));
    out[0].type = OPIHI_INT;
  }
  return (TRUE);
}

int ResetVector (Vector *vec, char type, int Nelements) {

  // if the supplied vector is a string but the output is not a string, we need to free
  // the unused elements
  if ((vec[0].type == OPIHI_STR) && (vec[0].type != type)) {
    for (int i = 0; i < vec[0].Nelements; i++) {
      FREE (vec[0].elements.Str[i]);
    }
  }

  // a vector can only have >= 0 elements
  vec[0].Nelements = MAX(Nelements,0);

  switch (type) {
    case OPIHI_FLT:
      REALLOCATE (vec[0].elements.Flt, opihi_flt, MAX(1, Nelements));
      vec[0].type = OPIHI_FLT;
      break;
    case OPIHI_INT:
      REALLOCATE (vec[0].elements.Int, opihi_int, MAX(1, Nelements));
      vec[0].type = OPIHI_INT;
      break;
    case OPIHI_STR:
      REALLOCATE (vec[0].elements.Str, char *, MAX(1, Nelements));
      vec[0].type = OPIHI_STR;
      break;
  }
  return TRUE;
}

int SetVectorValues (Vector *vec, void *data, char type, int Nelements) {

  // a vector can only have >= 0 elements
  vec[0].Nelements = MAX(Nelements,0);
  if (type == OPIHI_FLT) {
    free (vec[0].elements.Flt);
    vec[0].elements.Flt = (opihi_flt *) data;
    vec[0].type = OPIHI_FLT;
  } else {
    free (vec[0].elements.Int);
    vec[0].elements.Int = (opihi_int *) data;
    vec[0].type = OPIHI_INT;
  }
  return TRUE;
}

// SetVector (vecx, OPIHI_FLT, MAX (Npts, 1));
// Use this for an unallocated vector (e.g., static variable)
int SetVector (Vector *vec, char type, int Nelements) {

  vec[0].Nelements = MAX(Nelements,0);

  switch (type) {
    case OPIHI_FLT:
      ALLOCATE (vec[0].elements.Flt, opihi_flt, MAX(1,Nelements));
      vec[0].type = OPIHI_FLT;
      break;
    case OPIHI_INT:
      ALLOCATE (vec[0].elements.Int, opihi_int, MAX(1,Nelements));
      vec[0].type = OPIHI_INT;
      break;
    case OPIHI_STR:
      ALLOCATE (vec[0].elements.Ptr, char *, MAX(1,Nelements));
      vec[0].type = OPIHI_STR;
      break;
  }
  return TRUE;
}

// recast the vector to the specified type
int CastVector (Vector *vec, char type) {

  int i;

  // the trivial case
  if (vec[0].type == type) return TRUE;

  switch (type) {
    case OPIHI_FLT: {
      opihi_flt *temp;
      ALLOCATE (temp, opihi_flt, vec[0].Nelements);
      opihi_flt *vo = temp;
      opihi_int *vi = vec[0].elements.Int;
      for (i = 0; i < vec[0].Nelements; i++, vo++, vi++) {
	*vo = *vi;
      }
      free (vec[0].elements.Int);
      vec[0].elements.Flt = temp;
      vec[0].type = OPIHI_FLT;
      break;
    }
    case OPIHI_INT: {
      opihi_int *temp;
      ALLOCATE (temp, opihi_int, vec[0].Nelements);
      opihi_int *vo = temp;
      opihi_flt *vi = vec[0].elements.Flt;
      for (i = 0; i < vec[0].Nelements; i++, vo++, vi++) {
	*vo = *vi;
      }
      free (vec[0].elements.Flt);
      vec[0].elements.Int = temp;
      vec[0].type = OPIHI_INT;
      break;
    }
    case OPIHI_STR: 
    default: 
      // does it make sense to cast an int/flt vector to string?
      break;
  }
  return TRUE;
}

/* move data from in to out - use old memory space */
int MoveNamedVector (char *out, char *in) {
  Vector *In, *Out;
  if ((In  = SelectVector (in,  OLDVECTOR, FALSE)) == NULL) return (FALSE);
  if ((Out = SelectVector (out, ANYVECTOR, FALSE)) == NULL) return (FALSE);
  MoveVector (Out, In);
  return (TRUE);
}

int MoveVector (Vector *out, Vector *in) {
  int i, j;

  if (out[0].elements.Ptr) free (out[0].elements.Ptr);
  out[0].Nelements    = in[0].Nelements;
  out[0].elements.Ptr = in[0].elements.Ptr;
  out[0].type         = in[0].type;

  /* delete vector entry from vector list, if it exists */
  for (i = 0; (i < Nvectors) && (in != vectors[i]); i++);
  if (i == Nvectors) {
    free (in);
    return (TRUE);
  }

  pthread_mutex_lock (&mutex);
  for (j = i; j < Nvectors - 1; j++) vectors[j] = vectors[j + 1];
  Nvectors --;
  REALLOCATE (vectors, Vector *, MAX (Nvectors, 1));
  free (in);
  pthread_mutex_unlock (&mutex);
  return (TRUE);
}

int ListVectors () {

  int i;

  if (Nvectors == 0) {
    gprint (GP_ERR, "No defined vectors\n");
    return (FALSE);
  }

  gprint (GP_LOG, "    N       name      size\n");
  for (i = 0; i < Nvectors; i++) {
    switch (vectors[i][0].type) {
      case OPIHI_FLT:
	gprint (GP_LOG, "%5d %10s %10d (FLT)\n", i, vectors[i][0].name, vectors[i][0].Nelements);
	break;
      case OPIHI_INT:
	gprint (GP_LOG, "%5d %10s %10d (INT)\n", i, vectors[i][0].name, vectors[i][0].Nelements);
	break;
      case OPIHI_STR:
	gprint (GP_LOG, "%5d %10s %10d (STR)\n", i, vectors[i][0].name, vectors[i][0].Nelements);
	break;
    }
  }
  return (TRUE);
}

int ListVectorsToList (char *name) {

  int i;
  char line[1024];

  for (i = 0; i < Nvectors; i++) {
    sprintf (line, "%s:%d", name, i);
    set_str_variable (line, vectors[i][0].name);
  }
  sprintf (line, "%s:n", name);
  set_int_variable (line, Nvectors);
  return (Nvectors);
}

// Take two arrays of vectors and merge equal named vectors.
// Output is a single array in (vec), with vectors lengths of len(vec) + len(invec)
// For ease, require that the order of the names match & number of vectors match
Vector **MergeVectors (Vector **vec, int *Nvec, Vector **invec, int Ninvec) {

  int i, j;

  if (vec == NULL) {
    *Nvec = Ninvec;
    return invec;
  }

  myAssert (*Nvec == Ninvec, "programming error (1) %d vs %d", *Nvec, Ninvec);

  for (i = 0; i < Ninvec; i++) {
    myAssert (!strcmp(vec[i]->name, invec[i]->name), "programming error (2) %s vs %s", vec[i]->name, invec[i]->name);
    myAssert (vec[i]->type == invec[i]->type, "programming error (3), %d vs %d", vec[i]->type, invec[i]->type);

    int N = vec[i]->Nelements;
    if (vec[i]->type == OPIHI_FLT) {
      REALLOCATE (vec[i]->elements.Flt, opihi_flt, vec[i]->Nelements + invec[i]->Nelements);
      for (j = 0; j < invec[i]->Nelements; j++) {
	vec[i]->elements.Flt[N+j] = invec[i]->elements.Flt[j];
      }
    } else {
      REALLOCATE (vec[i]->elements.Int, opihi_int, vec[i]->Nelements + invec[i]->Nelements);
      for (j = 0; j < invec[i]->Nelements; j++) {
	vec[i]->elements.Int[N+j] = invec[i]->elements.Int[j];
      }
    }
    vec[i]->Nelements += invec[i]->Nelements;
  }
  return vec;
}

// Take two arrays of vectors and merge equal named vectors, where the last vector
// specifies the element in the output vector.  All input vectors must have a max sequence
// value of Nelements.  Output is a single array in (vec), with vector lengths of
// Nelements for ease, require that the order of the names match & number of vectors match
Vector **MergeVectorsByIndex (Vector **vec, int *Nvec, Vector **invec, int Ninvec, int Nelements) {

  int i, j;

  // on first call, allocate a new vector, excluding the index
  int newArray = FALSE;
  if (vec == NULL) {
    ALLOCATE (vec, Vector *, Ninvec - 1);
    *Nvec = Ninvec - 1;
    newArray = TRUE;
  } 

  myAssert (*Nvec == Ninvec - 1, "programming error (1)");

  // find the index vector
  int idx = Ninvec - 1;
  myAssert (!strcmp(invec[idx]->name, "index"), "failed to find index vector");
  
  for (i = 0; i < Ninvec - 1; i++) {
    // on first call, create the output vector
    if (newArray) {
      vec[i] = InitVector();
      strcpy (vec[i]->name, invec[i]->name);
      ResetVector (vec[i], invec[i]->type, Nelements);
      for (j = 0; j < Nelements; j++) {
	if (vec[i][0].type == OPIHI_FLT) {
	  vec[i][0].elements.Flt[j] = NAN;
	} else {
	  vec[i][0].elements.Int[j] = 0; // or NAN_INT?
	}
      }
    }

    myAssert (!strcmp(vec[i]->name, invec[i]->name), "programming error (2)");
    myAssert (vec[i]->type == invec[i]->type, "programming error (2)");

    // copy vector elements from input to output, matching location
    if (vec[i]->type == OPIHI_FLT) {
      for (j = 0; j < invec[i]->Nelements; j++) {
	int seq = invec[idx]->elements.Int[j];
	vec[i]->elements.Flt[seq] = invec[i]->elements.Flt[j];
      }
    } else {
      for (j = 0; j < invec[i]->Nelements; j++) {
	int seq = invec[idx]->elements.Int[j];
	vec[i]->elements.Int[seq] = invec[i]->elements.Int[j];
      }
    }
  }
  return vec;
}

