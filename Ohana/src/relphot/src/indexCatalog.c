# include "relphot.h"

static int   catIDmax = 0;
static int  *catIDseq = NULL;
static int  *objIDmax = NULL;
static int **objIDseq = NULL;

int indexCatalogs (Catalog *catalog, int Ncatalog) {

  int i;
  off_t j;

  // find the max value of catID
  for (i = 0; i < Ncatalog; i++) {
    catIDmax = MAX (catalog[i].catID, catIDmax);
  }

  ALLOCATE (catIDseq, int, catIDmax + 1);
  for (i = 0; i < catIDmax + 1; i++) {
    catIDseq[i] = -1;
  }

  for (i = 0; i < Ncatalog; i++) {
    int catID = catalog[i].catID;
    catIDseq[catID] = i;
  }
  
  ALLOCATE (objIDmax, int,   Ncatalog);
  ALLOCATE (objIDseq, int *, Ncatalog);
  for (i = 0; i < Ncatalog; i++) {
    objIDmax[i] = 0;
    for (j = 0; j < catalog[i].Naverage; j++) {
      objIDmax[i] = MAX (catalog[i].averageT[j].objID, objIDmax[i]);
    }

    ALLOCATE (objIDseq[i], int, objIDmax[i] + 1);
    for (j = 0; j < objIDmax[i] + 1; j++) {
      objIDseq[i][j] = -1;
    }

    for (j = 0; j < catalog[i].Naverage; j++) {
      int objID = catalog[i].averageT[j].objID;
      objIDseq[i][objID] = j;
    }
  }
  return TRUE;
}

int catID_and_objID_to_seq (int catID, int objID, int *catSeq, off_t *objSeq) {

  if (catID > catIDmax) return FALSE;

  int cat = catIDseq[catID];
  if (cat < 1) return FALSE;

  if (objID > objIDmax[cat]) return FALSE;

  int obj = objIDseq[cat][objID];
  if (obj < 1) return FALSE;

  *catSeq = cat;
  *objSeq = obj;
  return TRUE;
}

void freeCatalogIndexes (int Ncatalog) {

  for (int i = 0; i < Ncatalog; i++) {
    FREE (objIDseq[i]);
  }
  FREE (catIDseq);
  FREE (objIDseq);
  FREE (objIDmax);
}
