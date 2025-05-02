# include "dvoshell.h"

mySequenceType *mySequenceAlloc () {

  mySequenceType *mySequence;
  ALLOCATE (mySequence, mySequenceType, 1);
  ALLOCATE (mySequence->value,    int, 1);
  ALLOCATE (mySequence->sequence, int, 1);

  // an uninit'ed index is invalid
  mySequence->Nsequence = 0;
  mySequence->NSEQUENCE = 1;

  return mySequence;
}

int mySequenceFree (mySequenceType *mySequence) {

  free (mySequence->value);
  free (mySequence->sequence);
  free (mySequence);
  return TRUE;
}

// set minID and maxID externally
int mySequenceSetSize (mySequenceType *mySequence, int Nmax) {

  mySequence->Nsequence = Nmax;
  if (mySequence->Nsequence > mySequence->NSEQUENCE) {
    mySequence->NSEQUENCE = mySequence->Nsequence;
    REALLOCATE (mySequence->value,    int, mySequence->NSEQUENCE);
    REALLOCATE (mySequence->sequence, int, mySequence->NSEQUENCE);
  }
  return TRUE;
}

int mySequenceSetValue (mySequenceType *mySequence, int value, int entry) {

  mySequence->value[entry] = value;
  mySequence->sequence[entry] = entry;
  return TRUE;
}

int mySequenceSort (mySequenceType *mySequence) {

  isortpair (mySequence->value, mySequence->sequence, mySequence->Nsequence);
  return TRUE;
}

// find the entry by bisection:
int mySequenceGetEntry (mySequenceType *mySequence, int value) {

  int Nlo = 0; 
  int Nhi = mySequence->Nsequence - 1;

  if (value < mySequence->value[Nlo]) return -1;
  if (value > mySequence->value[Nhi]) return -1;

  int N;
  while (Nhi - Nlo > 4) {
    N = 0.5*(Nlo + Nhi);
    if (mySequence->value[N] < value) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, mySequence->Nsequence - 1);
    }
  }
  // mySequence->value[Nlo] <  value 
  // mySequence->value[Nhi] >= value

  for (N = Nlo; N <= Nhi; N++) {
    if (mySequence->value[N] == value) {
      return mySequence->sequence[N];
    }
  }
  return -1;
}

