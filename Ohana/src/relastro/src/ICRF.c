# include "relastro.h"

int Nicrf = 0;
int NICRF = 0;
int *ICRFtoCatalog = NULL;
int *ICRFtoAverage = NULL;
int *ICRFtoMeasure = NULL;

void ICRFinit () {

  Nicrf = 0;
  NICRF = 500;
  ALLOCATE (ICRFtoCatalog, int, NICRF);
  ALLOCATE (ICRFtoAverage, int, NICRF);
  ALLOCATE (ICRFtoMeasure, int, NICRF);
}

int ICRFsave (int cat, int ave, int meas) {
  
  // fprintf (stderr, "ICRF: %d %d %d\n", cat, ave, meas);

  ICRFtoCatalog[Nicrf] = cat;
  ICRFtoAverage[Nicrf] = ave;
  ICRFtoMeasure[Nicrf] = meas;

  Nicrf ++;
  if (Nicrf == NICRF) {
    NICRF += 500;

    REALLOCATE (ICRFtoCatalog, int, NICRF);
    REALLOCATE (ICRFtoAverage, int, NICRF);
    REALLOCATE (ICRFtoMeasure, int, NICRF);
  }
  return TRUE;
}

int ICRFmax () {
  return Nicrf;
}

int ICRFdata (int n, int *cat, int *ave, int *meas) {

  myAssert (n >= 0 && n < Nicrf, "out of range");

  *cat  = ICRFtoCatalog[n];
  *ave  = ICRFtoAverage[n];
  *meas = ICRFtoMeasure[n];
  return TRUE;
}

int select_catalog_ICRF (Catalog *catalog, int Ncatalog) {

  if (!USE_ICRF_CORRECT) return TRUE;

  int N = 0;

  char filename[1024];
  snprintf_nowarn (filename, 1024, "%s/test.icrf.dat", CATDIR);
  FILE *f = fopen (filename, "w");

  int c, i, j;
  for (c = 0; c < Ncatalog; c++) {

    for (i = 0; i < catalog[c].Naverage; i++) {
      if (!(catalog[c].average[i].flags & ID_OBJ_ICRF_QSO)) continue;

      // only save a single value
      int savedICRF = FALSE;
      for (j = 0; j < catalog[c].average[i].Nmeasure; j++) {
	  
	int offset = catalog[c].average[i].measureOffset + j;
	  
	if (!(catalog[c].measureT[offset].dbFlags & ID_MEAS_ICRF_QSO)) continue;

	fprintf (f, "%d %d %d : %f %f : %f %f\n", c, i, offset,
		 catalog[c].average[i].R, catalog[c].average[i].D,
		 catalog[c].measureT[offset].R, catalog[c].measureT[offset].D);

	if (savedICRF) continue;

	ICRFsave (c, i, offset);
	N ++;
	savedICRF = TRUE;
      }
    }
  }
  fclose (f);

  fprintf (stderr, "added %d ICRF QSO\n", N);

  return TRUE;
}
