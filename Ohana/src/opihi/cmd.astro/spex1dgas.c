# include "astro.h"

/* We have N objects with N(N-1)/2 paired distances.  We want to find the 1D distribution that
   best describes the observed distances.  Assume the distances are in a 1D space, though in
   principle it could be N-D or even non-euclidean.

   Model the points as a gas under pressure.  

   Start with the a guess for positions of the objects at some locations in 2D

   P = (d_now - d_tru)

   Iterate over the points and move in X,Y

   Find (dP/dX,dP/dY) for modest moves in X,Y

   Move based on dP

   USAGE: spex1dgas incdex1 index2 distance

   outline:

   * load data
   * generate the unique objects
   * determine the max distance needed
   * place each object in the 2D space

   * iterate:
   ** calculate dP/dX,dPdY for each object
   ** move each object proportionally to the pressure gradient

   */

typedef struct {
  int Nindex;   // number of tested relationships
  int NINDEX;   // number of allocated relationships
  int *index;  // indices for all other objects
  float *Dtgt; // target distance for this relationship
  float *Dcur; // current distance for this relationship
  float Xo,Yo; // current X,Y position of this object
  float dPdX;  // pressure in X
  float dPdY;  // pressure in Y
} Object;

static Object *object = NULL;
static int Nobject = 0;
// static int NOBJECT = 0;

void sortfriends (float *X, int *IDX1, int N);

static void get_pressure_gradient (int iObj, int nCloseMax, float farFrac, int nearNeighbors, float maxPressure) {

  int i;
  
  float dPdX = 0.0;

  // only use the first N friends
  for (i = 0; (i < object[iObj].Nindex); i++) {
    int jObj = object[iObj].index[i];

    // some options:
    // if the iterations are small, we should only worry about getting the near neighbors right
    // if the iterations are large, we should add in more distant objects
    if (nearNeighbors && (i >= nCloseMax)) break;
    if (!nearNeighbors && (i >= nCloseMax)) {
      if (drand48() > farFrac) continue;
    }

    float Dtgt = object[iObj].Dtgt[i];
    float dX = object[jObj].Xo - object[iObj].Xo;
    float Dcur = dX;

    // the force law as a function of (Dcur - Dtgt) : if Dcur is too large, dF is negative
    // float dF = (Dcur < 0.01*Dtgt) ? -100.0 : (Dcur - Dtgt) / Dcur; XXX modified spring constant : too crazy
    float dF = (Dcur - Dtgt);
    dF = MIN (maxPressure, MAX (-maxPressure, dF));

    float dPdXi;
    if (fabs(Dcur) < 1e-6) {
      dPdXi = 0.0;
    } else {
      dPdXi = dF;
    }

    if (isnan(Dtgt) || isnan(dX) || isnan(Dcur) || isnan(dF) || isnan(dPdXi)) abort();
    dPdX += dPdXi;
  }
  object[iObj].dPdX = dPdX;
  object[iObj].dPdY = 0.0;

  return;
}

static void move_object (int iObj) {

  object[iObj].Xo += 0.25*object[iObj].dPdX;
  return;
}

int spex1dgas (int argc, char **argv) {
  
  int i, j, iter;
  Vector *index1, *index2, *distance;
  float *XoList;
  int *IDList, *MidObj;

  MidObj = NULL;
  IDList = NULL;
  XoList = NULL;
 
  // srand48() is called by startup.c

  if (argc != 11) goto usage;

  // XXX enforce matching lengths on the three vectors
  if ((index1   = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((index2   = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((distance = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto escape;

  int Niter = atoi (argv[4]);
  int nCloseMax = atoi (argv[5]);
  int nCloseIter = atoi (argv[6]);
  float farFrac = atof (argv[7]);
  float maxPressure = atof (argv[8]);

  int idx1 = atoi(argv[9]);
  int idx2 = atoi(argv[10]);

  CastVector (index1, OPIHI_INT);
  CastVector (index2, OPIHI_INT);

  // how many objects do we have?
  Nobject = 0;
  for (i = 0; i < index1->Nelements; i++) {
    Nobject = MAX (Nobject, index1->elements.Int[i]);
    Nobject = MAX (Nobject, index2->elements.Int[i]);
  }
  Nobject ++;  // after the loop, Nobject has the value of the highest index, not the count

  // allocate the list of Object -- these list the possible friends of the given object[i] 
  ALLOCATE (object, Object, Nobject);
  for (i = 0; i < Nobject; i++) {
    ALLOCATE (object[i].index, int, Nobject);
    memset (object[i].index, 0, Nobject*sizeof(int));
    ALLOCATE (object[i].Dtgt, float, Nobject);
    memset (object[i].Dtgt, 0, Nobject*sizeof(float));
    ALLOCATE (object[i].Dcur, float, Nobject);
    memset (object[i].Dcur, 0, Nobject*sizeof(float));
    object[i].NINDEX = Nobject;
    object[i].Nindex = 0;
  }

  // generate the set of vectors of all distances for each entry
  // the object seq number (object[i]) will be used to match the index1 and index2 values
  for (i = 0; i < index1->Nelements; i++) {
    int N, m;

    N = index1->elements.Int[i];
    if (N >= Nobject) abort();
    m = object[N].Nindex;
    object[N].index[m] = index2->elements.Int[i];
    object[N].Dtgt[m] = distance->elements.Flt[i];
    object[N].Dcur[m] = 0.0;
    object[N].Nindex ++;
    if (object[N].Nindex == object[N].NINDEX) {
      object[N].NINDEX += 100;
      REALLOCATE (object[N].Dtgt, float, object[N].NINDEX);
      REALLOCATE (object[N].Dcur, float, object[N].NINDEX);
      REALLOCATE (object[N].index, int, object[N].NINDEX);
    }

    // Above we used the set (N,M,D_N,M) to assigned the distance of obj M to obj N.
    // Below we use that set to assign the distance of obj N to obj M.  IF we supply all
    // pairwise distances, then this duplicates all entries, so we need to skip that step
    // in such a case.

    // XX N = index2->elements.Int[i];
    // XX if (N >= Nobject) abort();
    // XX m = object[N].Nindex;
    // XX object[N].index[m] = index1->elements.Int[i];
    // XX object[N].Dtgt[m] = distance->elements.Flt[i];
    // XX object[N].Dcur[m] = 0.0;
    // XX object[N].Nindex ++;
    // XX if (object[N].Nindex == object[N].NINDEX) {
    // XX   object[N].NINDEX += 100;
    // XX   REALLOCATE (object[N].Dtgt, float, object[N].NINDEX);
    // XX   REALLOCATE (object[N].Dcur, float, object[N].NINDEX);
    // XX   REALLOCATE (object[N].index, int, object[N].NINDEX);
    // XX }
  }

  for (i = 0; i < Nobject; i++) {
    // sort so closest friends are first
    sortfriends (object[i].Dtgt, object[i].index, object[i].Nindex);
  }

  // find and save the max distance
  float Dmax = 0;
  for (i = 0; i < distance->Nelements; i++) {
    Dmax = MAX (Dmax, distance->elements.Flt[i]);
  }

  // place the objects at the initial guess locations
  // XXX let's try with just a simple grid dividing up the max range
  // int Ngrid = sqrt(Nobject);
  // float dgrid = 1.5 * Dmax / Ngrid; // XXX remove the fudge factor
  // XXX for (i = 0; i < Nobject; i++) {
  // XXX   // object[i].Xo = dgrid * (int) (i % Ngrid);
  // XXX   // object[i].Yo = dgrid * (int) (i / Ngrid);
  // XXX   object[i].Xo = Dmax*drand48();
  // XXX   object[i].Yo = Dmax*drand48();
  // XXX   object[i].dPdX = 0.0;
  // XXX   object[i].dPdY = 0.0;
  // XXX }

  // place the objects at the initial guess locations
  // use object 0 and its most distant friend to constrain:

  // we are going to use the projected X distance as the location in X, and ignore the Y
  // coordinate
  
  // need to get pretty close to a good starting point.  start with a guess using the first entry & its extremum
  // int idx1 = object[0].index[(int)(0.75*object[0].Nindex)];
  // int idx2 = object[idx1].index[(int)(0.75*object[idx1].Nindex)];
  float A = NAN;
  for (i = 0; isnan (A) && (i < object[idx2].Nindex); i++) {
      if (object[idx2].index[i] == idx1) {
	  A = object[idx2].Dtgt[i];
      }
  }
  fprintf (stderr, "idx1: %d, idx2: %d, A: %f\n", idx1, idx2, A);
  set_variable ("SP1D_A", A);
  object[idx1].Xo = 0.0;
  object[idx1].Yo = 0.0;
  object[idx1].dPdX = 0.0;
  object[idx1].dPdY = 0.0;

  object[idx2].Xo = A;
  object[idx2].Yo = 0.0;
  object[idx2].dPdX = 0.0;
  object[idx2].dPdY = 0.0;
		 
  Vector *tmpB = SelectVector ("sp1d_B", ANYVECTOR, TRUE); 
  Vector *tmpC = SelectVector ("sp1d_C", ANYVECTOR, TRUE); 
  ResetVector (tmpB, OPIHI_FLT, Nobject);
  ResetVector (tmpC, OPIHI_FLT, Nobject);

  for (i = 0; i < Nobject; i++) {
    if (i == idx1) continue;
    if (i == idx2) continue;
    float B = NAN;
    float C = NAN;
    for (j = 0; (isnan(B) || isnan(C)) && (j < object[i].Nindex); j++) {
      if (object[i].index[j] == idx1) { B = object[i].Dtgt[j]; }
      if (object[i].index[j] == idx2) { C = object[i].Dtgt[j]; }
    }
    if (isnan(B) || isnan(C)) abort();

    // C^2 = A^2 + B^2 - 2AB cos(t)
    // Xo = B cos(t)
    // Xo = A^2 + B^2 - C^2 / 2 A
    float Xo = (SQ(A) + SQ(B) - SQ(C)) / (2*A);

    object[i].Xo = Xo;
    object[i].dPdX = 0.0;
    object[i].Yo = 0.0;
    object[i].dPdY = 0.0;
    tmpB->elements.Flt[i] = B;
    tmpC->elements.Flt[i] = C;
  }
  tmpB->elements.Flt[idx1] = 0;
  tmpB->elements.Flt[idx2] = A;
  tmpC->elements.Flt[idx1] = A;
  tmpC->elements.Flt[idx2] = 0;

  if (Niter >= -1) {
    // now choose several in the middle of the range, and find the mean distance for each relative to that group
    ALLOCATE (XoList, float, Nobject);
    ALLOCATE (IDList, int, Nobject);
    for (i = 0; i < Nobject; i++) {
      XoList[i] = object[i].Xo;
      IDList[i] = i;
    }
    sortfriends (XoList, IDList, Nobject);
    float XoMid = XoList[(int)(Nobject/2)];

# define NMID 5
    ALLOCATE (MidObj, int, NMID);
    MidObj[0] = IDList[(int)(Nobject/2) + 0];
    MidObj[1] = IDList[(int)(Nobject/2) + 1];
    MidObj[2] = IDList[(int)(Nobject/2) - 1];
    MidObj[3] = IDList[(int)(Nobject/2) + 2];
    MidObj[4] = IDList[(int)(Nobject/2) - 2];

    for (i = 0; i < Nobject; i++) {
      for (j = 0; j < NMID; j++) {
	if (MidObj[j] == i) goto skip_object;
      }
      float Dmean = 0.0;
      for (j = 0; j < NMID; j++) {
	Dmean += object[i].Dtgt[MidObj[j]]; // desired distance between object (i) and object in middle section
      }
      Dmean = Dmean / NMID;
      if (object[i].Xo < XoMid) {
	object[i].Xo = XoMid - Dmean;
      } else {
	object[i].Xo = XoMid + Dmean;
      }
    skip_object:
      continue;
    }
  }

  if (Niter >= 0) {
    float XoMean = 0.0;
    // now choose one end of the range, and find the mean distance for each relative to THAT group
    for (i = 0; i < Nobject; i++) {
      XoList[i] = object[i].Xo;
      IDList[i] = i;
      XoMean += object[i].Xo;
    }
    XoMean = XoMean / Nobject;
    sortfriends (XoList, IDList, Nobject);
    float XoMin = XoList[0];

    MidObj[0] = IDList[0];
    MidObj[1] = IDList[1];
    MidObj[2] = IDList[2];
    MidObj[3] = IDList[3];
    MidObj[4] = IDList[4];

    for (i = 0; i < Nobject; i++) {
      for (j = 0; j < NMID; j++) {
	if (MidObj[j] == i) goto skip_object_p2;
      }
      float Dmean = 0.0;
      for (j = 0; j < NMID; j++) {
	Dmean += object[i].Dtgt[MidObj[j]]; // desired distance between object (i) and object in middle section
      }
      Dmean = Dmean / NMID;
      if (object[i].Xo < XoMin) {
	object[i].Xo = XoMin - Dmean;
      } else {
	object[i].Xo = XoMin + Dmean;
      }
    skip_object_p2:
      continue;
    }
  }

  for (iter = 0; iter < Niter; iter ++) {
    fprintf (stderr, "iter %d\n", iter);

    int nearNeighbors = (iter < nCloseIter);

    // save the result
    if (0) {
      char name[64];
      snprintf (name, 64, "output.%02d.dat", iter);
      FILE *output = fopen (name, "w");
      for (i = 0; i < Nobject; i++) {
	fprintf (output, "%f %f : %f %f\n", object[i].Xo, object[i].Yo, object[i].dPdX, object[i].dPdY);
      }
      fclose (output);
    }

    // measure (dP/dX) & move object
    for (i = 0; i < Nobject; i++) {
      get_pressure_gradient (i, nCloseMax, farFrac, nearNeighbors, maxPressure);
      move_object (i);
    }
  }

  // find the mean Xo, Yo positions for all objects
  float XoSum = 0.0;
  for (i = 0; i < Nobject; i++) {
    float dY2sum = 0.0;
    int Npts = 0;
    for (j = 0; (j < nCloseMax) && (j < object[i].Nindex); j++) {
      int idx = object[i].index[j];
      float dR2 = SQ(object[i].Dtgt[j]);
      float dX2 = SQ(object[i].Xo - object[idx].Xo);
      float dY2 = dR2 - dX2;
      dY2sum += dY2;
      Npts ++;
    }
    XoSum += object[i].Xo;
    object[i].Yo = (dY2sum > 0.0) ? sqrt(fabs(dY2sum / Npts)) : -sqrt(fabs(dY2sum / Npts));
  }
  float XoMean = XoSum / Nobject;

  // remove the mean Xo (uninteresting)
  for (i = 0; i < Nobject; i++) {
    object[i].Xo -= XoMean;
  }

  // save the result
  {
    Vector *outindex = SelectVector ("sp1d_idx", ANYVECTOR, TRUE); if (!outindex) goto escape;
    Vector *outXo    = SelectVector ("sp1d_Xo",  ANYVECTOR, TRUE); if (!outXo) goto escape;
    Vector *outYo    = SelectVector ("sp1d_Yo",  ANYVECTOR, TRUE); if (!outYo) goto escape;

    ResetVector (outindex, OPIHI_INT, Nobject);
    ResetVector (outXo, OPIHI_FLT, Nobject);
    ResetVector (outYo, OPIHI_FLT, Nobject);

    for (i = 0; i < Nobject; i++) {
      outindex->elements.Int[i] = i;
      outXo->elements.Flt[i] = object[i].Xo;
      outYo->elements.Flt[i] = object[i].Yo;
    }
  }

  return TRUE;
  
escape: 
  gprint (GP_ERR, "invalid vector\n");
  return FALSE;
  
usage:
  gprint (GP_ERR, "USAGE: spex2dgas (index1) (index2) (distance) (Niter) (nCloseMax) (nCloseIter) (farFrac) (maxPressure) (idx1) (idx2)\n");
  return FALSE;
}

/* this function takes 3 vectors: a set of distances and a pair of indicies identifying the end points.  
   
   the goal is to generate groups of the end points that are relatively closer.

   note that the distances may not be Euclidean: nothing can be assumed about relationships between d(1,2), d(2,3), and d(1,3)

   the indicies should be sequential.  the algorithm does not require it, but storage is much cleaner if it is

*/
