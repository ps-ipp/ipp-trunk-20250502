# include "imregister.h"
# include "imreg.h"

char *getcwd_cfht (char *, int);

RegImage *iminfo (char *filename) {

  RegImage *image;
  Header header;
  int extend;
  int Naxes, Nextend, Nseq;
  char Imagetype[80], line[80];
  struct timeval now;
  char *name, *cwd, *tempname;
  double tmp;

  ALLOCATE (image, RegImage, 1);
  bzero (image, sizeof (RegImage));

  /* load in FITS header from image */
  if (!gfits_read_header (filename, &header)) {
    fprintf (stderr, "ERROR: can't find image file %s\n", filename);
    exit (1);
  }

  /* find the important header keyword values */
  image[0].obstime = parse_time (&header);

  gettimeofday (&now, (void *) NULL);
  image[0].regtime = now.tv_sec;

  image[0].sky = image[0].fwhm = image[0].bias = 0;
  
  /* default for a single CCD frame */
  Naxes = 2; 
  extend = FALSE;
  image[0].mode     = M_SINGLE;
  image[0].flag     = 0;
  image[0].ccd      = 0;
  image[0].seq      = 0;
  image[0].seqtime  = 0;

  /* determine data layout (SINGLE, SPLIT, MEF, CUBE, SLICE) */
  gfits_scan_alt (&header, "EXTEND",  "%t", 1, &extend);
  gfits_scan (&header, "NAXIS",  "%d", 1, &Naxes);
  if (extend) { /* MEF file */
    gfits_scan (&header, "NEXTEND",  "%d", 1, &Nextend);
    image[0].mode = M_MEF;
    image[0].ccd  = Nextend;
  } 
  /* need to distinguish MEF, CUBE, and MEF-CUBE */
  if (Naxes == 3) { /* data cube */
    gfits_scan (&header, "NAXIS3",  "%d", 1, &Nseq);
    if (image[0].mode == M_MEF) {
      fprintf (stderr, "MEF-CUBE not ready\n");
      exit (1);
    }
    image[0].mode = M_CUBE;
    image[0].seq  = Nseq;
    /* abstract this name somewhere ? */
    gfits_scan (&header, "SEQTIME", "%f", 1, &image[0].seqtime);
  }
  if (SingleIsSplit && (image[0].mode == M_SINGLE)) {
    image[0].mode = M_SPLIT;
    image[0].ccd  = MatchCCDNameHeader (&header);
  }
  /* is there a better way to id a 'split' image? */

  /* extract other relevant data from header */
  gfits_scan (&header, ImagetypeKeyword,  "%s", 1, (char *)&Imagetype);

  /* grab the image type : if not defined, set to 'none' */
  image[0].type = get_image_type (Imagetype);
  if (NeedType && (image[0].type == T_UNDEF)) {
    fprintf (stderr, "ERROR: skipping unknown image type\n");
    exit (1);
  } else {
    image[0].type = T_NONE;
  }

  /* grab interesting info from header */
  gfits_scan (&header, ExptimeKeyword,    "%f", 1, &image[0].exptime);
  gfits_scan (&header, AirmassKeyword,    "%f", 1, &image[0].airmass);
  gfits_scan (&header, FocusKeyword,      "%f", 1, &image[0].telfocus);
  gfits_scan (&header, Teldata1Keyword,   "%f", 1, &image[0].xprobe);
  gfits_scan (&header, Teldata2Keyword,   "%f", 1, &image[0].yprobe);
  gfits_scan (&header, Teldata3Keyword,   "%f", 1, &image[0].zprobe);
  gfits_scan (&header, DettempKeyword,    "%f", 1, &image[0].dettemp);
  gfits_scan (&header, RotationKeyword,   "%f", 1, &image[0].rotangle);

  /* force strings to fit in available space (32 bytes) */
  gfits_scan (&header, CameraKeyword,     "%s", 1, line);
  strncpy_nowarn (image[0].instrument, line, 31);

  gfits_scan (&header, FilterKeyword, "%s", 1, line);
  MatchFilterList (line);
  strncpy_nowarn (image[0].filter, line, 31);

  /* header has RA & DEC in decimal degrees */
  if (RADecDegKeyword[0] & DECDecDegKeyword[0]) {
    gfits_scan (&header, RADecDegKeyword,  "%f", 1, &image[0].ra);
    gfits_scan (&header, DECDecDegKeyword, "%f", 1, &image[0].dec);
  } else {
    if (RASexigKeyword[0] & DECSexigKeyword[0]) {
      gfits_scan (&header, RASexigKeyword,  "%s", 1, line);
      ohana_dms_to_ddd (&tmp, line);
      image[0].ra = 15*tmp;
      gfits_scan (&header, DECSexigKeyword, "%s", 1, (char *)&line);
      ohana_dms_to_ddd (&tmp, line);
      image[0].dec = tmp;
    }
  }

  /* this segment is strongly CFHT dependent.  also it is somewhat
     poor: the file must have the probes in the right order.
     load_probes checks the data validity, but has no gurantee that
     the right range has been loaded */

  /* to change the selected probes, change:
     1) the probes[] entries below
     2) the entries in the program 'gettemps' */

  /* the probes have to agree with the entries in the program 'gettemps' */
  {
    double pvalues[4];
    static int probes[] = {2, 36, 43, 45};
    if (load_probes (TempLogFile, image[0].obstime, probes, pvalues, 4)) {
	image[0].teltemp_0 = pvalues[0];
	image[0].teltemp_1 = pvalues[1];
	image[0].teltemp_2 = pvalues[2];
	image[0].teltemp_3 = pvalues[3];
    } else {
      fprintf (stderr, "failure to get probe data\n");
      image[0].teltemp_0 = 200.0;
      image[0].teltemp_1 = 200.0;
      image[0].teltemp_2 = 200.0;
      image[0].teltemp_3 = 200.0;
    }
  }
  /* this should be a call to an external function... */
  
  /* extract the file name and the path */
  name = filebasename (filename);
  strcpy (image[0].filename, name);
  name = pathname (filename);
  if (name[0] != '/') {
    cwd = getcwd_cfht (NULL, 1024);
    ALLOCATE (tempname, char, strlen (cwd) + strlen (name) + 2);
    if (!strcmp (name, ".")) {
      sprintf (tempname, "%s", cwd);
    } else {
      sprintf (tempname, "%s/%s", cwd, name);
    }      
    free (name);
    free (cwd);
    name = tempname;
  }    
  strcpy (image[0].pathname, name);

  return (image);

}

