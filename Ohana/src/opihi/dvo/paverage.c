# include "dvoshell.h"
# define NCHUNK 10000

int paverage (int argc, char **argv) {
  
  off_t i, j;
  int kapa, Narg, Npts, NPTS, status, VERBOSE;
  int Nsecfilt, Nsec, Nloaded;
  double Mz, Mr, mag;
  double Radius, Rmin, Rmax, R, D;
  unsigned IDclip, IDchoice, LimExclude;
  float *Xvec, *Yvec, *Zvec;

  PhotCode *photcode;
  SkyTable *sky;
  SkyList *skylist;
  Catalog catalog;
  Graphdata graphmode;
  Average *average;
  SecFilt *secfilt;

  if (!InitPhotcodes ()) return (FALSE);
  Nsecfilt = GetPhotcodeNsecfilt ();

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  Mz = 17.0;
  Mr = -5.0;
  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;

  // require a photcode?  default to 0?
  Nsec = 0;
  if ((Narg = get_argument (argc, argv, "-p"))) {
    remove_argument (Narg, &argc, argv);
    photcode = GetPhotcodebyName (argv[Narg]);
    if (!photcode) {
      fprintf (stderr, "unknown photcode %s\n", argv[Narg]);
      return (FALSE);
    }
    remove_argument (Narg, &argc, argv);
    Nsec = GetPhotcodeNsec (photcode[0].code);
    if (Nsec == -1) {
      fprintf (stderr, "photcode %s is not an AVERAGE photcode\n", argv[Narg]);
      return (FALSE);
    }
  }

  IDchoice = 0;
  IDclip = FALSE;
  if ((Narg = get_argument (argc, argv, "-ID"))) {
    remove_argument (Narg, &argc, argv);
    IDchoice  = atoi(argv[Narg]);
    remove_argument (Narg, &argc, argv);
    IDclip = TRUE;
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
    gprint (GP_ERR, "USAGE: paverage (-all) [-m M M] [-p photcode] [-ID ID] [-flag value] [-x]\n");
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
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt = Nsecfilt;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, VERBOSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    average = catalog.average;
    secfilt = catalog.secfilt;

    /* project stars to screen display coords */
    for (i = 0; (i < catalog.Naverage) && !interrupt; i++) {
      if (IDclip && (average[i].flags != IDchoice)) continue;
      average[i].R = ohana_normalize_angle (average[i].R);
      while (average[i].R < Rmin) average[i].R += 360.0;
      while (average[i].R > Rmax) average[i].R -= 360.0;

      mag = secfilt[i*Nsecfilt+Nsec].MpsfChp;
      Zvec[Npts] = MIN (1.0, MAX (0.01, (mag - Mz) / Mr));
      if (LimExclude && (Zvec[Npts] > 0.99)) continue;
      if (Zvec[Npts] < 0.011) continue;
      R = average[i].R;
      D = average[i].D;
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
  
