# include "astro.h"

/* We have N objects with N(N-1)/2 paired chisq.  We want to find the 1D sequence / index that
   best describes the observed distances.  

   A sequence is best described by the distances from objects which are only modestly close.
   If we get too close to the reference, we (a) hit a non-euclidean (or N-D) minimum (A-B ~ A-C
   ~ B-C) and (b) the positive and negative sides of the sequence are ambiguous.  If we get too
   far away, the distances are all either equivalent or less consistent

   1) find the dynamic range of distances

   2) choose object (1) and select a friend (2) > a fixed fraction of the full dynamic range (say,
   10-20%)

   3) all nearby friends of (1) (D < f MaxDist) have sequences assigned based on the distance
   to (2).  center based on the distance (1)-(2) ie, (1) gets a sequence value of 0.0

   4) choose another object (1') and repeat [need to choose (2') on the same side as (2)?]

   ** reconciliation is a bit tricky
   ** edge effects imply that we prefer to start in the center
   ** is this well-defined? 

   */

typedef struct {
  int Nindex;   // number of tested relationships
  int NINDEX;   // number of allocated relationships
  int *index;  // index for distances to other objects: object[i].Dtgt[j] is distance from object[i] to object[idx] where idx = object[i].index[j] (sorted by Dtgt)
  int *rindex;  // reverse index for Dtgt: distance from object[i] to object[idx] is object[i].Dtgt[j] where object[i].rindex[idx] = j
  int shifted; // has this object already had the sequence adjusted?
  float *Dtgt; // target distance for this relationship
  float *seq;  // sequence value for this friend
  float So, dSo;
  float Xo, dXo;
  float Yo, dYo;
  float Srange;
  int nSeq;
} Object;

void sortfriends (float *X, int *IDX1, int N);

void get_sequence (Object *object, int Nobject, int idx1, float Dmax, float f1, float f2, int pin1, int pin2) {
  OHANA_UNUSED_PARAM(pin1);
  OHANA_UNUSED_PARAM(pin2);

  int i, j, N;

  // object (1) is provided above; choose object (2)
  int idx2 = -1;
  float A = NAN;
  for (i = 0; (idx2 == -1) && (i < object[idx1].Nindex); i++) {
    if (object[idx1].Dtgt[i] > f1*Dmax) {
      idx2 = object[idx1].index[i];
    }
    A = object[idx1].Dtgt[i];
  }
  // fprintf (stderr, "idx1: %d, idx2: %d, A: %f\n", idx1, idx2, A);
  // set_variable ("SP1D_A", A);

  // is idx2 closer to pin1 or pin2?
  // XX N = object[idx2].rindex[pin1];
  // XX float D1 = object[idx2].Dtgt[N];
  // XX N = object[idx2].rindex[pin2];
  // XX float D2 = object[idx2].Dtgt[N];
  // XX int parity = (D1 < D2) ? +1 : -1;

  int Nfriends = 0;
  for (i = 0; (object[idx1].Dtgt[i] < f2*Dmax) && (i < object[idx1].Nindex); i++) {
    // what is the distance from this friend to object (2)?
    N = object[idx1].index[i];
    j = object[idx2].rindex[N];
    assert (object[idx2].index[j] == N);
    // XX object[idx1].seq[i] = parity*(object[idx2].Dtgt[j] - A);
    object[idx1].seq[i] = (object[idx2].Dtgt[j] - A);
    Nfriends ++;
  }
  assert (Nfriends <= object[idx1].Nindex);
  assert (Nfriends <= Nobject);
  fprintf (stderr, "object %d sequenced by object %d, %d friends sequenced\n", idx1, idx2, Nfriends);
  return;
}

int spexseq (int argc, char **argv) {
  
  int i, j, k;
  Vector *index1, *index2, *distance;
  Object *object = NULL;
  int Nobject = 0;

  if (argc != 6) goto usage;

  // srand48() is called by startup.c

  // XXX enforce matching lengths on the three vectors
  if ((index1   = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((index2   = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((distance = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto escape;

  float f1 = atof (argv[4]);
  float f2 = atof (argv[5]);

  // int idx1 = atoi(argv[9]);
  // int idx2 = atoi(argv[10]);

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
    object[N].Nindex ++;
    if (object[N].Nindex == object[N].NINDEX) {
      object[N].NINDEX += 100;
      REALLOCATE (object[N].Dtgt, float, object[N].NINDEX);
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
    // XX object[N].Nindex ++;
    // XX if (object[N].Nindex == object[N].NINDEX) {
    // XX   object[N].NINDEX += 100;
    // XX   REALLOCATE (object[N].Dtgt, float, object[N].NINDEX);
    // XX   REALLOCATE (object[N].index, int, object[N].NINDEX);
    // XX }
  }

  for (i = 0; i < Nobject; i++) {
    // sort so closest friends are first
    sortfriends (object[i].Dtgt, object[i].index, object[i].Nindex);
    object[i].shifted = FALSE;
    object[i].So = NAN;
    object[i].dSo = NAN;
    object[i].Xo = NAN;
    object[i].dXo = NAN;
    // generate the reverse index
    REALLOCATE (object[i].rindex, int, object[i].Nindex);
    REALLOCATE (object[i].seq, float, object[i].Nindex);
    for (j = 0; j < object[i].Nindex; j++) {
      int N = object[i].index[j];
      object[i].rindex[N] = j;
      object[i].seq[j] = NAN;
    }
  }

  // find and save the max distance
  float Dmax = 0;
  for (i = 0; i < distance->Nelements; i++) {
    Dmax = MAX (Dmax, distance->elements.Flt[i]);
  }

  // find two objects at the opposite extremes
  float Dm1 = object[0].Dtgt[object[0].Nindex - 1];
  for (i = 0; (object[0].Dtgt[i] < 0.75*Dm1) && (i < object[0].Nindex); i++);
  int pin1 = object[0].index[i];
  assert (pin1 >= 0);
  assert (pin1 < Nobject);

  float Dm2 = object[pin1].Dtgt[object[pin1].Nindex - 1];
  for (i = 0; (object[pin1].Dtgt[i] < 0.75*Dm2) && (i < object[pin1].Nindex); i++);
  int pin2 = object[pin1].index[i];
  assert (pin2 >= 0);
  assert (pin2 < Nobject);

  // generate sequences for the near friends of all objects based on a somewhat distant friend
  for (i = 0; i < Nobject; i++) {
    get_sequence (object, Nobject, i, Dmax, f1, f2, pin1, pin2);
  }

  // now we need to reconcile these sequences.
  // start with object 0, find shift of all friends relative to 0
  object[0].shifted = TRUE;
  for (i = 0; i < Nobject; i++) {

    // don't use unreconciled objects as a reference (we'll catch them eventually)
    if (!object[i].shifted) continue;

    int Nfixed = 0;

    // shift all of the close friends of object (i) wrt object (i)
    for (j = 0; (j < object[i].Nindex) && (object[i].Dtgt[j] < f2*Dmax); j++) {

      // loop over the sequence for object Nj
      int Nj = object[i].index[j];

      // only reconcile once
      if (object[Nj].shifted == TRUE) continue;

      float dS1p = 0.0;
      float dS2p = 0.0;
      float dS1m = 0.0;
      float dS2m = 0.0;
      int nS = 0;

      float Smin = +1000.0;
      float Smax = -1000.0;

      for (k = 0; k < object[Nj].Nindex; k++) {
	int Nsj = object[Nj].index[k];
	int Nsi = object[i].rindex[Nsj];
	
	if (isnan(object[Nj].seq[k])) continue;
	if (isnan(object[i].seq[Nsi])) continue;
	
	Smin = MIN(object[Nj].seq[k], Smin);
	Smax = MAX(object[Nj].seq[k], Smax);

	float dSp = object[Nj].seq[k] - object[i].seq[Nsi];
	float dSm = -object[Nj].seq[k] - object[i].seq[Nsi];
	dS1p += dSp;
	dS2p += dSp*dSp;
	dS1m += dSm;
	dS2m += dSm*dSm;
	nS ++;
      }
      float Sp = dS1p / nS;
      float Sm = dS1m / nS;
      float dSp = sqrt(dS2p / nS - Sp*Sp);
      float dSm = sqrt(dS2m / nS - Sm*Sm);
      fprintf (stderr, "%d: %f +/- %f or %f +/- %f : %f range, %d matches\n", Nj, Sp, dSp, Sm, dSm, Smax - Smin, nS);

      if (dSm > dSp) {
	// parity is always 1 if we use the pins (but it does not seem to work)
	for (k = 0; k < object[Nj].Nindex; k++) {
	  if (isnan(object[Nj].seq[k])) continue;
	  object[Nj].seq[k] -= Sp;
	}
	object[Nj].So = Sp;
	object[Nj].dSo = dSp;
      } else {
	// flip the sequence and offset
	for (k = 0; k < object[Nj].Nindex; k++) {
	  if (isnan(object[Nj].seq[k])) continue;
	  object[Nj].seq[k] = -object[Nj].seq[k] - Sm; 
	}
	object[Nj].So = Sm;
	object[Nj].dSo = dSm;
      }
      object[Nj].Srange = Smax - Smin;
      object[Nj].nSeq = nS;
      object[Nj].shifted = TRUE;
      Nfixed ++;
    }
    // fprintf (stderr, "%d used to reconcile %d friends\n", i, Nfixed);
  }

  int Nfixed = 0;
  int Nunfix = 0;
  for (i = 0; i < Nobject; i++) {
    if (object[i].shifted) { 
      Nfixed ++;
    } else {
      Nunfix ++;
    }
  }
  fprintf (stderr, "%d fixed, %d unfixed\n", Nfixed, Nunfix);
      

  // calculate the mean Xo for each object using the values of the sequences calculated
  for (i = 0; i < Nobject; i++) {

    if (!object[i].shifted) {
      fprintf (stderr, "this object was not reconciled\n");
    }
    float XoSum = 0.0;
    float XoS2 = 0.0;
    int XoNum = 0;

    // find all the measurements of this object's sequence (from its friends values)
    for (j = 0; j < Nobject; j++) {
      int Ns = object[j].rindex[i];
      if (isnan(object[j].seq[Ns])) continue;
      XoSum += object[j].seq[Ns];
      XoS2  += SQ(object[j].seq[Ns]);
      XoNum ++;
    }
    float Xo = XoSum / XoNum;
    float dXo = sqrt(XoS2 / XoNum - Xo*Xo);
    object[i].Xo = Xo;
    object[i].dXo = dXo;
  }

  // XXX seq is effectively Xo, calculate Yo for the objects based on the local 
  // objects

  // save the result
  {
    Vector *outindex = SelectVector ("sp1d_idx", ANYVECTOR, TRUE); if (!outindex) goto escape;
    Vector *outXo    = SelectVector ("sp1d_Xo",  ANYVECTOR, TRUE); if (!outXo) goto escape;
    Vector *outYo    = SelectVector ("sp1d_Yo",  ANYVECTOR, TRUE); if (!outYo) goto escape;

    Vector *outSo    = SelectVector ("sp1d_So",  ANYVECTOR, TRUE); if (!outXo) goto escape;
    Vector *outdSo   = SelectVector ("sp1d_dSo", ANYVECTOR, TRUE); if (!outYo) goto escape;
    Vector *outdXo   = SelectVector ("sp1d_dXo", ANYVECTOR, TRUE); if (!outXo) goto escape;

    Vector *outSr   = SelectVector ("sp1d_Sr", ANYVECTOR, TRUE); if (!outSr) goto escape;
    Vector *outSn   = SelectVector ("sp1d_nS", ANYVECTOR, TRUE); if (!outSn) goto escape;

    ResetVector (outindex, OPIHI_INT, Nobject);
    ResetVector (outXo, OPIHI_FLT, Nobject);
    ResetVector (outYo, OPIHI_FLT, Nobject);

    ResetVector (outSo, OPIHI_FLT, Nobject);
    ResetVector (outdSo, OPIHI_FLT, Nobject);
    ResetVector (outdXo, OPIHI_FLT, Nobject);

    ResetVector (outSr, OPIHI_FLT, Nobject);
    ResetVector (outSn, OPIHI_INT, Nobject);

    for (i = 0; i < Nobject; i++) {
      outindex->elements.Int[i] = i;
      outXo->elements.Flt[i] = object[i].Xo;
      outYo->elements.Flt[i] = object[i].Yo;
      outSo->elements.Flt[i] = object[i].So;
      outdSo->elements.Flt[i] = object[i].dSo;
      outdXo->elements.Flt[i] = object[i].dXo;
      outSr->elements.Flt[i] = object[i].Srange;
      outSn->elements.Int[i] = object[i].nSeq;
    }
  }

  return TRUE;
  
escape: 
  gprint (GP_ERR, "invalid vector\n");
  return FALSE;
  
usage:
  gprint (GP_ERR, "USAGE: spexseq (index1) (index2) (distance) (Niter) (nCloseMax) (nCloseIter) (farFrac) (maxPressure) (idx1) (idx2)\n");
  return FALSE;
}

/* this function takes 3 vectors: a set of distances and a pair of indicies identifying the end points.  
   
   the goal is to generate groups of the end points that are relatively closer.

   note that the distances may not be Euclidean: nothing can be assumed about relationships between d(1,2), d(2,3), and d(1,3)

   the indicies should be sequential.  the algorithm does not require it, but storage is much cleaner if it is

*/
