# include "dvoshell.h"

typedef struct {
  float *mags;
  float color;
  float Temp;
  float Av;
} SEDtableRow;

typedef struct {
  float chisq;
  float Md;
  int row;
} SEDfit;

SEDtableRow **sort_SEDtable (SEDtableRow *raw, int N);
int SEDcolorBracket (SEDtableRow **table, int Ntable, float color);
SEDfit SEDchisq (SEDtableRow *ref, SEDtableRow *data, SEDtableRow *error, int Nfilter);

/* this function takes a photcode (and camera name?) and measures the  *
 * chip-to-chip slopes for all DEP photcodes equiv to the PRI/SEC code */

int fitsed (int argc, char **argv) {
  
  int *hashcode;
  off_t i, j, k, m;
  int N, done, Nfit;
  int status;
  char name[64], line[1024], key[20];
  float *fitmags, *fiterrs, *wavecode, *vegaToAB;
  float color;
  double X, Y, ZP, RA, DEC;
  int kapa, PLOT;
  int Nrow, NROW, idx, Nfilter, start, row;
  unsigned short colorP, colorM, code, USNOred, USNOblu;
  int codeP, codeM;
  FILE *f;

  Graphdata graphdata;
  KapaSection magSection, resSection;

  Catalog catalog;
  SkyList *skylist;
  SkyRegionSelection *selection;

  SEDtableRow *SEDtableRaw, **SEDtable;
  SEDtableRow sourceValue, sourceError;
  SEDfit minFit, testFit;
  struct sigaction *old_sigaction = NULL;

  /* defaults */
  skylist  = NULL;
  selection = NULL;

  catalog.average = NULL;
  catalog.measure = NULL;
  catalog.secfilt = NULL;

  // outcat.average = NULL;
  // outcat.measure = NULL;
  // outcat.secfilt = NULL;

  SEDtable = NULL;
  SEDtableRaw = NULL;
  sourceValue.mags = NULL;
  sourceError.mags = NULL;
  wavecode = NULL;
  hashcode = NULL;
  magSection.name = NULL;
  resSection.name = NULL;

  Nrow = 0;

  fiterrs = NULL;
  fitmags = NULL;

  /* load photcode information */
  if (!InitPhotcodes ()) goto escape;

  /* interpret command-line options */
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) goto escape;
  // if (!SetPhotSelections (&argc, argv, 4)) goto usage;

  PLOT = FALSE;
  if ((N = get_argument (argc, argv, "-plot"))) {
    remove_argument (N, &argc, argv);
    PLOT = TRUE;
  }

  /* interpret command-line options */
  if (argc != 6) goto usage;

  Nfit = 0;
  colorP = GetPhotcodeCodebyName (argv[3]);
  colorM = GetPhotcodeCodebyName (argv[5]);
  if (!colorP || !colorM) goto color_undefined;

  // artificially set USNOred and blu errors to 0.3
  USNOred = GetPhotcodeCodebyName ("USNO_RED");
  USNOblu = GetPhotcodeCodebyName ("USNO_BLUE");

  // load SED table
  f = fopen (argv[1], "r");
  if (f == NULL) goto table_missing;

  // XXX add error checks for header data
  scan_line (f, line);
  sscanf (line, "%*s %*s %d", &Nfilter);

  // load SED table photcodes, generate the photcode hashtable
  ALLOCATE (hashcode, int, 0x10000);
  ALLOCATE (wavecode, float, Nfilter);
  ALLOCATE (vegaToAB, float, Nfilter);

  for (i = 0; i < 0x10000; i++) hashcode[i] = -1;
  for (i = 0; i < Nfilter; i++) {
    scan_line (f, line);
    sscanf (line, "%*s %s %f %f", name, &wavecode[i], &vegaToAB[i]);
    code = GetPhotcodeCodebyName (name);
    if (code == 0) goto code_missing;
    hashcode[code] = i;
  }
  codeP = hashcode[colorP];
  codeM = hashcode[colorM];
  if ((codeP == -1) || (codeM == -1)) goto color_missing;
    
  // skip remaining header lines
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  
  // load the SED table data
  Nrow = 0;
  NROW = 100;
  ALLOCATE (SEDtableRaw, SEDtableRow, NROW);
  while (scan_line(f, line) != EOF) {
    fparse (&SEDtableRaw[Nrow].Temp, 1, line);
    fparse (&SEDtableRaw[Nrow].Av, 2, line);
    ALLOCATE (SEDtableRaw[Nrow].mags, float, Nfilter);
    for (i = 0; i < Nfilter; i++) {
      fparse (&SEDtableRaw[Nrow].mags[i], i + 3, line);
    }
    SEDtableRaw[Nrow].color = SEDtableRaw[Nrow].mags[codeP] - SEDtableRaw[Nrow].mags[codeM];
    Nrow ++;
    CHECK_REALLOCATE (SEDtableRaw, SEDtableRow, NROW, Nrow, 100);
  }      

  // sort the SEDtable by the reference colors
  SEDtable = sort_SEDtable (SEDtableRaw, Nrow);

  // create holder for the source data
  ALLOCATE (sourceValue.mags, float, Nfilter);
  ALLOCATE (sourceError.mags, float, Nfilter);

  if (PLOT) {
    if (!GetGraph (&graphdata, &kapa, NULL)) return (FALSE);
    SetLimitsRaw (wavecode, NULL, Nfilter, &graphdata);
    graphdata.style = KAPA_PLOT_POINTS; /* points */
    graphdata.ptype = KAPA_POINT_CIRCLE_SOLID;
    KapaClearSections (kapa);
    magSection.name = strcreate ("mag");
    magSection.x  = 0;
    magSection.dx = 1;
    magSection.y  = 0.5;
    magSection.dy = 0.5;
    resSection.name = strcreate ("res");
    resSection.x  = 0.0;
    resSection.dx = 1.0;
    resSection.y  = 0.0;
    resSection.dy = 0.5;
    
    KapaSetFont (kapa, "helvetica", 14);
    ALLOCATE (fitmags, float, Nfilter);
    ALLOCATE (fiterrs, float, Nfilter);
  }

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (selection)) == NULL) goto escape;

  /* loop over regions, extract data for each region */
  // XXX add interrupt checks
  old_sigaction = SetInterrupt();
  gprint (GP_ERR, "using "OFF_T_FMT" possible regions\n",  skylist[0].Nregions);
  for (k = 0; (k < skylist[0].Nregions) && !interrupt; k++) {
    /* lock, load, unlock catalog */
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = skylist[0].filename[k];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt = 0;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    // perform the fit to all sources
    for (i = 0; i < catalog.Naverage; i++) {

      // blank out the source array
      for (j = 0; j < Nfilter; j++) {
	sourceValue.mags[j] = 100;
      }	

      // load the measurements for this source
      m = catalog.average[i].measureOffset;
      for (j = 0; j < catalog.average[i].Nmeasure; j++) {
	idx = hashcode[catalog.measure[m+j].photcode];
	if (idx == -1) continue;
	// XXX do something more clever if more than one value exists per photcode
	sourceValue.mags[idx] = catalog.measure[m+j].M + vegaToAB[idx];
	sourceError.mags[idx] = catalog.measure[m+j].dM;
	if ((catalog.measure[m+j].photcode == USNOred) || (catalog.measure[m+j].photcode == USNOblu)) {
	  sourceError.mags[idx] = 0.3;
	}
      }

      // XXX for the moment, skip sources without ref color
      if (sourceValue.mags[codeP] > 50) continue;
      if (sourceValue.mags[codeM] > 50) continue;
      color = sourceValue.mags[codeP] - sourceValue.mags[codeM];

      // XXX find tableRow within 0.1 mag of color
      start = SEDcolorBracket (SEDtable, Nrow, color);
      minFit = SEDchisq (SEDtable[start], &sourceValue, &sourceError, Nfilter);
      minFit.row = start;

      // search for min chisq backwards
      done = FALSE;
      row = start - 1;
      while (!done && (row > 0)) {
	testFit = SEDchisq (SEDtable[row], &sourceValue, &sourceError, Nfilter);
	if (testFit.chisq < minFit.chisq) {
	  minFit = testFit;
	  minFit.row = row;
	}
	if (fabs(SEDtable[row][0].color - color) > 0.5) done = TRUE;
	row --;
      }

      // search for min chisq forwards
      done = FALSE;
      row = start + 1;
      while (!done && (row < Nrow)) {
	testFit = SEDchisq (SEDtable[row], &sourceValue, &sourceError, Nfilter);
	if (testFit.chisq < minFit.chisq) {
	  minFit = testFit;
	  minFit.row = row;
	}
	if (fabs(SEDtable[row][0].color - color) > 0.5) done = TRUE;
	row ++;
      }

      Nfit ++;
      // create the vectors for the example plots
      if (PLOT) {
	// find plot range
	SetLimitsRaw (NULL, SEDtable[minFit.row][0].mags, Nfilter, &graphdata);
	SWAP (graphdata.ymin, graphdata.ymax);

	KapaClearSections (kapa);
	KapaSetSection (kapa, &magSection);
    	KapaSetLimits (kapa, &graphdata);
	KapaBox (kapa, &graphdata);
	graphdata.color = KapaColorByName ("blue");
	graphdata.etype = 0;
	KapaPrepPlot (kapa, Nfilter, &graphdata);
	KapaPlotVector (kapa, Nfilter, wavecode, "x");
	KapaPlotVector (kapa, Nfilter, SEDtable[minFit.row][0].mags, "y");
	graphdata.color = KapaColorByName ("red");
	graphdata.etype = 1;
	for (j = 0; j < Nfilter; j++) {
	  fitmags[j] = 100;
	  fiterrs[j] = 0;
	  if (sourceValue.mags[j] > 50) continue;
	  fitmags[j] = sourceValue.mags[j] - minFit.Md;
	  fiterrs[j] = sourceError.mags[j];
	}
	KapaPrepPlot (kapa, Nfilter, &graphdata);
	KapaPlotVector (kapa, Nfilter, wavecode, "x");
	KapaPlotVector (kapa, Nfilter, fitmags, "y");
	KapaPlotVector (kapa, Nfilter, fiterrs, "dym");
	KapaPlotVector (kapa, Nfilter, fiterrs, "dyp");
	KapaSendLabel (kapa, "model,fit (mags)", 1);

	sprintf (line, "star: %10.6f %10.6f  T: %5.0fK  A_V|: %4.2f  M_D|: %5.2f  &sc&h^2|: %5.2f", 
		 catalog.average[i].R, catalog.average[i].D, 
		 SEDtable[minFit.row][0].Temp, SEDtable[minFit.row][0].Av, minFit.Md, minFit.chisq);
	KapaSendLabel (kapa, line, 2);
	KapaSendLabel (kapa, "model,fit (mags)", 1);

	KapaSetSection (kapa, &resSection);
	graphdata.ymin = -1.0;
	graphdata.ymax = +1.0;
    	KapaSetLimits (kapa, &graphdata);
	KapaBox (kapa, &graphdata);
	graphdata.color = KapaColorByName ("red");
	graphdata.etype = 1;

	for (j = 0; j < Nfilter; j++) {
	  fitmags[j] = 100;
	  fiterrs[j] = 0;
	  if (sourceValue.mags[j] > 50) continue;
	  fitmags[j] = sourceValue.mags[j] - minFit.Md - SEDtable[minFit.row][0].mags[j];
	  fiterrs[j] = sourceError.mags[j];
	}
	KapaPrepPlot (kapa, Nfilter, &graphdata);
	KapaPlotVector (kapa, Nfilter, wavecode, "x");
	KapaPlotVector (kapa, Nfilter, fitmags, "y");
	KapaPlotVector (kapa, Nfilter, fiterrs, "dym");
	KapaPlotVector (kapa, Nfilter, fiterrs, "dyp");
	KapaSendLabel (kapa, "wavelength (nm)", 0);
	KapaSendLabel (kapa, "resid (mags)", 1);

	KiiCursorOn (kapa);
	while (KiiCursorRead (kapa, &X, &Y, &ZP, &RA, &DEC, key)) {
	  gprint (GP_ERR, "window: %f %f (%s)\n", X, Y, key);
	  if (!strcasecmp (key, "Q")) {
	    KiiCursorOff (kapa);
	    break;
	  }
	  if (!strcasecmp (key, "ESCAPE")) {
	    KiiCursorOff (kapa);
	    goto escape;
	  }
	}
      }
      // we now have the min chisq row. use this to supply the other filter values....
    }
    dvo_catalog_free (&catalog);
  }
  gprint (GP_ERR, "fitted %d stars\n", Nfit);
  status = TRUE;
  goto finish;
  
usage:
  gprint (GP_ERR, "USAGE: fitset (sedtable) : (F) - (F)\n");
  goto escape;

table_missing:
  gprint (GP_ERR, "ERROR: can't open the SED table\n");
  goto escape;

color_missing:
  gprint (GP_ERR, "ERROR: reference color not in SED table\n");
  goto escape;

color_undefined:
  gprint (GP_ERR, "ERROR: undefined photcode in reference color\n");
  goto escape;

code_missing:
  gprint (GP_ERR, "ERROR: undefined photcode in SED table\n");
  goto escape;

escape:
  status = FALSE;
  goto finish;

finish:
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  if (wavecode != NULL) free (wavecode);
  if (hashcode != NULL) free (hashcode);
  if (SEDtableRaw != NULL) {
    for (i = 0; i < Nrow; i++) {
      free (SEDtableRaw[i].mags);
    }
    free (SEDtableRaw);
  }
  if (SEDtable != NULL) free (SEDtable);
  if (sourceValue.mags != NULL) free (sourceValue.mags);
  if (sourceError.mags != NULL) free (sourceError.mags);

  ClearInterrupt (old_sigaction);
  return (status);
}

// fit the data (with errors) to the given table row
SEDfit SEDchisq (SEDtableRow *ref, SEDtableRow *data, SEDtableRow *error, int Nfilter) {

  int i;
  double Sm, Sd, S2, wt, dM;
  SEDfit fit;

  Sm = Sd = S2 = 0.0;

  for (i = 0; i < Nfilter; i++) {
    if (data[0].mags[i] > 50.0) continue;

    if (error[0].mags[i] == 0.0) {
      wt = 1.0;
    } else {
      wt = 1.0 / SQ(error[0].mags[i]);
    }

    dM = data[0].mags[i] - ref[0].mags[i];
    S2 += SQ(dM) * wt;
    Sm += dM * wt;
    Sd += wt;
  }
    
  // row is assigned after fit
  fit.row = -1;
  fit.Md = Sm / Sd;
  fit.chisq = S2 + SQ(fit.Md) * Sd - 2*fit.Md*Sm;

  return (fit);
}

// find the first table row within 0.1 mag of the requested color (or within 10)
int SEDcolorBracket (SEDtableRow **table, int Ntable, float color) {

  int Nlo, Nhi, N;
  float tcolor;

  N = Nlo = 0; Nhi = Ntable;
  tcolor = table[Nlo][0].color;
  while ((Nhi - Nlo > 10) && (fabs(tcolor-color) > 0.1)) {
    N = 0.5*(Nlo + Nhi);
    N = MAX (N, 0);
    N = MIN (N, Ntable - 1);
    tcolor = table[N][0].color;
    if (tcolor < color) {
      Nlo = N;
    } else {
      Nhi = N + 1;
    }
  }
  return (N);
}

SEDtableRow **sort_SEDtable (SEDtableRow *raw, int N) {

  int i;
  SEDtableRow **value;
  
  if (N <= 0) return (NULL);

  ALLOCATE (value, SEDtableRow *, N);
  for (i = 0; i < N; i++) {
    value[i] = &raw[i];
  }

# define SWAPFUNC(A,B){ SEDtableRow *temp = value[A]; value[A] = value[B]; value[B] = temp; }
# define COMPARE(A,B)(value[A][0].color < value[B][0].color)

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

  return (value);
}

