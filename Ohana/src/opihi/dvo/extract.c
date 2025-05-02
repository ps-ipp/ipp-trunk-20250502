# include "dvoshell.h"
// XXX EAM : this function is deprecated

enum {ZERO, RA, DEC, MAG, dMAG, Xm, Xp, NMEAS, NMISS, REF, TYPE, NPHOT, NCODE, FLAG};

# define NBYTES 160000
# define BYTES_STAR 23
# define BLOCK 1000
# define DNSTARS 1000

int extract (int argc, char **argv) {
  
  FILE *f;
  int i, Col, N, Nbytes, nbytes, NPTS;
  int InRegion, GSC, ASCII, LONEOS, mode, loadmode;
  int j, k, m, Nregions;
  float M0, m0;
  char filename[128];
  float Radius;
  char catdir[256], gscdir[256];
  int PhotcodeSelect;
  char PhotCodeFile[256], code[64];
  double ZERO_POINT;
  int Ns, N1, n1, Nsec;
  int Ngraph;
  int value;
  Vector *vec;
  PhotCodeData photcodes;
  Graphdata graphmode;
  Catalog catalog;
  RegionFile *regions;

  if (!GetGraphdata (&graphmode, NULL, NULL)) return (FALSE);
  if (!InitPhotcodes ()) return (FALSE);

  VarConfig ("GSCDIR", "%s", gscdir);
  VarConfig ("CATDIR", "%s", catdir);

  regions = (RegionFile *) NULL;
  ASCII = FALSE;
  LONEOS = TRUE;
  GSC = FALSE;
  if (N = get_argument (argc, argv, "-g")) {
    remove_argument (N, &argc, argv);
    GSC = TRUE;
    ASCII = FALSE;
    LONEOS = FALSE;
  }

  Col = 1;
  if (N = get_argument (argc, argv, "-a")) {
    remove_argument (N, &argc, argv);
    ASCII = TRUE;
    GSC = FALSE;
    LONEOS = FALSE;
    Col = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* check for region-based selection */
  code = NULL;
  PhotcodeSelect = FALSE;
  if (N = get_argument (argc, argv, "-photcode")) {
    PhotcodeSelect = True;
    remove_argument (N, &argc, argv);
    if ((code = GetPhotcodebyName (argv[N])) == NULL) {
      gprint (GP_ERR, "ERROR: photcode %s not found in photcode table\n", argv[N]);
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: extract (filename) (value) [-g / -a Ncol] \n");
    return (FALSE);
  }
  
  InRegion = FALSE;
  if (!strcmp (argv[1], "-all")) {
    InRegion = TRUE;
  }

  /* identify selection */
  if (LONEOS) {
    mode = ZERO;
    if (!strcasecmp (argv[2], "ra")) mode = RA;
    if (!strcasecmp (argv[2], "dec")) mode = DEC;
    if (!strcasecmp (argv[2], "mag")) mode = MAG;
    if (!strcasecmp (argv[2], "Nmeas")) mode = NMEAS;
    if (!strcasecmp (argv[2], "Nmiss")) mode = NMISS;
    if (!strcasecmp (argv[2], "Xp")) mode = Xp;
    if (!strcasecmp (argv[2], "Xm")) mode = Xm;
    if (!strcasecmp (argv[2], "dM")) mode = dMAG;
    if (!strcasecmp (argv[2], "flag")) mode = FLAG;
    if (!strcasecmp (argv[2], "ref")) mode = REF;
    if (!strcasecmp (argv[2], "type")) mode = TYPE;
    if (!strcasecmp (argv[2], "Nphot")) mode = NPHOT;
    if (!strcasecmp (argv[2], "Ncode")) mode = NCODE;
    if (mode == ZERO) {
      gprint (GP_ERR, "value may be one of the following:\n");
      gprint (GP_ERR, " ra dec mag Nmeas Nmiss Xp Xm ID\n");
      return (FALSE);
    }
  }

  if ((mode == REF) && !PhotcodeSelect) {
    gprint (GP_ERR, "must specify photcode for Reference\n");
    return (FALSE);
  }
  if ((mode == TYPE) && !PhotcodeSelect) {
    gprint (GP_ERR, "must specify photcode for Type\n");
    return (FALSE);
  }

  /* check photcode data / selection validity */
  Ns = -1;
  Nsec = GetPhotcodeNsecfilt ();
  if (PhotcodeSelect) {
    Ns = GetPhotcodeNsec (code[0].code);
    if ((mode != REF) && (code[0].type != PHOT_SEC)) {
      gprint (GP_ERR, "filter must be a average photometry type\n");
      return (FALSE);
    }
    if ((mode == REF) && (code[0].type != PHOT_REF)) {
      gprint (GP_ERR, "filter must be a REFERENCE photometry type\n");
      return (FALSE);
    }
  }

  if (GSC) {
    mode = ZERO;
    if (!strcasecmp (argv[2], "ra")) 
      mode = RA;
    if (!strcasecmp (argv[2], "dec")) 
      mode = DEC;
    if (!strcasecmp (argv[2], "mag")) 
      mode = MAG;
    if (mode == ZERO) {
      gprint (GP_ERR, "for GSC, value may be one of the following:\n");
      gprint (GP_ERR, " ra dec mag\n");
      return (FALSE);
    }
  }

  if ((vec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  if (InRegion) {
    Radius = MAX (fabs(graphmode.xmax), fabs(graphmode.ymax));
    regions = find_regions (graphmode.coords.crval1, graphmode.coords.crval2, Radius, &Nregions);
  } else {
    Nregions = 1;
  }
  
  /* create storage vector */
  NPTS = 1000;
  REALLOCATE (vec[0].elements, float, NPTS);
  vec[0].Nelements = N = 0;

  /* we loop over Nregions, but for ASCII Nregions = 1 */
  for (j = 0; j < Nregions; j++) {
    
    /* Load in data from an ASCII file list of ra, dec, mag */
    if (ASCII) {
      char *tbuffer;
      double tmp;
      float *V;
      
      f = fopen (argv[1], "r");
      if (f == (FILE *) NULL) {
	gprint (GP_ERR, "ERROR: can't open data file: %s\n", argv[1]);
	/* delete new vector! */
	return (FALSE);
      }
      ALLOCATE (tbuffer, char, 1024);
      
      V = vec[0].elements;
      while (scan_line (f, tbuffer) != EOF) {
	dparse (&tmp, Col, tbuffer);
	*V = tmp;
	V++;
	N++;
	if (N == NPTS - 1) {
	  NPTS += 1000;
	  REALLOCATE (vec[0].elements, float, NPTS);
	  V = &vec[0].elements[N];
	}
      }
      free (tbuffer);
      vec[0].Nelements = N;
      REALLOCATE (vec[0].elements, float, MAX(1,N));
      fclose (f);
      return (TRUE);
    }
    
    /* load data from the GSC files */
    if (GSC) {
      char *tbuffer;
      double tmp;
      float *V;
      
      if (InRegion) {
	sprintf (filename, "%s/%s\0", gscdir, regions[j].name);
      } else {
	sprintf (filename, "%s/%s\0", gscdir, argv[1]);
      }
      
      f = fopen (filename, "r");
      if (f == (FILE *) NULL) {
	gprint (GP_ERR, "no stars in %s, skipping\n", filename);
	continue;
      }
      ALLOCATE (tbuffer, char, (BLOCK*BYTES_STAR));
      
      V = &vec[0].elements[N];
      Nbytes = BLOCK*BYTES_STAR;
      while ((nbytes = fread (tbuffer, 1, Nbytes, f)) > 0) {
	for (i = 0; i < nbytes / BYTES_STAR; i++) {
	  if (mode == RA) {
	    dparse (&tmp, 1, &tbuffer[i*BYTES_STAR]);
	    *V = tmp;
	  }
	  if (mode == DEC) {
	    dparse (&tmp, 2, &tbuffer[i*BYTES_STAR]);
	    *V = tmp;
	  }
	  if (mode == MAG) {
	    dparse (&tmp, 3, &tbuffer[i*BYTES_STAR]);
	    *V = tmp;
	  }
	  V++;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	    V = &vec[0].elements[N];
	  }
	}
      }
      free (tbuffer);
      fclose (f);
    }
  
    /* load data from the photometry database files */
    if (LONEOS) {
      /* find and open correct file */
      if (InRegion) {
	sprintf (filename, "%s/%s\0", catdir, regions[j].name);
      } else {
	sprintf (filename, "%s/%s\0", catdir, argv[1]);
      }
      catalog.average = (Average *) NULL;
      catalog.measure = (Measure *) NULL;
      catalog.secfilt = (SecFilt *) NULL;
      loadmode = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;
      if ((mode == REF) || (mode == TYPE) || (mode == NPHOT) || (mode == NCODE)) 
	loadmode = loadmode | DVO_LOAD_MEASURE;

      /* lock, load, unlock catalog */
      catalog.filename = filename;
      switch (lock_catalog (&catalog, LCK_SOFT)) {
      case 2:
	unlock_catalog (&catalog);
      case 0:
	continue;
      }
      catalog.catflags = loadmode;
      if (!load_catalog (&catalog, TRUE)) {
	unlock_catalog (&catalog);
	continue;
      }
      unlock_catalog (&catalog);

      /* assign vector values */
      switch (mode) {
      case (RA):
	for (i = 0; i < catalog.Naverage; i++) {
	  vec[0].elements[N] = catalog.average[i].R;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (DEC):
	for (i = 0; i < catalog.Naverage; i++) {
	  vec[0].elements[N] = catalog.average[i].D;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (MAG):
	for (i = 0; i < catalog.Naverage; i++) {
	  M0 = (Ns == -1) ? catalog.average[i].M : catalog.secfilt[i*Nsec+Ns].M;
	  vec[0].elements[N] = M0 / 1000.0;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (NMEAS):
	for (i = 0; i < catalog.Naverage; i++) {
	  vec[0].elements[N] = catalog.average[i].Nm;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (NMISS):
	for (i = 0; i < catalog.Naverage; i++) {
	  vec[0].elements[N] = catalog.average[i].Nn;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (Xp):
	for (i = 0; i < catalog.Naverage; i++) {
	  /* Xp is scatter in 1/100 arcsec */
	  vec[0].elements[N] = 0.01*catalog.average[i].Xp;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (NCODE):
	for (i = 0; i < catalog.Naverage; i++) {
	  m = catalog.average[i].offset;
	  Ncode = 0;
	  for (k = 0; k < catalog.average[i].Nm; k++, m++) {
	    if (code[0].code != GetPhotcodeEquivCodebyCode (catalog.measure[m].photcode)) continue;
	    Ncode ++;
	  }
	  vec[0].elements[N] = Ncode;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (NPHOT):
	for (i = 0; i < catalog.Naverage; i++) {
	  m = catalog.average[i].offset;
	  Ncode = 0;
	  for (k = 0; k < catalog.average[i].Nm; k++, m++) {
	    if (code[0].code != GetPhotcodeEquivCodebyCode (catalog.measure[m].photcode)) continue;
	    if (catalog.measure[m].photcode & (ID_MEAS_POOR_PHOTOM | ID_MEAS_SKIP_PHOTOM)) continue;
	    Ncode ++;
	  }
	  vec[0].elements[N] = Ncode;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (Xm):
	for (i = 0; i < catalog.Naverage; i++) {
	  M0 = (Ns == -1) ? catalog.average[i].Xm : catalog.secfilt[i*Nsec+Ns].Xm;
	  vec[0].elements[N] = (M0 == NO_MAG) ? -1.0 : pow (10.0, 0.01*M0);
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (dMAG):
	for (i = 0; i < catalog.Naverage; i++) {
	  /* dM is 1000.0 * error */
	  M0 = (Ns == -1) ? catalog.average[i].dM : catalog.secfilt[i*Nsec+Ns].dM;
	  vec[0].elements[N] = (M0 == NO_MAG) ? -1.0 : 0.001*M0;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (FLAG):
	for (i = 0; i < catalog.Naverage; i++) {
	  vec[0].elements[N] = catalog.average[i].code;
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (REF):
	for (i = 0; i < catalog.Naverage; i++) {
	  m = catalog.average[i].offset;
	  vec[0].elements[N] = -32;
	  for (k = 0; k < catalog.average[i].Nm; k++) {
	    if (catalog.measure[m+k].photcode == N1) {
	      vec[0].elements[N] = PhotCat (&catalog.measure[m+k]);
	      k = catalog.average[i].Nm;
	    }
	  }
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      case (TYPE):
	for (i = 0; i < catalog.Naverage; i++) {
	  m = catalog.average[i].offset;
	  vec[0].elements[N] = DetermineTypeCode (&catalog.average[i], &catalog.measure[m], &photcodes, N1);
	  N++;
	  if (N == NPTS - 1) {
	    NPTS += 1000;
	    REALLOCATE (vec[0].elements, float, NPTS);
	  }
	}
	break;
      }
      if (catalog.average != 0) free (catalog.average);
      if (catalog.measure != 0) free (catalog.measure);
      if (catalog.secfilt != 0) free (catalog.secfilt);
    }
  }

  vec[0].Nelements = N;
  REALLOCATE (vec[0].elements, float, MAX(1,N));
  return (TRUE);

}
  
  /* USAGE: extract (what) (where) (from) [option] */
  /* examples:
     extract n0000/n0001 mag m 
     extract -g n0000/n0020 ra r 
     extract -all Xm xm
     extract fred dec d -a 2 */
  
