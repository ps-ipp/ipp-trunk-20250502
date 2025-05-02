# include "relphot.h"

myIndexType *myIndexInit () {

  myIndexType *myIndex;
  ALLOCATE (myIndex, myIndexType, 1);

  myIndex->minID = MAX_INT;
  myIndex->maxID = 0;

  myIndex->NINDEX = 1000;
  myIndex->Nindex = 0;

  ALLOCATE (myIndex->index, int, myIndex->NINDEX);

  return myIndex;
}

int myIndexFree (myIndexType *myIndex) {

  if (!myIndex) return TRUE;
  free (myIndex->index);
  free (myIndex);
  return TRUE;
}

// set minID and maxID externally
int myIndexUpdateLimits (myIndexType *myIndex, int value) {

  myIndex->maxID = MAX(value, myIndex->maxID);
  myIndex->minID = MIN(value, myIndex->minID);

  return TRUE;
}

// once minID and maxID are set, this function defines the range and inits
int myIndexSetRange (myIndexType *myIndex) {

  myIndex->Nindex = myIndex->maxID - myIndex->minID + 1;
  myIndex->NINDEX = myIndex->Nindex;
  
  REALLOCATE (myIndex->index, int, myIndex->NINDEX);

  int i;
  for (i = 0; i < myIndex->Nindex; i++) {
    myIndex->index[i] = -1;
  }

  return TRUE;
}

// once minID and maxID are set, this function defines the range and inits
int myIndexSetEntry (myIndexType *myIndex, int value, int entry) {

  int n = value - myIndex->minID;

  myAssert (n >= 0, "impossible!");

  if (myIndex->index[n] != -1) {
    fprintf (stderr, "skipping duplicate ID value: %d\n", value);
    return -1;
  }

  myIndex->index[n] = entry;
  return n;
}

// once minID and maxID are set, this function defines the range and inits
int myIndexGetEntry (myIndexType *myIndex, int value) {

  if (value < myIndex->minID) return -1; // not in this index
  if (value > myIndex->maxID) return -1; // not in this index

  int n = value - myIndex->minID;

  return myIndex->index[n];
}
