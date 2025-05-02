# include "relphot.h"

// generate an index of stack image ra,dec centers sorted indexes to the image IDs

# define D_NSTACKS 1000
# define NGROUPS 5

typedef struct {
  unsigned short photcode;
  double *Rcenter;
  double *Dcenter;
  int    *imageID;
  int    *imageSeq;
  int Nstacks;
  int NSTACKS;
} StackGroupType;

// one group for each photcode
StackGroupType *stackgroup = NULL;

int MakeStackIndex (void) {

  off_t Nimage;
  Image *image = getimages (&Nimage, NULL);

  ALLOCATE (stackgroup, StackGroupType, NGROUPS);

  for (int i = 0; i < NGROUPS; i++) {
    stackgroup[i].photcode = 11000 + i*100;

    stackgroup[i].Nstacks = 0;
    stackgroup[i].NSTACKS = D_NSTACKS;
    ALLOCATE (stackgroup[i].Rcenter,  double, stackgroup[i].NSTACKS);
    ALLOCATE (stackgroup[i].Dcenter,  double, stackgroup[i].NSTACKS);
    ALLOCATE (stackgroup[i].imageID,  int,    stackgroup[i].NSTACKS);
    ALLOCATE (stackgroup[i].imageSeq, int,    stackgroup[i].NSTACKS);
  }

  // first generate a list of obstime values
  for (off_t i = 0; i < Nimage; i++) {
    if (!isGPC1stack(image[i].photcode)) continue;

    int Ng = -1;
    switch (image[i].photcode) {
      case 11000: Ng = 0; break;
      case 11100: Ng = 1; break;
      case 11200: Ng = 2; break;
      case 11300: Ng = 3; break;
      case 11400: Ng = 4; break;
      default: myAbort("impossible photcode");
    }
    myAssert (Ng < NGROUPS, "oops");

    int N = stackgroup[Ng].Nstacks;
    XY_to_RD (&stackgroup[Ng].Rcenter[N], &stackgroup[Ng].Dcenter[N], 0.5*image[i].NX, 0.5*image[i].NY, &image[i].coords);
    stackgroup[Ng].Rcenter[N] = ohana_normalize_angle (stackgroup[Ng].Rcenter[N]);

    stackgroup[Ng].imageSeq[N] = i;
    stackgroup[Ng].imageID[N] = image[i].imageID;

    stackgroup[Ng].Nstacks ++;

    if (stackgroup[Ng].Nstacks >= stackgroup[Ng].NSTACKS) {
      stackgroup[Ng].NSTACKS += D_NSTACKS;
      REALLOCATE (stackgroup[Ng].Rcenter,  double, stackgroup[Ng].NSTACKS);
      REALLOCATE (stackgroup[Ng].Dcenter,  double, stackgroup[Ng].NSTACKS);
      REALLOCATE (stackgroup[Ng].imageID,  int,    stackgroup[Ng].NSTACKS);
      REALLOCATE (stackgroup[Ng].imageSeq, int,    stackgroup[Ng].NSTACKS);
    }
  }

  // sort the list:
  for (int i = 0; i < NGROUPS; i++) { 
    sort_by_ra (stackgroup[i].Rcenter, stackgroup[i].Dcenter, stackgroup[i].imageID, stackgroup[i].imageSeq, stackgroup[i].Nstacks);
  }

  return TRUE;
}

// find the stack which yields the given Rstk, Dstk for the given X,Y (and photcode)
int GetStackSeq (Image *image, double Rstk, double Dstk, unsigned short photcode, float X, float Y) {

  if (!stackgroup) return -1;

  // we have the stack centers; find all stacks within 0.3 degrees of this point
  Rstk = ohana_normalize_angle (Rstk);
  double dD = 0.3;
  double dR = dD / cos(RAD_DEG*Dstk);
  double Rmin = Rstk - dR;
  double Rmax = Rstk + dR;
  double Dmin = Dstk - dD;
  double Dmax = Dstk + dD;

  int Ng = (photcode / 100) % 10;
  int N = ohana_bisection_double (stackgroup[Ng].Rcenter, stackgroup[Ng].Nstacks, Rmin);

  double dPosMin = NAN;
  int    nPosMin = -1;

  for (; N < stackgroup[Ng].Nstacks; N++) {
    if (stackgroup[Ng].Dcenter[N] < Dmin) continue;
    if (stackgroup[Ng].Dcenter[N] > Dmax) continue;
    if (stackgroup[Ng].Rcenter[N] > Rmax) break;
    
    int im = stackgroup[Ng].imageSeq[N];

    // project to image pixels
    double Xtst, Ytst;
    RD_to_XY (&Xtst, &Ytst, Rstk, Dstk, &image[im].coords);
    
    // find the pixel offset
    double dX = (Xtst - X);
    double dY = (Ytst - Y);
    
    // if dPos is small, we have the right image
    double dPos = hypot(dX,dY);
    if (dPos < 10.0) return im; // 10 pixels == 2.0 arcsec

    // check for multiple possible matches??

    // record the min pos so we can see how things fail
    if (isnan(dPosMin)) {
      dPosMin = dPos;
      nPosMin = im;
    } else {
      if (dPos < dPosMin) {
	dPosMin = dPos;
	nPosMin = im;
      }
    }
  }
  
  // we did not find the right image, return what we found (-1 or closest match)
  return nPosMin;
}

void FreeStackGroups (void) {

  if (!stackgroup) return;

  for (int i = 0; i < NGROUPS; i++) {
    free (stackgroup[i].Rcenter);
    free (stackgroup[i].Dcenter);
    free (stackgroup[i].imageID);
    free (stackgroup[i].imageSeq);
  }    
  free (stackgroup);
}

void sort_by_ra (double *R, double *D, int *I, int *S, int N) {

# define SWAPFUNC(A,B){ double dtmp; int itmp; 	\
    dtmp = R[A]; R[A] = R[B]; R[B] = dtmp;	\
    dtmp = D[A]; D[A] = D[B]; D[B] = dtmp;	\
    itmp = I[A]; I[A] = I[B]; I[B] = itmp;	\
    itmp = S[A]; S[A] = S[B]; S[B] = itmp;	\
  }
# define COMPARE(A,B)(R[A] < R[B])
  
  OHANA_SORT (N, COMPARE, SWAPFUNC);
  
# undef SWAPFUNC
# undef COMPARE
  
}
