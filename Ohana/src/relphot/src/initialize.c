# include "relphot.h"

RelphotMode initialize (int argc, char **argv) {

  init_error();
  relphot_help (argc, argv);
  ConfigInit (&argc, argv);
  RelphotMode mode = args (argc, argv);
  if (!mode) exit (2);

  // UPDATE_AVERAGES used to always operate on all photcodes
  // now, if a list of photcodes are given that is used, otherwise
  // all will be used
  if (!photcodes) {
    char tmpline1[256];
    int Ns;
    Nphotcodes = GetPhotcodeNsecfilt ();
    ALLOCATE (photcodes, PhotCode *, Nphotcodes);
    ALLOCATE (PhotcodeList, char, 256);
    for (Ns = 0; Ns < Nphotcodes; Ns++) {
      photcodes[Ns] = GetPhotcodebyNsec (Ns);
      if (Ns > 0) {
	snprintf (tmpline1, 256, "%s,%s", PhotcodeList, photcodes[Ns][0].name);
      } else {
	snprintf (tmpline1, 256, "%s", photcodes[Ns][0].name);
      }
      strcpy (PhotcodeList, tmpline1);
    }
  }    
    
  if (SHOW_PARAMS) {
    int Ns;
    fprintf (stderr, "subset selection criteria:\n");
    fprintf (stderr, "  photcodes ");
    for (Ns = 0; Ns < Nphotcodes; Ns++) {
      if (Ns == Nphotcodes - 1) {
	fprintf (stderr, "%s\n", photcodes[Ns][0].name);
      } else {
	fprintf (stderr, "%s, ", photcodes[Ns][0].name);
      }
    }
    if (TimeSelect) {
      fprintf (stderr, "TimeSelect: TRUE (%s - %s)\n", ohana_sec_to_date (TSTART), ohana_sec_to_date (TSTOP));
    } else {
      fprintf (stderr, "TimeSelect: FALSE\n");
    }
    if (DophotSelect) {
      fprintf (stderr, "DophotSelect: TRUE (%d)\n", DophotValue);
    } else {
      fprintf (stderr, "DophotSelect: FALSE\n");
    }
    fprintf (stderr, "PSF_QF limit: 0.85 (hardwired)\n");

    // fprintf (stderr, "Photom Bad Mask: 0x%08x, Photom Poor Mask: 0x%08x\n");

    fprintf (stderr, "MAG_LIM: %f, SIGMA_LIM: %f\n", MAG_LIM, SIGMA_LIM);
    fprintf (stderr, "INST_MAG_MIN: %f, INST_MAG_MAX: %f\n", ImagMin, ImagMax);

    fprintf (stderr, "STAR_TOOFEW: %d\n", STAR_TOOFEW);

    fprintf (stderr, "VERBOSE: %d, PLOTSTUFF: %d\n", VERBOSE, PLOTSTUFF);

    fprintf (stderr, "STAR_SCATTER           %lf\n", STAR_SCATTER);
    fprintf (stderr, "STAR_CHISQ             %lf\n", STAR_CHISQ);

    fprintf (stderr, "MOSAIC_SCATTER         %lf\n", MOSAIC_SCATTER);
    fprintf (stderr, "MOSAIC_CHISQ           %lf\n", MOSAIC_CHISQ);

    fprintf (stderr, "NIGHT_SCATTER          %lf\n", NIGHT_SCATTER);
    fprintf (stderr, "NIGHT_CHISQ            %lf\n", NIGHT_CHISQ);

    fprintf (stderr, "IMAGE_SCATTER          %lf\n", IMAGE_SCATTER);
    fprintf (stderr, "IMAGE_OFFSET           %lf\n", IMAGE_OFFSET);
    fprintf (stderr, "IMAGE_CATALOG          %s\n",  ImageCat);
    fprintf (stderr, "GSCFILE                %s\n",  GSCFILE);
    fprintf (stderr, "CATDIR                 %s\n",  CATDIR);
  }

  // init the random seed
  long A, B;
  A = time(NULL);
  for (B = 0; A == time(NULL); B++);
  srand48(B);

  return mode;
}

void initialize_client (int argc, char **argv) {

  // XXX need to determine which globals can affect relphot_client in either mode and pass appropriately
  PhotcodeList = NULL;
  photcodes = NULL;

  relphot_client_help (argc, argv);
  ConfigInit (&argc, argv);
  args_client (argc, argv);

  if (MODE == MODE_SYNTH_PHOT) return;

  if (MODE == MODE_UPDATE_OBJECTS) {
    char tmpline1[256];
    int Ns;
    Nphotcodes = GetPhotcodeNsecfilt ();
    ALLOCATE (photcodes, PhotCode *, Nphotcodes);
    ALLOCATE (PhotcodeList, char, 256);
    for (Ns = 0; Ns < Nphotcodes; Ns++) {
      photcodes[Ns] = GetPhotcodebyNsec (Ns);
      if (Ns > 0) {
	snprintf (tmpline1, 256, "%s,%s", PhotcodeList, photcodes[Ns][0].name);
      } else {
	snprintf (tmpline1, 256, "%s", photcodes[Ns][0].name);
      }
      strcpy (PhotcodeList, tmpline1);
    }
    return;
  }

  // load the list of photcodes into the globals (photcodes, Nphotcodes)
  PhotcodeList = strcreate (argv[1]);
  photcodes = ParsePhotcodeList (PhotcodeList, &Nphotcodes, TRUE); // require SEC photcodes
}

void ParsePhotcodeList_old (char *word) {

  Nphotcodes = 0;
  photcodes = NULL;
  int NPHOTCODES = 10;
  ALLOCATE (photcodes, PhotCode *, NPHOTCODES);

  /* parse the comma-separated list of photcodesKeep */
  char *myList = strcreate(word);
  char *list = myList;
  char *codename = NULL;
  char *ptr = NULL;
  while ((codename = strtok_r (list, ",", &ptr)) != NULL) {
    list = NULL; // pass NULL on successive strtok_r calls
    if ((photcodes[Nphotcodes] = GetPhotcodebyName (codename)) == NULL) {
      fprintf (stderr, "ERROR: photcode %s not found in photcode table\n", codename);
      exit (1);
    }
    if (photcodes[Nphotcodes][0].type != PHOT_SEC) {
      fprintf (stderr, "photcode %s is not an filter type (SEC)\n", codename);
      exit (1);
    }
    Nphotcodes ++;
    CHECK_REALLOCATE (photcodes, PhotCode *, NPHOTCODES, Nphotcodes, 10);
  }
}

