# include "dvomerge.h"

char **load_cptlist (char *filename, int *nlist) {

  int NLIST = 100;
  
  char **list = NULL;
  ALLOCATE(list, char *, NLIST);

  int i;
  for (i = 0; i < NLIST; i++) {
    ALLOCATE(list[i], char, DVO_MAX_PATH);
    memset(list[i], 0, DVO_MAX_PATH);
  }
  
  FILE *listfile = fopen(filename, "r");
  myAssert(listfile, "error opening list");

  int j;
  for (i = 0; fscanf (listfile, "%s", list[i]) != EOF; i++) {
    if (i == NLIST - 1) {
      NLIST += 100;
      REALLOCATE(list, char *, NLIST);
      for (j = i + 1; j < NLIST; j++) {
	ALLOCATE(list[j], char, DVO_MAX_PATH);
	memset(list[j], 0, DVO_MAX_PATH);
      }
    }
  }      
  *nlist = i;

  for (i = *nlist; i < NLIST; i ++) {
    FREE(list[i]);
  }

  fclose(listfile);

  return list;
}
