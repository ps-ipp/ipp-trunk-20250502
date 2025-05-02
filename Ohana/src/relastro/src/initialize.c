# include "relastro.h"

void initialize (int argc, char **argv) {

  photcodesKeep   = NULL; 
  photcodesSkip   = NULL; 
  photcodesReset  = NULL; 
  photcodesGroupA = NULL; 
  photcodesGroupB = NULL; 

  ConfigInit (&argc, argv);
  args (argc, argv);

  if (USE_GALAXY_MODEL) {
    if (!InitGalaxyModel (GALAXY_MODEL)) {
      fprintf (stderr, "failed to init galaxy model %s\n", GALAXY_MODEL);
      exit (2);
    }
  }

  if (RELASTRO_OP == OP_MERGE_SOURCE) return;

  if (DCR_BLUE_COLOR_POS)  fprintf (stderr, "DCR_BLUE_COLOR_POS:  %s - %s\n", DCR_BLUE_COLOR_POS, DCR_BLUE_COLOR_NEG);
  if (DCR_RED_COLOR_POS)   fprintf (stderr, "DCR_RED_COLOR_POS:   %s - %s\n", DCR_RED_COLOR_POS, DCR_RED_COLOR_NEG);

  if (PHOTCODE_KEEP_LIST)  fprintf (stderr, "PHOTCODE_KEEP_LIST:  %s\n", PHOTCODE_KEEP_LIST);
  if (PHOTCODE_SKIP_LIST)  fprintf (stderr, "PHOTCODE_SKIP_LIST:  %s\n", PHOTCODE_SKIP_LIST);
  if (PHOTCODE_RESET_LIST) fprintf (stderr, "PHOTCODE_RESET_LIST: %s\n", PHOTCODE_RESET_LIST);
  if (PHOTCODE_A_LIST)     fprintf (stderr, "PHOTCODE_A_LIST: 	  %s\n", PHOTCODE_A_LIST);
  if (PHOTCODE_B_LIST)     fprintf (stderr, "PHOTCODE_B_LIST: 	  %s\n", PHOTCODE_B_LIST);

  photcodesKeep   = ParsePhotcodeList (PHOTCODE_KEEP_LIST,  &NphotcodesKeep,   FALSE);
  photcodesSkip   = ParsePhotcodeList (PHOTCODE_SKIP_LIST,  &NphotcodesSkip,   FALSE);
  photcodesReset  = ParsePhotcodeList (PHOTCODE_RESET_LIST, &NphotcodesReset,  FALSE);
  photcodesGroupA = ParsePhotcodeList (PHOTCODE_A_LIST,     &NphotcodesGroupA, TRUE);
  photcodesGroupB = ParsePhotcodeList (PHOTCODE_B_LIST,     &NphotcodesGroupB, TRUE);

  // blue color elements
  DCR_BLUE_NSEC_POS = DCR_BLUE_NSEC_NEG = -1;
  if (DCR_BLUE_COLOR_POS) {
    DCR_BLUE_PHOTCODE_POS = GetPhotcodebyName (DCR_BLUE_COLOR_POS);
    if (!DCR_BLUE_PHOTCODE_POS) {
      fprintf (stderr, "ERROR: photcode %s not found in photcode table\n", DCR_BLUE_COLOR_POS);
      exit (1);
    }
    DCR_BLUE_NSEC_POS = GetPhotcodeNsec (DCR_BLUE_PHOTCODE_POS[0].code);
  }
  if (DCR_BLUE_COLOR_NEG) {
    DCR_BLUE_PHOTCODE_NEG = GetPhotcodebyName (DCR_BLUE_COLOR_NEG);
    if (!DCR_BLUE_PHOTCODE_NEG) {
      fprintf (stderr, "ERROR: photcode %s not found in photcode table\n", DCR_BLUE_COLOR_NEG);
      exit (1);
    }
    DCR_BLUE_NSEC_NEG = GetPhotcodeNsec (DCR_BLUE_PHOTCODE_NEG[0].code);
  }

  // red color elements
  DCR_RED_NSEC_POS = DCR_RED_NSEC_NEG = -1;
  if (DCR_RED_COLOR_POS) {
    DCR_RED_PHOTCODE_POS = GetPhotcodebyName (DCR_RED_COLOR_POS);
    if (!DCR_RED_PHOTCODE_POS) {
      fprintf (stderr, "ERROR: photcode %s not found in photcode table\n", DCR_RED_COLOR_POS);
      exit (1);
    }
    DCR_RED_NSEC_POS = GetPhotcodeNsec (DCR_RED_PHOTCODE_POS[0].code);
  }
  if (DCR_RED_COLOR_NEG) {
    DCR_RED_PHOTCODE_NEG = GetPhotcodebyName (DCR_RED_COLOR_NEG);
    if (!DCR_RED_PHOTCODE_NEG) {
      fprintf (stderr, "ERROR: photcode %s not found in photcode table\n", DCR_RED_COLOR_NEG);
      exit (1);
    }
    DCR_RED_NSEC_NEG = GetPhotcodeNsec (DCR_RED_PHOTCODE_NEG[0].code);
  }

  initstats (STATMODE);

  if (USE_ICRF_CORRECT) {
    if (!USE_ICRF_LOCAL && !USE_ICRF_SHFIT) {
      fprintf (stderr, "no ICRF correction method chosen\n");
      exit (2);
    }
  }
  if (USE_ICRF_CORRECT) ICRFinit();

  /* XXX drop irrelevant entries */
  if (SHOW_PARAMS) {
    fprintf (stderr, "current parameter settings:\n");
    if (TimeSelect) {
      fprintf (stderr, "TimeSelect: TRUE (%s - %s)\n", ohana_sec_to_date (TSTART), ohana_sec_to_date (TSTOP));
    } else {
      fprintf (stderr, "TimeSelect: FALSE\n");
    }
    fprintf (stderr, "VERBOSE: %d, PLOTSTUFF: %d\n", VERBOSE, PLOTSTUFF);

    fprintf (stderr, "IMAGE_CATALOG          %s\n",  ImageCat);
    fprintf (stderr, "GSCFILE                %s\n",  GSCFILE);
    fprintf (stderr, "CATDIR                 %s\n",  CATDIR);
    exit (0);
  }
}

void initialize_client (int argc, char **argv) {

  ConfigInit (&argc, argv);
  args_client (argc, argv);

  if (PHOTCODE_KEEP_LIST)  fprintf (stderr, "PHOTCODE_KEEP_LIST:  %s\n", PHOTCODE_KEEP_LIST);
  if (PHOTCODE_SKIP_LIST)  fprintf (stderr, "PHOTCODE_SKIP_LIST:  %s\n", PHOTCODE_SKIP_LIST);
  if (PHOTCODE_RESET_LIST) fprintf (stderr, "PHOTCODE_RESET_LIST: %s\n", PHOTCODE_RESET_LIST);
  if (PHOTCODE_A_LIST)     fprintf (stderr, "PHOTCODE_A_LIST:     %s\n", PHOTCODE_A_LIST);
  if (PHOTCODE_B_LIST)     fprintf (stderr, "PHOTCODE_B_LIST:     %s\n", PHOTCODE_B_LIST);

  photcodesKeep   = ParsePhotcodeList (PHOTCODE_KEEP_LIST,  &NphotcodesKeep,   FALSE);
  photcodesSkip   = ParsePhotcodeList (PHOTCODE_SKIP_LIST,  &NphotcodesSkip,   FALSE);
  photcodesReset  = ParsePhotcodeList (PHOTCODE_RESET_LIST, &NphotcodesReset,  FALSE);
  photcodesGroupA = ParsePhotcodeList (PHOTCODE_A_LIST,     &NphotcodesGroupA, TRUE);
  photcodesGroupB = ParsePhotcodeList (PHOTCODE_B_LIST,     &NphotcodesGroupB, TRUE);

  initstats (STATMODE);
}

