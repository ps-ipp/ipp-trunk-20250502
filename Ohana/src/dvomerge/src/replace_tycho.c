# include "dvomerge.h"

static short TYCHO_B;
static short TYCHO_V;

void replace_tycho_init () {
  
  TYCHO_B = GetPhotcodeCodebyName ("TYCHO_B"); myAssert (TYCHO_B, "TYCHO_B photcode not found\n");
  TYCHO_V = GetPhotcodeCodebyName ("TYCHO_V"); myAssert (TYCHO_V, "TYCHO_V photcode not found\n");
}

int replace_tycho (Average *averageOut, Measure *measureOut, off_t *next_meas, Average *averageInp, Measure *measureInp) {
  OHANA_UNUSED_PARAM(averageInp);

  int i;

  int Nindex = 0;
  int index[6];

  off_t m = averageOut->measureOffset;

  // find all tycho measurements
  for (i = 0; (i < averageOut->Nmeasure) && (Nindex < 6); i++) {

    myAssert (m > -1, "oops");

    int valid = FALSE;
    valid = valid || (measureOut[m].photcode == TYCHO_B);
    valid = valid || (measureOut[m].photcode == TYCHO_V);
    if (valid) { 
      index[Nindex] = m;
      Nindex ++;
    }
    m = next_meas[m];
  }

  if ((Nindex != 0) && (Nindex != 6)) {
    fprintf (stderr, "unexpected number of tycho measurements: (Nindex = %d)\n", Nindex);
  }

  for (i = 0; i < Nindex; i++) {
    int Nm = index[i];

    int averef = measureOut[Nm].averef;
    int objID  = measureOut[Nm].objID;
    int catID  = measureOut[Nm].catID;

    measureOut[Nm] = measureInp[i];

    measureOut[Nm].averef = averef;
    measureOut[Nm].objID  = objID;
    measureOut[Nm].catID  = catID;
  }

  return Nindex;
}
