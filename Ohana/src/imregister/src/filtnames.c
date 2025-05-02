# include "imregister.h"
static char *version = "filtnames $Revision: 1.2 $";

typedef struct {
  int code, calibrated;
  char name[64], ref[64], c1[64], c2[64];
} FiltCode;

int main (int argc, char **argv) {

  int i, code, Ncode, Nmatch, Nfield, N, Select, CalibrationData;
  int Nfiltcode, NFILTCODE;
  char *config, *file, *target;
  char FilterList[256];
  char name[64], ref[64], c1[64], c2[64], line[256], *c;
  FILE *f;
  FiltCode *filtcode;

  get_version (argc, argv, version);

  /*** load configuration info ***/
  file = SelectConfigFile (&argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }
  if (!ScanConfig (config, "FILTER_LIST", "%s", 0, FilterList)) {
    fprintf (stderr, "ERROR: can't find FILTER_LIST in configuration file\n");
    exit (1);
  }
  free (config);
  free (file);

  /* image type (dark, flat, bias, etc) */
  Select = TRUE;
  if ((N = get_argument (argc, argv, "-all"))) {
    remove_argument (N, &argc, argv);
    Select = FALSE;
  }
 
  /* image type (dark, flat, bias, etc) */
  CalibrationData = FALSE;
  if ((N = get_argument (argc, argv, "-cal"))) {
    remove_argument (N, &argc, argv);
    CalibrationData = TRUE;
  }
 
  if (argc != 2) { 
    fprintf (stderr, "USAGE: filtnames (filtername)\n");
    fprintf (stderr, "       filtnames list : list all unique filters\n");
    fprintf (stderr, "       [-all] : list all names for given filter(s)\n");
    fprintf (stderr, "       [-cal] : print calibration data for filter(s)\n");
    exit (1);
  }

  target = argv[1];
  for (i = 0; i < strlen (target); i++) { if (isspace (target[i])) target[i] = '.'; }

  /* open filter list file */
  f = fopen (FilterList, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "error reading photcodes\n");
    exit (1);
  }

  /* load filter list & codes */
  NFILTCODE = 100;
  Nfiltcode = 0;
  ALLOCATE (filtcode, FiltCode, NFILTCODE);

  while (scan_line (f, line) != EOF) {
    for (c = line; isspace (*c); c++);
    if (*c == '#') continue;
    if (*c == 0) continue;
    Nfield = sscanf (c, "%d %s %s %s %*s %s", &code, name, ref, c1, c2);
    if ((Nfield != 2) && (Nfield != 5)) {
      fprintf (stderr, "error reading line: %s\n", c);
      exit (1);
    }

    filtcode[Nfiltcode].code = code;
    filtcode[Nfiltcode].calibrated = FALSE;
    strcpy (filtcode[Nfiltcode].name, name);
    filtcode[Nfiltcode].ref[0] = 0;
    filtcode[Nfiltcode].c1[0] = 0;
    filtcode[Nfiltcode].c2[0] = 0;
    if (Nfield == 5) {
      filtcode[Nfiltcode].calibrated = TRUE;
      strcpy (filtcode[Nfiltcode].ref, ref);
      strcpy (filtcode[Nfiltcode].c1, c1);
      strcpy (filtcode[Nfiltcode].c2, c2);
    }
    Nfiltcode ++;

    if (Nfiltcode == NFILTCODE - 1) {
      NFILTCODE += 100;
      REALLOCATE (filtcode, FiltCode, NFILTCODE);
    }
  }

  /* special target: list -- list all filters */
  if (!strcasecmp (target, "list")) {
    
    int *uniq, Nuniq, j, found;
    ALLOCATE (uniq, int, Nfiltcode);

    /* identify unique filter codes */
    Nuniq = 0;
    for (i = 0; i < Nfiltcode; i++) {
      found = FALSE;
      for (j = 0; !found && (j < Nuniq); j++) {
	if (filtcode[i].code == uniq[j]) found = TRUE;
      }
      if (found) continue;
      uniq[Nuniq] = filtcode[i].code;
      Nuniq ++;
    }

    /* list the entries of the unique codes.  skip code == 0 (none) */
    for (i = 0; i < Nuniq; i++) {
      if (uniq[i] == 0) continue;
      for (j = 0; j < Nfiltcode; j++) {
	if (uniq[i] != filtcode[j].code) continue;
	if (CalibrationData) {
	  fprintf (stdout, "%s %d %s %s %s\n", filtcode[j].name, filtcode[j].calibrated, filtcode[j].ref, filtcode[j].c1, filtcode[j].c2);
	} else {
	  fprintf (stdout, "%s\n", filtcode[j].name);
	}
	if (Select) break;
      }
    }
    exit (0);
  }

  /* find given name in filter list (case insensitive) */
  Ncode = 0;
  for (i = 0; i < Nfiltcode; i++) {
    if (strcasecmp (target, filtcode[i].name)) continue;
    Ncode = filtcode[i].code;
    break;
  }
  if (!Ncode) {
    fprintf (stderr, "no filter match found\n");
    exit (1);
  }

  /* find first entry with this code */
  Nmatch = 0;
  for (i = 0; i < Nfiltcode; i++) {
    if (Ncode != filtcode[i].code) continue;
    if (CalibrationData) {
      fprintf (stdout, "%s %d %s %s %s\n", filtcode[i].name, filtcode[i].calibrated, filtcode[i].ref, filtcode[i].c1, filtcode[i].c2);
    } else {
      fprintf (stdout, "%s\n", filtcode[i].name);
    }
    Nmatch ++;
    if (Select) break;
  }

  if (!Nmatch) {
    fprintf (stderr, "no filter code error: code mis-match\n");
    exit (1);
  }

  exit (0);
}
