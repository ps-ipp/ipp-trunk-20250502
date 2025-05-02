# include "astro.h"

/* We have N objects with N(N-1)/2 paired distances.  We want to find the 2D distribution that
   best describes the observed distances.  Assume the distances are in a 2D space, though in
   principle it could be N-D or even non-euclidean.

   Model the points as a gas under pressure.  

   Start with the a guess for positions of the objects at some locations in 2D

   P = (d_now - d_tru)

   Iterate over the points and move in X,Y

   Find (dP/dX,dP/dY) for modest moves in X,Y

   Move based on dP

   USAGE: spex2dgas incdex1 index2 distance

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
  float dPdY = 0.0;

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
    float dY = object[jObj].Yo - object[iObj].Yo;
    float Dcur = hypot(dX,dY);

    // the force law as a function of (Dcur - Dtgt) : if Dcur is too large, dF is negative
    // float dF = (Dcur < 0.01*Dtgt) ? -100.0 : (Dcur - Dtgt) / Dcur; XXX modified spring constant : too crazy
    float dF = (Dcur - Dtgt);
    dF = MIN (maxPressure, MAX (-maxPressure, dF));

    float dPdXi, dPdYi;
    if (fabs(Dcur) < 1e-6) {
      dPdXi = 0.0;
      dPdYi = 0.0;
    } else {
      dPdXi = dF * dX / Dcur;
      dPdYi = dF * dY / Dcur;
    }

    if (isnan(Dtgt) || isnan(dX) || isnan(dY) || isnan(Dcur) || isnan(dF) || isnan(dPdXi) || isnan(dPdYi)) abort();

    // if we are too close, then dX/Dcur is too ill-defined, just jump away by 5% of Dtgt
    if (Dcur < 0.01*Dtgt) {
      dPdXi = (drand48() - 0.5)*0.1*Dtgt;
      dPdYi = (drand48() - 0.5)*0.1*Dtgt;
    }

    if (i >= nCloseMax) {
      fprintf (stderr, "Dcur,Dtgt : %f %f : dX,dY,dP : %f %f : %f : %f %f\n", Dcur, Dtgt, dX, dY, dF, dPdXi, dPdYi);
    }
    dPdX += dPdXi;
    dPdY += dPdYi;
  }
  object[iObj].dPdX = dPdX;
  object[iObj].dPdY = dPdY;

  return;
}

static void move_object (int iObj) {

  object[iObj].Xo += 0.25*object[iObj].dPdX;
  object[iObj].Yo += 0.25*object[iObj].dPdY;
  return;
}

int spex2dgas (int argc, char **argv) {
  
  int i, j, iter, IoMax;
  Vector *index1, *index2, *distance;
  float XoMax, YoMax;

  // srand48() is called by startup.c

  if (argc != 9) goto usage;

  if ((index1   = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((index2   = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((distance = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto escape;
  int Niter = atoi (argv[4]);
  int nCloseMax = atoi (argv[5]);
  int nCloseIter = atoi (argv[6]);
  float farFrac = atof (argv[7]);
  float maxPressure = atof (argv[8]);
  // XXX enforce matching lengths on the three vectors

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

    N = index2->elements.Int[i];
    if (N >= Nobject) abort();
    m = object[N].Nindex;
    object[N].index[m] = index1->elements.Int[i];
    object[N].Dtgt[m] = distance->elements.Flt[i];
    object[N].Dcur[m] = 0.0;
    object[N].Nindex ++;
    if (object[N].Nindex == object[N].NINDEX) {
      object[N].NINDEX += 100;
      REALLOCATE (object[N].Dtgt, float, object[N].NINDEX);
      REALLOCATE (object[N].Dcur, float, object[N].NINDEX);
      REALLOCATE (object[N].index, int, object[N].NINDEX);
    }
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
  
  int idx1 = 0;
  int idx2 = object[idx1].index[object[idx1].Nindex-1];
  float A = object[idx1].Dtgt[object[idx1].Nindex-1];
  object[idx1].Xo = 0.0;
  object[idx1].Yo = 0.0;
  object[idx1].dPdX = 0.0;
  object[idx1].dPdY = 0.0;

  object[idx2].Xo = A;
  object[idx2].Yo = 0.0;
  object[idx2].dPdX = 0.0;
  object[idx2].dPdY = 0.0;
		 
  // choose the correct Y side by comparing to the distance from the most deviant
  YoMax = 0;
  XoMax = 0;
  IoMax = 0;

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

    float Xo = (SQ(A) + SQ(B) - SQ(C)) / (2*A);
    float Y2 = SQ(B) - SQ(Xo);

    float Yo = (Y2 < 0) ? 0.0 : sqrt(Y2);
    if (isnan(Yo)) abort();

    object[i].Xo = Xo;
    object[i].Yo = Yo;
    object[i].dPdX = 0.0;
    object[i].dPdY = 0.0;
    if (Yo > YoMax) {
      YoMax = Yo;
      XoMax = Xo;
      IoMax = i;
    }
  }

  for (i = 0; i < Nobject; i++) {
    if (i == idx1) continue;
    if (i == idx2) continue;
    if (i == IoMax) continue;
    for (j = 0; j < object[i].Nindex; j++) {
      if (object[i].index[j] == IoMax) { 
	float d02 = SQ(object[i].Dtgt[j]);
	float dX  = XoMax - object[i].Xo;
	float dY1 = YoMax - object[i].Yo;
	float dY2 = YoMax + object[i].Yo;
	float d12 = SQ(dX) + SQ(dY1);
	float d22 = SQ(dX) + SQ(dY2);
	if (fabs(d12 - d02) > fabs(d22 - d02)) {
	  object[i].Yo = -object[i].Yo;
	  break;
	}
      }
    }
  }

  for (iter = 0; iter < Niter; iter ++) {
    fprintf (stderr, "iter %d\n", iter);

    int nearNeighbors = (iter < nCloseIter);

    // save the result
    char name[64];
    snprintf (name, 64, "output.%02d.dat", iter);
    FILE *output = fopen (name, "w");
    for (i = 0; i < Nobject; i++) {
      fprintf (output, "%f %f : %f %f\n", object[i].Xo, object[i].Yo, object[i].dPdX, object[i].dPdY);
    }
    fclose (output);

    // measure (dP/dX),(dP/dY) for all objects
    for (i = 0; i < Nobject; i++) {
      get_pressure_gradient (i, nCloseMax, farFrac, nearNeighbors, maxPressure);
      move_object (i);
    }

    // given (dP/dX),(dP/dY), move each object 
    // for (i = 0; i < Nobject; i++) {
    // }
  }

  // save the result
  FILE *output = fopen ("output.dat", "w");
  for (i = 0; i < Nobject; i++) {
    fprintf (output, "%f %f : %f %f\n", object[i].Xo, object[i].Yo, object[i].dPdX, object[i].dPdY);
  }
  fclose (output);

  return TRUE;
  
escape: 
  gprint (GP_ERR, "invalid vector\n");
  return FALSE;
  
usage:
  gprint (GP_ERR, "USAGE: spex2dgas (index1) (index2) (distance) (Niter) (nCloseMax) (nCloseIter) (farFrac) (maxPressure)\n");
  return FALSE;
}

/* this function takes 3 vectors: a set of distances and a pair of indicies identifying the end points.  
   
   the goal is to generate groups of the end points that are relatively closer.

   note that the distances may not be Euclidean: nothing can be assumed about relationships between d(1,2), d(2,3), and d(1,3)

   the indicies should be sequential.  the algorithm does not require it, but storage is much cleaner if it is

*/
