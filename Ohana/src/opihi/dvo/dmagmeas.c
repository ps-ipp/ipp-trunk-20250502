# include "dvoshell.h"

int dmagmeas (int argc, char **argv) {
  
  gprint (GP_ERR, "this function is deprecated\n");
  return (FALSE);

} 

# if (0) 
  char *RegionName, *RegionList;
  double *M1, *M3;
  int i, j, m, i1, i3, N1, N3, N;
  int Npts, NPTS, param, mode[3];
  int Nsecfilt, KeepNulls;

  Catalog catalog;
  PhotCode *code[3];
  SkyList *skylist;
  Vector *xvec, *yvec;

  /* defaults */
  catalog.average = NULL; 
  catalog.secfilt = NULL;
  catalog.measure = NULL;
  RegionName = NULL;
  RegionList = NULL;
  skylist = NULL;
  code[2] = NULL;

  /* load photcode information */
  if (!InitPhotcodes ()) goto escape;
  Nsecfilt = GetPhotcodeNsecfilt ();

  /* interpret command-line options */
  if (!SetRegionSelection (&argc, argv, &RegionName, &RegionList)) goto escape;
  if (!SetPhotSelections (&argc, argv, 3)) goto usage;

  KeepNulls = FALSE;
  if ((N = get_argument (argc, argv, "-nulls"))) {
    KeepNulls = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* interpret command-line arguments: dmagmeas F1 - F2 : (value) */
  if (argc != 6) { goto usage; }
  if (strcmp (argv[2], "-")) goto usage;
  if (strcmp (argv[4], ":")) goto usage;
  if (!GetPhotcodeInfo (argv[1], &code[0], &mode[0])) goto usage;
  if (!GetPhotcodeInfo (argv[3], &code[1], &mode[1])) goto usage;
  if ((param = GetMeasureParam (argv[5])) == MEAS_ZERO) goto usage;
  if (!TestPhotSelections (&code[2], &mode[2], MEAS_ZERO)) goto escape;

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (RegionName, RegionList)) == NULL) goto escape;
  if (!SetImageSelection (((param == MEAS_XMOSAIC) || (param == MEAS_YMOSAIC)), ((RegionName == NULL) && (RegionList == NULL)))) goto escape;

  /* init vectors to save data */
  Npts = 0;
  NPTS = 1;
  if ((xvec = SelectVector ("xv", ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((yvec = SelectVector ("yv", ANYVECTOR, TRUE)) == NULL) goto escape;

  /* loop over regions, extract data for each region */
  for (j = 0; j < skylist[0].Nregions; j++) {
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
    for (i = 0; i < catalog.Naverage; i++) {
      M1 = M3 = NULL;
      m = catalog.average[i].offset;

      SetSelectionParam (0);
      M1 = ExtractDMag (code, mode, &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], &N1);
      if (N1 == 0) goto skip;

      SetSelectionParam (2);
      M3 = ExtractMeasures (code[2], mode[2], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], &N3, param);
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
	  xvec[0].elements[Npts] = M1[i1];
	  yvec[0].elements[Npts] = M3[i3];
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
      if (M3 != NULL) free (M3);
    }
    dvo_catalog_free (&catalog);
  }
  FreeImageSelection ();
  SkyListFree (skylist);
  xvec[0].Nelements = yvec[0].Nelements = Npts;
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: dmagmeas F - F : measure.param\n");
  return (FALSE);

escape:
  FreeImageSelection ();
  SkyListFree (skylist);
  dvo_catalog_free (&catalog);
  if (RegionName != NULL) free (RegionName);
  if (RegionList != NULL) free (RegionList);
  return (FALSE);
}

# endif
