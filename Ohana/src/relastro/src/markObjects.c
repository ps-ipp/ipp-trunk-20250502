# include "relastro.h"

int markObjects (Catalog *catalog, int Ncatalog) {

  int i, n;
  off_t j;

  // How strongly do I own this object?
  for (i = 0; i < Ncatalog; i++) {
    ALLOCATE (catalog[i].nOwn_t, int, catalog[i].Naverage);
    memset (catalog[i].nOwn_t, 0, catalog[i].Naverage*sizeof(int));
    for (j = 0; j < catalog[i].Naverage; j++) {
      int nOwn = 0;
      int m = catalog[i].average[j].measureOffset;
      for (n = 0; n < catalog[i].average[j].Nmeasure; n++) {
	// this is a detection I own (it is on one of my images)
	if (catalog[i].measureT[m+n].myDet) {
	  nOwn ++;
	  continue;
	}
	// 2MASS detections are counted as 'owned' (I completely control this object is nOwn_t == Nmeasure; no sharing needed)
	if ((catalog[i].measureT[m+n].photcode == 2011) || 
	    (catalog[i].measureT[m+n].photcode == 2012) || 
	    (catalog[i].measureT[m+n].photcode == 2013)) {
	  nOwn ++;
	  continue;
	}
      }
      catalog[i].nOwn_t[j] = nOwn;
    }
  }
  return TRUE;
}

