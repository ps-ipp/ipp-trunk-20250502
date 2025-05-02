# include "data.h"

int find_matches1d (Vector *X1, Vector *X2, double Radius, Vector *index1, Vector *index2, Vector *radiusMatch);
int find_matches1d_closest (Vector *X1, Vector *X2, double Radius, Vector *index);

// match1d (X1) (X2) (Radius) [-index1 (index1)] [-index2 (index2)] [-nomatch1 nomatch1] [-nomatch2 nomatch2]
int match1d (int argc, char **argv) {
  
  int N, CLOSEST;
  double Radius;
  char *endptr;
  Vector *X1vec, *X2vec;
  Vector *index1, *index2;

  if ((N = get_argument (argc, argv, "-h"))) goto usage;
  if ((N = get_argument (argc, argv, "--help"))) goto usage;

  CLOSEST = FALSE;
  if ((N = get_argument (argc, argv, "-closest"))) {
    remove_argument (N, &argc, argv);
    CLOSEST = TRUE;
  }

  if ((N = get_argument (argc, argv, "-index1"))) {
    remove_argument (N, &argc, argv);
    if ((index1 = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  } else {
    if ((index1 = SelectVector ("index1", ANYVECTOR, TRUE)) == NULL) return (FALSE);    
  }

  if ((N = get_argument (argc, argv, "-index2"))) {
    remove_argument (N, &argc, argv);
    if ((index2 = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  } else {
    if ((index2 = SelectVector ("index2", ANYVECTOR, TRUE)) == NULL) return (FALSE);    
  }

  Vector *radiusMatch = NULL;
  if ((N = get_argument (argc, argv, "-radius"))) {
    if (CLOSEST) {
      gprint (GP_ERR, "error: -radius and -closest are currently incompatible\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if ((radiusMatch = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: match1d X1 X2 Radius [-index1 (index1)] [-index2 (index2)] [-closest]\n");
    gprint (GP_ERR, "  use -h or --help for more detail\n");
    return (FALSE);
  }

  /*
    we have two modes of operation:  

    without -closest, we are finding all matched pairs within the match radius.  in this
    case, the two index vectors have the same length, one entry per matched pair.
    x1[index1],y1[index1] matches to x2[index2],y2[index2].

    with -closest selected, we are finding the closest element of set 1 to each of set 2
    and vice versa.  in this case, index1 is always the same length as x1,y1, while index2
    is the same lengths as x2,y2.  x2[index1],y2[index1] matches x1,y1 while
    x1[index2],y1[index2] matches x2,y2

   */

  if ((X1vec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((X2vec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    

  REQUIRE_VECTOR_FLT (X1vec, FALSE); 
  REQUIRE_VECTOR_FLT (X2vec, FALSE); 

  Radius = strtod (argv[3], &endptr);
  if (*endptr) {
    gprint (GP_ERR, "Radius must be numerical (%s)\n", argv[3]);
    return (FALSE);
  }

  if (CLOSEST) {
    find_matches1d_closest (X1vec, X2vec, Radius, index1);
    find_matches1d_closest (X2vec, X1vec, Radius, index2);
  } else {
    find_matches1d (X1vec, X2vec, Radius, index1, index2, radiusMatch);
  }
  return (TRUE);

usage:
  gprint (GP_ERR, "we have two modes of operation:\n\n");

  gprint (GP_ERR, "without -closest, we are finding all matched pairs within the match radius.  in this\n");
  gprint (GP_ERR, "case, the two index vectors have the same length, one entry per matched pair.\n");
  gprint (GP_ERR, "x1[index1] matches to x2[index2].\n\n");

  gprint (GP_ERR, "with -closest selected, we are finding the closest element of set 1 to each of set 2\n");
  gprint (GP_ERR, "and vice versa.  in this case, index1 is always the same length as x1, while index2\n");
  gprint (GP_ERR, "is the same lengths as x2.  x2[index1] matches x1 while\n");
  gprint (GP_ERR, "x1[index2] matches x2\n\n");

  gprint (GP_ERR, "if -index1 or -index2 is not supplied, the vectors are created with names index1 or index2\n");
  gprint (GP_ERR, "use 'reindex' to generate new vectors based on these index vectors\n");

  gprint (GP_ERR, "if -radius (vector) is supplied, the vector will be filled with the distance between the matched pairs\n");
  gprint (GP_ERR, "  not valid with -closest\n");
  return FALSE;
}

// Radius is a 1-D separation
int find_matches1d (Vector *X1, Vector *X2, double Radius, Vector *index1, Vector *index2, Vector *radiusMatch) {
  
  off_t i, j, first_j, I, J, *N1, *N2, Nmatch, NMATCH, DMATCH;
  double dX, dR;

  NMATCH = MAX(MAX(0.01*X1->Nelements, 0.01*X2->Nelements), 100);
  DMATCH = NMATCH;

  ResetVector (index1, OPIHI_INT, NMATCH);
  ResetVector (index2, OPIHI_INT, NMATCH);
  if (radiusMatch) ResetVector (radiusMatch, OPIHI_FLT, NMATCH);

  ALLOCATE (N1, off_t, X1->Nelements);
  ALLOCATE (N2, off_t, X2->Nelements);

  for (i = 0; i < X1->Nelements; i++) { N1[i] = i; }
  for (i = 0; i < X2->Nelements; i++) { N2[i] = i; }

  dsort_indexonly (X1->elements.Flt, N1, X1->Nelements);
  dsort_indexonly (X2->elements.Flt, N2, X2->Nelements);

  Nmatch = 0;
  for (i = j = 0; (i < X1->Nelements) && (j < X2->Nelements);) {
    I = N1[i];
    J = N2[j];

    if (!isfinite(X1->elements.Flt[I])) { i++; continue; }
    if (!isfinite(X2->elements.Flt[J])) { j++; continue; }

    dX = X1->elements.Flt[I] - X2->elements.Flt[J];

    if (dX <= -1.02*Radius) { i++; continue; }
    if (dX >= +1.02*Radius) { j++; continue; }

    // look for all matches of list2() to list1(i)
    first_j = j;
    for (j = first_j; (dX > -1.02*Radius) && (j < X2->Nelements); j++) {
      J = N2[j];
      dX = X1->elements.Flt[I] - X2->elements.Flt[J];
      dR = fabs(dX);
      if (dR < Radius) {
	index1->elements.Int[Nmatch] = I;
	index2->elements.Int[Nmatch] = J;
	if (radiusMatch) radiusMatch->elements.Flt[Nmatch] = dR;

	// XXX track matches 1 and 2 with internal vector, save new nomatch index vectors
	// after this loop

	Nmatch ++;
	if (Nmatch >= NMATCH) {
	  NMATCH += DMATCH;
	  REALLOCATE (index1->elements.Int, opihi_int, NMATCH);
	  REALLOCATE (index2->elements.Int, opihi_int, NMATCH);
	  if (radiusMatch) { REALLOCATE (radiusMatch->elements.Flt, opihi_flt, NMATCH); }
	}
      }
    }
    j = first_j;
    i++;
  }
  index1->Nelements = Nmatch;
  index2->Nelements = Nmatch;
  if (radiusMatch) radiusMatch->Nelements = Nmatch;

  free (N1);
  free (N2);

  return (TRUE);
}

// find the elements of X2,Y2 which are closest to each element of X1,Y1 (-1 if no match)
int find_matches1d_closest (Vector *X1, Vector *X2, double Radius, Vector *index) {
  
  off_t i, j, Jmin, Ji, I, J, *N1, *N2, NMATCH;
  double dX, dR, Rmin;

  NMATCH = X1->Nelements;
  ResetVector (index, OPIHI_INT, NMATCH);

  for (i = 0; i < index->Nelements; i++) { index->elements.Int[i] = -1; }

  ALLOCATE (N1, off_t, X1->Nelements);
  ALLOCATE (N2, off_t, X2->Nelements);

  for (i = 0; i < X1->Nelements; i++) { N1[i] = i; }
  for (i = 0; i < X2->Nelements; i++) { N2[i] = i; }

  dsort_indexonly (X1->elements.Flt, N1, X1->Nelements);
  dsort_indexonly (X2->elements.Flt, N2, X2->Nelements);

  // find the closest entry in list 2 to the current entry in list 1:
  for (i = j = 0; (i < X1->Nelements) && (j < X2->Nelements);) {
    I = N1[i];
    J = N2[j];

    dX = X1->elements.Flt[I] - X2->elements.Flt[J];

    if (dX <= -1.02*Radius) { 
      // no match in list 2 to this entry
      index->elements.Int[I] = -1;
      i++; 
      continue; 
    }
    if (dX >= +1.02*Radius) { j++; continue; }

    // look for closest matches of list2() to list1(i)
    Jmin = -1;
    Rmin = Radius;
    for (Ji = j; (dX > -1.02*Radius) && (Ji < X2->Nelements); Ji++) {
      J = N2[Ji];
      dX = X1->elements.Flt[I] - X2->elements.Flt[J];
      dR = fabs(dX);
      if (dR > Radius) continue;
      if (dR < Rmin) {
	Rmin = dR;
	Jmin  = J;
      }
    }

    // no match in list 2 to this entry
    if (Jmin == -1) {
      index->elements.Int[I] = -1;
      i++;
      continue;
    }
    index->elements.Int[I] = Jmin;
    i++;
  }
  index->Nelements = NMATCH;

  free (N1);
  free (N2);

  return (TRUE);
}



