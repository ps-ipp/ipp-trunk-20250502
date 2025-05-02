# include "dvoshell.h"

enum {Nd, Nm1, Nm2, Nc, Ns, Nt, Nz, NR, ND, Nxc, Nyc, Nxm, Nym, NT, NP, Nd1, Nd2, NVEC};
int ConcatMeasures (Vector *vec, PhotCode *code, int mode, Average *average, SecFilt *secfilt, Measure *measure, int param, int Nin);

int calmextract (int argc, char **argv) {
  
  off_t i, k, Nr;
  int NSTAR, N, mode[2];

  Catalog catalog;
  PhotCode *code[2];
  SkyList *skylist;
  SkyRegionSelection *selection;
  Vector **vec;

  /* these need to be freed in the end */
  catalog.average = NULL; 
  catalog.secfilt = NULL;
  catalog.measure = NULL;
  skylist = NULL;
  selection = NULL;
  vec = NULL;

  /* load photcode information */
  if (!InitPhotcodes ()) goto escape;

  /* command line arguments */
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) goto escape;
  if (!SetPhotSelections (&argc, argv, 2)) goto usage;

  /* interpret required command-line arguments: calmextract F1 - F2 */
  if (argc != 4) goto usage;
  if (strcmp (argv[2], "-")) goto usage;
  if (!GetPhotcodeInfo (argv[1], &code[0], &mode[0])) goto usage;
  if (!GetPhotcodeInfo (argv[3], &code[1], &mode[1])) goto usage;
  if (!TestPhotSelections (&code[0], &mode[0], MEAS_ZERO)) goto escape;

  gprint (GP_ERR, "warning: this function may be deprecated in the future -- use avextract for better control\n");

  /* returned vectors are dmag, mag, color, time, airmass, ra, dec, x, y, exptime */
  ALLOCATE (vec, Vector *, NVEC);
  if ((vec[Nd ] = SelectVector ("cal:dmag",     ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nm1] = SelectVector ("cal:mag1",     ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nm2] = SelectVector ("cal:mag2",     ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nc ] = SelectVector ("cal:color",    ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Ns ] = SelectVector ("cal:star",     ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nt ] = SelectVector ("cal:time",     ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nz ] = SelectVector ("cal:airmass",  ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[NR ] = SelectVector ("cal:ra",       ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[ND ] = SelectVector ("cal:dec",      ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nxc] = SelectVector ("cal:xccd",     ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nyc] = SelectVector ("cal:yccd",     ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nxm] = SelectVector ("cal:xmosaic",  ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nym] = SelectVector ("cal:ymosaic",  ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[NT ] = SelectVector ("cal:exptime",  ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[NP ] = SelectVector ("cal:photcode", ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nd1] = SelectVector ("cal:dm1",      ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((vec[Nd2] = SelectVector ("cal:dm2",      ANYVECTOR, TRUE)) == NULL) goto escape;

  N = 0;
  NSTAR = 100;
  for (k = 0; k < NVEC; k++) {
    ResetVector (vec[k], OPIHI_FLT, NSTAR);
  }

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (selection)) == NULL) goto escape;
  if (!SetImageSelection (TRUE, selection)) goto escape;

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
    int Nstar = 0;
    int Nsecfilt = GetPhotcodeNsecfilt ();

    /* extract values, assign to vectors */
    for (i = 0; i < catalog.Naverage; i++) {
      m = catalog.average[i].offset;

      /* PRI/SEC must have data for color term */
      if (code[0][0].c1 && code[0][0].c2 && !PhotColor (&catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], code[0][0].c1, code[0][0].c2, &color)) continue;

      /* find data for filter 2 (REF) */
      M2 = NAN;
      dM2 = NAN;
      for (j = 0; j < catalog.average[i].Nm; j++) {
	if (catalog.measure[m+j].photcode != code[1][0].code) continue;
	M2 = PhotCat  (&catalog.measure[m+j]); 
	dM2 = catalog.measure[m+j].dM;
      }	
      if (isnan(M2)) continue;
      
      /* find data for filter 1 */
      M1 = ExtractMeasures (code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], &N1, MEAS_MAG);
      if (N1 == 0) goto skip;

      /* extend storage vectors to take new data, if needed */
      if (N + N1 >= NSTAR) {
	NSTAR += N1 + 100;
	for (k = 0; k < NVEC; k++) {
	  REALLOCATE (vec[k][0].elements.Flt, opihi_flt, NSTAR);
	}
      }

      ConcatMeasures (vec[Nt ], code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], N1, MEAS_TIME); 
      ConcatMeasures (vec[Nz ], code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], N1, MEAS_AIRMASS); 
      ConcatMeasures (vec[NT ], code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], N1, MEAS_EXPTIME); 
      ConcatMeasures (vec[NP ], code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], N1, MEAS_PHOTCODE); 
      // ConcatMeasures (vec[Nd1], code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], N1, MEAS_dMAG); 
      ConcatMeasures (vec[Nxc], code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], N1, MEAS_XCCD); 
      ConcatMeasures (vec[Nyc], code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], N1, MEAS_YCCD); 
      ConcatMeasures (vec[Nxm], code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], N1, MEAS_XMOSAIC); 
      ConcatMeasures (vec[Nym], code[0], mode[0], &catalog.average[i], &catalog.secfilt[i*Nsecfilt], &catalog.measure[m], N1, MEAS_YMOSAIC); 

      for (j = 0; j < N1; j++, N++) {
	vec[Nd ][0].elements.Flt[N] = M1[j] - M2;
	vec[Nm1][0].elements.Flt[N] = M1[j];
	vec[Nm2][0].elements.Flt[N] = M2;
	vec[Nd2][0].elements.Flt[N] = dM2;
	vec[Nc ][0].elements.Flt[N] = color;
	vec[Ns ][0].elements.Flt[N] = Nstar;
	vec[NR ][0].elements.Flt[N] = catalog.average[i].R;
	vec[ND ][0].elements.Flt[N] = catalog.average[i].D;
      }
      Nstar ++; 
    skip:
      if (M1 != NULL) free (M1);
    }
    # endif
    dvo_catalog_free (&catalog);
  }

  for (i = 0; i < NVEC; i++) {
    vec[i][0].Nelements = N;
  }
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  FreeImageSelection ();
  return (TRUE);
  
usage:
  gprint (GP_ERR, "USAGE: dmags F - F : measure.param\n");
  return (FALSE);

escape:
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  FreeImageSelection ();
  for (i = 0; i < NVEC; i++) {
    DeleteVector (vec[i]);
  }
  free (vec);
  return (FALSE);
}

int ConcatMeasures (Vector *vec, PhotCode *code, int mode, Average *average, SecFilt *secfilt, Measure *measure, int Nin, int param) {

  off_t i, N;
  int Ns;
  double *value;

  value = ExtractMeasures (code, mode, average, secfilt, measure, &N, param); 
  if (N != Nin) {
    gprint (GP_ERR, "error!\n");
    return (FALSE);
  }

  Ns = vec[0].Nelements;
  for (i = 0; i < N; i++) {
    vec[0].elements.Flt[Ns+i] = value[i];
  }
  vec[0].Nelements = Ns + N;

  free (value);
  return (TRUE);
}
