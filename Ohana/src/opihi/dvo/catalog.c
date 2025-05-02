# include "dvoshell.h"
# define NBYTES 160000
# define BYTES_STAR 23
# define BLOCK 1000
# define DNSTARS 1000

# define MAGSCALE 0
# define NUMSCALE 1
# define MISSCALE 2

// XXX EAM : should this function be dropped? 
int catlog (int argc, char **argv) {
  
  FILE *f;
  Catalog catalog;
  Vector Xvec, Yvec, Zvec;
  int i, N, Nm, Nn, NN, Nbytes, nbytes, Bytes_Star;
  int Ar, Ad, Am, InRegion, GSC, ASCII, DVO, FIXED;
  char filename[128];
  double Mz, Mr, Nz, Nr;
  int clip, mode, IDclip, IDchoice, LimExclude;
  RegionFile *regions;
  int j, Nregions;
  double Radius, Rmin, Rmax;
  Graphdata graphmode;
  double epoch, current_epoch;
  char gscdir[256], catdir[256];
  int Ngraph;

  if (!GetGraph (&graphmode, NULL, NULL)) return (FALSE);

  VarConfig ("GSCDIR", "%s", gscdir);
  VarConfig ("CATDIR", "%s", catdir);

  Mz = 17.0;
  Mr = -5.0;
  mode = MAGSCALE;
  clip = FALSE;
  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;

  regions = (RegionFile *) NULL;
  f = (FILE *) NULL;
  Nz = Nr = Am = Ar = Ad = 0;
  /* either MagScale or NumScale, whichever is first is scale */
  Nm = get_argument (argc, argv, "-m");
  Nn = get_argument (argc, argv, "+n");
  NN = get_argument (argc, argv, "-n");
  if (NN && Nn) {
    gprint (GP_ERR, "can't mix meas and miss scaling\n");
    return (FALSE);
  }
 
  if (Nm)
    mode = MAGSCALE;
  if (Nn)
    mode = NUMSCALE;
  if (NN)
    mode = MISSCALE;
    
  if (Nm && Nn) {
    clip = TRUE;
    if (Nm < Nn) 
      mode = MAGSCALE;
    else 
      mode = NUMSCALE;
  }
  if (Nm && NN) {
    clip = TRUE;
    if (Nm < NN) 
      mode = MAGSCALE;
    else 
      mode = MISSCALE;
  }
   
  current_epoch = 2000.0;
  epoch = 2000.0;
  if ((N = get_argument (argc, argv, "-e"))) {
    remove_argument (N, &argc, argv);
    epoch  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  IDchoice = 0;
  IDclip = FALSE;
  if ((N = get_argument (argc, argv, "-ID"))) {
    remove_argument (N, &argc, argv);
    IDchoice  = atof(argv[N]);
    remove_argument (N, &argc, argv);
    IDclip = TRUE;
  }

  LimExclude = FALSE;
  if ((N = get_argument (argc, argv, "-x"))) {
    remove_argument (N, &argc, argv);
    LimExclude = TRUE;
  }

  if ((Nm = get_argument (argc, argv, "-m"))) {
    remove_argument (Nm, &argc, argv);
    Mr  = atof(argv[Nm]);
    remove_argument (Nm, &argc, argv);
    Mz = atof(argv[Nm]);
    Mr = Mr - Mz;
    remove_argument (Nm, &argc, argv);
  }

  if ((Nn = get_argument (argc, argv, "+n"))) {
    remove_argument (Nn, &argc, argv);
    Nz  = atof(argv[Nn]);
    remove_argument (Nn, &argc, argv);
    Nr = atof(argv[Nn]) - Nz;
    remove_argument (Nn, &argc, argv);
  }

  if ((Nn = get_argument (argc, argv, "-n"))) {
    remove_argument (Nn, &argc, argv);
    Nz  = atof(argv[Nn]);
    remove_argument (Nn, &argc, argv);
    Nr = atof(argv[Nn]) - Nz;
    remove_argument (Nn, &argc, argv);
  }

  InRegion = FALSE;
  if ((N = get_argument (argc, argv, "-all"))) {
    remove_argument (N, &argc, argv);
    InRegion = TRUE;
  }

  Bytes_Star = 0;
  ASCII = FALSE;
  DVO = TRUE;
  GSC = FALSE;
  FIXED = FALSE;
  if ((N = get_argument (argc, argv, "-g"))) {
    remove_argument (N, &argc, argv);
    GSC = TRUE;
    ASCII = FALSE;
    DVO = FALSE;
  }

  if ((N = get_argument (argc, argv, "-a"))) {
    remove_argument (N, &argc, argv);
    ASCII = TRUE;
    GSC = FALSE;
    DVO = FALSE;
    Ar = atof(argv[N]);
    remove_argument (N, &argc, argv);
    Ad = atof(argv[N]);
    remove_argument (N, &argc, argv);
    Am = atof(argv[N]);
    remove_argument (N, &argc, argv);
    if ((N = get_argument (argc, argv, "-f"))) {
      remove_argument (N, &argc, argv);
      FIXED = TRUE;
      ASCII = FALSE;
      Bytes_Star = atof(argv[N]);
      remove_argument (N, &argc, argv);
    }
  }

  
  if ((InRegion || (argc != 2)) && (!InRegion || (argc != 1))) {
    gprint (GP_ERR, "USAGE: catalog (filename / -all) [-m M M] [-n N N] [-g] [-a RA DEC MAG] \n");
    return (FALSE);
  }
  
  if (InRegion) {
    Radius = MAX (fabs(graphmode.xmax), fabs(graphmode.ymax));
    regions = find_regions (graphmode.coords.crval1, graphmode.coords.crval2, Radius, &Nregions);
  } else {
    Nregions = 1;
  }
  
  for (j = 0; j < Nregions; j++) {
    catalog.average = 0;
    
    /* Load in data from an ASCII file list of ra, dec, mag */
    if (ASCII) {
      char *tbuffer;
      int nstar, NSTARS;
      double R, D, M;
      
      f = fopen (argv[1], "r");
      if (f == (FILE *) NULL) {
	gprint (GP_ERR, "ERROR: can't open catalog file: %s\n", argv[1]);
	return (FALSE);
      }
      
      nstar = 0;
      NSTARS = DNSTARS;
      ALLOCATE (tbuffer, char, 1024);
      ALLOCATE (catalog.average, Average, NSTARS);
      while (scan_line (f, tbuffer) != EOF) {
	dparse (&R, Ar, tbuffer);
	dparse (&D, Ad, tbuffer);
	dparse (&M, Am, tbuffer);
	catalog.average[nstar].R = R;
	catalog.average[nstar].D = D;
	catalog.average[nstar].M = M;
	nstar++;
	if (nstar == NSTARS - 1) {
	  NSTARS += DNSTARS;
	  REALLOCATE (catalog.average, Average, NSTARS);
	}
      }
      fclose (f);
      free (tbuffer);
      REALLOCATE (catalog.average, Average, nstar);
      catalog.Naverage = nstar;

      if (epoch != current_epoch) {
	cprecess (catalog.average, catalog.Naverage, epoch, current_epoch);
      }

    }
    
    /* Load in data from an ASCII file list of ra, dec, mag */
    if (FIXED) {
      char *tbuffer;
      int nstar, NSTARS;
      double R, D, M;
      
      f = fopen (argv[1], "r");
      if (f == (FILE *) NULL) {
	gprint (GP_ERR, "ERROR: can't open catalog file: %s\n", argv[1]);
	return (FALSE);
      }
      
      nstar = 0;
      NSTARS = DNSTARS;
      ALLOCATE (tbuffer, char, (BLOCK*Bytes_Star));
      ALLOCATE (catalog.average, Average, NSTARS);
      Nbytes = BLOCK*Bytes_Star;
      while ((nbytes = fread (tbuffer, 1, Nbytes, f)) > 0) {
	for (i = 0; i < nbytes / Bytes_Star; i++) {
	  dparse (&R, Ar, &tbuffer[i*Bytes_Star]);
	  dparse (&D, Ad, &tbuffer[i*Bytes_Star]);
	  dparse (&M, Am, &tbuffer[i*Bytes_Star]);
	  catalog.average[nstar].R = R;
	  catalog.average[nstar].D = D;
	  catalog.average[nstar].M = M;
	  nstar++;
	  if (nstar == NSTARS - 1) {
	    NSTARS += DNSTARS;
	    REALLOCATE (catalog.average, Average, NSTARS);
	  }
	}
      }
      fclose (f);
      free (tbuffer);
      REALLOCATE (catalog.average, Average, nstar);
      catalog.Naverage = nstar;

      if (epoch != current_epoch) {
	cprecess (catalog.average, catalog.Naverage, epoch, current_epoch);
      }

    }
    
    /* load data from the GSC files */
    if (GSC) {
      char *tbuffer;
      int nstar, NSTARS;
      double R, D, M;
      
      if (InRegion) {
	sprintf (filename, "%s/%s", gscdir, regions[j].name);
      } else {
	sprintf (filename, "%s/%s", gscdir, argv[1]);
      }
      
      f = fopen (filename, "r");
      if (f == (FILE *) NULL) {
	gprint (GP_ERR, "no stars in %s, skipping\n", filename);
	continue;
	/* return (FALSE); */
      }
      
      nstar = 0;
      NSTARS = DNSTARS;
      ALLOCATE (tbuffer, char, (BLOCK*BYTES_STAR));
      ALLOCATE (catalog.average, Average, NSTARS);
      Nbytes = BLOCK*BYTES_STAR;
      while ((nbytes = fread (tbuffer, 1, Nbytes, f)) > 0) {
	for (i = 0; i < nbytes / BYTES_STAR; i++) {
	  dparse (&R, 1, &tbuffer[i*BYTES_STAR]);
	  dparse (&D, 2, &tbuffer[i*BYTES_STAR]);
	  dparse (&M, 3, &tbuffer[i*BYTES_STAR]);
	  catalog.average[nstar].R = R;
	  catalog.average[nstar].D = D;
	  catalog.average[nstar].M = M;
	  nstar++;
	  if (nstar == NSTARS - 1) {
	    NSTARS += DNSTARS;
	    REALLOCATE (catalog.average, Average, NSTARS);
	  }
	}
      }
      fclose (f);
      free (tbuffer);
      REALLOCATE (catalog.average, Average, nstar);
      catalog.Naverage = nstar;
    }
  
    /* load data from the photometry database files */
    if (DVO) {
      
      if (InRegion) {
	sprintf (filename, "%s/%s", catdir, regions[j].name);
      } else {
	sprintf (filename, "%s/%s", catdir, argv[1]);
      }
      
      /* lock, load, unlock catalog */
      dvo_catalog_init (&catalog, TRUE);
      catalog.filename = filename;
      catalog.catflags = DVO_LOAD_AVERAGE;

      // an error exit status here is a significant error
      if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
	  fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
	  exit (2);
      }
      dvo_catalog_unlock (&catalog);
    }
    
    /* data has been loaded, get ready to plot it */
    Xvec.Nelements = catalog.Naverage;
    Yvec.Nelements = catalog.Naverage;
    Zvec.Nelements = catalog.Naverage;
    ALLOCATE (Xvec.elements, float, Xvec.Nelements);
    ALLOCATE (Yvec.elements, float, Yvec.Nelements);
    ALLOCATE (Zvec.elements, float, Zvec.Nelements);
    /* project stars to screen display coords */
    Xvec.Nelements = 0;
    switch (mode) {
    case (MAGSCALE):
      for (N = i = 0; i < catalog.Naverage; i++) {
	if (clip && ((catalog.average[i].Nm < Nz) || (catalog.average[i].Nm > Nr+Nz))) 
	  continue;
	if (IDclip && (catalog.average[i].code != IDchoice))
	  continue;
	Zvec.elements[N] = MIN (1.0, MAX (0.01, (catalog.average[i].M - Mz) / Mr));
	if (LimExclude && (Zvec.elements[N] > 0.99)) continue;
	if (Zvec.elements[N] < 0.011) continue;
	catalog.average[i].R = ohana_normalize_angle (catalog.average[i].R);
	while (catalog.average[i].R < Rmin) catalog.average[i].R += 360.0;
	while (catalog.average[i].R > Rmax) catalog.average[i].R -= 360.0;
	if (fRD_to_XY (&Xvec.elements[N], &Yvec.elements[N], catalog.average[i].R, catalog.average[i].D, &graphmode.coords)) N ++;
      }
      break;
    case (NUMSCALE):
      for (N = i = 0; i < catalog.Naverage; i++) {
	if (clip && ((catalog.average[i].M > Mz) || (catalog.average[i].M < Mr+Mz))) 
	  continue;
	if (IDclip && (catalog.average[i].code != IDchoice))
	  continue;
	Zvec.elements[N] = MIN (1.0, MAX (0.01, (catalog.average[i].Nm - Nz) / Nr));
	if (LimExclude && (Zvec.elements[N] == 1.0)) continue;
	if (Zvec.elements[N] == 0.01) 
	  continue;
	catalog.average[i].R = ohana_normalize_angle (catalog.average[i].R);
	while (catalog.average[i].R < Rmin) catalog.average[i].R += 360.0;
	while (catalog.average[i].R > Rmax) catalog.average[i].R -= 360.0;
	if (fRD_to_XY (&Xvec.elements[N], &Yvec.elements[N], catalog.average[i].R, catalog.average[i].D, &graphmode.coords)) N++;
      }
      break;
    case (MISSCALE):
      for (N = i = 0; i < catalog.Naverage; i++) {
	if (clip && ((catalog.average[i].M > Mz) || (catalog.average[i].M < Mr+Mz))) 
	  continue;
	if (IDclip && (catalog.average[i].code != IDchoice))
	  continue;
	Zvec.elements[N] = MIN (1.0, MAX (0.01, (catalog.average[i].Nn - Nz) / Nr));
	if (LimExclude && (Zvec.elements[N] == 1.0)) continue;
	if (Zvec.elements[N] == 0.01) 
	  continue;
	catalog.average[i].R = ohana_normalize_angle (catalog.average[i].R);
	while (catalog.average[i].R < Rmin) catalog.average[i].R += 360.0;
	while (catalog.average[i].R > Rmax) catalog.average[i].R -= 360.0;
	if (fRD_to_XY (&Xvec.elements[N], &Yvec.elements[N], catalog.average[i].R, catalog.average[i].D, &graphmode.coords)) N++;
      }
      break;
    }

    Zvec.Nelements = Yvec.Nelements = Xvec.Nelements = N;
    REALLOCATE (Xvec.elements, float, MAX (Xvec.Nelements, 1));
    REALLOCATE (Yvec.elements, float, MAX (Yvec.Nelements, 1));
    REALLOCATE (Zvec.elements, float, MAX (Zvec.Nelements, 1));
    
    graphmode.style = KAPA_PLOT_POINTS; /* points */
    graphmode.size = -1; /* point size determined by Zvec */
    graphmode.etype = 0; /* no errorbars */
    PrepPlotting (N, &graphmode);
    
    PlotVector (N, Xvec.elements, "x");
    PlotVector (N, Yvec.elements, "y");
    PlotVector (N, Zvec.elements, "z");
    
    free (Xvec.elements);
    free (Yvec.elements);
    free (Zvec.elements);

    if (catalog.average != 0) free (catalog.average);

  }
  return (TRUE);

}
  
