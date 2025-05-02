# include "dvoshell.h"

int imdata (int argc, char **argv) {
  
  off_t i, j, k, I;
  int N, NPTS, found, mode, TimeSelect, TimeFormat;
  off_t Nregions, NREGIONS;
  off_t *subset, Nsubset, Nimage;
  double trange;
  time_t tzero, start, stop, TimeReference;
  Image *image;
  Catalog catalog;
  SkyTable *sky;
  SkyList *skylist, *skyset;
  Vector *vec;
  SkyRegionSelection *selection;

  // parse skyregion options
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) {
    gprint (GP_ERR, "invalid sky region selection\n");
    return FALSE;
  }

  start = stop = 0;
  TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tzero)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    TimeSelect = TRUE;
    gprint (GP_ERR, "searching in range %ds - %ds (%f seconds)\n", (int)tzero, (int)(tzero + trange), trange);
    if (trange > 0) {
      start = tzero;
      stop  = tzero + trange;
    } else {
      stop = tzero;
      start  = tzero + trange;
    }      
  }

  gprint (GP_ERR, "function is poorly defined; disabled and may be removed\n");
  gprint (GP_ERR, "this function extracts measure data corresponding to a given image?\n");
  return (FALSE);

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: imdata (value) [-time t dt]\n");
    return (FALSE);
  }

  /* identify selection */
  mode = 0;
  if (!strcasecmp (argv[1], "ra")) 
    mode = 1;
  if (!strcasecmp (argv[1], "dec")) 
    mode = 2;
  if (!strcasecmp (argv[1], "mag")) 
    mode = 3;
  if (!strcasecmp (argv[1], "dmag")) 
    mode = 4;
  if (!strcasecmp (argv[1], "Mcal")) 
    mode = 5;
  if (!strcasecmp (argv[1], "Mrel")) 
    mode = 6;
  if (!strcasecmp (argv[1], "source")) 
    mode = 7;
  if (!strcasecmp (argv[1], "x")) 
    mode = 8;
  if (!strcasecmp (argv[1], "y")) 
    mode = 9;
  if (!strcasecmp (argv[1], "time")) 
    mode = 10;
  if (mode == 0) {
    gprint (GP_ERR, "value may be one of the following:\n");
    gprint (GP_ERR, " ra dR dec dD mag dmag Mrel Mcal source time\n");
    return (FALSE);
  }
  if ((vec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (FALSE);
  image_subset (image, Nimage, &subset, &Nsubset, selection, tzero, trange, TimeSelect);
  // BuildChipMatch (image, Nimage);
  GetTimeFormat (&TimeReference, &TimeFormat);

  /* load sky from correct table */
  sky = GetSkyTable ();

  Nregions = 0;
  NREGIONS = 10;
  ALLOCATE (skylist, SkyList, 1);
  ALLOCATE (skylist[0].regions, SkyRegion *, NREGIONS);
  skylist[0].ownElements = FALSE;

  /* for each image of interest, find the appropriate region files */
  for (i = 0; i < Nsubset; i++) {
    I = subset[i];

    skyset = SkyListByImage (sky, -1, &image[I]);

    for (j = 0; j < skyset[0].Nregions; j++) {
      found = FALSE;
      for (k = 0; (k < skylist[0].Nregions) && !found; k++) {
	found = !strcmp (skylist[0].regions[k][0].name, skyset[0].regions[j][0].name);
      }
      if (found) continue;
      skylist[0].regions[Nregions] = skyset[0].regions[j];
      Nregions ++;
      CHECK_REALLOCATE (skylist[0].regions, SkyRegion *, NREGIONS, Nregions, 10);
    }
    SkyListFree (skyset);
  }	
  free (subset);
  for (i = 0; i < skylist[0].Nregions; i++) {
    gprint (GP_ERR, "try %s\n", skylist[0].regions[i][0].name);
  } 
  
  /* create output vector */
  N = 0;
  NPTS = 1000;
  ResetVector (vec, OPIHI_FLT, NPTS);

  // prepare to handle interrupt signals
  struct sigaction *old_sigaction = SetInterrupt();

  /* for each region file, extract the data of interest in the right time range */
  for (j = 0; (j < skylist[0].Nregions) && !interrupt; j++) {

    /* get file name and open */
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

    /* assign vector values */
    switch (mode) {
      case (1):  /* ra */
	for (i = 0; i < catalog.Nmeasure; i++) {
	  if ((catalog.measure[i].t < start) || (catalog.measure[i].t > stop)) continue;
	  vec[0].elements.Flt[N] = catalog.measure[i].R;
	  N++;
	  CHECK_REALLOCATE (vec[0].elements.Flt, opihi_flt, NPTS, N, 1000);
	}
	break;
      case (2):  /* dec */
	for (i = 0; i < catalog.Nmeasure; i++) {
	  if ((catalog.measure[i].t < start) || (catalog.measure[i].t > stop)) continue;
	  vec[0].elements.Flt[N] = catalog.measure[i].D;
	  N++;
	  CHECK_REALLOCATE (vec[0].elements.Flt, opihi_flt, NPTS, N, 1000);
	}
	break;
      case (3):  /* mag */
	for (i = 0; i < catalog.Nmeasure; i++) {
	  if ((catalog.measure[i].t < start) || (catalog.measure[i].t > stop)) continue;
	  vec[0].elements.Flt[N] = catalog.measure[i].M;
	  N++;
	  CHECK_REALLOCATE (vec[0].elements.Flt, opihi_flt, NPTS, N, 1000);
	}
	break;
      case (4):  /* dmag */
	for (i = 0; i < catalog.Nmeasure; i++) {
	  if ((catalog.measure[i].t < start) || (catalog.measure[i].t > stop)) continue;
	  vec[0].elements.Flt[N] = catalog.measure[i].dM;
	  N++;
	  CHECK_REALLOCATE (vec[0].elements.Flt, opihi_flt, NPTS, N, 1000);
	}
	break;
      case (5):  /* Mcal */
	for (i = 0; i < catalog.Nmeasure; i++) {
	  if ((catalog.measure[i].t < start) || (catalog.measure[i].t > stop)) continue;
	  vec[0].elements.Flt[N] = catalog.measure[i].McalPSF;
	  N++;
	  CHECK_REALLOCATE (vec[0].elements.Flt, opihi_flt, NPTS, N, 1000);
	}
	break;
      case (6):  /* Mrel */
	for (i = 0; i < catalog.Nmeasure; i++) {
	  if ((catalog.measure[i].t < start) || (catalog.measure[i].t > stop)) continue;
	  //n = catalog.measure[i].averef;
	  // vec[0].elements.Flt[N] = catalog.average[n].M;
	  N++;
	}
	break;
      case (7):  /* source */
	for (i = 0; i < catalog.Nmeasure; i++) {
	  if ((catalog.measure[i].t < start) || (catalog.measure[i].t > stop)) continue;
	  vec[0].elements.Flt[N] = catalog.measure[i].photcode;
	  N++;
	  CHECK_REALLOCATE (vec[0].elements.Flt, opihi_flt, NPTS, N, 1000);
	}
	break;
      case (10):  /* time */
	for (i = 0; i < catalog.Nmeasure; i++) {
	  if ((catalog.measure[i].t < start) || (catalog.measure[i].t > stop)) continue;
	  vec[0].elements.Flt[N] = TimeValue (catalog.measure[i].t, TimeReference, TimeFormat);
	  N++;
	  CHECK_REALLOCATE (vec[0].elements.Flt, opihi_flt, NPTS, N, 1000);
	}
	break;
    }
    dvo_catalog_free (&catalog);
  }
  ClearInterrupt (old_sigaction);
  
  vec[0].Nelements = N;
  REALLOCATE (vec[0].elements.Flt, opihi_flt, MAX(1,N));
  FreeImagesDVO(image);
  return (TRUE);
}
