# include "relastro.h"

static unsigned int   catIDmax = 0;
static          int  *catIDseq = NULL;
static unsigned int  *objIDmax = NULL;
static          int **objIDseq = NULL;

int indexCatalogs (Catalog *catalog, int Ncatalog) {

  int i;
  off_t j;

  if (!Ncatalog) return TRUE;

  // find the max value of catID
  for (i = 0; i < Ncatalog; i++) {
    catIDmax = MAX (catalog[i].catID, catIDmax);
  }

  ALLOCATE (catIDseq, int, catIDmax + 1);

  unsigned int ID;
  for (ID = 0; ID < catIDmax + 1; ID++) {
    catIDseq[ID] = -1;
  }

  for (i = 0; i < Ncatalog; i++) {
    unsigned int catID = catalog[i].catID;
    catIDseq[catID] = i;
  }
  
  ALLOCATE (objIDmax, unsigned int,   Ncatalog);
  ALLOCATE (objIDseq,          int *, Ncatalog);
  for (i = 0; i < Ncatalog; i++) {
    objIDmax[i] = 0;
    for (j = 0; j < catalog[i].Naverage; j++) {
      objIDmax[i] = MAX (catalog[i].average[j].objID, objIDmax[i]);
    }

    ALLOCATE (objIDseq[i], int, objIDmax[i] + 1);
    for (ID = 0; ID < objIDmax[i] + 1; ID++) {
      objIDseq[i][ID] = -1;
    }

    for (j = 0; j < catalog[i].Naverage; j++) {
      unsigned int objID = catalog[i].average[j].objID;
      objIDseq[i][objID] = j;
    }
  }
  return TRUE;
}

void freeCatalogIndexes (int Ncatalog) {
  int i;

  for (i = 0; i < Ncatalog; i++) {
    free (objIDseq[i]);
  }
  free (objIDmax);
  free (objIDseq);
  free (catIDseq);
}

int catID_and_objID_to_seq (unsigned int catID, unsigned int objID, int *catSeq, off_t *objSeq) {

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
