# include "dvoshell.h"

int cmd (int argc, char **argv) { /* really need to think about upper limits & how to represent them */
  
  double *M1, *M3;
  off_t i, j, m, N1, N3, i1, i3;
  int N;
  int Npts, NPTS, mode[3];
  int Nsecfilt, KeepNulls;

  PhotCode *code[3];
  Catalog catalog;
  SkyList *skylist;
  SkyRegionSelection *selection;
  Vector *xvec, *yvec;

  /* defaults */
  catalog.average = NULL; 
  catalog.secfilt = NULL;
  catalog.measure = NULL;
  skylist = NULL;
  selection = NULL;
  struct sigaction *old_sigaction = NULL:

  /* load photcode information */
  if (!InitPhotcodes ()) goto escape;
  Nsecfilt = GetPhotcodeNsecfilt ();

  /* interpret command-line options */
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) goto escape;
  if (!SetPhotSelections (&argc, argv, 3)) goto usage;

  KeepNulls = FALSE;
  if ((N = get_argument (argc, argv, "-nulls"))) {
    KeepNulls = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* interpret command-line options */
  if (argc != 6) { goto usage; }
  if (strcmp (argv[2], "-")) goto usage;
  if (strcmp (argv[4], ":")) goto usage;
  if (!GetPhotcodeInfo (argv[1], &code[0], &mode[0])) return (FALSE);
  if (!GetPhotcodeInfo (argv[3], &code[1], &mode[1])) return (FALSE);
  if (!GetPhotcodeInfo (argv[5], &code[2], &mode[2])) return (FALSE);
  if (!TestPhotSelections (&code[0], &mode[0], MEAS_ZERO)) goto escape;

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (selection)) == NULL) goto escape;

  /* init vectors to save data */
  if ((xvec = SelectVector ("xv", ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((yvec = SelectVector ("yv", ANYVECTOR, TRUE)) == NULL) goto escape;

  gprint (GP_ERR, "warning: this function may be deprecated in the future -- use avextract for better control\n");

  Npts = 0;
  NPTS = 100;
  ResetVector (xvec, OPIHI_FLT, NPTS);
  ResetVector (yvec, OPIHI_FLT, NPTS);

  // grab data from all selected sky regions
  old_sigaction = SetInterrupt();

  /* loop over regions, extract data for each region */
  for (j = 0; (j < skylist[0].Nregions) && !interrupt; j++) {
    /* lock, load, unlock catalog */
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = skylist[0].filename[j];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt = 0;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);
    
    /* get correct mags, convert to X,Y */
    for (i = 0; (i < catalog.Naverage) && !interrupt; i++) {
      M1 = M3 = NULL;
      m = catalog.average[i].measureOffset;

      SetSelectionParam (0);
      M1 = ExtractDMag (code, mode, &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], &N1);
      if (N1 == 0) goto skip;

      SetSelectionParam (2);
      M3 = ExtractMagnitudes (code[2], mode[2], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], &N3);
      if (N3 == 0) {
	if (KeepNulls) {
	  ALLOCATE (M3, double, 1);
	  N3 = 1;
	  M3[0] = NAN;
	} else {
	  goto skip;
	}
      }

      for (i1 = 0; i1 < N1; i1++) {
	for (i3 = 0; i3 < N3; i3++) {
	  xvec[0].elements.Flt[Npts] = M1[i1];
	  yvec[0].elements.Flt[Npts] = M3[i3];
	  Npts++;
	  if (Npts >= NPTS) {
	    NPTS += 2000;
	    REALLOCATE (xvec[0].elements.Flt, opihi_flt, NPTS);
	    REALLOCATE (yvec[0].elements.Flt, opihi_flt, NPTS);
	  }
	}
      }
    skip:
      if (M1 != NULL) free (M1);
      if (M3 != NULL) free (M3);
    }
    dvo_catalog_free (&catalog);
  }
  ClearInterrupt (old_sigaction);

  xvec[0].Nelements = yvec[0].Nelements = Npts;
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: cmd F - F : F\n");
  return (FALSE);

escape:
  ClearInterrupt (old_sigaction);
  dvo_catalog_free (&catalog);
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  return (FALSE);
}
    
