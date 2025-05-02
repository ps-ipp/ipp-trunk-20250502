# include "dvoshell.h"

int ddmags (int argc, char **argv) {
  
  gprint (GP_ERR, "this function is deprecated\n");
  return (FALSE);

} 

# if (0) 
  char *RegionName, *RegionList;
  double *M1, *M2;
  int i, m, k, N, Npts, NPTS;
  int N1, N2, i1, i2, mode[4];
  int Nsecfilt, KeepNulls;

  Catalog catalog;
  PhotCode *code[4];
  SkyList *skylist;
  Vector *xvec, *yvec;

  /* defaults */
  catalog.average = NULL; 
  catalog.secfilt = NULL;
  catalog.measure = NULL;
  RegionName = NULL;
  RegionList = NULL;
  skylist = NULL;

  /* load photcode information */
  if (!InitPhotcodes ()) goto escape;
  Nsecfilt = GetPhotcodeNsecfilt ();

  /* interpret command-line options */
  if (!SetRegionSelection (&argc, argv, &RegionName, &RegionList)) goto escape;
  if (!SetPhotSelections (&argc, argv, 4)) goto usage;

  KeepNulls = FALSE;
  if ((N = get_argument (argc, argv, "-nulls"))) {
    KeepNulls = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* interpret command-line options */
  if (argc != 8) goto usage;
  if (strcmp (argv[2], "-")) goto usage;
  if (strcmp (argv[4], ":")) goto usage;
  if (strcmp (argv[6], "-")) goto usage;
  if (!GetPhotcodeInfo (argv[1], &code[0], &mode[0])) return (FALSE);
  if (!GetPhotcodeInfo (argv[3], &code[1], &mode[1])) return (FALSE);
  if (!GetPhotcodeInfo (argv[5], &code[2], &mode[2])) return (FALSE);
  if (!GetPhotcodeInfo (argv[7], &code[3], &mode[3])) return (FALSE);
  if (!TestPhotSelections (&code[0], &mode[0], MEAS_ZERO)) goto escape;

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (RegionName, RegionList)) == NULL) goto escape;

  /* init vectors to save data */
  Npts = 0;
  NPTS = 1;
  if ((xvec = SelectVector ("xv", ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((yvec = SelectVector ("yv", ANYVECTOR, TRUE)) == NULL) goto escape;

  /* loop over regions, extract data for each region */
  for (k = 0; k < skylist[0].Nregions; k++) {
    /* lock, load, unlock catalog */
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = skylist[0].filename[k];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt = 0;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    // gprint (GP_ERR, "seaching %s with %d stars\n", catalog.filename, catalog.Naverage);
    // ListPhotSelections ();

    /* get correct mags, convert to X,Y */
    for (i = 0; i < catalog.Naverage; i++) {
      M1 = M2 = NULL;
      m = catalog.average[i].offset;

      SetSelectionParam (0);
      M1 = ExtractDMag (&code[0], &mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], &N1);
      if (N1 == 0) goto skip;

      SetSelectionParam (2);
      M2 = ExtractDMag (&code[2], &mode[2], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], &N2);
      if (N2 == 0) {
	if (KeepNulls) {
	  ALLOCATE (M2, double, 1);
	  N2 = 1;
	  M2[0] = NAN;
	} else {
	  goto skip;
	}
      }

      for (i1 = 0; i1 < N1; i1++) {
	for (i2 = 0; i2 < N2; i2++) {
	  xvec[0].elements[Npts] = M1[i1];
	  yvec[0].elements[Npts] = M2[i2];
	  Npts++;
	  if (Npts >= NPTS) {
	    NPTS += 2000;
	    REALLOCATE (xvec[0].elements, float, NPTS);
	    REALLOCATE (yvec[0].elements, float, NPTS);
	  }
	}
      }
    skip:
      if (M1 != NULL) free (M1);
      if (M2 != NULL) free (M2);
    }
    // gprint (GP_ERR, "selected %d stars\n", Npts);
    dvo_catalog_free (&catalog);
  }
  SkyListFree (skylist);
  xvec[0].Nelements = yvec[0].Nelements = Npts;
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: ddmags F - F : measure.param\n");

escape:
  SkyListFree (skylist);
  dvo_catalog_free (&catalog);
  if (RegionName != NULL) free (RegionName);
  if (RegionList != NULL) free (RegionList);
  return (FALSE);
}

# endif
