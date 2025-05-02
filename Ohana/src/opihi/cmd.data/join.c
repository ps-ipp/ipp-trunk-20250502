# include "data.h"

int join_IDs_inner (Vector *ID1, Vector *ID2, Vector *index1, Vector *index2);
int join_IDs_outer (Vector *ID1, Vector *ID2, Vector *index);

// join (ID1) (ID2) [-index1 (index1)] [-index2 (index2)]
// generate indexes for ID1 and ID2 to identify the matches of ID1 and ID2 (must be int?)
int join (int argc, char **argv) {
  
  int N, OUTER;
  Vector *ID1vec, *ID2vec;
  Vector *index1, *index2;

  if ((N = get_argument (argc, argv, "-h"))) goto usage;
  if ((N = get_argument (argc, argv, "--help"))) goto usage;

  OUTER = FALSE;
  if ((N = get_argument (argc, argv, "-outer"))) {
    remove_argument (N, &argc, argv);
    OUTER = TRUE;
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

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: join ID1 ID2 [-index1 (index1)] [-index2 (index2)] [-outer]\n");
    gprint (GP_ERR, "  use -h or --help for more detail\n");
    return FALSE;
  }

  if ((ID1vec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((ID2vec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    

  REQUIRE_VECTOR_INT (ID1vec, FALSE); 
  REQUIRE_VECTOR_INT (ID2vec, FALSE); 

  if (OUTER) {
      join_IDs_outer (ID1vec, ID2vec, index1);
      join_IDs_outer (ID2vec, ID1vec, index2);
  } else {
      join_IDs_inner (ID1vec, ID2vec, index1, index2);
  }

  return (TRUE);

usage:
  gprint (GP_ERR, "we have two modes of operation:\n\n");

  gprint (GP_ERR, "without -outer, we are finding all matched ID pairs.  in this\n");
  gprint (GP_ERR, "case, the two index vectors have the same length, one entry per matched pair.\n");
  gprint (GP_ERR, "ID1[index1] matches to ID2[index2].\n\n");

  gprint (GP_ERR, "with -outer selected, we are finding the matched element of set 1 for each of set 2\n");
  gprint (GP_ERR, "and vice versa.  in this case, index1 is always the same length as ID1, while index2\n");
  gprint (GP_ERR, "is the same length as ID2.  ID2[index1] matches ID1 while x1[ID2] matches ID1\n");

  return FALSE;
}

// find the entries in ID1 which match ID2 (duplicates allows)
int join_IDs_inner (Vector *ID1, Vector *ID2, Vector *index1, Vector *index2) {
  
  off_t i, j, first_j, I, J, *N1, *N2, Nmatch, NMATCH, DMATCH;
  opihi_int dID;

  NMATCH = MAX(MAX(0.01*ID1->Nelements, 0.01*ID2->Nelements), 100);
  DMATCH = NMATCH;

  ResetVector (index1, OPIHI_INT, NMATCH);
  ResetVector (index2, OPIHI_INT, NMATCH);

  ALLOCATE (N1, off_t, ID1->Nelements);
  ALLOCATE (N2, off_t, ID2->Nelements);

  for (i = 0; i < ID1->Nelements; i++) { N1[i] = i; }
  for (i = 0; i < ID2->Nelements; i++) { N2[i] = i; }

  sort_IDs_indexonly (ID1->elements.Int, N1, ID1->Nelements);
  sort_IDs_indexonly (ID2->elements.Int, N2, ID2->Nelements);

  Nmatch = 0;
  for (i = j = 0; (i < ID1->Nelements) && (j < ID2->Nelements);) {
    I = N1[i];
    J = N2[j];

    dID = ID1->elements.Int[I] - ID2->elements.Int[J];

    if (dID < 0) { i++; continue; }
    if (dID > 0) { j++; continue; }

    // look for all matches of list2() to list1(i)
    // this allows for multiple values of ID1 or ID2
    first_j = j;
    for (j = first_j; (dID == 0) && (j < ID2->Nelements); j++) {
      J = N2[j];
      dID = ID1->elements.Int[I] - ID2->elements.Int[J];
      if (dID == 0) {
	index1->elements.Int[Nmatch] = I;
	index2->elements.Int[Nmatch] = J;

	Nmatch ++;
	if (Nmatch >= NMATCH) {
	  NMATCH += DMATCH;
	  REALLOCATE (index1->elements.Int, opihi_int, NMATCH);
	  REALLOCATE (index2->elements.Int, opihi_int, NMATCH);
	}
      }
    }
    j = first_j;
    i++;
  }
  index1->Nelements = Nmatch;
  index2->Nelements = Nmatch;

  free (N1);
  free (N2);

  return (TRUE);
}

// find the elements of ID1 which match ID2 (-1 if no match)
int join_IDs_outer (Vector *ID1, Vector *ID2, Vector *index) {
  
  off_t i, j, Jfirst, Ji, I, J, *N1, *N2, NMATCH;
  opihi_int dID;

  NMATCH = ID1->Nelements;
  ResetVector (index, OPIHI_INT, NMATCH);

  for (i = 0; i < index->Nelements; i++) { index->elements.Int[i] = -1; }

  ALLOCATE (N1, off_t, ID1->Nelements);
  ALLOCATE (N2, off_t, ID2->Nelements);

  for (i = 0; i < ID1->Nelements; i++) { N1[i] = i; }
  for (i = 0; i < ID2->Nelements; i++) { N2[i] = i; }

  sort_IDs_indexonly (ID1->elements.Int, N1, ID1->Nelements);
  sort_IDs_indexonly (ID2->elements.Int, N2, ID2->Nelements);

  // find the closest entry in list 2 to the current entry in list 1:
  for (i = j = 0; (i < ID1->Nelements) && (j < ID2->Nelements);) {
    I = N1[i];
    J = N2[j];

    dID = ID1->elements.Int[I] - ID2->elements.Int[J];

    if (dID < 0) { 
      // no match in list 2 to this entry
      index->elements.Int[I] = -1;
      i++; 
      continue; 
    }
    if (dID > 0) { j++; continue; }

    // look for closest matches of list2() to list1(i)
    Jfirst = -1;
    for (Ji = j; (Jfirst == -1) && (dID == 0) && (Ji < ID2->Nelements); Ji++) {
      J = N2[Ji];
      dID = ID1->elements.Int[I] - ID2->elements.Int[J];
      if (dID == 0) {
	Jfirst = J;
      }
    }

    // no match in list 2 to this entry
    if (Jfirst == -1) {
      index->elements.Int[I] = -1;
      i++;
      continue;
    }
    index->elements.Int[I] = Jfirst;
    i++;
  }
  index->Nelements = NMATCH;

  free (N1);
  free (N2);

  return (TRUE);
}
