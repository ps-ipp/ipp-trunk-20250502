# include "dvoshell.h"

int dmagaves (int argc, char **argv) {
  
  gprint (GP_ERR, "this function is deprecated\n");
  return (FALSE);

} 

# if (0) 
  char *RegionName, *RegionList;
  double *M1, M2;
  int i, j, k, m, N1;
  int Npts, NPTS, param, mode[3];
  int Nsecfilt;

  PhotCode *code[3];
  Catalog catalog;
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

  /* interpret command-line options: dmagaves F1 - F2 : (value) */
  if (argc != 6) { goto usage; }
  if (strcmp (argv[2], "-")) goto usage;
  if (strcmp (argv[4], ":")) goto usage;
  if (!GetPhotcodeInfo (argv[1], &code[0], &mode[0])) goto usage;
  if (!GetPhotcodeInfo (argv[3], &code[1], &mode[1])) goto usage;
  if ((param = GetAverageParam (argv[5])) == AVE_ZERO) goto usage;
  if (!TestPhotSelections (&code[2], &mode[2], param)) goto escape;

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (RegionName, RegionList)) == NULL) goto escape;

  /* init vectors to save data */
  Npts = 0;
  NPTS = 1;
  if ((xvec = SelectVector ("xv", ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((yvec = SelectVector ("yv", ANYVECTOR, TRUE)) == NULL) goto escape;

  for (j = 0; j < skylist[0].Nregions; j++) {
    /* lock, load, unlock catalog */
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
      M1 = NULL;
      m = catalog.average[i].offset;

      SetSelectionParam (0);
      M1 = ExtractDMag (code, mode, &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], &N1);
      if (N1 == 0) goto skip;

      SetSelectionParam (2);
      M2 = ExtractAverages (code[2], mode[2], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], param);

      for (k = 0; k < N1; k++) {
	xvec[0].elements[Npts] = M1[k];
	yvec[0].elements[Npts] = M2;
	Npts++;
	if (Npts >= NPTS) {
	  NPTS += 2000;
	  REALLOCATE (xvec[0].elements, float, NPTS);
	  REALLOCATE (yvec[0].elements, float, NPTS);
	}
      }
    skip:
      if (M1 != NULL) free (M1);
    }
    dvo_catalog_free (&catalog);
  }
  SkyListFree (skylist);
  xvec[0].Nelements = yvec[0].Nelements = Npts;
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: dmagaves F - F : average.param\n");
  return (FALSE);

escape:
  SkyListFree (skylist);
  dvo_catalog_free (&catalog);
  if (RegionName != NULL) free (RegionName);
  if (RegionList != NULL) free (RegionList);
  return (FALSE);
}

# endif
