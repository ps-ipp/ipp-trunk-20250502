# include "dvoshell.h"

int lightcurve (int argc, char **argv) {
  
  double Ra, Dec, Radius, Radius2, r;
  double *RA, *DEC;
  off_t i, j, k, m, Nstars, *N1;
  int found, PhotCodeSelect;
  int N, NPTS, Nsecfilt, RELPHOT, TimeFormat;
  time_t TimeReference;

  PhotCode *code;
  Catalog catalog;
  SkyTable *sky;
  SkyList *skylist;
  Vector *tvec, *mvec, *dmvec;

  if (!InitPhotcodes ()) return (FALSE);
  Nsecfilt = GetPhotcodeNsecfilt ();

  if ((tvec = SelectVector ("tc", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((mvec = SelectVector ("mc", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((dmvec = SelectVector ("dmc", ANYVECTOR, TRUE)) == NULL) return (FALSE);

  RELPHOT = FALSE;
  if ((N = get_argument (argc, argv, "-rel"))) {
    remove_argument (N, &argc, argv);
    RELPHOT = TRUE;
  }

  code = NULL;
  PhotCodeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    if ((code = GetPhotcodebyName (argv[N])) == NULL) {
      gprint (GP_ERR, "ERROR: photcode not found in photcode table\n");
      return (FALSE);
    }
    PhotCodeSelect = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc < 4) {
    gprint (GP_ERR, "USAGE: lightcurve RA DEC Radius\n");
    return (FALSE);
  }
  
  Ra = atof (argv[1]);
  Dec = atof (argv[2]);
  Radius = atof (argv[3]);

  sky = GetSkyTable ();
  skylist = SkyListByRadius (sky, -1, Ra, Dec, Radius);

  if (skylist[0].Nregions > 1) {
    gprint (GP_ERR, "warning, radius overlaps region boundary, not yet implemented\n");
  }

  /* set filename, read in header */
  dvo_catalog_init (&catalog, TRUE);
  catalog.filename = skylist[0].filename[0];
  catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
  catalog.Nsecfilt = 0;

  // an error exit status here is a significant error
  if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
  }
  dvo_catalog_unlock (&catalog);

  Nstars = catalog.Naverage;
  ALLOCATE (RA, double, Nstars);
  ALLOCATE (DEC, double, Nstars);
  ALLOCATE (N1, off_t, Nstars);

  /* find star(s) in RA, DEC list -- use a dumb algorithm for now, improve later */
  /* stars are not guaranteed to be sorted in RA or in DEC, so first sort the list */
  for (i = 0; i < Nstars; i++) {
    RA[i] = catalog.average[i].R;
    DEC[i] = catalog.average[i].D;
    N1[i] = i;
  }
  /* sort list by DEC */
  if (Nstars > 1) sort_coords_index (DEC, RA, N1, Nstars);
  /* at this point, RA, DEC, and N1 are sorted by DEC.  
     catalog.average[N1[i]].R = RA[i] */

  N = 0;
  NPTS = 100;
  ResetVector (tvec, OPIHI_FLT, NPTS);
  ResetVector (mvec, OPIHI_FLT, NPTS);
  ResetVector (dmvec, OPIHI_FLT, NPTS);
  
  GetTimeFormat (&TimeReference, &TimeFormat);

  Radius2 = Radius*Radius;
  found = FALSE;
  for (i = 0; (i < catalog.Naverage) && !found; i++) {

    /* this can be improved by using a couple of jumps to get within range */
    if (Dec > DEC[i] + Radius)
      continue;
    
    r = SQ(Dec - DEC[i]) + SQ(Ra - RA[i]);
    if (r < Radius2) {
      k = N1[i];
      /* found star, extract measurements */
      m = catalog.average[k].measureOffset;
      for (j = 0; j < catalog.average[k].Nmeasure; j++, m++) {

	if (PhotCodeSelect) {
	  if ((code[0].type == PHOT_REF) || (code[0].type == PHOT_DEP)) {
	    if (code[0].code != catalog.measure[m].photcode) continue;
	  } 
	  if (code[0].type == PHOT_SEC) {
	    if (code[0].code != GetPhotcodeEquivCodebyCode (catalog.measure[m].photcode)) continue;
	  } 
	}      

	tvec[0].elements.Flt[N] = TimeValue (catalog.measure[m].t, TimeReference, TimeFormat);
	dmvec[0].elements.Flt[N] = catalog.measure[m].dM;
	if (RELPHOT) {
	  mvec[0].elements.Flt[N] = PhotCat (&catalog.measure[m], MAG_CLASS_PSF);
	} else {
	  mvec[0].elements.Flt[N] = PhotRel (&catalog.measure[m], &catalog.average[k], &catalog.secfilt[k*Nsecfilt], MAG_CLASS_PSF);
	}
	N++; 
	if (N == NPTS) {
	  NPTS += 100;
	  REALLOCATE (tvec[0].elements.Flt, opihi_flt, NPTS);
	  REALLOCATE (mvec[0].elements.Flt, opihi_flt, NPTS);
	  REALLOCATE (dmvec[0].elements.Flt, opihi_flt, NPTS);
	}
      }      
    }
  }
  dsortthree (tvec[0].elements.Flt, mvec[0].elements.Flt, dmvec[0].elements.Flt, N);
  tvec[0].Nelements = mvec[0].Nelements = dmvec[0].Nelements = N;

  free (RA);
  free (DEC);
  free (N1);
  dvo_catalog_free (&catalog);
  SkyListFree (skylist);
  return (TRUE);
}
