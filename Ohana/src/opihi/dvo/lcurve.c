# include "dvoshell.h"

int lcurve (int argc, char **argv) {
  
  char string[128], *p;
  double Ra, Dec, Radius, Radius2, r;
  double *RA, *DEC;
  int kapa, TimeFormat;
  int found, AutoLimits, ErrorBars, SaveVectors;
  off_t i, j, m, Nstars, *N1;
  int N, NPTS;
  time_t TimeReference;
  struct tm *timeptr;
  Vector *xvec, *yvec;
  Vector Xvec, Yvec, dYvec;
  Catalog catalog;
  Graphdata graphmode;
  SkyTable *sky;
  SkyList *skylist;

  if (!InitPhotcodes ()) return (FALSE);
  if (!style_args (&graphmode, &argc, argv, &kapa)) return (FALSE);

  AutoLimits = FALSE;
  if ((N = get_argument (argc, argv, "-l"))) {
    remove_argument (N, &argc, argv);
    AutoLimits = TRUE;
  }

  xvec = yvec = NULL;
  SaveVectors = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    SaveVectors = TRUE;
    if ((xvec = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
    if ((yvec = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  // int AbsPhot = FALSE;
  // if ((N = get_argument (argc, argv, "-abs"))) {
  //   remove_argument (N, &argc, argv);
  //   AbsPhot = TRUE;
  // }

  // int GalMag = FALSE;
  // if ((N = get_argument (argc, argv, "-gal"))) {
  //   gprint (GP_ERR, "galaxy magnitudes currently disabled\n");
  //   return (FALSE);
  // }

  ErrorBars = FALSE;
  if ((N = get_argument (argc, argv, "-d"))) {
    remove_argument (N, &argc, argv);
    ErrorBars = TRUE;
  }

  if (argc < 4) {
    gprint (GP_ERR, "USAGE: lcurve RA DEC Radius\n");
    return (FALSE);
  }
  
  Ra = atof (argv[1]);
  Dec = atof (argv[2]);
  Radius = atof (argv[3]);

  /* load sky from correct table */
  sky = GetSkyTable ();
  skylist = SkyListByRadius (sky, -1, Ra, Dec, Radius);

  if (skylist[0].Nregions > 1) {
    gprint (GP_ERR, "warning, radius overlaps region boundary, not yet implemented\n");
  }

  /* set filename, read in header */
  dvo_catalog_init (&catalog, TRUE);
  catalog.filename = skylist[0].filename[0];
  catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE;
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
  SetVector (&Xvec, OPIHI_FLT, NPTS);
  SetVector (&Yvec, OPIHI_FLT, NPTS);
  dYvec.elements.Flt = NULL;
  if (ErrorBars) {   
    SetVector (&dYvec, OPIHI_FLT, NPTS);
  }

  GetTimeFormat (&TimeReference, &TimeFormat);

  Radius2 = Radius*Radius;
  found = FALSE;
  for (i = 0; (i < catalog.Naverage) && !found; i++) {

    /* this can be improved by using a couple of jumps to get within range */
    if (Dec > DEC[i] + Radius)
      continue;
    
    r = SQ(Dec - DEC[i]) + SQ(Ra - RA[i]);
    if (r < Radius2) {
      /* found star, extract measurements */
      m = catalog.average[N1[i]].measureOffset;
      for (j = 0; j < catalog.average[N1[i]].Nmeasure; j++, m++) {
	if (ErrorBars) dYvec.elements.Flt[N] = catalog.measure[m].dM;
	Xvec.elements.Flt[N] = TimeValue (catalog.measure[m].t, TimeReference, TimeFormat);
	Yvec.elements.Flt[N] = PhotCat (&catalog.measure[m], MAG_CLASS_PSF);
	/**** need to use PhotRel optionally here ****/
	N++; 
	if (N == NPTS) {
	  NPTS += 100;
	  REALLOCATE (Xvec.elements.Flt, opihi_flt, NPTS);
	  REALLOCATE (Yvec.elements.Flt, opihi_flt, NPTS);
	  if (ErrorBars) { REALLOCATE (dYvec.elements.Flt, opihi_flt, NPTS); }
	}
      }      
    }
  }
  Xvec.Nelements = Yvec.Nelements = N;
  if (ErrorBars) dYvec.Nelements = N;
  
  if (ErrorBars)
    dsortthree (Xvec.elements.Flt, Yvec.elements.Flt, dYvec.elements.Flt, N);
  else
    dsortpair (Xvec.elements.Flt, Yvec.elements.Flt, N);

  /* autoscale the plot */
  if (AutoLimits) SetLimits (&Xvec, &Yvec, &graphmode);

  if (ErrorBars) 
    graphmode.etype = 1;  /* y errors only in lcurves */
  else
    graphmode.etype = 0;  

  if (ErrorBars) {
    PlotVectorPairErrors (kapa, &Xvec, &Yvec, &dYvec, &graphmode);
  } else {
    PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);
  }

  timeptr = gmtime ((time_t *)&TimeReference);

  if ((p = get_variable ("TIMEFORMAT")) == (char *) NULL) p = strcreate ("days");
  sprintf (string, "%s since %02d/%02d/%02d UT", p,
	   timeptr[0].tm_year, timeptr[0].tm_mon+1, timeptr[0].tm_mday);
  free (p);
  KapaSendLabel (kapa, string, 0);

  free (RA);
  free (DEC);
  free (N1);

  if (SaveVectors) {
    xvec->type = OPIHI_FLT;
    yvec->type = OPIHI_FLT;
    free (xvec[0].elements.Flt);
    free (yvec[0].elements.Flt);
    xvec[0].elements.Flt = Xvec.elements.Flt;
    yvec[0].elements.Flt = Yvec.elements.Flt;
    xvec[0].Nelements = yvec[0].Nelements = Xvec.Nelements;
  } else {
    free (Xvec.elements.Flt);
    free (Yvec.elements.Flt);
  }

  if (ErrorBars) free (dYvec.elements.Flt);
  dvo_catalog_free (&catalog);

  SkyListFree (skylist);
  return (TRUE);
}
