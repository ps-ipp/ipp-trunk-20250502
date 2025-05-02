# include "imregister.h"

void ConfigCamera () { 

  int i;
  char *config, ID[64], field[128], line[128];

  /* load camera config file */
  config = LoadConfigFile (CameraConfig);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find camera config file %s\n", CameraConfig);
    exit (1);
  }
  
  /* load data from config file */
  ScanConfig (config, "NCCD", "%d", 1, &Nccd);
  ALLOCATE (ccds, char *, Nccd);
  ALLOCATE (ccdn, char *, Nccd);

  for (i = 0; i < Nccd; i++) {
    sprintf (field, "CCD.%d", i);
    ScanConfig (config, field, "%s", 1, line);
    sscanf (line, "%s", ID);
    ccds[i] = strcreate (ID);
  }

  for (i = 0; i < Nccd; i++) {
    sprintf (ID, "%02d", i);
    ccdn[i] = strcreate (ID);
  }
}

int MatchCCDNameHeader (Header *header) {

  int i;
  char ID[64];

  ID[0] = 0;
  
  gfits_scan (header, CCDnumKeyword,  "%s", 1, ID);
  if (!ID[0]) { 
    fprintf (stderr, "warning, ccd id not found in header\n");
    return (-1);
  }
  
  /* compare as number if ID is a complete number (ie, 00 equiv to 0, but 00b not equiv to 0b */
  for (i = 0; i < Nccd; i++) {
    if (strnumcmp (ccds[i], ID)) {
      return (i);
    }
  }
  
  fprintf (stderr, "warning: ccd %s not found in camera config file\n", ID);
  return (-1);
}

int MatchCCDName (char *ID) {

  int i;

  /* compare as number if ID is a complete number (ie, 00 equiv to 0, but 00b not equiv to 0b */
  for (i = 0; i < Nccd; i++) {
    if (strnumcmp (ccds[i], ID)) {
      return (i);
    }
  }
  
  fprintf (stderr, "warning: ccd %s not found in camera config file\n", ID);
  return (-1);
}

