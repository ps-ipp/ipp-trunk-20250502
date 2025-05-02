# include "imregister.h"
# include "detrend.h"

Criteria *ImageCriteria (Criteria base, char *filename, char *ImageExtend, char *ImageMode, int *ncrit) {

  int i, Ncrit;
  Header header;
  Criteria *crit;
  char line[80];

  Ncrit = 1;
  ALLOCATE (crit, Criteria, Ncrit);

  /* load options from the image header */
  if (!gfits_read_header (filename, &header)) {
    if (output.verbose) fprintf (stderr, "ERROR: trouble reading image header\n");
    exit (1);
  }

  /* get Time from header */
  base.tstop = base.tstart = parse_time (&header);
  base.TimeSelect = TRUE;

  /* get the CCD */
  if (!strcasecmp (ImageMode, "SPLIT")) {
    ALLOCATE (ImageExtend, char, 80);
    if (!gfits_scan (&header, CCDnumKeyword, "%s", 1, ImageExtend)) {
      fprintf (stderr, "ERROR: failure to read %s from header\n", CCDnumKeyword);
      exit (1);
    }
  }
  /* lookup CCD ID from camera config */
  base.CCD = -1;
  for (i = 0; (i < Nccd) && (base.CCD == -1); i++) {
    if (strnumcmp (ccds[i], ImageExtend)) {
      base.CCD = i;
    }
  }
  if (base.CCD == -1) {
    fprintf (stderr, "warning: ccd id %s not found\n", ImageExtend);
    base.CCD = 0;
  }

  /* get filter from header */
  if (!gfits_scan (&header, FilterKeyword, "%s", 1, line)) {
    fprintf (stderr, "ERROR: trouble reading FILTER from header\n");
    exit (1);
  }
  /* filter names in database have . for space */
  base.Filter = FILTER_NONE;
  for (i = 0; i < strlen (line); i++) { if (isspace (line[i])) line[i] = '.'; }
  for (i = 0; (i < NFILTER) && (base.Filter == FILTER_NONE); i++) {
    if (!strcasecmp (line, filtername[i])) {
      base.Filter = filternum[i];
    }
  }      
  if (base.Filter == FILTER_NONE) {
    fprintf (stderr, "ERROR: invalid filter %s\n", line);
    exit (1);
  }

  /* get exptime from header */
  if (!gfits_scan (&header, ExptimeKeyword, "%f", 1, &base.Exptime)) {
    fprintf (stderr, "ERROR: trouble reading EXPTIME from header\n");
    exit (1);
  }

  /* define selections implicit in ImageSelect */
  if (base.Type != T_MODES) {
    base.CCDSelect = TRUE;
  }
  if ((base.Type != T_DARK) && (base.Type != T_BIAS) && (base.Type != T_MASK)) {
    base.FilterSelect = TRUE;
  } else {
    base.FilterSelect = FALSE;
  }      
  if (base.Type == T_DARK) {
    base.ExptimeSelect = TRUE;
  } else {
    base.ExptimeSelect = FALSE;
  }      

  /* controvesial... */
  output.Select = TRUE;

  *ncrit = Ncrit;
  crit[0] = base;
  return (crit);
}

