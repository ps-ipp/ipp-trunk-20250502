# include "dvoshell.h"
# define NCHUNK 10000

enum {FLAG_IGNORE, FLAG_SKIP, FLAG_KEEP};

int pmeasure (int argc, char **argv) {
  
  off_t i, j, k, m;
  int kapa, Narg, Npts, NPTS, status, VERBOSE, TimeSelect, Nloaded;
  double Mz, Mr, mag;
  double Radius, Rmin, Rmax, R, D, trange;
  unsigned IDclip, IDchoice, LimExclude;
  unsigned dbFlagChoice, dbFlagClip;
  unsigned photFlagChoice, photFlagClip;
  int PhotcodeClip;
  float *Xvec, *Yvec, *Zvec;
  time_t tzero, tend;

  SkyTable *sky;
  SkyList *skylist;
  Catalog catalog;
  Graphdata graphmode;

  if (!InitPhotcodes ()) return (FALSE);
  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  Mz = 17.0;
  Mr = -5.0;
  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;

  PhotcodeClip = -1;
  if ((Narg = get_argument (argc, argv, "-p"))) {
    remove_argument (Narg, &argc, argv);
    PhotcodeClip = GetPhotcodeCodebyName (argv[Narg]);
    remove_argument (Narg, &argc, argv);
  }
  if ((Narg = get_argument (argc, argv, "-photcode"))) {
    remove_argument (Narg, &argc, argv);
    PhotcodeClip = GetPhotcodeCodebyName (argv[Narg]);
    remove_argument (Narg, &argc, argv);
  }
  IDchoice = 0;
  IDclip = FALSE;
  if ((Narg = get_argument (argc, argv, "-ID"))) {
    remove_argument (Narg, &argc, argv);
    IDchoice  = strtol(argv[Narg], NULL, 0);
    remove_argument (Narg, &argc, argv);
    IDclip = TRUE;
  }
  dbFlagChoice = 0;
  dbFlagClip = FLAG_IGNORE;
  if ((Narg = get_argument (argc, argv, "-dbflag"))) {
    remove_argument (Narg, &argc, argv);
    dbFlagChoice  = strtol(argv[Narg], NULL, 0);
    remove_argument (Narg, &argc, argv);
    dbFlagClip = FLAG_SKIP;
  }
  if ((Narg = get_argument (argc, argv, "+dbflag"))) {
    remove_argument (Narg, &argc, argv);
    dbFlagChoice  = strtol(argv[Narg], NULL, 0);
    remove_argument (Narg, &argc, argv);
    dbFlagClip = FLAG_KEEP;
  }
  photFlagChoice = 0;
  photFlagClip = FLAG_IGNORE;
  if ((Narg = get_argument (argc, argv, "-photflag"))) {
    remove_argument (Narg, &argc, argv);
    photFlagChoice  = strtol(argv[Narg], NULL, 0);
    remove_argument (Narg, &argc, argv);
    photFlagClip = FLAG_SKIP;
  }
  if ((Narg = get_argument (argc, argv, "+photflag"))) {
    remove_argument (Narg, &argc, argv);
    photFlagChoice  = strtol(argv[Narg], NULL, 0);
    remove_argument (Narg, &argc, argv);
    photFlagClip = FLAG_KEEP;
  }

  TimeSelect = FALSE;
  if ((Narg = get_argument (argc, argv, "-time"))) {
    remove_argument (Narg, &argc, argv);
    if (!ohana_str_to_time (argv[Narg], &tzero)) {
      gprint (GP_ERR, "syntax error\n");
      return FALSE;
    }
    remove_argument (Narg, &argc, argv);
    if (!ohana_str_to_dtime (argv[Narg], &trange)) { 
      gprint (GP_ERR, "syntax error\n");
      return FALSE;
    }
    remove_argument (Narg, &argc, argv);
    if (trange < 0) {
      trange = fabs (trange);
      tzero -= trange;
    }
    TimeSelect = TRUE;
  }
  if ((Narg = get_argument (argc, argv, "-trange"))) {
    remove_argument (Narg, &argc, argv);
    if (!ohana_str_to_time (argv[Narg], &tzero)) { 
      gprint (GP_ERR, "syntax error\n");
      return FALSE;
    }
    remove_argument (Narg, &argc, argv);
    if (!ohana_str_to_time (argv[Narg], &tend)) { 
      gprint (GP_ERR, "syntax error\n");
      return FALSE;
    }
    remove_argument (Narg, &argc, argv);
    trange = tend - tzero;
    if (trange < 0) {
      trange = fabs (trange);
      tzero -= trange;
    }
    TimeSelect = TRUE;
  }

  LimExclude = FALSE;
  if ((Narg = get_argument (argc, argv, "-x"))) {
    remove_argument (Narg, &argc, argv);
    LimExclude = TRUE;
  }

  VERBOSE = FALSE;
  if ((Narg = get_argument (argc, argv, "-v"))) {
    remove_argument (Narg, &argc, argv);
    VERBOSE = TRUE;
  }

  if ((Narg = get_argument (argc, argv, "-m"))) {
    remove_argument (Narg, &argc, argv);
    Mr  = atof(argv[Narg]);
    remove_argument (Narg, &argc, argv);
    Mz = atof(argv[Narg]);
    Mr = Mr - Mz;
    remove_argument (Narg, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: pmeasure (-all) [-m M M]\n");
    gprint (GP_ERR, " options:\n");
    gprint (GP_ERR, " [-p photcode]\n");
    gprint (GP_ERR, " [-photcode photcode]\n");
    gprint (GP_ERR, " [-ID ID]\n");
    gprint (GP_ERR, " [-dbflag value] : skip matches to these flags\n");
    gprint (GP_ERR, " [+dbflag value] : keep matches to these flags\n");
    gprint (GP_ERR, " [-photflag value] : skip matches to these flags\n");
    gprint (GP_ERR, " [+photflag value] : keep matches to these flags\n");
    gprint (GP_ERR, " [-time (start) (duration)]\n");
    gprint (GP_ERR, " [-trange (start) (stop)]\n");
    gprint (GP_ERR, " [-x] : exclude points larger / smaller than mag limits\n");
    gprint (GP_ERR, " [-v] : verbose mode\n");
    return (FALSE);
  }
  graphmode.style = KAPA_PLOT_POINTS; /* points */
  graphmode.size = -1; /* point size determined by Zvec */
  graphmode.etype = 0; /* no errorbars */

  Radius = MAX (fabs(graphmode.xmax), fabs(graphmode.ymax));

  /* load sky from correct table */
  sky = GetSkyTable ();
  skylist = SkyListByRadius (sky, -1, graphmode.coords.crval1, graphmode.coords.crval2, Radius);
  
  /* storage for plotting the points */
  Npts = 0;
  NPTS = 1000;
  ALLOCATE (Xvec, float, NPTS);
  ALLOCATE (Yvec, float, NPTS);
  ALLOCATE (Zvec, float, NPTS);

  // prepare to handle interrupt signals
  struct sigaction *old_sigaction = SetInterrupt();

  Nloaded = 0;
  for (j = 0; (j < skylist[0].Nregions) && !interrupt; j++) {
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = skylist[0].filename[j];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE;
    catalog.Nsecfilt = 0;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, VERBOSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    /* project stars to screen display coords */
    for (i = 0; (i < catalog.Naverage) && !interrupt; i++) {
      if (IDclip && (catalog.average[i].flags != IDchoice)) continue;
      catalog.average[i].R = ohana_normalize_angle (catalog.average[i].R);
      while (catalog.average[i].R < Rmin) catalog.average[i].R += 360.0;
      while (catalog.average[i].R > Rmax) catalog.average[i].R -= 360.0;
      m = catalog.average[i].measureOffset;
      for (k = 0; k < catalog.average[i].Nmeasure; k++) {
	if ((dbFlagClip == FLAG_SKIP) &&  (catalog.measure[m+k].dbFlags & dbFlagChoice)) continue;
	if ((dbFlagClip == FLAG_KEEP) && !(catalog.measure[m+k].dbFlags & dbFlagChoice)) continue;
	if ((photFlagClip == FLAG_SKIP) &&  (catalog.measure[m+k].photFlags & photFlagChoice)) continue;
	if ((photFlagClip == FLAG_KEEP) && !(catalog.measure[m+k].photFlags & photFlagChoice)) continue;
	if (TimeSelect && (catalog.measure[m+k].t < tzero)) continue;
	if (TimeSelect && (catalog.measure[m+k].t > tzero + trange)) continue;
	if ((PhotcodeClip != -1) && (catalog.measure[m+k].photcode != PhotcodeClip)) continue;
	mag = PhotCat (&catalog.measure[m+k], MAG_CLASS_PSF);
	Zvec[Npts] = MIN (1.0, MAX (0.01, (mag - Mz) / Mr));
	if (LimExclude && (Zvec[Npts] > 0.99)) continue;
	if (Zvec[Npts] < 0.011) continue;
	R = catalog.measure[m+k].R;
	D = catalog.measure[m+k].D;
	// XXX drop this check
	if ((R < Rmin) || (R > Rmax) || (D < -90.0) || (D > 90.0)) {
	  char *date;
	  date = ohana_sec_to_date (catalog.measure[m+k].t);
	  gprint (GP_LOG, "out: %f, %f : %s : (%f, %f) + (%f, %f)\n", R, D, date, catalog.average[i].R, catalog.average[i].D, catalog.measure[m+k].R, catalog.measure[m+k].D);
	  free (date);
	}
	status = fRD_to_XY (&Xvec[Npts], &Yvec[Npts], R, D, &graphmode.coords);
	if (!status) continue;
	Npts ++;

	if (Npts == NPTS - 1) {
	  NPTS += 1000;
	  REALLOCATE (Xvec, float, NPTS);
	  REALLOCATE (Yvec, float, NPTS);
	  REALLOCATE (Zvec, float, NPTS);
	}
	if ((Npts > NCHUNK) || (Nloaded >= 25)) {
	  KapaPrepPlot (kapa, Npts, &graphmode);
	  KapaPlotVector (kapa, Npts, Xvec, "x");
	  KapaPlotVector (kapa, Npts, Yvec, "y");
	  KapaPlotVector (kapa, Npts, Zvec, "z");
	  Npts = 0;
	  Nloaded = 0;
	}
      }
    }
    Nloaded ++;
    dvo_catalog_free (&catalog);
  }
  ClearInterrupt (old_sigaction);

  if (Npts > 0) {
    KapaPrepPlot (kapa, Npts, &graphmode);
    KapaPlotVector (kapa, Npts, Xvec, "x");
    KapaPlotVector (kapa, Npts, Yvec, "y");
    KapaPlotVector (kapa, Npts, Zvec, "z");
  }
  free (Xvec);
  free (Yvec);
  free (Zvec);

  return (TRUE);

}
  
