# include "data.h"

int find_matches2d (Vector *X1, Vector *Y1, Vector *X2, Vector *Y2, double Radius, Vector *index1, Vector *index2, Vector *radiusMatch);
int find_matches2d_closest (Vector *X1, Vector *Y1, Vector *X2, Vector *Y2, double Radius, Vector *index);

int find_matches2d_sphere (Vector *X1, Vector *Y1, Vector *X2, Vector *Y2, double Radius, Vector *index1, Vector *index2, Vector *radiusMatch);
int find_matches2d_sphere_closest (Vector *X1, Vector *Y1, Vector *X2, Vector *Y2, double Radius, Vector *index);

// match2d (X1) (Y1) (X2) (Y2) (Radius) [-index1 (index1)] [-index2 (index2)] [-nomatch1 nomatch1] [-nomatch2 nomatch2]
// X1[Index1] <=> X2[Index2] (etc)
int match2d (int argc, char **argv) {
  
  int N, CLOSEST;
  double Radius;
  char *endptr;
  Vector *X1vec, *Y1vec, *X2vec, *Y2vec;
  Vector *index1, *index2;

  if ((N = get_argument (argc, argv, "-h"))) goto usage;
  if ((N = get_argument (argc, argv, "--help"))) goto usage;

  CLOSEST = FALSE;
  if ((N = get_argument (argc, argv, "-closest"))) {
    remove_argument (N, &argc, argv);
    CLOSEST = TRUE;
  }

  int SPHERE_DISTANCE = FALSE;
  if ((N = get_argument (argc, argv, "-sphere"))) {
    remove_argument (N, &argc, argv);
    SPHERE_DISTANCE = TRUE;
  }
  if ((N = get_argument (argc, argv, "-sky"))) {
    remove_argument (N, &argc, argv);
    SPHERE_DISTANCE = TRUE;
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

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: match2d X1 Y1 X2 Y2 Radius [-index1 (index1)] [-index2 (index2)] [-closest]\n");
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
  if ((Y1vec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((X2vec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((Y2vec = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);    

  if (X1vec[0].Nelements != Y1vec[0].Nelements) {
    gprint (GP_ERR, "X1 & Y1 vectors must have same length\n");
    return (FALSE);
  }
  if (X2vec[0].Nelements != Y2vec[0].Nelements) {
    gprint (GP_ERR, "X2 & Y2 vectors must have same length\n");
    return (FALSE);
  }

  REQUIRE_VECTOR_FLT (X1vec, FALSE); 
  REQUIRE_VECTOR_FLT (Y1vec, FALSE); 
  REQUIRE_VECTOR_FLT (X2vec, FALSE); 
  REQUIRE_VECTOR_FLT (Y2vec, FALSE); 

  Radius = strtod (argv[5], &endptr);
  if (*endptr) {
    gprint (GP_ERR, "Radius must be numerical (%s)\n", argv[5]);
    return (FALSE);
  }

  if (SPHERE_DISTANCE) {
    if (CLOSEST) {
      find_matches2d_sphere_closest (X1vec, Y1vec, X2vec, Y2vec, Radius, index1);
      find_matches2d_sphere_closest (X2vec, Y2vec, X1vec, Y1vec, Radius, index2);
    } else {
      find_matches2d_sphere (X1vec, Y1vec, X2vec, Y2vec, Radius, index1, index2, radiusMatch);
    }
  } else {
    if (CLOSEST) {
      find_matches2d_closest (X1vec, Y1vec, X2vec, Y2vec, Radius, index1);
      find_matches2d_closest (X2vec, Y2vec, X1vec, Y1vec, Radius, index2);
    } else {
      find_matches2d (X1vec, Y1vec, X2vec, Y2vec, Radius, index1, index2, radiusMatch);
    }
  }
  return (TRUE);

usage:
  gprint (GP_ERR, "we have two modes of operation:\n\n");

  gprint (GP_ERR, "without -closest, we are finding all matched pairs within the match radius.  in this\n");
  gprint (GP_ERR, "case, the two index vectors have the same length, one entry per matched pair.\n");
  gprint (GP_ERR, "x1[index1],y1[index1] matches to x2[index2],y2[index2].\n\n");

  gprint (GP_ERR, "with -closest selected, we are finding the closest element of set 1 to each of set 2\n");
  gprint (GP_ERR, "and vice versa.  in this case, index1 is always the same length as x1,y1, while index2\n");
  gprint (GP_ERR, "is the same lengths as x2,y2.  x2[index1],y2[index1] matches x1,y1 while\n");
  gprint (GP_ERR, "x1[index2],y1[index2] matches x2,y2\n\n");

  gprint (GP_ERR, "if -index1 or -index2 is not supplied, the vectors are created with names index1 or index2\n");
  gprint (GP_ERR, "use 'reindex' to generate new vectors based on these index vectors\n");

  gprint (GP_ERR, "examples:\n");
  gprint (GP_ERR, " for 'match2d x1 y1 x2 y2 radius -closest'\n");
  gprint (GP_ERR, " use 'reindex x2m = x2 using index1 -keep-unmatched'\n");
  gprint (GP_ERR, "   x2m will have values which correspond to x1 (or NAN if not matched)\n");  
  gprint (GP_ERR, " \n");  
  gprint (GP_ERR, " for 'match2d x1 y1 x2 y2 radius'\n");
  gprint (GP_ERR, " use 'reindex x1m = x1 using index1'\n");
  gprint (GP_ERR, " use 'reindex x2m = x2 using index2'\n");
  gprint (GP_ERR, "   x1m will have values which correspond to x2m\n");  
  gprint (GP_ERR, " \n");  

  gprint (GP_ERR, "if -sphere or -sky is supplied, (x1,y1) and (x2,y2) are treaded as (ra,dec) or (long,lat) pairs in degrees\n");

  gprint (GP_ERR, "if -radius (vector) is supplied, the vector will be filled with the distance between the matched pairs\n");
  gprint (GP_ERR, "  not valid with -closest\n");
  return FALSE;
}

// we are not defining a relative offset DX,DY for now
int find_matches2d (Vector *X1, Vector *Y1, Vector *X2, Vector *Y2, double Radius, Vector *index1, Vector *index2, Vector *radiusMatch) {
  
  off_t i, j, first_j, I, J, *N1, *N2, Nmatch, NMATCH, DMATCH;
  double dX, dY, dR, Radius2;

  NMATCH = MAX(MAX(0.01*X1->Nelements, 0.01*X2->Nelements), 100);
  DMATCH = NMATCH;

  ResetVector (index1, OPIHI_INT, NMATCH);
  ResetVector (index2, OPIHI_INT, NMATCH);
  if (radiusMatch) ResetVector (radiusMatch, OPIHI_FLT, NMATCH);

  ALLOCATE (N1, off_t, X1->Nelements);
  ALLOCATE (N2, off_t, X2->Nelements);

  for (i = 0; i < X1->Nelements; i++) { N1[i] = i; }
  for (i = 0; i < X2->Nelements; i++) { N2[i] = i; }

  sort_coords_indexonly (X1->elements.Flt, Y1->elements.Flt, N1, X1->Nelements);
  sort_coords_indexonly (X2->elements.Flt, Y2->elements.Flt, N2, X2->Nelements);

  Radius2 = Radius*Radius;

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
      dY = Y1->elements.Flt[I] - Y2->elements.Flt[J];
      dR = dX*dX + dY*dY;
      if (dR < Radius2) {
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
int find_matches2d_closest (Vector *X1, Vector *Y1, Vector *X2, Vector *Y2, double Radius, Vector *index) {
  
  off_t i, j, Jmin, Ji, I, J, *N1, *N2, NMATCH;
  double dX, dY, dR, Radius2, Rmin;

  NMATCH = X1->Nelements;
  ResetVector (index, OPIHI_INT, NMATCH);

  for (i = 0; i < index->Nelements; i++) { index->elements.Int[i] = -1; }

  ALLOCATE (N1, off_t, X1->Nelements);
  ALLOCATE (N2, off_t, X2->Nelements);

  for (i = 0; i < X1->Nelements; i++) { N1[i] = i; }
  for (i = 0; i < X2->Nelements; i++) { N2[i] = i; }

  sort_coords_indexonly (X1->elements.Flt, Y1->elements.Flt, N1, X1->Nelements);
  sort_coords_indexonly (X2->elements.Flt, Y2->elements.Flt, N2, X2->Nelements);

  Radius2 = Radius*Radius;

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
    Rmin = Radius2;
    for (Ji = j; (dX > -1.02*Radius) && (Ji < X2->Nelements); Ji++) {
      J = N2[Ji];
      dX = X1->elements.Flt[I] - X2->elements.Flt[J];
      dY = Y1->elements.Flt[I] - Y2->elements.Flt[J];
      dR = dX*dX + dY*dY;
      if (dR > Radius2) continue;
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

double gcdist (double r1, double d1, double r2, double d2) {
  double num,den;
  r1 *= RAD_DEG;
  d1 *= RAD_DEG;
  r2 *= RAD_DEG;
  d2 *= RAD_DEG;

  num = sqrt(pow((cos(d2) * sin(r2 - r1)),2) +
	     pow((cos(d1) * sin(d2) -
		  sin(d1) * cos(d2) * cos(r2 - r1)),2));
  den = (sin(d1) * sin(d2) + cos(d1) * cos(d2) * cos(r2 - r1));
  return(atan2(num,den) * (180 / M_PI));
}

typedef struct {
  double sD;
  double cD;
  double sR;
  double cR;
} Match2D_PreCalc;

double gcdist_PreCalc_v0 (Match2D_PreCalc *p1, Match2D_PreCalc *p2) {
  double num,den;

  num = sqrt(pow((p2->cD * (p2->sR*p1->cR - p1->sR*p2->cR)),2) +
	     pow((p1->cD * p2->sD -
		  p1->sD * p2->cD * (p2->cR*p1->cR + p2->sR*p1->sR)),2));
  den = (p1->sD * p2->sD + p1->cD * p2->cD * (p2->cR*p1->cR + p2->sR*p1->sR));
  return(atan2(num,den) * (180 / M_PI));
}

// we are not defining a relative offset DX,DY for now
double gcdist_PreCalc (Match2D_PreCalc *p1, Match2D_PreCalc *p2) {
  double num,den;

  // double Qd = p1->sD * p1->cD * p2->sD * p2->cD;
  // double Qr = p1->sR * p1->cR * p2->sR * p2->cR;

  //  double Xa 
  //    = SQ(p2->cD * p1->cR * p2->sR) 
  //    + SQ(p2->cD * p1->sR * p2->cR) -
  //    - 2 * SQ(p2->cD) * Qr;

  double cdR = (p2->cR*p1->cR + p2->sR*p1->sR);

  double Xa = SQ(p2->cD * p2->sR * p1->cR - p2->cD * p1->sR * p2->cR);
  double Xb = SQ(p1->cD * p2->sD - p1->sD * p2->cD * cdR);

  num = sqrt(Xa + Xb);
  den = (p1->sD * p2->sD + p1->cD * p2->cD * cdR);
  return(atan2(num,den) * (180 / M_PI));
}

// we are not defining a relative offset DX,DY for now
int find_matches2d_sphere (Vector *X1, Vector *Y1, Vector *X2, Vector *Y2, double Radius, Vector *index1, Vector *index2, Vector *radiusMatch) {
  
  off_t i, j, first_j, I, J, *N1, *N2, Nmatch, NMATCH, DMATCH;
  double dY, dR;

  NMATCH = MAX(MAX(0.05*X1->Nelements, 0.05*X2->Nelements), 1000);
  DMATCH = NMATCH;

  ResetVector (index1, OPIHI_INT, NMATCH);
  ResetVector (index2, OPIHI_INT, NMATCH);
  if (radiusMatch) ResetVector (radiusMatch, OPIHI_FLT, NMATCH);

  ALLOCATE (N1, off_t, X1->Nelements);
  ALLOCATE (N2, off_t, X2->Nelements);

  ALLOCATE_PTR (A1, Match2D_PreCalc, X1->Nelements);
  ALLOCATE_PTR (A2, Match2D_PreCalc, X2->Nelements);

  for (i = 0; i < X1->Nelements; i++) { 
    A1[i].sR = sin(RAD_DEG*X1->elements.Flt[i]);
    A1[i].cR = cos(RAD_DEG*X1->elements.Flt[i]);
    A1[i].sD = sin(RAD_DEG*Y1->elements.Flt[i]);
    A1[i].cD = cos(RAD_DEG*Y1->elements.Flt[i]);
    N1[i] = i; 
  }
  for (i = 0; i < X2->Nelements; i++) { 
    A2[i].sR = sin(RAD_DEG*X2->elements.Flt[i]);
    A2[i].cR = cos(RAD_DEG*X2->elements.Flt[i]);
    A2[i].sD = sin(RAD_DEG*Y2->elements.Flt[i]);
    A2[i].cD = cos(RAD_DEG*Y2->elements.Flt[i]);
    N2[i] = i; 
  }

  // sort from one pole to the other
  sort_coords_indexonly (Y1->elements.Flt, X1->elements.Flt, N1, X1->Nelements);
  sort_coords_indexonly (Y2->elements.Flt, X2->elements.Flt, N2, X2->Nelements);

  Nmatch = 0;
  for (i = j = 0; (i < X1->Nelements) && (j < X2->Nelements);) {
    I = N1[i];
    J = N2[j];

    // we can use dY as minimal requirement: if dY > Radius, we are too far apart
    dY = Y1->elements.Flt[I] - Y2->elements.Flt[J];

    if (dY <= -1.02*Radius) { i++; continue; }
    if (dY >= +1.02*Radius) { j++; continue; }

    // look for all matches of list2() to list1(i)
    first_j = j;
    for (j = first_j; (dY > -1.02*Radius) && (j < X2->Nelements); j++) {
      J = N2[j];

      dR = gcdist_PreCalc (&A1[I], &A2[J]);
      // dR = gcdist (X1->elements.Flt[I], Y1->elements.Flt[I], X2->elements.Flt[J], Y2->elements.Flt[J]);

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

  free (A1);
  free (A2);

  free (N1);
  free (N2);

  return (TRUE);
}

// we are not defining a relative offset DX,DY for now
int find_matches2d_sphere_closest (Vector *X1, Vector *Y1, Vector *X2, Vector *Y2, double Radius, Vector *index) {
  
  off_t i, j, Jmin, Ji, I, J, *N1, *N2;
  double dY, dR, Rmin;

  ResetVector (index, OPIHI_INT, X1->Nelements);
  for (i = 0; i < index->Nelements; i++) { index->elements.Int[i] = -1; }

  ALLOCATE (N1, off_t, X1->Nelements);
  ALLOCATE (N2, off_t, X2->Nelements);

  ALLOCATE_PTR (A1, Match2D_PreCalc, X1->Nelements);
  ALLOCATE_PTR (A2, Match2D_PreCalc, X2->Nelements);

  for (i = 0; i < X1->Nelements; i++) { 
    A1[i].sR = sin(RAD_DEG*X1->elements.Flt[i]);
    A1[i].cR = cos(RAD_DEG*X1->elements.Flt[i]);
    A1[i].sD = sin(RAD_DEG*Y1->elements.Flt[i]);
    A1[i].cD = cos(RAD_DEG*Y1->elements.Flt[i]);
    N1[i] = i; 
  }
  for (i = 0; i < X2->Nelements; i++) { 
    A2[i].sR = sin(RAD_DEG*X2->elements.Flt[i]);
    A2[i].cR = cos(RAD_DEG*X2->elements.Flt[i]);
    A2[i].sD = sin(RAD_DEG*Y2->elements.Flt[i]);
    A2[i].cD = cos(RAD_DEG*Y2->elements.Flt[i]);
    N2[i] = i; 
  }

  // sort from one pole to the other
  sort_coords_indexonly (Y1->elements.Flt, X1->elements.Flt, N1, X1->Nelements);
  sort_coords_indexonly (Y2->elements.Flt, X2->elements.Flt, N2, X2->Nelements);

  for (i = j = 0; (i < X1->Nelements) && (j < X2->Nelements);) {
    I = N1[i];
    J = N2[j];

    // we can use dY as minimal requirement: if dY > Radius, we are too far apart
    dY = Y1->elements.Flt[I] - Y2->elements.Flt[J];

    if (dY <= -1.02*Radius) { 
      // no match in list 2 to this entry
      index->elements.Int[I] = -1; // (probably not needed --- index is init'ed above)o
      i++; 
      continue; 
    }
    if (dY >= +1.02*Radius) { j++; continue; }

    // look for all matches of list2() to list1(i)
    Jmin = -1;
    Rmin = Radius;
    for (Ji = j; (dY > -1.02*Radius) && (Ji < X2->Nelements); Ji++) {
      J = N2[Ji];

      dR = gcdist_PreCalc (&A1[I], &A2[J]);
      // dR = gcdist (X1->elements.Flt[I], Y1->elements.Flt[I], X2->elements.Flt[J], Y2->elements.Flt[J]);

      if (dR < Rmin) {
	Rmin = dR;
	Jmin = J;
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

  free (A1);
  free (A2);

  free (N1);
  free (N2);

  return (TRUE);
}


