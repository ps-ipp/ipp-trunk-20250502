# include "imregister.h"
# include "detrend.h"

int DefineImage (char *filename, Descriptor *descriptor) {

  int i, Extend, Nextend;
  char line[512];
  Header header;

  /* load remaining options from the image header */
  if (!gfits_read_header (filename, &header)) {
    fprintf (stderr, "ERROR: trouble reading image header\n");
    exit (1);
  }

  /* first decide on image TYPE */
  if (descriptor[0].type == T_UNDEF) {
    if (!gfits_scan (&header, ImagetypeKeyword, "%s", 1, line)) {
      fprintf (stderr, "ERROR: failure to read %s from header\n", ImagetypeKeyword);
      exit (1);
    }
    descriptor[0].type = get_image_type (line);
    if (descriptor[0].type == T_UNDEF) {
      fprintf (stderr, "ERROR: invalid image type %s\n", line);
      exit (1);
    }
  }    
  /* conflicts */
  if ((descriptor[0].type == T_DARK) && (descriptor[0].filter != FILTER_NONE)) {
    fprintf (stderr, "ERROR: dark can't have a filter\n");
    exit (1);
  }
  if ((descriptor[0].type == T_BIAS) && (descriptor[0].filter != FILTER_NONE)) {
    fprintf (stderr, "ERROR: bias can't have a filter\n");
    exit (1);
  }
  if ((descriptor[0].type == T_MASK) && (descriptor[0].filter != FILTER_NONE)) {
    fprintf (stderr, "ERROR: mask can't have a filter\n");
    exit (1);
  }

  /* identify MODE (MEF / SPLIT)  */
  descriptor[0].mode = M_SPLIT;
  Extend = FALSE;
  gfits_scan_alt (&header, "EXTEND", "%t", 1, &Extend);
  if (Extend) {
    descriptor[0].mode      = M_MEF;
    descriptor[0].CCD       = Nccd;
    descriptor[0].CCDSelect = TRUE;
    gfits_scan (&header, "NEXTEND",  "%d", 1, &Nextend);
    if (Nextend != Nccd) { 
      fprintf (stderr, "warning: NEXTEND != Nccd (%d, %d)\n", Nextend, Nccd);
    }      
  }

  /* modes: a special case */
  if (descriptor[0].type == T_MODES) {
    descriptor[0].mode      = M_MODES;
    descriptor[0].CCD       = Nccd;
    descriptor[0].CCDSelect = TRUE;
  }

  /* now identify CCD number */
  if (!descriptor[0].CCDSelect) {
    char ID[64];

    descriptor[0].CCD = -1;
    if (!gfits_scan (&header, CCDnumKeyword, "%s", 1, ID)) {
      fprintf (stderr, "ERROR: failure to read %s from header\n", CCDnumKeyword);
      exit (1);
    }
    for (i = 0; (i < Nccd) && (descriptor[0].CCD == -1); i++) {
      if (strnumcmp (ID, ccds[i])) {
	descriptor[0].CCD = i;
      }
    }
    if (descriptor[0].CCD == -1) {
      fprintf (stderr, "warning: ccd id not found\n");
      descriptor[0].CCD = 0;
    }
    descriptor[0].CCDSelect = TRUE;
  }

  /* now get time range */
  if (!descriptor[0].TimeSelect) {
    if (!gfits_scan (&header, "TVSTART",   "%s", 1, line)) {
      fprintf (stderr, "ERROR: missing start time\n");
      exit (1);
    }
    if (!ohana_str_to_time (line, &descriptor[0].tstart)) { 
      fprintf (stderr, "ERROR: invalid time %s\n", line);
      exit (1);
    }
    
    if (!gfits_scan (&header, "TVSTOP",   "%s", 1, line)) {
      fprintf (stderr, "ERROR: missing stop time\n");
      exit (1);
    }
    if (!ohana_str_to_time (line, &descriptor[0].tstop)) { 
      fprintf (stderr, "ERROR: invalid time %s\n", line);
      exit (1);
    }
  }

  /* select the filter */
  if (descriptor[0].type == T_DARK) goto skip_filter;
  if (descriptor[0].type == T_BIAS) goto skip_filter;
  if (descriptor[0].type == T_MASK) goto skip_filter;
  if (descriptor[0].filter != FILTER_NONE) goto skip_filter;
  if (!gfits_scan (&header, FilterKeyword, "%s", 1, line)) {
    fprintf (stderr, "ERROR: failure to read %s from header\n", FilterKeyword);
    exit (1);
  }
  for (i = 0; (i < NFILTER) && (descriptor[0].filter == FILTER_NONE); i++) {
    if (!strcasecmp (line, filtername[i])) {
      descriptor[0].filter = filternum[i];
    }
  }      
  if (descriptor[0].filter == FILTER_NONE) {
    fprintf (stderr, "ERROR: invalid filter %s\n", line);
    exit (1);
  }
skip_filter:

  /* set the exposure time, if needed */
  if (descriptor[0].ExptimeSelect && (descriptor[0].type != T_DARK)) {
    fprintf (stderr, "exposure time is not allowed for type %s\n", get_type_name(descriptor[0].type));
    exit (1);
  }
  if (!descriptor[0].ExptimeSelect && (descriptor[0].type == T_DARK)) {
    if (!gfits_scan (&header, ExptimeKeyword,  "%f", 1, &descriptor[0].Exptime)) {
      fprintf (stderr, "ERROR: failure to read %s from header\n", ExptimeKeyword);
      exit (1);
    }
  }

  /* set the image ID (if not supplied) based on the camera header */
  if (descriptor[0].imageID == NULL) {
    if (gfits_scan (&header, "CRUNID",   "%s", 1, line)) {
      descriptor[0].imageID = strcreate (line);
    }
  }
  if (descriptor[0].imageID == NULL) {
    descriptor[0].imageID = strcreate ("test");
  }  
  
  return (TRUE);
}

