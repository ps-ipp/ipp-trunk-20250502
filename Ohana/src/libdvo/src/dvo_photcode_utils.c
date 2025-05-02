# include <dvo.h>

// Create a map between the secfilt values from one photcode table to another
int *GetSecFiltMap(PhotCodeData *output, PhotCodeData *input) {
  int i, j;
  int NsecfiltIn  = input[0].Nsecfilt;
  int NsecfiltOut = output[0].Nsecfilt;
  int *map;
  ALLOCATE(map, int, NsecfiltIn);

  // loop over entries in the input table
  for (i = 0; i < NsecfiltIn; i++) {
    int code  = input[0].codeNsec[i];
    int entry = -1;
    // find the matching entry in the output table
    for (j = 0; j < NsecfiltOut; j++) {
      int outcode = output[0].codeNsec[j];
      if (code == outcode) {
	 entry = j;
	 break;
      }
    }
    // if (entry == -1) {
    //   // entry is missing fail (no printfs in this file)
    //   free(map);
    //   return(FALSE);
    // }

    // if entry is still -1, we will skip this one (not map into the output db)
    map[i] = entry;
  }

  return map;
}

PhotCode **ParsePhotcodeList (char *rawlist, int *nphotcodes, int needAve) {

  *nphotcodes = 0;
  if (!rawlist) return NULL;

  int Nphotcodes = 0;
  int NPHOTCODES = 10;
  PhotCode **photcodes = NULL;
  ALLOCATE (photcodes, PhotCode *, NPHOTCODES);

  /* parse the comma-separated list of photcodes */
  char *myList = strcreate(rawlist);
  char *list = myList;
  char *codename = NULL;
  char *ptr = NULL;
  while ((codename = strtok_r (list, ",", &ptr)) != NULL) {
    list = NULL; // pass NULL on successive strtok_r calls
    if ((photcodes[Nphotcodes] = GetPhotcodebyName (codename)) == NULL) {
      fprintf (stderr, "ERROR: photcode %s not found in photcode table\n", codename);
      exit (1);
    }
    if (needAve && (photcodes[Nphotcodes][0].type != PHOT_SEC)) {
      fprintf (stderr, "photcode %s is not an filter type (SEC)\n", codename);
      exit (1);
    }
    Nphotcodes ++;
    CHECK_REALLOCATE (photcodes, PhotCode *, NPHOTCODES, Nphotcodes, 10);
  }
  free (myList);

  *nphotcodes = Nphotcodes;
  return photcodes;
}
