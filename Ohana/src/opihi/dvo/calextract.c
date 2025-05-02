# include "dvoshell.h"

enum {Nd, Nm, NC, NR, ND, Np, Nc, Nt, Nx, Nd1, Nd2, NVEC};

int calextract (int argc, char **argv) {
  
  off_t i, Nr;
  int N, mode[2];

  PhotCode *code[2];
  Catalog catalog;
  Vector **vec;
  SkyList *skylist;
  SkyRegionSelection *selection;

  /* these need to be freed in the end */
  catalog.average = NULL; 
  catalog.secfilt = NULL;
  catalog.measure = NULL;
  skylist = NULL;
  selection = NULL;
  vec = NULL;

  /* load photcode information */
  if (!InitPhotcodes ()) return (FALSE);

  /* command line arguments */
  SetSelectionParam (0);

  // parse skyregion options
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) goto escape;
  if (!SetPhotSelections (&argc, argv, 2)) goto usage;

  if (argc != 4) goto usage;
  if (strcmp (argv[2], "-")) goto usage;
  if (!GetPhotcodeInfo (argv[1], &code[0], &mode[0])) goto usage;
  if (!GetPhotcodeInfo (argv[3], &code[1], &mode[1])) goto usage;
  /* code.type must be PHOT_REF */

  /* one unique value per star */
  N = 0;
  ALLOCATE (vec, Vector *, NVEC);
  if ((vec[Nd] 	= SelectVector ("cal:dmag",     ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nm] 	= SelectVector ("cal:mag",      ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[NC] 	= SelectVector ("cal:color",    ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[NR] 	= SelectVector ("cal:ra",       ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[ND] 	= SelectVector ("cal:dec",      ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Np] 	= SelectVector ("cal:nphot",    ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nc] 	= SelectVector ("cal:ncode",    ANYVECTOR, TRUE)) == NULL) goto escape;
  // if ((vec[Nt] 	= SelectVector ("cal:ncrit",    ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nx] 	= SelectVector ("cal:chisq",    ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nd1] = SelectVector ("cal:dm1",      ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nd2] = SelectVector ("cal:dm2",      ANYVECTOR, TRUE)) == NULL) goto escape;

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (selection)) == NULL) goto escape;

  for (Nr = 0; Nr < skylist[0].Nregions; Nr++) {
    if (Nr && !(Nr % 500)) { gprint (GP_ERR, "."); }

    /* lock, load, unlock catalog */
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = skylist[0].filename[Nr];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt = 0;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    # if (0)
    int NSTAR = 1;
    int Nsecfilt = GetPhotcodeNsecfilt ();
    /* extract values, assign to vectors */
    for (i = 0; i < catalog.Naverage; i++) {
      if (i && !(i % 10000)) { gprint (GP_ERR, ","); }
      m = catalog.average[i].offset;

      if (code[0][0].c1 && code[0][0].c2 && !PhotColor (&catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], code[0][0].c1, code[0][0].c2, &color)) continue;

      /* find data for filter 2 (PHOT_REF) */
      M2 = NAN;
      dM2 = NAN;
      for (j = 0; j < catalog.average[i].Nm; j++) {
	if (catalog.measure[m+j].photcode != code[1][0].code) continue;
	M2 = PhotCat  (&catalog.measure[m+j]);
	dM2 = catalog.measure[m+j].dM;
      }	
      if (isnan(M2)) continue;

      /* find data for filter 1 */
      M1 = ExtractAverages (code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], AVE_MAG);
      if (isnan(M1)) continue;

      vec[Nd ][0].elements[N] = M1 - M2;
      vec[Nm ][0].elements[N] = M2;
      vec[NC ][0].elements[N] = color;
      vec[NR ][0].elements[N] = catalog.average[i].R;
      vec[ND ][0].elements[N] = catalog.average[i].D;
      vec[Nd1][0].elements[N] = ExtractAverages (code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], AVE_dMAG);
      vec[Nd2][0].elements[N] = dM2;
      vec[Nx ][0].elements[N] = ExtractAverages (code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], AVE_Xm);
      vec[Nc ][0].elements[N] = ExtractAverages (code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], AVE_NCODE);
      vec[Np ][0].elements[N] = ExtractAverages (code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], AVE_NPHOT);
      // vec[Nt ][0].elements[N] = ExtractAverages (code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], AVE_NCRIT);
      N ++;
      if (N == NSTAR) {
	NSTAR += 100;
	for (j = 0; j < NVEC; j++) {
	  REALLOCATE (vec[j][0].elements, float, NSTAR);
	}
      }
    }
    # endif
    dvo_catalog_free (&catalog);
  }

  for (i = 0; i < NVEC; i++) {
    vec[i][0].Nelements = N;
  }

  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  return (TRUE);
  
usage:
  gprint (GP_ERR, "USAGE: calextract F - F\n");
  return (FALSE);

escape:
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  for (i = 0; i < NVEC; i++) {
    DeleteVector (vec[i]);
  }
  free (vec);

  return (FALSE);
}
