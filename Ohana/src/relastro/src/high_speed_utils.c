# include "relastro.h"
# include "kapa.h"

                          //   !ID_OBJ_EXT_ALT          ID_OBJ_GOOD_ALT         
#define DEFAULT_WHERE_A "((flags & 0x02000000 == 0) && (flags & 0x08000000))"

                          //    !ID_OBJ_EXT             ID_OBJ_GOOD
#define DEFAULT_WHERE_B  "((flags & 0x01000000) == 0) && ((flags & 0x04000000) && ((y:nphot > 1) || (z:nphot > 1)) &&((y:err < 0.2) || (z:err < 0.2)))"


int print_error();

static dbField *fieldsA = NULL;
static int NfieldsA = 0;
static dbStack *stackA = NULL;
static int NstackA = 0;

static dbField *fieldsB = NULL;
static int NfieldsB = 0;
static dbStack *stackB = NULL;
static int NstackB = 0;

static dbValue *valuesA;
// static int NvaluesA;
static dbValue *valuesB;
// static int NvaluesB;

void setupAreaSelection(SkyRegion *region) {
    SELECTION.name = NULL;
    SELECTION.list = NULL;
    SELECTION.useDisplay = FALSE;
    SELECTION.useSkyregion = TRUE;
    set_skyregion(region->Rmin, region->Rmax, region->Dmin, region->Dmax);
}


// split string into an array of words using space as delimiter by a space.
static char ** splitToWords(char *string, int *n) {
    char **words;
    int NwordsAllocated = 20;
    ALLOCATE(words, char *, NwordsAllocated);

    char *copy_of_str = strcreate(string);

    char *saveptr = NULL;
    char *p = strtok_r(copy_of_str, " ", &saveptr);
    *n = 0;
    while (p) {
        if (*n == NwordsAllocated) {
            NwordsAllocated += 20;
            REALLOCATE (words, char *, NwordsAllocated);
        }
        words[(*n)++] = p;
        p = strtok_r(NULL, " ", &saveptr);
    }

    return words;
}

// Create the data strucutres required to apply avextract like cuts
int setupConstraints(char *whereString, dbField **pFields, int *pNfields, dbStack **pStack, int *pNstack) {

  // an empty where is allowed (no further filtering)
  if (!whereString[0]) return TRUE;

    // split the string into words
    int nWords;
    char **words = splitToWords(whereString, &nWords);
    if (!nWords) {
        fprintf(stderr, "unable to parse constraint\n");
        return FALSE;
    }

    // parse it into elements of the where condition
    char **cstack = NULL;
    unsigned int Ncstack;
    cstack = isolate_elements(nWords, words, &Ncstack);
    free(words);

    *pStack = dbRPN(Ncstack, cstack, pNstack);
    if (Ncstack && !*pNstack) {
        print_error();
        return FALSE;
    }
    dbAstroRegionLimits(pStack, pNstack, &SELECTION, DVO_TABLE_AVERAGE);

    *pFields = NULL;
    *pNfields = 0;

    return dbCheckStack(*pStack, *pNstack, DVO_TABLE_AVERAGE, pFields, pNfields);
}

int initializeConstraints() {

  // if constaints haven't been set by the user use 2mass
  // XXX  if (!WHERE_A[0]) {
  // XXX      strcpy(WHERE_A, DEFAULT_WHERE_A);
  // XXX  }
  // XXX  if (!WHERE_B[0]) {
  // XXX      strcpy(WHERE_B, DEFAULT_WHERE_B);
  // XXX  }

    fprintf (stderr, "where A: %s\n", WHERE_A);
    fprintf (stderr, "where B: %s\n", WHERE_B);

    // pull TIMEFORMAT & TIMEREF from the Config system
    dbExtractAveragesInit();

    if (! setupConstraints(WHERE_A, &fieldsA, &NfieldsA, &stackA, &NstackA)) {
        print_error();
        fprintf(stderr, "failed to set up constratints for group A: %s\n", WHERE_A);
        exit(1);
    }
    if (NfieldsA) {
      ALLOCATE(valuesA, dbValue, NfieldsA);
    }

    if (! setupConstraints(WHERE_B, &fieldsB, &NfieldsB, &stackB, &NstackB)) {
        print_error();
        fprintf(stderr, "failed to set up constratints for group A: %s\n", WHERE_A);
        exit(1);
    }
    if (NfieldsB) {
      ALLOCATE(valuesB, dbValue, NfieldsB);
    }

    return TRUE;
}

// XXX: TODO: We should be able to combine fieldsA and fieldsB and do a single extract

int applyConstraintsA(Catalog *catalog, off_t i) {
    off_t m = catalog[0].average[i].measureOffset;
    int Nsecfilt = GetPhotcodeNsecfilt();
    
    // an empty WHERE matches all objects
    if (!NfieldsA) return TRUE;

    off_t n;
    for (n = 0; n < NfieldsA; n++) {
      valuesA[n] = dbExtractAverages (&catalog[0].average[i], &catalog[0].secfilt[i*Nsecfilt], &catalog[0].measure[m], NULL, NULL, NULL, &fieldsA[n]);
    }
    return dbBooleanCond(stackA, NstackA, valuesA);
}

int applyConstraintsB(Catalog *catalog, off_t i) {
    off_t m = catalog[0].average[i].measureOffset;
    int Nsecfilt = GetPhotcodeNsecfilt();
    
    // an empty WHERE matches all objects
    if (!NfieldsB) return TRUE;

    off_t n;
    for (n = 0; n < NfieldsB; n++) {
      valuesB[n] = dbExtractAverages (&catalog[0].average[i], &catalog[0].secfilt[i*Nsecfilt], &catalog[0].measure[m], NULL, NULL, NULL, &fieldsB[n]);
    }
    return dbBooleanCond(stackB, NstackB, valuesB);
}
