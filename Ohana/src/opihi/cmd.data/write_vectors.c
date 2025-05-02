# include "data.h"

int write_vectors (int argc, char **argv) {
  
  int i, j, Nvec, Ne, N;
  FILE *f;
  char **fmtlist, *fmttype;
  char *p0, *p1, *p2, *format;
  Vector **vec;

  if ((N = get_argument (argc, argv, "-h"))) goto usage;
  if ((N = get_argument (argc, argv, "--h"))) goto usage;
  if ((N = get_argument (argc, argv, "-help"))) goto usage;
  if ((N = get_argument (argc, argv, "--help"))) goto usage;

  /* look for format option */
  format = (char *) NULL;
  if ((N = get_argument (argc, argv, "-f"))) {
    remove_argument (N, &argc, argv);
    format = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-format"))) {
    if (format) {
      gprint (GP_ERR, "ERROR: do not mix -f and -format\n");
      free (format);
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    format = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // option generate a FITS output table (FITS holds the filename) 
  // in FITS output context, -header is interpretted as a buffer containing
  // header keywords to supplement the FITS table header
  char *FITS = NULL;
  Header *fitsheader = NULL;
  Buffer *headbuffer = NULL;
  if ((N = get_argument (argc, argv, "-fits"))) {
    remove_argument (N, &argc, argv);
    FITS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);

    if ((N = get_argument (argc, argv, "-header"))) {
      remove_argument (N, &argc, argv);
      if ((headbuffer = SelectBuffer (argv[N], OLDBUFFER, TRUE)) == NULL) return (FALSE);
      fitsheader = &headbuffer->header;
      remove_argument (N, &argc, argv);
    }
  }
  
  /* option generate a FITS output table */
  int CSV = FALSE;
  if ((N = get_argument (argc, argv, "-csv"))) {
    if (format) {
      gprint (GP_ERR, "ERROR: do not mix -csv and -format\n");
      free (format);
      return (FALSE);
    }
    if (FITS) {
      gprint (GP_ERR, "ERROR: do not mix -csv and -fits\n");
      free (FITS);
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    CSV = TRUE;
  }

  int append = FALSE;
  if ((N = get_argument (argc, argv, "-append"))) {
    remove_argument (N, &argc, argv);
    append = TRUE;
  }

  char *compress = NULL;
  if ((N = get_argument (argc, argv, "-compress-mode"))) {
    remove_argument (N, &argc, argv);
    compress = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  } else if ((N = get_argument (argc, argv, "-compress"))) {
    remove_argument (N, &argc, argv);
    compress = strcreate("GZIP_1");
    if (!FITS) {
      fprintf (stderr, "NOTE: write_vectors -compress has no effect on non-FITS\n");
    }
  }

  int Ntile = 0;
  if ((N = get_argument (argc, argv, "-compress-Ntile"))) {
    remove_argument (N, &argc, argv);
    Ntile = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int ADD_HEADER = FALSE;
  if ((N = get_argument (argc, argv, "-header"))) {
    remove_argument (N, &argc, argv);
    ADD_HEADER = TRUE;
  }

  if (argc < 3) {
    gprint (GP_ERR, "USAGE: write [options] file vector vector ...\n");
    gprint (GP_ERR, "OPTIONS: [-header [buf]] [-append] [-f \"format\"] [-fits NAME] [-csv] [-h,--help]\n");
    return (FALSE);
  }

  /* find number of output vectors */
  Nvec = (argc - 2);
  if (Nvec < 1) {
    gprint (GP_ERR, "USAGE: write (file) vector vector ...\n");
    return (FALSE);
  }
  ALLOCATE (vec, Vector *, Nvec);

  /* select/check vectors from list */
  for (i = 0; i < Nvec; i++) {
    if ((vec[i] = SelectVector (argv[i + 2], OLDVECTOR, FALSE)) == NULL) {
      gprint (GP_ERR, "unknown vector %s\n", argv[i+2]);
      gprint (GP_ERR, "USAGE: write (file) vector vector ...\n");
      free (vec);
      return (FALSE);    
    }
  }
  
  /* select vector lengths */
  Ne = vec[0][0].Nelements;
  for (i = 0; i < Nvec; i++) {
    if (vec[0][0].Nelements != Ne) {
      gprint (GP_ERR, "error: vectors must all be the same size\n");
      free (vec);
      return (FALSE);    
    }
  }

  if (FITS) {
    int status = WriteVectorTableFITS (argv[1], FITS, fitsheader, vec, Nvec, append, compress, format, Ntile);
    free (vec);
    return status;
  }

  /* open file for output */
  if (append) {
    f = fopen (argv[1], "a");
  } else {
    f = fopen (argv[1], "w");
  }
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't open file for write\n");
    return (FALSE);
  }

  /* default output format */
  if (ADD_HEADER) {
    for (j = 0; j < Nvec; j++) {
      if (j == 0) fprintf (f, "# ");
      if (CSV) {
	fprintf (f, "%s,", vec[j][0].name);
      } else {
	fprintf (f, "%s ", vec[j][0].name);
      }
    }
    fprintf (f, "\n");
  }

  /* default output format */
  if (format == (char *) NULL) {
    char padChar = CSV ? ',' : ' ';
    for (i = 0; i < vec[0][0].Nelements; i++) {
      for (j = 0; j < Nvec; j++) {
	switch (vec[j][0].type) {
	  case OPIHI_FLT:
	    fprintf (f, "%.12g%c", vec[j][0].elements.Flt[i], padChar);
	    break;
	  case OPIHI_INT:
	    fprintf (f, OPIHI_INT_FMT"%c", vec[j][0].elements.Int[i], padChar);
	    break;
	  case OPIHI_STR:
	    fprintf (f, "%s%c", vec[j][0].elements.Str[i], padChar);
	    break;
	}
      }
      fprintf (f, "\n");
    } 
    fclose (f);
    free (vec);
    fflush (f);
    return (TRUE);
  }

  /* construct an array of format strings */
  ALLOCATE (fmttype, char, Nvec);
  ALLOCATE (fmtlist, char *, Nvec);
  for (i = 0; i < Nvec; i++) {
    ALLOCATE (fmtlist[i], char, 1024);
    bzero (fmtlist[i], 1024);
  }

  p0 = format;
  for (j = 0; j < Nvec; j++) {
    /* find this format character */
    p1 = strchr (p0, '%');
    if (p1 == (char *) NULL) {
      gprint (GP_ERR, "mismatch between format and values\n");
      free (fmttype);
      for (i = 0; i < Nvec; i++) free (fmtlist[i]);
      free (fmtlist);
      free (format);
      fclose (f);
      fflush (f);
      return (FALSE);
    }
    
    /* identify type (%NNNNd %NNNNf) */
    for (p2 = p1 + 1; (*p2 == '.') || (*p2 == '-') || (*p2 == '+') || (*p2 == ' ') || isdigit(*p2); p2++);
    strncpy_nowarn (fmtlist[j], p0, p2 - p0 + 1);
    switch (*p2) {
      case 'e':
      case 'f':
	fmttype[j] = 'f';
	break;
      case 'd':
      case 'c':
      case 'x':
	fmttype[j] = 'd';
	break;
      case 's':
	fmttype[j] = 's';
	break;
      default:
	gprint (GP_ERR, "syntax error in format (only e,f,d,c,x,s allowed)\n");
	return (FALSE);
    }
    p0 = p2 + 1;
  }
  strcat (fmtlist[Nvec-1], p0);
  
  // check format types against vector types:
  for (j = 0; j < Nvec; j++) {
    switch (vec[j][0].type) {
      case OPIHI_FLT:
      case OPIHI_INT:
	if (fmttype[j] == 's') {
	  gprint (GP_ERR, "mismatch between string format and numerical vector for %s\n", vec[j][0].name);
	  return FALSE;
	}
	break;
      case OPIHI_STR:
	if (fmttype[j] != 's') {
	  gprint (GP_ERR, "mismatch between numerical format and string vector for %s\n", vec[j][0].name);
	  return FALSE;
	}
	break;
    }
  }

  for (i = 0; i < vec[0][0].Nelements; i++) {
    for (j = 0; j < Nvec; j++) {
      if (fmttype[j] == 'd') {
	if (vec[j][0].type == OPIHI_FLT) {
	  fprintf (f, fmtlist[j], (opihi_int)(vec[j][0].elements.Flt[i]));
	} else {
	  fprintf (f, fmtlist[j], (opihi_int)(vec[j][0].elements.Int[i]));
	}
      } 
      if (fmttype[j] == 'f') {
	if (vec[j][0].type == OPIHI_FLT) {
	  fprintf (f, fmtlist[j], (opihi_flt)(vec[j][0].elements.Flt[i]));
	} else {
	  fprintf (f, fmtlist[j], (opihi_flt)(vec[j][0].elements.Int[i]));
	}
      } 
      if (fmttype[j] == 's') {
	fprintf (f, fmtlist[j], vec[j][0].elements.Str[i]);
      } 
    }
    fprintf (f, "\n");
  }
  fclose (f);
  fflush (f);

  free (fmttype);
  for (i = 0; i < Nvec; i++) free (fmtlist[i]);
  free (fmtlist);
  free (format);
  
  return (TRUE);

 usage:
    gprint (GP_ERR, "USAGE: write [options] file vector vector ...\n");
    gprint (GP_ERR, "OPTIONS: [-header [buf]] [-append] [-f \"format\"] [-fits NAME] [-csv] [-h,--help]\n\n");

    gprint (GP_ERR, "OPTIONS: \n");
    gprint (GP_ERR, "  -header : add a descriptive header line to ascii or csv output (see below for FITS)\n");
    gprint (GP_ERR, "  -append : write to the end of the existing file\n");
    gprint (GP_ERR, "  -fits NAME : write a fits table (extention name is NAME, column names match vector names)\n");
    gprint (GP_ERR, "     in FITS output context, -header takes an additional argument which is interpretted\n");
    gprint (GP_ERR, "     as a buffer containing header keywords to supplement the FITS table header\n");
    gprint (GP_ERR, "  -csv : write a comma-separated values file (eg, to read in excel)\n");
    gprint (GP_ERR, "  -f \"format\" : provide formatting codes for output:\n");
    gprint (GP_ERR, "    ascii / csv : format consists of c-style format codes in form %%NN.Md\n");
    gprint (GP_ERR, ",     the following codes are allowed\n");
    gprint (GP_ERR, "        %%e -- double, %%f -- float\n");
    gprint (GP_ERR, "        %%d -- int, %%c -- char, %%x -- hex int\n");
    gprint (GP_ERR, "    FITS : format consists of the following FITS binary table format codes:\n");
    gprint (GP_ERR, "      B : 1 byte int\n");    
    gprint (GP_ERR, "      I : 2 byte int\n");    
    gprint (GP_ERR, "      J : 4 byte int\n");    
    gprint (GP_ERR, "      K : 8 byte int\n");    
    gprint (GP_ERR, "      E : 4 byte float\n");    
    gprint (GP_ERR, "      D : 8 byte float\n");    
    gprint (GP_ERR, "   -h : this help listing\n");
    gprint (GP_ERR, "   --h : this help listing\n");
    gprint (GP_ERR, "   -help : this help listing\n");
    gprint (GP_ERR, "   --help : this help listing\n");
    return FALSE;
}


