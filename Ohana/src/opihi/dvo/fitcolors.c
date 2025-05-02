# include "dvoshell.h"
# define NMIN_PTS 100

static void free_catalog (Catalog *catalog, int Ncatalog);

/* this function takes a photcode (and camera name?) and measures the  *
 * chip-to-chip slopes for all DEP photcodes equiv to the PRI/SEC code */

int fitcolors (int argc, char **argv) {
  
  int *list, Nlist;
  off_t i, k, m, N1, N2, i1, i2;
  int N, NP1, NP2, NP, Np, Npts, NPTS;
  int mode[4];
  int Nsecfilt, status;
  char *cmd, *outcmd, *camera;
  char name[64], filename[64], plotname[64], label[64];
  double *M1, *M2;
  float *out;
  float *colorFit, *deltaFit, dColor, C0, C1;
  opihi_flt minDelta, maxDelta, minColor, maxColor;
  int kapa, Npx, Npy, NPX, NPY, Nplot, PLOT;
  Graphdata graphdata;
  KapaSection section;

  Catalog *catalog;
  PhotCode **codelist, *tcode, *code[4];
  SkyList *skylist;
  SkyRegionSelection *selection;
  Vector *xvec, *yvec;
  Buffer *buf;

  /* defaults */
  catalog  = NULL;
  skylist  = NULL;
  selection = NULL;
  codelist = NULL;
  xvec = yvec = NULL;
  colorFit = NULL;
  deltaFit = NULL;

  /* load photcode information */
  if (!InitPhotcodes ()) goto escape;
  Nsecfilt = GetPhotcodeNsecfilt ();

  /* interpret command-line options */
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) goto escape;
  if (!SetPhotSelections (&argc, argv, 4)) goto usage;

  int textcolor = KapaColorByName ("black");

  // range for valid data points (exclude extreme outliers)
  minDelta = -0.2;
  maxDelta = +0.2;
  minColor = -1.0;
  maxColor = +3.0;
  if ((N = get_argument (argc, argv, "-color-range"))) {
    remove_argument (N, &argc, argv);
    minColor = atof (argv[N]);
    remove_argument (N, &argc, argv);
    maxColor = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-delta-range"))) {
    remove_argument (N, &argc, argv);
    minDelta = atof (argv[N]);
    remove_argument (N, &argc, argv);
    maxDelta = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  PLOT = FALSE;
  NPX = NPY = 0;
  if ((N = get_argument (argc, argv, "-plot"))) {
    remove_argument (N, &argc, argv);
    strcpy (plotname, argv[N]);
    remove_argument (N, &argc, argv);
    NPX = atof(argv[N]);
    remove_argument (N, &argc, argv);
    NPY = atof(argv[N]);
    remove_argument (N, &argc, argv);
    PLOT = TRUE;
  }

  /* interpret command-line options */
  if (argc != 4) goto usage;

  if (PLOT) {
    if (!GetGraph (&graphdata, &kapa, NULL)) return (FALSE);
    Nplot = 0;
    Npx = Npy = 0;
    graphdata.xmin = minColor;
    graphdata.xmax = maxColor;
    graphdata.ymin = minDelta;
    graphdata.ymax = maxDelta;
    graphmode.style = KAPA_PLOT_POINTS; /* points */
    graphmode.ptype = KAPA_POINT_CIRCLE_SOLID; /* connect pairs of points */
    KapaClearSections (kapa);
    KapaSetFont (kapa, "helvetica", 14);

    ALLOCATE (colorFit, float, 11);
    ALLOCATE (deltaFit, float, 11);
    dColor = (maxColor - minColor) / 10.0;
    for (i = 0; i < 11; i++) {
      colorFit[i] = minColor + i*dColor;
    }
  }

  /* determine relevant photcodes, colors */
  if (!(Np = GetPhotcodeCodebyName (argv[2]))) {
    gprint (GP_ERR, "ERROR: photcode not found in photcode table\n");
    goto usage;
  }
  camera = argv[3];

  /* reduce the list of codes */
  list = GetPhotcodeEquivList (Np, &Nlist);
  ALLOCATE (codelist, PhotCode *, Nlist);
  for (i = NP = 0; i < Nlist; i++) {
    tcode = GetPhotcodebyCode (list[i]);
    if (strncmp (tcode[0].name, camera, strlen(camera))) continue;
    codelist[NP] = tcode;
    NP++;
  }
  mode[0] = mode[1] = MAG_REL;  /* we should be applying any relative photometry corrections here */
  mode[2] = mode[3] = MAG_AVE;

  /* set the reference colors */
  code[2] = GetPhotcodebyCode (codelist[0][0].c1);
  code[3] = GetPhotcodebyCode (codelist[0][0].c2);
  if ((code[2] == NULL) || (code[3] == NULL)) goto color_missing;

  /* all codes must have the same colors (validate) */
  for (i = 0; i < NP; i++) {
    if (codelist[i][0].c1 != codelist[0][0].c1) goto color_mismatch;
    if (codelist[i][0].c2 != codelist[0][0].c2) goto color_mismatch;
  }
  gprint (GP_ERR, "using %d photcodes\n", NP);

  /* output is a named buffer */
  if ((buf = SelectBuffer (argv[1], ANYVECTOR, TRUE)) == NULL) goto usage;

  gfits_free_matrix (&buf[0].matrix);
  gfits_free_header (&buf[0].header);
  if (!CreateBuffer (buf, NP, NP, -32, 0.0, 1.0)) return FALSE;
  strcpy (buf[0].file, "(empty)");

  out = (float *) buf[0].matrix.buffer;
  /* we set a default flag value of -1 */
  for (i = 0; i < NP*NP; i++) {
    out[i] = -1;
  }

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (selection)) == NULL) goto escape;

  /* loop over regions, extract data for each region */
  ALLOCATE (catalog, Catalog, skylist[0].Nregions);
  for (k = 0; k < skylist[0].Nregions; k++) {
    /* lock, load, unlock catalog */
    dvo_catalog_init (&catalog[k], TRUE);
    catalog[k].filename = skylist[0].filename[k];
    catalog[k].catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog[k].Nsecfilt = 0;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog[k], NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog[k].filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog[k]);
    // XXX make a subset catalog consisting of only Average and Measure values which meet
    // the selection criteria
  }
  gprint (GP_ERR, "using "OFF_T_FMT" possible regions\n",  skylist[0].Nregions);

  /* vectors to save data */
  Npts = 0;
  NPTS = 64;
  if ((xvec = SelectVector ("tmp_x", ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((yvec = SelectVector ("tmp_y", ANYVECTOR, TRUE)) == NULL) goto escape;

  ResetVector (xvec, OPIHI_FLT, NPTS);
  ResetVector (yvec, OPIHI_FLT, NPTS);

  // set up basic windows
  if (PLOT) {
    Nplot = 0;
    Npx = Npy = 0;
    NPX = NPY = 6;
    KapaInitGraph (&graphdata);
    graphdata.xmin = minColor;
    graphdata.xmax = maxColor;
    graphdata.ymin = minDelta;
    graphdata.ymax = maxDelta;
    graphmode.style = KAPA_PLOT_POINTS; /* points */
    graphmode.ptype = KAPA_POINT_CIRCLE_SOLID; /* connect pairs of points */
  }

  struct sigaction *old_sigaction = SetInterrupt();

  /*** generate the color-color vectors for the pairs ***/
  // XXXX this function also needs to check for interrupts
  // XXX exclude obvious outliers (eg, fabs(dM) > 0.2)
  /* loop over chip photcode pairs */
  for (NP1 = 0; NP1 < NP; NP1++) {
    for (NP2 = NP1 + 1; NP2 < NP; NP2++) {
      code[0] = codelist[NP1];
      code[1] = codelist[NP2];
      
      /* extract all magnitude pairs from catalog tables */
      Npts = 0;
      for (k = 0; k < skylist[0].Nregions; k++) {
	if (catalog[k].Naverage == 0) continue;

	// gprint (GP_ERR, "seaching %s with %d stars\n", catalog[k].filename, catalog[k].Naverage);
	// ListPhotSelections ();

	/* get correct mags, convert to X,Y */
	for (i = 0; i < catalog[k].Naverage; i++) {
	  if (interrupt) goto escape;

	  M1 = M2 = NULL;
	  m = catalog[k].average[i].measureOffset;

	  SetSelectionParam (0);
	  M1 = ExtractDMag (&code[0], &mode[0], &catalog[k].average[i], &catalog[k].secfilt[i*Nsecfilt], &catalog[k].measure[m], &N1);
	  if (N1 == 0) goto skip_star;

	  SetSelectionParam (2);
	  M2 = ExtractDMag (&code[2], &mode[2], &catalog[k].average[i], &catalog[k].secfilt[i*Nsecfilt], &catalog[k].measure[m], &N2);
	  if (N2 == 0) goto skip_star;

	  for (i1 = 0; i1 < N1; i1++) {
	    for (i2 = 0; i2 < N2; i2++) {
	      if (M1[i1] < minDelta) continue;
	      if (M1[i1] > maxDelta) continue;
	      if (M2[i2] < minColor) continue;
	      if (M2[i2] > maxColor) continue;
	      yvec[0].elements.Flt[Npts] = M1[i1];
	      xvec[0].elements.Flt[Npts] = M2[i2];
	      Npts++;
	      if (Npts >= NPTS) {
		NPTS += 2000;
		REALLOCATE (xvec[0].elements.Flt, opihi_flt, NPTS);
		REALLOCATE (yvec[0].elements.Flt, opihi_flt, NPTS);
	      }
	    }
	  }
	skip_star:
	  if (M1 != NULL) free (M1);
	  if (M2 != NULL) free (M2);
	}
	// gprint (GP_ERR, "selected %d stars\n", Npts);
      }

      if (Npts < NMIN_PTS) continue;
      xvec[0].Nelements = Npts;
      yvec[0].Nelements = Npts;

      /* perform robust fit on dmag vs color */
      cmd = strcreate ("fit tmp_x tmp_y 1 -clip 3 3 -quiet");
      status = command (cmd, &outcmd, TRUE);
      if (outcmd != NULL) free (outcmd);
      
      C0 = get_double_variable ("C0", &status);
      C1 = get_double_variable ("C1", &status);
      
      /* do something useful with the results (stored in Cn, C0, C1, etc) */
      gprint (GP_LOG, "%s - %s : ", code[0][0].name, code[1][0].name);
      gprint (GP_LOG, "%7.4f %7.4f   %7.4f   ", 
	       C0, C1, get_double_variable ("dC", &status));
      gprint (GP_LOG, "%5s of %5d\n", get_variable ("Cnv"), Npts);
      out[NP1 + NP2*NP] = C1;

      // make an illustrating plot of each chip vs each other chip
      // each page should have, say, a 6x6 grid of plots. each one should show a single chip pair
      // as the page fills up, it gets written and a new one created.  
      if (PLOT) {
	sprintf (name, "s%02d.%02d", Npx, Npy);
	section.name = strcreate (name);
	if (Npx || Npy) {
	  section.dx = 0.9 / NPX;
	  section.dy = 0.9 / NPY;
	  section.x = 0.1 + Npx * section.dx;
	  section.y = 0.1 + Npy * section.dy;
	  strcpy (graphdata.labels, "0000");
	} 
	if (Npx == 0) {
	  section.dx = 0.9 / NPX + 0.1;
	  section.dy = 0.9 / NPY;
	  section.x = 0.0;
	  section.y = 0.1 + Npy * section.dy;
	  strcpy (graphdata.labels, "0100");
	}
	if (Npy == 0) {
	  section.dx = 0.9 / NPX;
	  section.dy = 0.9 / NPY + 0.1;
	  section.x = 0.1 + Npx * section.dx;
	  section.y = 0.0;
	  strcpy (graphdata.labels, "1000");
	}
	if (!Npx && !Npy) {
	  section.dx = 0.9 / NPX + 0.1;
	  section.dy = 0.9 / NPY + 0.1;
	  section.x = 0.0;
	  section.y = 0.0;
	  strcpy (graphdata.labels, "1100");
	}
	KapaSetSection (kapa, &section);
	KapaSetLimits (kapa, &graphdata);
	KapaBox (kapa, &graphdata);

	PlotVectorPair (kapa, xvec, yvec, NULL, &graphdata);

	for (i = 0; i < 11; i++) {
	  deltaFit[i] = C0 + C1*colorFit[i];
	}
	graphmode.style = KAPA_PLOT_CONNECT; /* lines */
	graphdata.color = KapaColorByName ("red");

	KapaPrepPlot (kapa, 11, &graphdata);
	KapaPlotVector (kapa, 11, colorFit, "x");
	KapaPlotVector (kapa, 11, deltaFit, "y");

	KapaSetFont (kapa, "helvetica", 8);
	sprintf (label, "%s", code[0][0].name);
	KapaSendTextline (kapa, label, 0.2*maxColor + 0.8*minColor, 0.8*maxDelta + 0.2*minDelta, 0.0, textcolor);
	sprintf (label, "%s", code[1][0].name);
	KapaSendTextline (kapa, label, 0.2*maxColor + 0.8*minColor, 0.2*maxDelta + 0.8*minDelta, 0.0, textcolor);
	KapaSetFont (kapa, "helvetica", 14);

	graphmode.style = KAPA_PLOT_POINTS; /* points */
	graphdata.color = KapaColorByName ("black");

	free (section.name);

	Npx++;
	if (Npx == NPX) {
	  Npx = 0;
	  Npy ++;
	  if (Npy == NPY) {
	    Npy = 0;
	    sprintf (filename, "%s.%02d.png", plotname, Nplot);
	    KapaPNG (kapa, filename);
	    KapaClearSections (kapa);
	    Nplot++;
	  }
	}
      }	
    }
  }
  if (skylist != NULL) free_catalog (catalog, skylist[0].Nregions);
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  if (codelist != NULL) free (codelist);
  if (colorFit != NULL) free (colorFit);
  if (deltaFit != NULL) free (deltaFit);
  ClearInterrupt (old_sigaction);
  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: chipcolors (output) (photcode) (camera)\n");
  goto escape;

color_missing:
  gprint (GP_ERR, "error: chips are missing a color reference\n");
  goto escape;

color_mismatch:
  gprint (GP_ERR, "error: all chips must have the same colors\n");
  goto escape;

escape:
  if (skylist != NULL) free_catalog (catalog, skylist[0].Nregions);
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  if (codelist != NULL) free (codelist);
  if (colorFit != NULL) free (colorFit);
  if (deltaFit != NULL) free (deltaFit);
  DeleteVector (xvec);
  DeleteVector (yvec);
  ClearInterrupt (old_sigaction);

  return (FALSE);
}

static void free_catalog (Catalog *catalog, int Ncatalog) {

  int i;

  if (catalog == NULL) return;
  for (i = 0; i < Ncatalog; i++) {
    dvo_catalog_free (&catalog[i]);
  }
  free (catalog);
}
